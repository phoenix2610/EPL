#define _POSIX_C_SOURCE 200809L
#define _DEFAULT_SOURCE 1
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include <strings.h>
#include <dirent.h>
#include <limits.h>
#include <libgen.h>
#include "actions.h"
#include "lexer.h"
#include "parser.h"
#include "epc.h"

#include "ast_serial.inc"

typedef struct { ASTNode **items; int count, cap; } NodeVec;

static void vec_push(NodeVec *v, ASTNode *n) {
    if (v->count >= v->cap) {
        v->cap = v->cap ? v->cap * 2 : 16;
        v->items = realloc(v->items, (size_t)v->cap * sizeof *v->items);
    }
    v->items[v->count++] = n;
}

static void collect(ASTNode *n, int t1, int t2, NodeVec *out) {
    if (!n) return;
    if ((int)n->type == t1 || (int)n->type == t2) vec_push(out, n);
    if ((int)n->type < 0 || (int)n->type >= AST_DESC_COUNT) return;
    const NodeDesc *d = &AST_DESC[n->type];
    for (unsigned i = 0; i < d->n; i++) {
        char *base = (char *)n + d->f[i].off;
        if (d->f[i].kind == F_NODE) {
            collect(*(ASTNode **)base, t1, t2, out);
        } else if (d->f[i].kind == F_LIST) {
            NodeList *l = (NodeList *)base;
            for (int k = 0; k < l->count; k++) collect(l->items[k], t1, t2, out);
        }
    }
}

static char *slurp(const char *path) {
    FILE *f = fopen(path, "rb");
    if (!f) return NULL;
    fseek(f, 0, SEEK_END);
    long n = ftell(f);
    fseek(f, 0, SEEK_SET);
    char *s = malloc((size_t)n + 1);
    size_t got = fread(s, 1, (size_t)n, f);
    s[got] = '\0';
    fclose(f);
    return s;
}

static ASTNode *load_ep(const char *path) {
    ASTNode *ast = epc_load(path);
    if (ast) return ast;
    char *src = slurp(path);
    if (!src) return NULL;
    FILE *saved = stderr;
    FILE *null = fopen("/dev/null", "w");
    if (null) stderr = null;
    Lexer lx;
    lexer_init(&lx, src, LEX_EP);
    int count = 0;
    Token *tk = lexer_tokenize_all(&lx, &count);
    ASTNode *out = NULL;
    if (tk) {
        Parser p;
        parser_init(&p, tk, count);
        out = parser_parse(&p);
        if (p.had_error) { ast_free(out); out = NULL; }
        lexer_free_tokens(tk);
    }
    if (null) { stderr = saved; fclose(null); }
    free(src);
    return out;
}

typedef struct {
    char     name[MAX_IDENT_LEN];
    char     module[PATH_MAX];
    ASTNode *def;
} Def;

typedef struct {
    Def      *defs;   int ndefs, cdefs;
    char    **seen;   int nseen;
    ASTNode **asts;   int nasts;
    char      broken[PATH_MAX];
} Defs;

static void defs_add_program(Defs *d, ASTNode *prog, const char *path) {
    char real[PATH_MAX];
    if (!realpath(path, real)) snprintf(real, sizeof real, "%s", path);
    for (int i = 0; i < d->nseen; i++)
        if (strcmp(d->seen[i], real) == 0) return;
    d->seen = realloc(d->seen, (size_t)(d->nseen + 1) * sizeof *d->seen);
    d->seen[d->nseen++] = strdup(real);

    char dirbuf[PATH_MAX];
    snprintf(dirbuf, sizeof dirbuf, "%s", real);
    const char *dir = dirname(dirbuf);

    NodeList *body = &prog->as.program.body;
    for (int i = 0; i < body->count; i++) {
        ASTNode *n = body->items[i];
        if (!n) continue;
        if (n->type == NODE_ACTIVITY_DEF) {
            if (d->ndefs >= d->cdefs) {
                d->cdefs = d->cdefs ? d->cdefs * 2 : 32;
                d->defs = realloc(d->defs, (size_t)d->cdefs * sizeof *d->defs);
            }
            Def *df = &d->defs[d->ndefs++];
            snprintf(df->name, sizeof df->name, "%s", n->as.activity_def.name);
            snprintf(df->module, sizeof df->module, "%s", real);
            df->def = n;
        } else if (n->type == NODE_IMPORT) {
            char ip[PATH_MAX * 2];
            const char *raw = n->as.import_stmt.source;
            if (raw[0] == '/') snprintf(ip, sizeof ip, "%s", raw);
            else               snprintf(ip, sizeof ip, "%s/%s", dir, raw);
            ASTNode *mod = load_ep(ip);
            if (!mod) {
                if (!d->broken[0]) snprintf(d->broken, sizeof d->broken, "%s", raw);
                continue;
            }
            d->asts = realloc(d->asts, (size_t)(d->nasts + 1) * sizeof *d->asts);
            d->asts[d->nasts++] = mod;
            defs_add_program(d, mod, ip);
        }
    }
}

static void defs_free(Defs *d) {
    for (int i = 0; i < d->nseen; i++) free(d->seen[i]);
    for (int i = 0; i < d->nasts; i++) ast_free(d->asts[i]);
    free(d->defs); free(d->seen); free(d->asts);
}

static Def *defs_find(Defs *d, const char *name, int *dups) {
    Def *hit = NULL;
    *dups = 0;
    for (int i = 0; i < d->ndefs; i++)
        if (strcmp(d->defs[i].name, name) == 0) { if (!hit) hit = &d->defs[i]; (*dups)++; }
    return hit;
}

static int edit_distance(const char *a, const char *b) {
    int la = (int)strlen(a), lb = (int)strlen(b);
    if (la > 63 || lb > 63) return 99;
    int dp[64][64];
    for (int i = 0; i <= la; i++) dp[i][0] = i;
    for (int j = 0; j <= lb; j++) dp[0][j] = j;
    for (int i = 1; i <= la; i++)
        for (int j = 1; j <= lb; j++) {
            int c = dp[i-1][j-1] + (a[i-1] != b[j-1]);
            if (dp[i-1][j] + 1 < c) c = dp[i-1][j] + 1;
            if (dp[i][j-1] + 1 < c) c = dp[i][j-1] + 1;
            dp[i][j] = c;
        }
    return dp[la][lb];
}

static const char *base_name(const char *path) {
    const char *s = strrchr(path, '/');
    return s ? s + 1 : path;
}

static void params_text(ASTNode *def, char *out, size_t n) {
    NodeList *ps = &def->as.activity_def.params;
    out[0] = '\0';
    for (int i = 0; i < ps->count; i++) {
        if (i) strncat(out, ", ", n - strlen(out) - 1);
        strncat(out, ps->items[i]->as.identifier.name, n - strlen(out) - 1);
    }
    if (!out[0]) snprintf(out, n, "nothing");
}

static int check_offering(const char *file, ASTNode *serve, Defs *d) {
    int errors = 0;
    NodeList *off = &serve->as.serve_port.offered;
    for (int i = 0; i < off->count; i++) {
        ASTNode *id = off->items[i];
        const char *nm = id->as.identifier.name;
        int dups;
        Def *df = defs_find(d, nm, &dups);
        if (dups > 1) {
            fprintf(stderr, "[EPL Error] %s line %d: '%s' is defined %d times "
                    "(in %s and others) — rename one so the page's action is unambiguous\n",
                    file, id->line, nm, dups, base_name(df->module));
            errors++;
        } else if (!df) {
            const char *best = NULL;
            int bd = 99;
            for (int k = 0; k < d->ndefs; k++) {
                int e = edit_distance(nm, d->defs[k].name);
                if (e < bd) { bd = e; best = d->defs[k].name; }
            }
            if (d->broken[0])
                fprintf(stderr, "[EPL Error] %s line %d: cannot offer '%s': the imported "
                        "module \"%s\" is missing or doesn't parse — run: epl --check %s\n",
                        file, id->line, nm, d->broken, d->broken);
            else if (best && bd <= 3)
                fprintf(stderr, "[EPL Error] %s line %d: cannot offer '%s': no activity "
                        "has that name — did you mean '%s'?\n", file, id->line, nm, best);
            else
                fprintf(stderr, "[EPL Error] %s line %d: cannot offer '%s': no activity "
                        "has that name here or in an imported module\n", file, id->line, nm);
            errors++;
        }
        for (int j = 0; j < i; j++)
            if (strcmp(off->items[j]->as.identifier.name, nm) == 0) {
                fprintf(stderr, "[EPL Error] %s line %d: '%s' is offered twice\n",
                        file, id->line, nm);
                errors++;
            }
    }
    return errors;
}

int actions_check_server(const char *ep_path, ASTNode *program) {
    NodeVec serves = {0};
    collect(program, NODE_SERVE_PORT, -1, &serves);
    if (!serves.count) { free(serves.items); return 0; }
    Defs d = {0};
    defs_add_program(&d, program, ep_path);
    int errors = 0;
    for (int i = 0; i < serves.count; i++)
        errors += check_offering(base_name(ep_path), serves.items[i], &d);
    defs_free(&d);
    free(serves.items);
    return errors ? 1 : 0;
}

static int is_ep_file(const char *name) {
    const char *ext = strrchr(name, '.');
    return ext && strcasecmp(ext, ".EP") == 0;
}

int actions_check_page(const char *epf_path, ASTNode *program) {
    NodeVec acts = {0};
    collect(program, NODE_ACTION_FORM, NODE_ACTION_BUTTON, &acts);
    if (!acts.count) { free(acts.items); return 0; }

    const char *page = base_name(epf_path);
    char dirbuf[PATH_MAX];
    snprintf(dirbuf, sizeof dirbuf, "%s", epf_path);
    char *dir = dirname(dirbuf);

    char server[PATH_MAX * 2] = "", others[1024] = "";
    ASTNode *server_ast = NULL, *serve = NULL;
    int servers = 0;
    DIR *dh = opendir(dir);
    struct dirent *de;
    while (dh && (de = readdir(dh))) {
        if (!is_ep_file(de->d_name)) continue;
        char path[PATH_MAX * 2];
        snprintf(path, sizeof path, "%s/%s", dir, de->d_name);
        ASTNode *ast = load_ep(path);
        if (!ast) continue;
        NodeVec sv = {0};
        collect(ast, NODE_SERVE_PORT, -1, &sv);
        if (sv.count) {
            servers++;
            if (others[0]) strncat(others, ", ", sizeof others - strlen(others) - 1);
            strncat(others, de->d_name, sizeof others - strlen(others) - 1);
            if (!server_ast) {
                snprintf(server, sizeof server, "%s", path);
                server_ast = ast;
                serve = sv.items[0];
                ast = NULL;
            }
        }
        free(sv.items);
        if (ast) ast_free(ast);
    }
    if (dh) closedir(dh);

    int errors = 0;
    if (servers == 0) {
        ASTNode *a = acts.items[0];
        fprintf(stderr, "[EPL Error] %s line %d: no backend in this folder serves pages, "
                "so '%s' cannot be called. Add to your server file: "
                "serve on port 8080 offering %s\n",
                page, a->line, a->as.action_form.activity, a->as.action_form.activity);
        free(acts.items);
        return 1;
    }
    if (servers > 1) {
        fprintf(stderr, "[EPL Error] %s: more than one program here serves on a port (%s) — "
                "keep one, so each page action has exactly one backend\n", page, others);
        ast_free(server_ast);
        free(acts.items);
        return 1;
    }

    Defs d = {0};
    defs_add_program(&d, server_ast, server);
    const char *sname = base_name(server);
    int bad_server = check_offering(sname, serve, &d);
    if (bad_server) {
        fprintf(stderr, "[EPL Error] %s: fix %s first (epl --check %s)\n", page, sname, sname);
        errors++;
    }

    NodeList *off = &serve->as.serve_port.offered;
    for (int i = 0; !bad_server && i < acts.count; i++) {
        ASTNode *a = acts.items[i];
        const char *act = a->as.action_form.activity;
        const char *kind = a->type == NODE_ACTION_FORM ? "form" : "button";

        int offered = 0;
        const char *best = NULL;
        int bd = 99;
        for (int k = 0; k < off->count; k++) {
            const char *o = off->items[k]->as.identifier.name;
            if (strcmp(o, act) == 0) offered = 1;
            int e = edit_distance(act, o);
            if (e < bd) { bd = e; best = o; }
        }
        int dups;
        Def *df = defs_find(&d, act, &dups);
        if (!offered) {
            if (df)
                fprintf(stderr, "[EPL Error] %s line %d: %s for %s: %s exists, but %s "
                        "doesn't offer it — add it to: serve on port … offering\n",
                        page, a->line, kind, act, act, sname);
            else if (best && bd <= 3)
                fprintf(stderr, "[EPL Error] %s line %d: %s for %s: %s offers no '%s' "
                        "— did you mean '%s'?\n", page, a->line, kind, act, sname, act, best);
            else
                fprintf(stderr, "[EPL Error] %s line %d: %s for %s: %s offers no '%s' "
                        "(it offers the activities listed after 'offering')\n",
                        page, a->line, kind, act, sname, act);
            errors++;
            continue;
        }

        char takes[1024];
        params_text(df->def, takes, sizeof takes);
        NodeList *ps = &df->def->as.activity_def.params;
        NodeList *ins = &a->as.action_form.inputs;

        if (a->type == NODE_ACTION_BUTTON) {
            if (ps->count) {
                fprintf(stderr, "[EPL Error] %s line %d: button for %s: %s takes %s, "
                        "so use a form with an input for each: make form \"…\" for %s\n",
                        page, a->line, act, act, takes, act);
                errors++;
            }
            continue;
        }
        for (int k = 0; k < ins->count; k++) {
            ASTNode *in = ins->items[k];
            const char *nm = in->as.action_input.name;
            int known = 0;
            for (int j = 0; j < ps->count; j++)
                if (strcmp(ps->items[j]->as.identifier.name, nm) == 0) known = 1;
            int repeat = 0;
            for (int j = 0; j < k; j++)
                if (strcmp(ins->items[j]->as.action_input.name, nm) == 0) repeat = 1;
            if (repeat) {
                fprintf(stderr, "[EPL Error] %s line %d: form for %s has input \"%s\" twice\n",
                        page, in->line, act, nm);
                errors++;
            } else if (!known) {
                const char *pb = NULL;
                int pd = 99;
                for (int j = 0; j < ps->count; j++) {
                    int e = edit_distance(nm, ps->items[j]->as.identifier.name);
                    if (e < pd) { pd = e; pb = ps->items[j]->as.identifier.name; }
                }
                if (pb && pd <= 3)
                    fprintf(stderr, "[EPL Error] %s line %d: form for %s: %s takes %s — "
                            "did you mean input \"%s\"?\n", page, in->line, act, act, takes, pb);
                else
                    fprintf(stderr, "[EPL Error] %s line %d: form for %s: %s takes %s, "
                            "not \"%s\"\n", page, in->line, act, act, takes, nm);
                errors++;
            }
        }
        for (int j = 0; j < ps->count; j++) {
            const char *pn = ps->items[j]->as.identifier.name;
            int has = 0;
            for (int k = 0; k < ins->count; k++)
                if (strcmp(ins->items[k]->as.action_input.name, pn) == 0) has = 1;
            if (!has) {
                fprintf(stderr, "[EPL Error] %s line %d: form for %s is missing "
                        "input \"%s\" (%s takes %s)\n", page, a->line, act, pn, act, takes);
                errors++;
            }
        }
    }

    defs_free(&d);
    ast_free(server_ast);
    free(acts.items);
    return errors ? 1 : 0;
}
