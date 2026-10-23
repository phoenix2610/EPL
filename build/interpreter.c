#define _POSIX_C_SOURCE 200809L
#define _DEFAULT_SOURCE 1
#define __USE_MINGW_ANSI_STDIO 1
#undef __STRICT_ANSI__
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <ctype.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <strings.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include "interpreter.h"
#include "codegen.h"
#include "binding.h"
#include "json.h"
#include "lexer.h"
#include "parser.h"
#include "epc.h"
#include "sha256.h"
#include <libgen.h>

extern char *read_file(const char *path);

static int _nova_conn_fd = -1;
static int _nova_replied = 0;

#define VALUE_FREELIST_MAX 4096
static Value *value_freelist[VALUE_FREELIST_MAX];
static int    value_freelist_n = 0;

static Value *val_alloc(void) {
    if (value_freelist_n > 0) return value_freelist[--value_freelist_n];
    Value *v = malloc(sizeof(Value));
    if (!v) { fprintf(stderr, "EPL: out of memory\n"); exit(1); }
    return v;
}

Value *val_number(double n) {
    Value *v = val_alloc();
    v->type = VAL_NUMBER;
    v->as.number = n;
    return v;
}

Value *val_string(const char *s) {
    Value *v = val_alloc();
    v->type = VAL_STRING;
    v->as.string = strdup(s);
    return v;
}

Value *val_boolean(int b) {
    Value *v = val_alloc();
    v->type = VAL_BOOLEAN;
    v->as.boolean = b ? 1 : 0;
    return v;
}

Value *val_null(void) {
    Value *v = val_alloc();
    v->type = VAL_NULL;
    return v;
}

Value *val_list(void) {
    Value *v = val_alloc();
    v->type = VAL_LIST;
    v->as.list = malloc(sizeof(ValueList));
    vallist_init(v->as.list);
    return v;
}

Value *val_activity(ASTNode *node) {
    Value *v = val_alloc();
    v->type = VAL_ACTIVITY;
    v->as.activity = node;
    return v;
}

Value *val_graph(void) {
    Value *v = val_alloc();
    v->type  = VAL_GRAPH;
    v->as.graph = calloc(1, sizeof(EPLGraph));
    return v;
}

Value *val_copy(Value *src) {
    if (!src) return val_null();
    switch (src->type) {
        case VAL_NUMBER:   return val_number(src->as.number);
        case VAL_STRING:   return val_string(src->as.string);
        case VAL_BOOLEAN:  return val_boolean(src->as.boolean);
        case VAL_NULL:     return val_null();
        case VAL_ACTIVITY: return val_activity(src->as.activity);
        case VAL_GRAPH: {
            Value *copy = val_graph();
            memcpy(copy->as.graph, src->as.graph, sizeof(EPLGraph));
            return copy;
        }
        case VAL_LIST: {
            Value *copy = val_list();
            for (int i = 0; i < src->as.list->count; i++)
                vallist_push(copy->as.list, val_copy(src->as.list->items[i]));
            return copy;
        }
        case VAL_MAP: {
            Value *copy = val_map();
            for (int i = 0; i < src->as.map->count; i++)
                valmap_set(copy->as.map, src->as.map->keys[i],
                           val_copy(src->as.map->values[i]));
            return copy;
        }
    }
    return val_null();
}

void val_free(Value *v) {
    if (!v) return;
    if (v->type == VAL_STRING) free(v->as.string);
    if (v->type == VAL_LIST) {
        vallist_free(v->as.list);
        free(v->as.list);
    }
    if (v->type == VAL_GRAPH) free(v->as.graph);
    if (v->type == VAL_MAP) {
        valmap_free(v->as.map);
        free(v->as.map);
    }
    if (value_freelist_n < VALUE_FREELIST_MAX) value_freelist[value_freelist_n++] = v;
    else free(v);
}

Value *val_map(void) {
    Value *v = val_alloc();
    v->type   = VAL_MAP;
    v->as.map = calloc(1, sizeof(ValueMap));
    return v;
}

void valmap_set(ValueMap *m, const char *key, Value *val) {
    for (int i = 0; i < m->count; i++) {
        if (strcmp(m->keys[i], key) == 0) {
            val_free(m->values[i]);
            m->values[i] = val;
            return;
        }
    }
    if (m->count >= m->capacity) {
        m->capacity = m->capacity == 0 ? 8 : m->capacity * 2;
        m->keys   = realloc(m->keys,   m->capacity * sizeof(char *));
        m->values = realloc(m->values, m->capacity * sizeof(Value *));
    }
    m->keys[m->count]     = strdup(key);
    m->values[m->count++] = val;
}

Value *valmap_get(ValueMap *m, const char *key) {
    for (int i = 0; i < m->count; i++)
        if (strcmp(m->keys[i], key) == 0) return m->values[i];
    return NULL;
}

void valmap_free(ValueMap *m) {
    for (int i = 0; i < m->count; i++) {
        free(m->keys[i]);
        val_free(m->values[i]);
    }
    free(m->keys);
    free(m->values);
    m->keys = NULL; m->values = NULL;
    m->count = 0; m->capacity = 0;
}

int val_is_truthy(Value *v) {
    if (!v) return 0;
    switch (v->type) {
        case VAL_BOOLEAN: return v->as.boolean;
        case VAL_NUMBER:  return v->as.number != 0;
        case VAL_STRING:  return strlen(v->as.string) > 0;
        case VAL_NULL:    return 0;
        case VAL_LIST:    return v->as.list->count > 0;
        case VAL_ACTIVITY:return 1;
        case VAL_GRAPH:   return 1;
        case VAL_MAP:     return v->as.map->count > 0;
    }
    return 0;
}

static void format_number(double d, char *buf, size_t n) {
    if (d == floor(d) && fabs(d) < 1e15)
        snprintf(buf, n, "%lld", (long long)d);
    else
        snprintf(buf, n, "%.15g", d);
}

static int text_as_number(const char *s, double *out) {
    while (*s == ' ' || *s == '\t') s++;
    if (!*s) return 0;
    char *end;
    double d = strtod(s, &end);
    if (end == s) return 0;
    while (*end == ' ' || *end == '\t') end++;
    if (*end) return 0;
    *out = d;
    return 1;
}

int val_equals(Value *a, Value *b) {
    if (!a || !b) return 0;
    if (a->type != b->type) {
        double d;
        if (a->type == VAL_NUMBER && b->type == VAL_STRING)
            return text_as_number(b->as.string, &d) && d == a->as.number;
        if (a->type == VAL_STRING && b->type == VAL_NUMBER)
            return text_as_number(a->as.string, &d) && d == b->as.number;
        return 0;
    }
    switch (a->type) {
        case VAL_NUMBER:  return a->as.number == b->as.number;
        case VAL_STRING:  return strcmp(a->as.string, b->as.string) == 0;
        case VAL_BOOLEAN: return a->as.boolean == b->as.boolean;
        case VAL_NULL:    return 1;
        default:          return 0;
    }
}

char *val_to_string(Value *v) {
    if (!v) return strdup("null");
    char buf[256];
    switch (v->type) {
        case VAL_NUMBER:
            format_number(v->as.number, buf, sizeof buf);
            return strdup(buf);
        case VAL_STRING:  return strdup(v->as.string);
        case VAL_BOOLEAN: return strdup(v->as.boolean ? "true" : "false");
        case VAL_NULL:    return strdup("null");
        case VAL_ACTIVITY:return strdup("[activity]");
        case VAL_GRAPH: {
            char buf[64];
            snprintf(buf, sizeof(buf), "[graph: %d nodes, %d edges]",
                v->as.graph->node_count,
                v->as.graph->edge_count);
            return strdup(buf);
        }
        case VAL_LIST: {
            size_t cap = 256, len = 0;
            char *result = malloc(cap);
            result[0] = '\0';

            #define APPEND(str) do {                               \
                const char *_s = (str);                            \
                size_t _n = strlen(_s);                            \
                if (len + _n + 1 > cap) {                          \
                    while (len + _n + 1 > cap) cap *= 2;           \
                    result = realloc(result, cap);                 \
                }                                                  \
                memcpy(result + len, _s, _n);                      \
                len += _n; result[len] = '\0';                     \
            } while (0)

            APPEND("[");
            for (int i = 0; i < v->as.list->count; i++) {
                char *s = val_to_string(v->as.list->items[i]);
                APPEND(s);
                free(s);
                if (i < v->as.list->count - 1) APPEND(", ");
            }
            APPEND("]");
            #undef APPEND
            return result;
        }
        case VAL_MAP:
            return json_serialize(v);
    }
    return strdup("null");
}

void val_print(Value *v) {
    char *s = val_to_string(v);
    printf("%s", s);
    free(s);
}

void vallist_init(ValueList *list) {
    list->items    = NULL;
    list->count    = 0;
    list->capacity = 0;
}

void vallist_push(ValueList *list, Value *v) {
    if (list->count >= list->capacity) {
        list->capacity = list->capacity == 0 ? 8 : list->capacity * 2;
        list->items = realloc(list->items, list->capacity * sizeof(Value *));
    }
    list->items[list->count++] = v;
}

void vallist_free(ValueList *list) {
    for (int i = 0; i < list->count; i++)
        val_free(list->items[i]);
    free(list->items);
    list->items    = NULL;
    list->count    = 0;
    list->capacity = 0;
}

static unsigned long env_hash(const char *s) {
    unsigned long h = 1469598103934665603UL;
    while (*s) { h ^= (unsigned char)*s++; h *= 1099511628211UL; }
    return h;
}

static int env_slot(Env *env, const char *name, unsigned long h, int *found) {
    int mask = env->capacity - 1;
    int i = (int)(h & (unsigned long)mask);
    while (env->names[i]) {
        if (strcmp(env->names[i], name) == 0) { *found = 1; return i; }
        i = (i + 1) & mask;
    }
    *found = 0;
    return i;
}

static void env_grow(Env *env) {
    int newcap = env->capacity ? env->capacity * 2 : 8;
    char  **nn = calloc((size_t)newcap, sizeof(char *));
    Value **nv = calloc((size_t)newcap, sizeof(Value *));
    int mask = newcap - 1;
    for (int i = 0; i < env->capacity; i++) {
        if (!env->names[i]) continue;
        unsigned long h = env_hash(env->names[i]);
        int j = (int)(h & (unsigned long)mask);
        while (nn[j]) j = (j + 1) & mask;
        nn[j] = env->names[i];
        nv[j] = env->values[i];
    }
    free(env->names);
    free(env->values);
    env->names    = nn;
    env->values   = nv;
    env->capacity = newcap;
}

Env *env_new(Env *parent) {
    Env *env = calloc(1, sizeof(Env));
    env->parent = parent;
    return env;
}

void env_free(Env *env) {
    if (!env) return;
    for (int i = 0; i < env->capacity; i++) {
        if (!env->names[i]) continue;
        free(env->names[i]);
        val_free(env->values[i]);
    }
    free(env->names);
    free(env->values);
    free(env);
}

void env_set(Env *env, const char *name, Value *value) {
    if ((env->count + 1) * 4 > env->capacity * 3) env_grow(env);

    unsigned long h = env_hash(name);
    int found;
    int i = env_slot(env, name, h, &found);
    if (found) {
        val_free(env->values[i]);
        env->values[i] = value;
        return;
    }
    env->names[i]  = strdup(name);
    env->values[i] = value;
    env->count++;
}

Value *env_get(Env *env, const char *name) {
    unsigned long h = env_hash(name);
    for (Env *scope = env; scope; scope = scope->parent) {
        if (scope->capacity == 0) continue;
        int found;
        int i = env_slot(scope, name, h, &found);
        if (found) return scope->values[i];
    }
    return NULL;
}

int env_update(Env *env, const char *name, Value *value) {
    unsigned long h = env_hash(name);
    for (Env *scope = env; scope; scope = scope->parent) {
        if (scope->capacity == 0) continue;
        int found;
        int i = env_slot(scope, name, h, &found);
        if (found) {
            val_free(scope->values[i]);
            scope->values[i] = value;
            return 1;
        }
    }
    return 0;
}

static void   db_index_cache_free(DBIndexCache *cache);
static Value *db_try_index_query(Interpreter *interp, ASTNode *table_expr,
                                 ASTNode *condition, Env *env);

void interpreter_init(Interpreter *interp, const char *source_file) {
    interp->global = env_new(NULL);
    interp->signal = (Signal){SIG_NONE, NULL};
    interp->had_error = 0;
    interp->call_depth = 0;
    interp->last_error[0] = '\0';
    interp->attempt_depth = 0;
    interp->record_scope = 0;
    interp->perms = NULL;
    interp->source_file = source_file;
    interp->page_manifest = NULL;
    interp->imported_paths = NULL;
    interp->imported_count = 0;
    interp->imported_cap   = 0;
    interp->imported_asts  = NULL;
    interp->imported_ast_count = 0;
    interp->import_depth   = 0;
    interp->db_index       = NULL;
}

void interpreter_free(Interpreter *interp) {
    env_free(interp->global);
    for (int i = 0; i < interp->imported_ast_count; i++)
        ast_free(interp->imported_asts[i]);
    free(interp->imported_asts);
    for (int i = 0; i < interp->imported_count; i++)
        free(interp->imported_paths[i]);
    free(interp->imported_paths);
    db_index_cache_free(interp->db_index);
}

static void runtime_error(Interpreter *interp, ASTNode *node,
                           const char *msg) {
    snprintf(interp->last_error, sizeof(interp->last_error), "%s", msg);

    if (interp->attempt_depth == 0) {
        fflush(stdout);
        fprintf(stderr, "[EPL Runtime Error] Line %d: %s\n",
            node ? node->line : 0, msg);
    }

    interp->had_error    = 1;
    interp->signal.type  = SIG_ERROR;
    interp->signal.value = NULL;
}

static Value *call_activity(Interpreter *interp, const char *name,
                             ValueList *args, Env *env, ASTNode *site);
static void db_bind_record_fields(Env *env, Value *record);

static Env *loop_pass_scope(Env *scope, Env *parent, int has_var) {
    if (scope && scope->count <= has_var) return scope;
    env_free(scope);
    return env_new(parent);
}

static Permissions *perms_of(Interpreter *interp) {
    if (!interp->perms) {
        static Permissions fallback;
        static int loaded = 0;
        if (!loaded) {
            char err[1024];
            const char *src = interp->source_file;
            if (!src || strcmp(src, "REPL") == 0) src = "./repl";
            if (perm_load(&fallback, src, err, sizeof err) != 0)
                perm_load(&fallback, "/nonexistent/none", err, sizeof err);
            loaded = 1;
        }
        interp->perms = &fallback;
    }
    return interp->perms;
}

#define PERMIT(check, arg) permit_(interp, node, (check)(perms_of(interp), (arg), perm_msg, sizeof perm_msg))
static char perm_msg[2048];
static int permit_(Interpreter *interp, ASTNode *node, int ok) {
    if (!ok) runtime_error(interp, node, perm_msg);
    return ok;
}
static int utf8_seq_len(const char *c) {
    int n = 1;
    while (c[n] && ((unsigned char)c[n] & 0xC0) == 0x80) n++;
    return n;
}

static int utf8_length(const char *s) {
    int n = 0;
    while (*s) { s += utf8_seq_len(s); n++; }
    return n;
}

static char *utf8_char_at(const char *s, int idx) {
    for (int i = 0; *s; i++) {
        int n = utf8_seq_len(s);
        if (i == idx) {
            char *out = malloc(n + 1);
            memcpy(out, s, n);
            out[n] = '\0';
            return out;
        }
        s += n;
    }
    return NULL;
}

static long utf8_decode(const char *s) {
    const unsigned char *u = (const unsigned char *)s;
    if (u[0] < 0x80) return u[0];
    if ((u[0] & 0xE0) == 0xC0) return ((u[0] & 0x1F) << 6) | (u[1] & 0x3F);
    if ((u[0] & 0xF0) == 0xE0) return ((u[0] & 0x0F) << 12) | ((u[1] & 0x3F) << 6) | (u[2] & 0x3F);
    return ((u[0] & 0x07) << 18) | ((u[1] & 0x3F) << 12) | ((u[2] & 0x3F) << 6) | (u[3] & 0x3F);
}

static int utf8_encode(long cp, char *out) {
    if (cp < 0x80)    { out[0] = (char)cp; out[1] = 0; return 1; }
    if (cp < 0x800)   { out[0] = 0xC0 | (cp >> 6); out[1] = 0x80 | (cp & 0x3F); out[2] = 0; return 2; }
    if (cp < 0x10000) { out[0] = 0xE0 | (cp >> 12); out[1] = 0x80 | ((cp >> 6) & 0x3F);
                        out[2] = 0x80 | (cp & 0x3F); out[3] = 0; return 3; }
    out[0] = 0xF0 | (cp >> 18); out[1] = 0x80 | ((cp >> 12) & 0x3F);
    out[2] = 0x80 | ((cp >> 6) & 0x3F); out[3] = 0x80 | (cp & 0x3F); out[4] = 0;
    return 4;
}

static const char *val_type_name(Value *v) {
    switch (v ? v->type : VAL_NULL) {
        case VAL_NUMBER:   return "a number";
        case VAL_STRING:   return "text";
        case VAL_BOOLEAN:  return "a boolean";
        case VAL_NULL:     return "null";
        case VAL_LIST:     return "a list";
        case VAL_MAP:      return "a record";
        case VAL_GRAPH:    return "a graph";
        case VAL_ACTIVITY: return "an activity";
        default:           return "a value";
    }
}

static int num_operand(Value *v, double *out) {
    if (!v) return 0;
    if (v->type == VAL_NUMBER) { *out = v->as.number; return 1; }
    if (v->type == VAL_STRING) return text_as_number(v->as.string, out);
    return 0;
}

static void arith_type_error(Interpreter *interp, ASTNode *node,
                             const char *opname, Value *left, Value *right) {
    double tmp;
    Value *bad = num_operand(left, &tmp) ? right : left;
    char msg[512];
    if (bad && bad->type == VAL_STRING)
        snprintf(msg, sizeof msg, "'%s' needs numbers, but got text \"%.60s\"",
                 opname, bad->as.string);
    else
        snprintf(msg, sizeof msg, "'%s' needs numbers, but got %s",
                 opname, val_type_name(bad));
    runtime_error(interp, node, msg);
}

static int compare_values(Value *a, Value *b, int *ok) {
    double x, y;
    *ok = 1;
    if (num_operand(a, &x) && num_operand(b, &y))
        return (x > y) - (x < y);
    if (a && b && a->type == VAL_STRING && b->type == VAL_STRING) {
        int c = strcmp(a->as.string, b->as.string);
        return (c > 0) - (c < 0);
    }
    *ok = 0;
    return 0;
}

static int edit_distance(const char *a, const char *b) {
    int la = (int)strlen(a), lb = (int)strlen(b);
    if (la > 64 || lb > 64) return 99;
    int row[65];
    for (int j = 0; j <= lb; j++) row[j] = j;
    for (int i = 1; i <= la; i++) {
        int diag = row[0];
        row[0] = i;
        for (int j = 1; j <= lb; j++) {
            int up = row[j];
            int cost = (a[i - 1] == b[j - 1]) ? 0 : 1;
            int best = diag + cost;
            if (up + 1 < best)         best = up + 1;
            if (row[j - 1] + 1 < best) best = row[j - 1] + 1;
            row[j] = best;
            diag = up;
        }
    }
    return row[lb];
}

static const struct { const char *word; const char *use; } NAME_HINTS[] = {
    { "sort",      "sorted LIST" },
    { "reverse",   "reversed LIST" },
    { "len",       "size of LIST" },
    { "length",    "size of LIST" },
    { "count",     "size of LIST" },
    { "sum",       "total of LIST" },
    { "max",       "maximum of LIST" },
    { "min",       "minimum of LIST" },
    { "avg",       "average of LIST" },
    { "mean",      "average of LIST" },
    { "upper",     "TEXT in uppercase" },
    { "uppercase", "TEXT in uppercase" },
    { "lower",     "TEXT in lowercase" },
    { "lowercase", "TEXT in lowercase" },
    { "split",     "TEXT split by \",\"" },
    { "join",      "LIST joined by \", \"" },
    { "print",     "show VALUE" },
    { "mod",       "A modulo B" },
    { "abs",       "VALUE if VALUE at least 0 otherwise 0 minus VALUE" },
    { "str",       "\"\" plus VALUE" },
    { "append",    "add VALUE to LIST" },
    { "push",      "add VALUE to LIST" },
    { NULL, NULL }
};

static void unknown_name_error(Interpreter *interp, ASTNode *node, Env *env,
                               const char *name) {
    char msg[512];

    char first[64];
    snprintf(first, sizeof first, "%s", name);
    char *sp = strchr(first, ' ');
    if (sp) *sp = '\0';
    for (int i = 0; NAME_HINTS[i].word; i++) {
        if (strcmp(first, NAME_HINTS[i].word) == 0) {
            snprintf(msg, sizeof msg,
                     "unknown name '%s' — EPL has no '%s'; write: %s",
                     name, first, NAME_HINTS[i].use);
            runtime_error(interp, node, msg);
            return;
        }
    }

    const char *best = NULL;
    int best_d = 99;
    int limit = (int)strlen(name) / 3;
    if (limit < 1) limit = 1;
    if (limit > 3) limit = 3;
    for (Env *scope = env; scope; scope = scope->parent) {
        for (int i = 0; i < scope->capacity; i++) {
            if (!scope->names[i]) continue;
            int d = edit_distance(name, scope->names[i]);
            if (d < best_d) { best_d = d; best = scope->names[i]; }
        }
    }
    if (best && best_d <= limit)
        snprintf(msg, sizeof msg, "unknown name '%s' — did you mean '%s'?",
                 name, best);
    else
        snprintf(msg, sizeof msg,
                 "unknown name '%s' — set it first with: set %s as VALUE",
                 name, name);
    runtime_error(interp, node, msg);
}

static Value *db_load_all(Interpreter *interp, const char *table);
static char  *db_table_name(Interpreter *interp, ASTNode *table_expr, Env *env);

static void graph_add_node(EPLGraph *g, const char *name) {
    for (int i = 0; i < g->node_count; i++) {
        if (strcmp(g->nodes[i], name) == 0) return;
    }
    if (g->node_count < EPL_GRAPH_MAX_NODES) {
        strncpy(g->nodes[g->node_count++], name, MAX_TOKEN_LEN - 1);
    }
}

static void graph_add_edge(EPLGraph *g, const char *from, const char *to) {
    graph_add_node(g, from);
    graph_add_node(g, to);
    if (g->edge_count < EPL_GRAPH_MAX_EDGES) {
        strncpy(g->edges[g->edge_count].from, from, MAX_TOKEN_LEN - 1);
        strncpy(g->edges[g->edge_count].to,   to,   MAX_TOKEN_LEN - 1);
        g->edge_count++;
    }
}

static Value *graph_bfs(EPLGraph *g, const char *start, const char *end) {
    if (g->node_count == 0) return val_list();

    int visited[EPL_GRAPH_MAX_NODES] = {0};
    int parent[EPL_GRAPH_MAX_NODES];
    memset(parent, -1, sizeof(parent));

    int start_idx = -1, end_idx = -1;
    for (int i = 0; i < g->node_count; i++) {
        if (strcmp(g->nodes[i], start) == 0) start_idx = i;
        if (strcmp(g->nodes[i], end)   == 0) end_idx   = i;
    }
    if (start_idx < 0 || end_idx < 0) return val_list();
    if (start_idx == end_idx) {
        Value *path = val_list();
        vallist_push(path->as.list, val_string(start));
        return path;
    }

    int queue[EPL_GRAPH_MAX_NODES];
    int head = 0, tail = 0;
    queue[tail++] = start_idx;
    visited[start_idx] = 1;

    int found = 0;
    while (head < tail) {
        int curr = queue[head++];
        if (curr == end_idx) { found = 1; break; }

        for (int e = 0; e < g->edge_count; e++) {
            if (strcmp(g->edges[e].from, g->nodes[curr]) == 0) {
                for (int n = 0; n < g->node_count; n++) {
                    if (strcmp(g->nodes[n], g->edges[e].to) == 0 &&
                        !visited[n]) {
                        visited[n]    = 1;
                        parent[n]     = curr;
                        queue[tail++] = n;
                    }
                }
            }
        }
    }

    if (!found) return val_list();

    int path_buf[EPL_GRAPH_MAX_NODES];
    int path_len = 0;
    int cur = end_idx;
    while (cur != -1) {
        path_buf[path_len++] = cur;
        cur = parent[cur];
    }

    Value *result = val_list();
    for (int i = path_len - 1; i >= 0; i--) {
        vallist_push(result->as.list,
            val_string(g->nodes[path_buf[i]]));
    }
    return result;
}

static Value *graph_connected(EPLGraph *g, const char *start) {
    Value *result = val_list();
    if (g->node_count == 0) return result;

    int visited[EPL_GRAPH_MAX_NODES] = {0};
    int queue[EPL_GRAPH_MAX_NODES];
    int head = 0, tail = 0;

    int start_idx = -1;
    for (int i = 0; i < g->node_count; i++) {
        if (strcmp(g->nodes[i], start) == 0) { start_idx = i; break; }
    }
    if (start_idx < 0) return result;

    queue[tail++] = start_idx;
    visited[start_idx] = 1;

    while (head < tail) {
        int curr = queue[head++];
        if (curr != start_idx)
            vallist_push(result->as.list, val_string(g->nodes[curr]));

        for (int e = 0; e < g->edge_count; e++) {
            if (strcmp(g->edges[e].from, g->nodes[curr]) == 0) {
                for (int n = 0; n < g->node_count; n++) {
                    if (strcmp(g->nodes[n], g->edges[e].to) == 0 &&
                        !visited[n]) {
                        visited[n]    = 1;
                        queue[tail++] = n;
                    }
                }
            }
        }
    }
    return result;
}

static void exec_link(Interpreter *interp, ASTNode *node, Env *env) {
    Value *graph_val = env_get(env, node->as.link_stmt.graph);
    if (!graph_val || graph_val->type != VAL_GRAPH) {
        fprintf(stderr,
            "[EPL Error] Line %d: '%s' is not a graph\n",
            node->line, node->as.link_stmt.graph);
        interp->had_error = 1;
        return;
    }
    graph_add_edge(graph_val->as.graph,
        node->as.link_stmt.from,
        node->as.link_stmt.to);
}

static Value *eval_route(Interpreter *interp, ASTNode *node, Env *env) {
    Value *graph_val = env_get(env, node->as.route_stmt.graph);
    if (!graph_val || graph_val->type != VAL_GRAPH) {
        fprintf(stderr,
            "[EPL Error] Line %d: '%s' is not a graph\n",
            node->line, node->as.route_stmt.graph);
        interp->had_error = 1;
        return val_list();
    }
    return graph_bfs(graph_val->as.graph,
        node->as.route_stmt.from,
        node->as.route_stmt.to);
}

static Value *eval_connected(Interpreter *interp, ASTNode *node, Env *env) {
    Value *graph_val = env_get(env, node->as.connected_stmt.graph);
    if (!graph_val || graph_val->type != VAL_GRAPH) {
        fprintf(stderr,
            "[EPL Error] Line %d: '%s' is not a graph\n",
            node->line, node->as.connected_stmt.graph);
        interp->had_error = 1;
        return val_list();
    }
    return graph_connected(graph_val->as.graph,
        node->as.connected_stmt.target);
}

static Value *eval_collection_op(Interpreter *interp,
                                  ASTNode *node, Env *env) {
    Value *collection = eval(interp, node->as.collection_op.collection, env);

    CollectionOp cop = node->as.collection_op.op;

    if (cop == COLLOP_LENGTH) {
        int len = -1;
        if (collection->type == VAL_STRING) len = utf8_length(collection->as.string);
        else if (collection->type == VAL_LIST) len = collection->as.list->count;
        else if (collection->type == VAL_MAP)  len = collection->as.map->count;
        if (len < 0) {
            char msg[256];
            snprintf(msg, sizeof msg, "'size of' needs text, a list or a record, but got %s",
                     val_type_name(collection));
            val_free(collection);
            runtime_error(interp, node, msg);
            return val_null();
        }
        val_free(collection);
        return val_number(len);
    }

    if (cop == COLLOP_KEYS) {
        if (collection->type != VAL_MAP) {
            char msg[256];
            snprintf(msg, sizeof msg, "'keys of' needs a record, but got %s",
                     val_type_name(collection));
            val_free(collection);
            runtime_error(interp, node, msg);
            return val_null();
        }
        Value *keys = val_list();
        for (int i = 0; i < collection->as.map->count; i++)
            vallist_push(keys->as.list, val_string(collection->as.map->keys[i]));
        val_free(collection);
        return keys;
    }

    if (cop == COLLOP_CODE) {
        if (collection->type != VAL_STRING || !collection->as.string[0]) {
            val_free(collection);
            runtime_error(interp, node, "'code of' needs a single character of text, like code of \"A\"");
            return val_null();
        }
        long cp = utf8_decode(collection->as.string);
        val_free(collection);
        return val_number((double)cp);
    }

    if (cop == COLLOP_CHARACTER) {
        double cp;
        if (!num_operand(collection, &cp) || cp < 0 || cp > 0x10FFFF || cp != floor(cp)) {
            val_free(collection);
            runtime_error(interp, node, "'character with code' needs a whole number from 0 to 1114111");
            return val_null();
        }
        val_free(collection);
        char buf[8];
        utf8_encode((long)cp, buf);
        return val_string(buf);
    }

    if (cop == COLLOP_SHA256) {
        char *text = val_to_string(collection);
        char hex[65];
        sha256_hex((const unsigned char *)text, strlen(text), hex);
        free(text);
        val_free(collection);
        return val_string(hex);
    }

    if (cop == COLLOP_RANDOM_TOKEN) {
        double d;
        if (!num_operand(collection, &d) || d < 1 || d > 4096 || d != floor(d)) {
            val_free(collection);
            runtime_error(interp, node, "'random token of N' needs a whole number of characters from 1 to 4096");
            return val_null();
        }
        val_free(collection);
        int n = (int)d;
        unsigned char *raw = malloc((n + 1) / 2);
        char *hex = malloc(n + 2);
        if (random_bytes(raw, (n + 1) / 2) != 0) {
            free(raw); free(hex);
            runtime_error(interp, node, "could not read randomness from the operating system");
            return val_null();
        }
        for (int i = 0; i < (n + 1) / 2; i++) snprintf(hex + i * 2, 3, "%02x", raw[i]);
        hex[n] = '\0';
        Value *v = val_string(hex);
        free(raw); free(hex);
        return v;
    }

    if (!collection || collection->type != VAL_LIST) {
        static const char *names[] = { "average of", "total of", "maximum of",
                                       "minimum of", "size of", "sorted", "reversed" };
        char msg[256];
        snprintf(msg, sizeof msg, "'%s' needs a list, but got %s",
                 (int)cop < 7 ? names[cop] : "this operation", val_type_name(collection));
        val_free(collection);
        runtime_error(interp, node, msg);
        return val_null();
    }

    ValueList *lst = collection->as.list;
    int n = lst->count;

    switch (node->as.collection_op.op) {

        case COLLOP_TOTAL: {
            double sum = 0;
            for (int i = 0; i < n; i++) {
                if (lst->items[i]->type == VAL_NUMBER)
                    sum += lst->items[i]->as.number;
            }
            val_free(collection);
            return val_number(sum);
        }

        case COLLOP_AVERAGE: {
            if (n == 0) { val_free(collection); return val_number(0); }
            double sum = 0;
            for (int i = 0; i < n; i++) {
                if (lst->items[i]->type == VAL_NUMBER)
                    sum += lst->items[i]->as.number;
            }
            val_free(collection);
            return val_number(sum / n);
        }

        case COLLOP_MAXIMUM: {
            if (n == 0) { val_free(collection); return val_null(); }
            double max = lst->items[0]->type == VAL_NUMBER
                         ? lst->items[0]->as.number : -1e300;
            for (int i = 1; i < n; i++) {
                if (lst->items[i]->type == VAL_NUMBER &&
                    lst->items[i]->as.number > max)
                    max = lst->items[i]->as.number;
            }
            val_free(collection);
            return val_number(max);
        }

        case COLLOP_MINIMUM: {
            if (n == 0) { val_free(collection); return val_null(); }
            double min = lst->items[0]->type == VAL_NUMBER
                         ? lst->items[0]->as.number : 1e300;
            for (int i = 1; i < n; i++) {
                if (lst->items[i]->type == VAL_NUMBER &&
                    lst->items[i]->as.number < min)
                    min = lst->items[i]->as.number;
            }
            val_free(collection);
            return val_number(min);
        }

        case COLLOP_SORTED: {
            Value *result = val_copy(collection);
            val_free(collection);
            ValueList *rl = result->as.list;
            for (int i = 0; i < rl->count - 1; i++) {
                for (int j = 0; j < rl->count - 1 - i; j++) {
                    Value *a = rl->items[j];
                    Value *b = rl->items[j + 1];
                    int swap = 0;
                    if (a->type == VAL_NUMBER && b->type == VAL_NUMBER) {
                        swap = a->as.number > b->as.number;
                    } else {
                        char *sa = val_to_string(a);
                        char *sb = val_to_string(b);
                        swap = strcmp(sa, sb) > 0;
                        free(sa); free(sb);
                    }
                    if (swap) { rl->items[j] = b; rl->items[j+1] = a; }
                }
            }
            return result;
        }

        case COLLOP_REVERSED: {
            Value *result = val_copy(collection);
            val_free(collection);
            ValueList *rl = result->as.list;
            for (int i = 0; i < rl->count / 2; i++) {
                Value *tmp = rl->items[i];
                rl->items[i] = rl->items[rl->count - 1 - i];
                rl->items[rl->count - 1 - i] = tmp;
            }
            return result;
        }

        default:
            val_free(collection);
            return val_null();
    }
}

static Value *eval_where_filter(Interpreter *interp,
                                 ASTNode *node, Env *env) {
    if (node->as.where_filter.collection->type == NODE_DB_QUERY) {
        Value *fast = db_try_index_query(interp,
                          node->as.where_filter.collection->as.db_table.table,
                          node->as.where_filter.condition, env);
        if (fast) return fast;
    }

    Value *collection = eval(interp, node->as.where_filter.collection, env);

    if (!collection || collection->type != VAL_LIST) {
        val_free(collection);
        return val_list();
    }

    Value *result = val_list();
    const char *item_var = node->as.where_filter.item_var;

    for (int i = 0; i < collection->as.list->count; i++) {
        Value *item = collection->as.list->items[i];

        Env *filter_env = env_new(env);
        env_set(filter_env, item_var, val_copy(item));

        if (item->type == VAL_MAP)
            db_bind_record_fields(filter_env, item);

        if (item->type == VAL_MAP) interp->record_scope++;
        Value *passes = eval(interp, node->as.where_filter.condition, filter_env);
        if (item->type == VAL_MAP) interp->record_scope--;
        int keep = val_is_truthy(passes);
        val_free(passes);
        env_free(filter_env);

        if (keep)
            vallist_push(result->as.list, val_copy(item));
    }

    val_free(collection);
    return result;
}

static Value *eval_string_op(Interpreter *interp,
                              ASTNode *node, Env *env) {
    Value *target = eval(interp, node->as.string_op.target, env);
    Value *arg1   = node->as.string_op.arg1
                    ? eval(interp, node->as.string_op.arg1, env) : NULL;
    Value *arg2   = node->as.string_op.arg2
                    ? eval(interp, node->as.string_op.arg2, env) : NULL;

    Value *result = NULL;

    switch (node->as.string_op.op) {

        case STROP_UPPERCASE: {
            char *s = val_to_string(target);
            for (int i = 0; s[i]; i++) s[i] = toupper((unsigned char)s[i]);
            result = val_string(s);
            free(s);
            break;
        }

        case STROP_LOWERCASE: {
            char *s = val_to_string(target);
            for (int i = 0; s[i]; i++) s[i] = tolower((unsigned char)s[i]);
            result = val_string(s);
            free(s);
            break;
        }

        case STROP_TRIMMED: {
            char *s = val_to_string(target);
            char *start = s;
            while (*start && isspace((unsigned char)*start)) start++;
            char *end = start + strlen(start) - 1;
            while (end > start && isspace((unsigned char)*end)) end--;
            *(end + 1) = '\0';
            result = val_string(start);
            free(s);
            break;
        }

        case STROP_SPLIT: {
            char *s   = val_to_string(target);
            char *delim = arg1 ? val_to_string(arg1) : strdup(" ");
            result = val_list();
            int dlen = strlen(delim);
            if (dlen == 0) {
                for (char *c = s; *c; ) {
                    int n = 1;
                    while (c[n] && ((unsigned char)c[n] & 0xC0) == 0x80) n++;
                    char ch[8] = {0};
                    memcpy(ch, c, n < 7 ? n : 7);
                    vallist_push(result->as.list, val_string(ch));
                    c += n;
                }
                free(s); free(delim);
                break;
            }
            char *pos = s;
            char *found;
            while ((found = strstr(pos, delim)) != NULL) {
                int seglen = found - pos;
                char *seg  = malloc(seglen + 1);
                strncpy(seg, pos, seglen);
                seg[seglen] = '\0';
                vallist_push(result->as.list, val_string(seg));
                free(seg);
                pos = found + dlen;
            }
            vallist_push(result->as.list, val_string(pos));
            free(s); free(delim);
            break;
        }

        case STROP_JOINED: {
            char *delim = arg1 ? val_to_string(arg1) : strdup("");
            if (!target || target->type != VAL_LIST) {
                result = val_string("");
                free(delim);
                break;
            }
            int total = 0;
            int dlen  = strlen(delim);
            for (int i = 0; i < target->as.list->count; i++) {
                char *s = val_to_string(target->as.list->items[i]);
                total += strlen(s) + dlen;
                free(s);
            }
            char *buf = malloc(total + 1);
            buf[0] = '\0';
            for (int i = 0; i < target->as.list->count; i++) {
                char *s = val_to_string(target->as.list->items[i]);
                strcat(buf, s);
                free(s);
                if (i < target->as.list->count - 1) strcat(buf, delim);
            }
            result = val_string(buf);
            free(buf); free(delim);
            break;
        }

        case STROP_REPLACED: {
            char *s       = val_to_string(target);
            char *search  = arg1 ? val_to_string(arg1) : strdup("");
            char *replace = arg2 ? val_to_string(arg2) : strdup("");
            int slen = strlen(search);
            int rlen = strlen(replace);

            if (slen == 0) {
                result = val_string(s);
                free(s); free(search); free(replace);
                break;
            }

            int count = 0;
            char *pos = s;
            while ((pos = strstr(pos, search)) != NULL) { count++; pos += slen; }

            char *out = malloc(strlen(s) + count * (rlen - slen) + 1);
            char *w   = out;
            pos = s;
            char *found;
            while ((found = strstr(pos, search)) != NULL) {
                int before = found - pos;
                memcpy(w, pos, before); w += before;
                memcpy(w, replace, rlen); w += rlen;
                pos = found + slen;
            }
            strcpy(w, pos);
            result = val_string(out);
            free(s); free(search); free(replace); free(out);
            break;
        }

        case STROP_STARTS_WITH: {
            char *s      = val_to_string(target);
            char *prefix = arg1 ? val_to_string(arg1) : strdup("");
            result = val_boolean(strncmp(s, prefix, strlen(prefix)) == 0);
            free(s); free(prefix);
            break;
        }

        case STROP_ENDS_WITH: {
            char *s      = val_to_string(target);
            char *suffix = arg1 ? val_to_string(arg1) : strdup("");
            int slen = strlen(s);
            int elen = strlen(suffix);
            int matches = (slen >= elen) &&
                          (strcmp(s + slen - elen, suffix) == 0);
            result = val_boolean(matches);
            free(s); free(suffix);
            break;
        }

        case STROP_CONTAINS: {
            char *s      = val_to_string(target);
            char *needle = arg1 ? val_to_string(arg1) : strdup("");
            result = val_boolean(strstr(s, needle) != NULL);
            free(s); free(needle);
            break;
        }

        default:
            result = val_null();
            break;
    }

    val_free(target);
    if (arg1) val_free(arg1);
    if (arg2) val_free(arg2);
    return result ? result : val_null();
}

static Value *eval_format_op(Interpreter *interp,
                              ASTNode *node, Env *env) {
    Value *target = eval(interp, node->as.format_op.target, env);
    double num    = 0.0;
    if (!num_operand(target, &num)) {
        char msg[256];
        if (target && target->type == VAL_STRING)
            snprintf(msg, sizeof msg, "can only round or format a number, but got text \"%.60s\"",
                     target->as.string);
        else
            snprintf(msg, sizeof msg, "can only round or format a number, but got %s",
                     val_type_name(target));
        val_free(target);
        runtime_error(interp, node, msg);
        return val_null();
    }
    char buf[128];
    Value *result = NULL;

    switch (node->as.format_op.op) {

        case FMTOP_CURRENCY: {
            snprintf(buf, sizeof(buf), "$%.2f", num);
            result = val_string(buf);
            break;
        }

        case FMTOP_PERCENTAGE: {
            snprintf(buf, sizeof(buf), "%.4g%%", num);
            result = val_string(buf);
            break;
        }

        case FMTOP_ROUNDED: {
            int places = 0;
            if (node->as.format_op.precision) {
                Value *p = eval(interp, node->as.format_op.precision, env);
                places   = (p && p->type == VAL_NUMBER) ? (int)p->as.number : 0;
                val_free(p);
            }
            double factor = pow(10.0, places);
            double rounded = round(num * factor) / factor;
            if (places == 0)
                snprintf(buf, sizeof(buf), "%ld", (long)rounded);
            else
                snprintf(buf, sizeof(buf), "%.*f", places, rounded);

            result = val_string(buf);
            break;
        }

        case FMTOP_FORMATTED: {
            long whole = (long)num;
            char tmp[64];
            snprintf(tmp, sizeof(tmp), "%ld", whole < 0 ? -whole : whole);
            int len    = strlen(tmp);
            int commas = (len - 1) / 3;
            char *out  = malloc(len + commas + 3);
            int wi = 0, ti = 0;
            if (whole < 0) out[wi++] = '-';
            int first = len % 3;
            if (first == 0) first = 3;
            for (int i = 0; i < first; i++) out[wi++] = tmp[ti++];
            while (ti < len) {
                out[wi++] = ',';
                for (int i = 0; i < 3; i++) out[wi++] = tmp[ti++];
            }
            out[wi] = '\0';
            result = val_string(out);
            free(out);
            break;
        }

        default:
            result = val_null();
            break;
    }

    val_free(target);
    return result ? result : val_null();
}

static Value *eval_type_check(Interpreter *interp,
                               ASTNode *node, Env *env) {
    Value *target = eval(interp, node->as.type_check.target, env);
    int result    = 0;

    switch (node->as.type_check.kind) {
        case TYPECHECK_NUMBER:  result = target && target->type == VAL_NUMBER;  break;
        case TYPECHECK_STRING:  result = target && target->type == VAL_STRING;  break;
        case TYPECHECK_BOOLEAN: result = target && target->type == VAL_BOOLEAN; break;
        case TYPECHECK_LIST:    result = target && target->type == VAL_LIST;    break;
        case TYPECHECK_NULL:    result = !target || target->type == VAL_NULL;   break;
        case TYPECHECK_EMPTY:
            result = !target || target->type == VAL_NULL
                  || (target->type == VAL_STRING && target->as.string[0] == '\0')
                  || (target->type == VAL_LIST   && target->as.list->count == 0)
                  || (target->type == VAL_MAP    && target->as.map->count == 0);
            break;
    }

    val_free(target);
    return val_boolean(result);
}
Value *eval(Interpreter *interp, ASTNode *node, Env *env) {
    if (!node) return val_null();
    if (interp->signal.type != SIG_NONE &&
        interp->signal.type != SIG_RETURN) return val_null();

    switch (node->type) {

        case NODE_NUMBER:
            return val_number(node->as.number.value);

        case NODE_STRING:
            return val_string(node->as.string.value);

        case NODE_BOOLEAN:
            return val_boolean(node->as.boolean.value);

        case NODE_NULL:
            return val_null();

        case NODE_IDENTIFIER: {
            Value *v = env_get(env, node->as.identifier.name);
            if (!v) {
                if (interp->record_scope == 0)
                    unknown_name_error(interp, node, env, node->as.identifier.name);
                return val_null();
            }
            if (v->type == VAL_ACTIVITY) {
                ValueList args;
                vallist_init(&args);
                Value *result = call_activity(interp,
                    node->as.identifier.name, &args, env, node);
                vallist_free(&args);
                return result;
            }
            return val_copy(v);
        }

        case NODE_ACTIVITY_CALL: {
            ValueList args;
            vallist_init(&args);
            for (int i = 0; i < node->as.activity_call.args.count; i++) {
                Value *arg = eval(interp,
                    node->as.activity_call.args.items[i], env);
                vallist_push(&args, arg);
            }
            Value *result = call_activity(interp,
                node->as.activity_call.name, &args, env, node);
            vallist_free(&args);
            return result;
        }

        case NODE_LIST: {
            Value *list = val_list();
            for (int i = 0; i < node->as.list.elements.count; i++) {
                Value *elem = eval(interp,
                    node->as.list.elements.items[i], env);
                vallist_push(list->as.list, elem);
            }
            return list;
        }

        case NODE_INLINE_IF: {
            Value *cond = eval(interp, node->as.inline_if.condition, env);
            int truthy  = val_is_truthy(cond);
            val_free(cond);
            if (truthy)
                return eval(interp, node->as.inline_if.then_val, env);
            else
                return eval(interp, node->as.inline_if.else_val, env);
        }

        case NODE_MAKE_GRAPH:
            return val_graph();

        case NODE_ROUTE:
            return eval_route(interp, node, env);

        case NODE_CONNECTED:
            return eval_connected(interp, node, env);

        case NODE_COLLECTION_OP:
            return eval_collection_op(interp, node, env);

        case NODE_WHERE_FILTER:
            return eval_where_filter(interp, node, env);

        case NODE_DB_QUERY: {
            char *table = db_table_name(interp, node->as.db_table.table, env);
            if (!table) { runtime_error(interp, node, "records in: table name must be a string"); return val_list(); }
            Value *rows = db_load_all(interp, table);
            free(table);
            return rows;
        }
        case NODE_DB_COUNT: {
            if (node->as.db_filter.condition) {
                Value *hits = db_try_index_query(interp,
                                  node->as.db_filter.table,
                                  node->as.db_filter.condition, env);
                if (hits) {
                    double n = hits->as.list->count;
                    val_free(hits);
                    return val_number(n);
                }
            }

            char *table = db_table_name(interp, node->as.db_filter.table, env);
            if (!table) { runtime_error(interp, node, "count records in: table name must be a string"); return val_number(0); }
            Value *rows = db_load_all(interp, table);
            free(table);

            double c;
            if (!node->as.db_filter.condition) {
                c = rows->as.list->count;
            } else {
                c = 0;
                for (int i = 0; i < rows->as.list->count; i++) {
                    Value *rec = rows->as.list->items[i];
                    Env *fe = env_new(env);
                    env_set(fe, node->as.db_filter.item_var, val_copy(rec));
                    db_bind_record_fields(fe, rec);
                    interp->record_scope++;
                    Value *passes = eval(interp, node->as.db_filter.condition, fe);
                    interp->record_scope--;
                    if (val_is_truthy(passes)) c++;
                    val_free(passes);
                    env_free(fe);
                }
            }
            val_free(rows);
            return val_number(c);
        }

        case NODE_INDEX_ACCESS: {
            Value *list  = eval(interp, node->as.index_access.list,  env);
            Value *idx_v = eval(interp, node->as.index_access.index, env);
            double d;
            char msg[256];
            if (!num_operand(idx_v, &d) || d != floor(d)) {
                snprintf(msg, sizeof msg, "'item N of' needs a whole number for N, but got %s",
                         val_type_name(idx_v));
                val_free(list); val_free(idx_v);
                runtime_error(interp, node, msg);
                return val_null();
            }
            val_free(idx_v);
            int idx = (int)d;
            int count = list->type == VAL_LIST   ? list->as.list->count
                      : list->type == VAL_STRING ? utf8_length(list->as.string) : -1;
            if (count < 0) {
                snprintf(msg, sizeof msg, "'item N of' needs a list or text, but got %s",
                         val_type_name(list));
                val_free(list);
                runtime_error(interp, node, msg);
                return val_null();
            }
            if (idx < 0 || idx >= count) {
                snprintf(msg, sizeof msg, "there is no item %d: it has %d item%s, numbered 0 to %d",
                         idx, count, count == 1 ? "" : "s", count - 1);
                val_free(list);
                runtime_error(interp, node, msg);
                return val_null();
            }
            Value *result;
            if (list->type == VAL_LIST) {
                result = val_copy(list->as.list->items[idx]);
            } else {
                char *ch = utf8_char_at(list->as.string, idx);
                result = val_string(ch);
                free(ch);
            }
            val_free(list);
            return result;
        }

        case NODE_STRING_OP:
            return eval_string_op(interp, node, env);

        case NODE_FORMAT_OP:
            return eval_format_op(interp, node, env);

        case NODE_TYPE_CHECK:
            return eval_type_check(interp, node, env);

        case NODE_UNARY_OP: {
            Value *operand = eval(interp, node->as.unary_op.operand, env);
            if (node->as.unary_op.operator == TOKEN_NOT) {
                int result = !val_is_truthy(operand);
                val_free(operand);
                return val_boolean(result);
            }
            if (node->as.unary_op.operator == TOKEN_MINUS) {
                if (operand->type != VAL_NUMBER) {
                    val_free(operand);
                    runtime_error(interp, node, "cannot negate a value that is not a number");
                    return val_null();
                }
                double n = operand->as.number;
                val_free(operand);
                return val_number(-n);
            }
            return operand;
        }

        case NODE_BINARY_OP: {
            TokenType op = node->as.binary_op.operator;

            if (op == TOKEN_AND) {
                Value *left = eval(interp, node->as.binary_op.left, env);
                if (!val_is_truthy(left)) return left;
                val_free(left);
                return eval(interp, node->as.binary_op.right, env);
            }
            if (op == TOKEN_OR) {
                Value *left = eval(interp, node->as.binary_op.left, env);
                if (val_is_truthy(left)) return left;
                val_free(left);
                return eval(interp, node->as.binary_op.right, env);
            }

            Value *left  = eval(interp, node->as.binary_op.left,  env);
            Value *right = eval(interp, node->as.binary_op.right, env);
            Value *result = NULL;

            if ((op == TOKEN_IS_ANY_OF || op == TOKEN_IS_NONE_OF) &&
                right->type == VAL_LIST && right->as.list->count == 1 &&
                right->as.list->items[0]->type == VAL_LIST) {
                Value *inner = val_copy(right->as.list->items[0]);
                val_free(right);
                right = inner;
            }
            double na = 0, nb = 0;
            int cmp_ok = 0, cmp = 0;

            switch (op) {

                case TOKEN_PLUS:
                    if (left->type == VAL_STRING || right->type == VAL_STRING) {
                        char *ls = val_to_string(left);
                        char *rs = val_to_string(right);
                        char *cat = malloc(strlen(ls) + strlen(rs) + 1);
                        strcpy(cat, ls); strcat(cat, rs);
                        result = val_string(cat);
                        free(ls); free(rs); free(cat);
                    } else if (!num_operand(left, &na) || !num_operand(right, &nb)) {
                        arith_type_error(interp, node, "plus", left, right);
                        result = val_null();
                    } else {
                        result = val_number(na + nb);
                    }
                    break;

                case TOKEN_MINUS:
                    if (!num_operand(left, &na) || !num_operand(right, &nb)) {
                        arith_type_error(interp, node, "minus", left, right);
                        result = val_null();
                    } else result = val_number(na - nb);
                    break;

                case TOKEN_TIMES:
                case TOKEN_TIMES_OP:
                    if (!num_operand(left, &na) || !num_operand(right, &nb)) {
                        arith_type_error(interp, node, "times", left, right);
                        result = val_null();
                    } else result = val_number(na * nb);
                    break;

                case TOKEN_DIVIDED_BY:
                    if (!num_operand(left, &na) || !num_operand(right, &nb)) {
                        arith_type_error(interp, node, "divided by", left, right);
                        result = val_null();
                    } else if (nb == 0) {
                        runtime_error(interp, node, "Division by zero");
                        result = val_string("Error: Division by zero");
                    } else {
                        result = val_number(na / nb);
                    }
                    break;

                case TOKEN_MODULO:
                    if (!num_operand(left, &na) || !num_operand(right, &nb)) {
                        arith_type_error(interp, node, "modulo", left, right);
                        result = val_null();
                    } else if (nb == 0) {
                        runtime_error(interp, node, "Modulo by zero");
                        result = val_null();
                    } else result = val_number(fmod(na, nb));
                    break;

                case TOKEN_RANDOM_NUMBER: {
                    if (!num_operand(left, &na) || !num_operand(right, &nb) ||
                        na != floor(na) || nb != floor(nb) || nb < na) {
                        runtime_error(interp, node, "'random number from A to B' needs whole numbers with A at most B");
                        result = val_null();
                        break;
                    }
                    unsigned long long r = 0;
                    if (random_bytes((unsigned char *)&r, sizeof r) != 0) {
                        runtime_error(interp, node, "could not read randomness from the operating system");
                        result = val_null();
                        break;
                    }
                    double span = nb - na + 1;
                    result = val_number(na + (double)(r % (unsigned long long)span));
                    break;
                }

                case TOKEN_POWER_OF:
                    if (!num_operand(left, &na) || !num_operand(right, &nb)) {
                        arith_type_error(interp, node, "power of", left, right);
                        result = val_null();
                    } else result = val_number(pow(na, nb));
                    break;

                case TOKEN_EQUALS:
                case TOKEN_IS:
                    result = val_boolean(val_equals(left, right));
                    break;

                case TOKEN_IS_NOT:
                case TOKEN_NOT_EQUALS:
                    result = val_boolean(!val_equals(left, right));
                    break;

                case TOKEN_GREATER_THAN:
                    cmp = compare_values(left, right, &cmp_ok);
                    result = val_boolean(cmp_ok && cmp > 0);
                    break;

                case TOKEN_LESS_THAN:
                    cmp = compare_values(left, right, &cmp_ok);
                    result = val_boolean(cmp_ok && cmp < 0);
                    break;

                case TOKEN_AT_LEAST:
                    cmp = compare_values(left, right, &cmp_ok);
                    result = val_boolean(cmp_ok && cmp >= 0);
                    break;

                case TOKEN_AT_MOST:
                    cmp = compare_values(left, right, &cmp_ok);
                    result = val_boolean(cmp_ok && cmp <= 0);
                    break;

                case TOKEN_IS_ANY_OF: {
                    int found = 0;
                    if (right->type == VAL_LIST) {
                        for (int i = 0; i < right->as.list->count; i++) {
                            if (val_equals(left, right->as.list->items[i])) {
                                found = 1; break;
                            }
                        }
                    }
                    result = val_boolean(found);
                    break;
                }

                case TOKEN_IS_NONE_OF: {
                    int found = 0;
                    if (right->type == VAL_LIST) {
                        for (int i = 0; i < right->as.list->count; i++) {
                            if (val_equals(left, right->as.list->items[i])) {
                                found = 1; break;
                            }
                        }
                    }
                    result = val_boolean(!found);
                    break;
                }

                case TOKEN_IN: {
                    int found = 0;
                    if (right->type == VAL_LIST) {
                        for (int i = 0; i < right->as.list->count; i++) {
                            if (val_equals(left, right->as.list->items[i])) {
                                found = 1; break;
                            }
                        }
                    }
                    result = val_boolean(found);
                    break;
                }

                default:
                    result = val_null();
                    break;
            }

            val_free(left);
            val_free(right);
            return result ? result : val_null();
        }

        case NODE_READ_FILE: {
            Value *path_val = eval(interp, node->as.file_op.path_expr, env);
            char *path = val_to_string(path_val);
            char actual_path[1024];
            perm_expand_home(path, actual_path, sizeof actual_path);
            val_free(path_val); free(path);
            if (!PERMIT(perm_check_read, actual_path)) return val_string("");

            FILE *f = fopen(actual_path, "r");
            if (!f) {
                char msg[1152];
                snprintf(msg, sizeof(msg), "cannot read file '%s': %s",
                         actual_path, strerror(errno));
                runtime_error(interp, node, msg);
                return val_string("");
            }
            fseek(f, 0, SEEK_END);
            long fsize = ftell(f);
            fseek(f, 0, SEEK_SET);
            char *buf = malloc(fsize + 1);
            size_t read_bytes = fread(buf, 1, fsize, f);
            buf[read_bytes] = '\0';
            fclose(f);
            Value *res = val_string(buf);
            free(buf);
            return res;
        }

        case NODE_FILE_EXISTS: {
            Value *path_val = eval(interp, node->as.file_op.path_expr, env);
            char *path = val_to_string(path_val);
            char actual_path[1024];
            perm_expand_home(path, actual_path, sizeof actual_path);
            val_free(path_val); free(path);
            if (!PERMIT(perm_check_read, actual_path)) return val_boolean(0);

            int exists = (access(actual_path, F_OK) == 0);
            return val_boolean(exists);
        }

        case NODE_RUN_CMD: {
            Value *cmd_val = eval(interp, node->as.run_cmd.cmd_expr, env);
            char *cmd = val_to_string(cmd_val);
            val_free(cmd_val);
            if (!PERMIT(perm_check_command, cmd)) { free(cmd); return val_string(""); }

            FILE *f = popen(cmd, "r");
            if (!f) {
                char msg[512];
                snprintf(msg, sizeof(msg), "cannot run command '%.400s'", cmd);
                runtime_error(interp, node, msg);
                free(cmd);
                return val_string("");
            }

            char buf[4096] = {0};
            size_t len = fread(buf, 1, sizeof(buf) - 1, f);
            int status = pclose(f);
            if (len > 0 && buf[len - 1] == '\n') buf[len - 1] = '\0';

            if (status != 0) {
                char msg[512];
                snprintf(msg, sizeof(msg),
                         "command '%.380s' exited with status %d",
                         cmd, WIFEXITED(status) ? WEXITSTATUS(status) : status);
                runtime_error(interp, node, msg);
            }
            free(cmd);
            return val_string(buf);
        }

        case NODE_GET_ENV: {
            Value *var_val = eval(interp, node->as.get_env.var_expr, env);
            char *var = val_to_string(var_val);
            val_free(var_val);
            if (!PERMIT(perm_check_env, var)) { free(var); return val_null(); }

            char *env_val = getenv(var);
            free(var);
            if (!env_val) return val_null();
            return val_string(env_val);
        }

        case NODE_ERROR_MSG:
            return val_string(interp->last_error);

        case NODE_NOW: {
            time_t t = time(NULL);
            struct tm *tm = localtime(&t);
            char buf[64];
            strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%S", tm);
            return val_string(buf);
        }

        case NODE_SEND_SOCKET: {
            Value *json_val = eval(interp, node->as.send_socket.json_expr, env);
            Value *path_val = eval(interp, node->as.send_socket.path_expr, env);
            char *json = val_to_string(json_val);
            char *path = val_to_string(path_val);
            val_free(json_val); val_free(path_val);
            if (!PERMIT(perm_check_socket, path)) {
                free(json); free(path);
                return val_string("");
            }

            int sock = socket(AF_UNIX, SOCK_STREAM, 0);
            struct sockaddr_un addr;
            memset(&addr, 0, sizeof(addr));
            addr.sun_family = AF_UNIX;
            strncpy(addr.sun_path, path, sizeof(addr.sun_path) - 1);

            Value *res = val_string("");
            char msg[1152];
            if (sock < 0) {
                snprintf(msg, sizeof(msg), "cannot create socket: %s",
                         strerror(errno));
                runtime_error(interp, node, msg);
            } else if (connect(sock, (struct sockaddr*)&addr, sizeof(addr)) != 0) {
                snprintf(msg, sizeof(msg), "cannot connect to socket '%s': %s",
                         path, strerror(errno));
                runtime_error(interp, node, msg);
            } else {
                dprintf(sock, "%s\n", json);
                char buf[8192] = {0};
                int n = read(sock, buf, sizeof(buf) - 1);
                if (n > 0) {
                    if (buf[n - 1] == '\n') buf[n - 1] = '\0';
                    val_free(res);
                    res = val_string(buf);
                } else if (n < 0) {
                    snprintf(msg, sizeof(msg),
                             "no response from socket '%s': %s",
                             path, strerror(errno));
                    runtime_error(interp, node, msg);
                }
            }
            if (sock >= 0) close(sock);
            free(json); free(path);
            return res;
        }

        case NODE_POST_HTTP: {
            Value *json_val = eval(interp, node->as.post_http.json_expr, env);
            Value *url_val = eval(interp, node->as.post_http.url_expr, env);
            char *json = val_to_string(json_val);
            char *url = val_to_string(url_val);
            val_free(json_val); val_free(url_val);

            char host[256] = {0};
            char path[512] = {0};
            int port = 80;
            char msg[1152];

            if (strncmp(url, "https://", 8) == 0) {
                snprintf(msg, sizeof(msg),
                    "https is not supported (no TLS in the EPL runtime): '%.200s'",
                    url);
                runtime_error(interp, node, msg);
                free(json); free(url);
                return val_string("");
            }
            if (strncmp(url, "http://", 7) != 0) {
                snprintf(msg, sizeof(msg),
                    "malformed URL '%.200s' (expected http://host[:port]/path)",
                    url);
                runtime_error(interp, node, msg);
                free(json); free(url);
                return val_string("");
            }
            {
                char *p = url + 7;
                char *colon = strchr(p, ':');
                char *slash = strchr(p, '/');
                if (colon && (!slash || colon < slash)) {
                    strncpy(host, p, (size_t)(colon - p) < sizeof(host) - 1
                                     ? (size_t)(colon - p) : sizeof(host) - 1);
                    port = atoi(colon + 1);
                } else if (slash) {
                    strncpy(host, p, (size_t)(slash - p) < sizeof(host) - 1
                                     ? (size_t)(slash - p) : sizeof(host) - 1);
                } else {
                    strncpy(host, p, sizeof(host) - 1);
                }
                if (slash) snprintf(path, sizeof(path), "%s", slash);
                else       strcpy(path, "/");
            }

            if (!permit_(interp, node, perm_check_network(perms_of(interp), host, port,
                                                          perm_msg, sizeof perm_msg))) {
                free(json); free(url);
                return val_string("");
            }

            struct hostent *server = gethostbyname(host);
            if (!server) {
                snprintf(msg, sizeof(msg), "cannot resolve host '%s'", host);
                runtime_error(interp, node, msg);
                free(json); free(url);
                return val_string("");
            }

            int sock = socket(AF_INET, SOCK_STREAM, 0);
            if (sock < 0) {
                snprintf(msg, sizeof(msg), "cannot create socket: %s",
                         strerror(errno));
                runtime_error(interp, node, msg);
                free(json); free(url);
                return val_string("");
            }

            struct sockaddr_in serv_addr;
            memset((char *)&serv_addr, 0, sizeof(serv_addr));
            serv_addr.sin_family = AF_INET;
            memcpy((char *)&serv_addr.sin_addr.s_addr,
                   (char *)server->h_addr_list[0], server->h_length);
            serv_addr.sin_port = htons(port);

            Value *res = val_string("");
            if (connect(sock, (struct sockaddr*)&serv_addr,
                        sizeof(serv_addr)) != 0) {
                snprintf(msg, sizeof(msg), "cannot connect to %s:%d: %s",
                         host, port, strerror(errno));
                runtime_error(interp, node, msg);
            } else {
                char header[1024];
                snprintf(header, sizeof(header),
                    "POST %s HTTP/1.1\r\n"
                    "Host: %s\r\n"
                    "Content-Type: application/json\r\n"
                    "Content-Length: %zu\r\n"
                    "Connection: close\r\n\r\n", path, host, strlen(json));
                write(sock, header, strlen(header));
                write(sock, json, strlen(json));

                char buf[16384] = {0};
                int total = 0;
                int n;
                while (total < (int)sizeof(buf) - 1 &&
                       (n = read(sock, buf + total,
                                 sizeof(buf) - total - 1)) > 0) {
                    total += n;
                }
                buf[total] = '\0';

                int status = 0;
                if (strncmp(buf, "HTTP/1.", 7) == 0)
                    status = atoi(buf + 9);

                char *body = strstr(buf, "\r\n\r\n");
                val_free(res);
                res = val_string(body ? body + 4 : "");

                if (status && (status < 200 || status >= 300)) {
                    snprintf(msg, sizeof(msg),
                             "HTTP %d from %s%s", status, host, path);
                    runtime_error(interp, node, msg);
                }
            }
            close(sock);
            free(json); free(url);
            return res;
        }

        case NODE_JSON_FIELD: {
            Value *key_val  = eval(interp, node->as.json_field.key_expr, env);
            Value *json_val = eval(interp, node->as.json_field.json_expr, env);
            char  *key      = val_to_string(key_val);

            Value *doc      = NULL;
            Value *root     = NULL;

            if (json_val && (json_val->type == VAL_MAP ||
                             json_val->type == VAL_LIST)) {
                root = json_val;
            } else {
                char *raw = val_to_string(json_val);
                char  perr[256];
                doc = json_parse(raw, perr, sizeof(perr));
                if (!doc) {
                    char msg[512];
                    snprintf(msg, sizeof(msg), "malformed JSON: %s", perr);
                    runtime_error(interp, node, msg);
                    free(raw); free(key);
                    val_free(key_val); val_free(json_val);
                    return val_null();
                }
                free(raw);
                root = doc;
            }

            Value *found = json_path_get(root, key);
            if (!found) {
                char msg[512];
                snprintf(msg, sizeof(msg), "no field '%.200s' in JSON value", key);
                runtime_error(interp, node, msg);
            }
            Value *res = found ? val_copy(found) : val_null();

            if (doc) val_free(doc);
            free(key);
            val_free(key_val); val_free(json_val);
            return res;
        }

        case NODE_PARSE_JSON: {
            Value *sv  = eval(interp, node->as.json_convert.value_expr, env);
            char  *raw = val_to_string(sv);
            char   perr[256];
            Value *doc = json_parse(raw, perr, sizeof(perr));
            if (!doc) {
                char msg[512];
                snprintf(msg, sizeof(msg), "cannot parse JSON: %s", perr);
                runtime_error(interp, node, msg);
                doc = val_null();
            }
            free(raw); val_free(sv);
            return doc;
        }

        case NODE_TO_JSON: {
            Value *v = eval(interp, node->as.json_convert.value_expr, env);
            char *s  = json_serialize(v);
            Value *res = val_string(s);
            free(s); val_free(v);
            return res;
        }

        default:
            return val_null();
    }
}

static Value *call_activity(Interpreter *interp, const char *name,
                             ValueList *args, Env *env, ASTNode *site) {
    char msg[512];

    if (interp->call_depth >= MAX_CALL_DEPTH) {
        runtime_error(interp, site, "maximum call depth exceeded");
        return val_null();
    }

    Value *activity_val = env_get(env, name);
    if (!activity_val || activity_val->type != VAL_ACTIVITY) {
        snprintf(msg, sizeof(msg), "unknown activity '%s'", name);
        runtime_error(interp, site, msg);
        return val_null();
    }

    ASTNode *def   = activity_val->as.activity;
    Env     *local = env_new(env);

    NodeList *params = &def->as.activity_def.params;
    if (args->count != params->count) {
        snprintf(msg, sizeof(msg),
                 "activity '%s' expects %d argument%s but received %d",
                 name, params->count, params->count == 1 ? "" : "s",
                 args->count);
        runtime_error(interp, site, msg);
        env_free(local);
        return val_null();
    }
    for (int i = 0; i < params->count; i++) {
        env_set(local, params->items[i]->as.identifier.name, args->items[i]);
        args->items[i] = NULL;
    }

    interp->call_depth++;
    exec_block(interp, &def->as.activity_def.body, local);
    interp->call_depth--;

    Value *result = val_null();
    if (interp->signal.type == SIG_RETURN) {
        result = interp->signal.value
                    ? interp->signal.value : val_null();
        interp->signal.type  = SIG_NONE;
        interp->signal.value = NULL;
    }

    env_free(local);
    return result;
}

#define HTTP_MAX_HEAD 16384
#define HTTP_MAX_BODY (1 << 20)

static void http_write_all(int fd, const char *buf, size_t len) {
    while (len > 0) {
        ssize_t n = send(fd, buf, len, MSG_NOSIGNAL);
        if (n <= 0) return;
        buf += n; len -= (size_t)n;
    }
}

static void http_send(int fd, int status, const char *type,
                      const char *body, size_t len) {
    const char *reason =
        status == 200 ? "OK"          : status == 400 ? "Bad Request" :
        status == 403 ? "Forbidden"   : status == 404 ? "Not Found"   :
        status == 405 ? "Method Not Allowed" :
        status == 413 ? "Payload Too Large"  :
        status == 415 ? "Unsupported Media Type" : "Internal Server Error";
    char head[512];
    int n = snprintf(head, sizeof head,
        "HTTP/1.1 %d %s\r\nContent-Type: %s\r\nContent-Length: %zu\r\n"
        "Cache-Control: no-store\r\nX-Content-Type-Options: nosniff\r\n"
        "Connection: close\r\n\r\n", status, reason, type, len);
    http_write_all(fd, head, (size_t)n);
    http_write_all(fd, body, len);
}

static void http_json(int fd, int status, int ok, const char *key, Value *v) {
    Value *m = val_map();
    valmap_set(m->as.map, "ok", val_boolean(ok));
    valmap_set(m->as.map, key, v ? val_copy(v) : val_null());
    char *s = json_serialize(m);
    http_send(fd, status, "application/json", s, strlen(s));
    free(s);
    val_free(m);
}

static void http_error(int fd, int status, const char *msg) {
    Value *v = val_string(msg);
    http_json(fd, status, 0, "error", v);
    val_free(v);
}

static void http_header(const char *head, const char *name, char *out, size_t n) {
    out[0] = '\0';
    size_t nl = strlen(name);
    for (const char *l = strstr(head, "\r\n"); l; l = strstr(l, "\r\n")) {
        l += 2;
        if (strncasecmp(l, name, nl) == 0 && l[nl] == ':') {
            const char *v = l + nl + 1;
            while (*v == ' ' || *v == '\t') v++;
            size_t k = 0;
            while (v[k] && v[k] != '\r' && k < n - 1) { out[k] = v[k]; k++; }
            out[k] = '\0';
            return;
        }
    }
}

static void http_url_decode(const char *in, char *out, size_t n) {
    size_t k = 0;
    for (size_t i = 0; in[i] && k < n - 1; i++) {
        if (in[i] == '%' && isxdigit((unsigned char)in[i+1]) &&
            isxdigit((unsigned char)in[i+2])) {
            char hex[3] = { in[i+1], in[i+2], 0 };
            out[k++] = (char)strtol(hex, NULL, 16);
            i += 2;
        } else {
            out[k++] = in[i];
        }
    }
    out[k] = '\0';
}

static int http_rebuild_page(const char *source) {
    char exe[4096];
    ssize_t n = readlink("/proc/self/exe", exe, sizeof exe - 1);
    if (n <= 0) return -1;
    exe[n] = '\0';
    char *gone = strstr(exe, " (deleted)");
    if (gone && gone[10] == '\0') *gone = '\0';
    fflush(stdout);
    pid_t pid = fork();
    if (pid < 0) return -1;
    if (pid == 0) {
        int dn = open("/dev/null", O_WRONLY);
        if (dn >= 0) { dup2(dn, STDOUT_FILENO); close(dn); }
        execl(exe, exe, source, (char *)NULL);
        _exit(127);
    }
    int st = 0;
    if (waitpid(pid, &st, 0) < 0) return -1;
    return (WIFEXITED(st) && WEXITSTATUS(st) == 0) ? 0 : -1;
}

static void http_page(int fd, const char *target) {
    static const struct { const char *ext, *type; } TYPES[] = {
        { ".html", "text/html; charset=utf-8" }, { ".css", "text/css" },
        { ".js", "text/javascript" }, { ".png", "image/png" },
        { ".jpg", "image/jpeg" }, { ".jpeg", "image/jpeg" },
        { ".svg", "image/svg+xml" }, { ".webp", "image/webp" },
        { ".ico", "image/x-icon" }, { ".woff2", "font/woff2" },
    };
    char raw[2048], path[2048];
    snprintf(raw, sizeof raw, "%s", target);
    char *q = strchr(raw, '?');
    if (q) *q = '\0';
    http_url_decode(raw, path, sizeof path);
    const char *file = path[0] == '/' ? path + 1 : path;
    if (!*file) file = "index.html";

    int bad = strstr(file, "..") || strchr(file, '\\') || file[0] == '.' ||
              strstr(file, "/.");
    const char *type = NULL;
    const char *ext = strrchr(file, '.');
    for (size_t i = 0; !bad && ext && i < sizeof TYPES / sizeof TYPES[0]; i++)
        if (strcasecmp(ext, TYPES[i].ext) == 0) type = TYPES[i].type;
    if (bad || !type) { http_error(fd, 404, "not found"); return; }

    char src[512];
    if (strcasecmp(ext, ".html") == 0 && binding_source_of(file, src, sizeof src) &&
        access(src, R_OK) == 0 && http_rebuild_page(src) != 0)
        fprintf(stderr, "[EPL] rebuilding %s from %s failed — serving the last good copy\n",
                file, src);

    char cwd[4096], real[4096];
    if (!getcwd(cwd, sizeof cwd) || !realpath(file, real) ||
        strncmp(real, cwd, strlen(cwd)) != 0 || real[strlen(cwd)] != '/') {
        http_error(fd, 404, "not found");
        return;
    }
    FILE *f = fopen(real, "rb");
    if (!f) { http_error(fd, 404, "not found"); return; }
    fseek(f, 0, SEEK_END);
    long len = ftell(f);
    fseek(f, 0, SEEK_SET);
    char *buf = malloc(len > 0 ? (size_t)len : 1);
    size_t got = buf ? fread(buf, 1, (size_t)len, f) : 0;
    fclose(f);
    http_send(fd, 200, type, buf ? buf : "", got);
    free(buf);
}

static const char *http_offered_list(ASTNode *node, char *out, size_t n) {
    out[0] = '\0';
    NodeList *off = &node->as.serve_port.offered;
    for (int i = 0; i < off->count; i++) {
        if (i) strncat(out, ", ", n - strlen(out) - 1);
        strncat(out, off->items[i]->as.identifier.name, n - strlen(out) - 1);
    }
    return out;
}

static void http_action(Interpreter *interp, ASTNode *node, Env *env, int fd,
                        int port, const char *target, const char *head,
                        const char *body) {
    char name[512], msg[2048], hv[256], offered[1024];
    http_url_decode(target + 5, name, sizeof name);

    http_header(head, "Origin", hv, sizeof hv);
    if (hv[0]) {
        char a[64], b[64];
        snprintf(a, sizeof a, "http://localhost:%d", port);
        snprintf(b, sizeof b, "http://127.0.0.1:%d", port);
        if (strcmp(hv, a) != 0 && strcmp(hv, b) != 0) {
            http_error(fd, 403, "requests from other sites are refused");
            return;
        }
    }
    http_header(head, "Content-Type", hv, sizeof hv);
    if (strncasecmp(hv, "application/json", 16) != 0) {
        http_error(fd, 415, "send the inputs as JSON (Content-Type: application/json)");
        return;
    }

    int is_offered = 0;
    NodeList *off = &node->as.serve_port.offered;
    for (int i = 0; i < off->count; i++)
        if (strcmp(off->items[i]->as.identifier.name, name) == 0) is_offered = 1;
    if (!is_offered) {
        snprintf(msg, sizeof msg, "'%s' is not offered by this server (it offers: %s)",
                 name, http_offered_list(node, offered, sizeof offered));
        http_error(fd, 404, msg);
        return;
    }
    ASTNode *def = env_get(env, name)->as.activity;
    NodeList *params = &def->as.activity_def.params;
    char takes[1024] = "";
    for (int i = 0; i < params->count; i++) {
        if (i) strncat(takes, ", ", sizeof takes - strlen(takes) - 1);
        strncat(takes, params->items[i]->as.identifier.name, sizeof takes - strlen(takes) - 1);
    }
    if (!takes[0]) snprintf(takes, sizeof takes, "nothing");

    char jerr[256];
    Value *in = body[0] ? json_parse(body, jerr, sizeof jerr) : val_map();
    if (!in || in->type != VAL_MAP) {
        if (in) val_free(in);
        http_error(fd, 400, "the body must be a JSON object of the activity's inputs");
        return;
    }
    for (int k = 0; k < in->as.map->count; k++) {
        int known = 0;
        for (int i = 0; i < params->count; i++)
            if (strcmp(in->as.map->keys[k], params->items[i]->as.identifier.name) == 0) known = 1;
        if (!known) {
            snprintf(msg, sizeof msg, "%s does not take '%s' (it takes %s)",
                     name, in->as.map->keys[k], takes);
            http_error(fd, 400, msg);
            val_free(in);
            return;
        }
    }
    ValueList args;
    vallist_init(&args);
    for (int i = 0; i < params->count; i++) {
        const char *pn = params->items[i]->as.identifier.name;
        Value *v = valmap_get(in->as.map, pn);
        if (!v) {
            snprintf(msg, sizeof msg, "%s needs '%s' (it takes %s)", name, pn, takes);
            http_error(fd, 400, msg);
            for (int j = 0; j < args.count; j++) val_free(args.items[j]);
            free(args.items);
            val_free(in);
            return;
        }
        vallist_push(&args, val_copy(v));
    }
    val_free(in);

    int prev_error = interp->had_error;
    interp->had_error = 0;
    interp->attempt_depth++;
    Value *res = call_activity(interp, name, &args, env, node);
    interp->attempt_depth--;
    for (int j = 0; j < args.count; j++) if (args.items[j]) val_free(args.items[j]);
    free(args.items);

    if (interp->had_error) {
        printf("[EPL] %s → refused: %s\n", name, interp->last_error);
        http_error(fd, 200, interp->last_error);
        interp->had_error   = 0;
        interp->signal.type = SIG_NONE;
    } else {
        printf("[EPL] %s → ok\n", name);
        http_json(fd, 200, 1, "reply", res);
    }
    fflush(stdout);
    if (res) val_free(res);
    interp->had_error = prev_error;
}

static void serve_http(Interpreter *interp, ASTNode *node, Env *env) {
    char msg[2048], offered[1024];
    Value *pv = eval(interp, node->as.serve_port.port_expr, env);
    double pd = (pv && pv->type == VAL_NUMBER) ? pv->as.number : -1;
    val_free(pv);
    if (pd < 1 || pd > 65535 || pd != (int)pd) {
        runtime_error(interp, node, "serve on port needs a whole number from 1 to 65535");
        return;
    }
    int port = (int)pd;
    if (!permit_(interp, node, perm_check_serve(perms_of(interp), port,
                                                perm_msg, sizeof perm_msg)))
        return;

    NodeList *off = &node->as.serve_port.offered;
    for (int i = 0; i < off->count; i++) {
        const char *nm = off->items[i]->as.identifier.name;
        for (int j = 0; j < i; j++)
            if (strcmp(off->items[j]->as.identifier.name, nm) == 0) {
                snprintf(msg, sizeof msg, "'%s' is offered twice", nm);
                runtime_error(interp, off->items[i], msg);
                return;
            }
        Value *a = env_get(env, nm);
        if (a && a->type == VAL_ACTIVITY) continue;
        const char *best = NULL;
        int best_d = 99;
        for (Env *s = env; s; s = s->parent)
            for (int k = 0; k < s->capacity; k++)
                if (s->names[k] && s->values[k] && s->values[k]->type == VAL_ACTIVITY) {
                    int d = edit_distance(nm, s->names[k]);
                    if (d < best_d) { best_d = d; best = s->names[k]; }
                }
        if (best && best_d <= 3)
            snprintf(msg, sizeof msg, "cannot offer '%s': no activity has that name — did you mean '%s'?", nm, best);
        else
            snprintf(msg, sizeof msg, "cannot offer '%s': no activity has that name (define it with: to %s taking …)", nm, nm);
        runtime_error(interp, off->items[i], msg);
        return;
    }

    int sfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sfd < 0) { runtime_error(interp, node, "cannot create a network socket"); return; }
    fcntl(sfd, F_SETFD, FD_CLOEXEC);
    int one = 1;
    setsockopt(sfd, SOL_SOCKET, SO_REUSEADDR, &one, sizeof one);
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof addr);
    addr.sin_family      = AF_INET;
    addr.sin_port        = htons((uint16_t)port);
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    if (bind(sfd, (struct sockaddr *)&addr, sizeof addr) != 0 || listen(sfd, 16) != 0) {
        snprintf(msg, sizeof msg, "cannot serve on port %d: %s", port, strerror(errno));
        runtime_error(interp, node, msg);
        close(sfd);
        return;
    }
    printf("[EPL] Serving http://localhost:%d/ — offering %s\n", port,
           http_offered_list(node, offered, sizeof offered));
    fflush(stdout);

    for (;;) {
        int fd = accept(sfd, NULL, NULL);
        if (fd < 0) continue;
        fcntl(fd, F_SETFD, FD_CLOEXEC);
        struct timeval tv = { 5, 0 };
        setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof tv);

        char *buf = malloc(HTTP_MAX_HEAD + 1);
        size_t have = 0;
        char *end = NULL;
        while (have < HTTP_MAX_HEAD) {
            ssize_t n = recv(fd, buf + have, HTTP_MAX_HEAD - have, 0);
            if (n <= 0) break;
            have += (size_t)n;
            buf[have] = '\0';
            if ((end = strstr(buf, "\r\n\r\n"))) break;
        }
        if (!end) { close(fd); free(buf); continue; }
        *end = '\0';
        char *head = buf;
        char method[16] = "", target[2048] = "";
        sscanf(head, "%15s %2047s", method, target);

        char cl[32];
        http_header(head, "Content-Length", cl, sizeof cl);
        long blen = cl[0] ? strtol(cl, NULL, 10) : 0;
        if (blen < 0 || blen > HTTP_MAX_BODY) {
            http_error(fd, 413, "request too large");
            close(fd); free(buf); continue;
        }
        char *body = calloc(1, (size_t)blen + 1);
        size_t already = have - (size_t)(end + 4 - buf);
        if (already > (size_t)blen) already = (size_t)blen;
        memcpy(body, end + 4, already);
        while (already < (size_t)blen) {
            ssize_t n = recv(fd, body + already, (size_t)blen - already, 0);
            if (n <= 0) break;
            already += (size_t)n;
        }

        if (strcmp(method, "POST") == 0 && strncmp(target, "/epl/", 5) == 0)
            http_action(interp, node, env, fd, port, target, head, body);
        else if (strcmp(method, "GET") == 0)
            http_page(fd, target);
        else
            http_error(fd, 405, "only page requests (GET) and actions (POST /epl/ACTIVITY) are served");

        free(body);
        free(buf);
        close(fd);
    }
}

static int import_is_pure_literal(ASTNode *v) {
    if (!v) return 1;
    switch (v->type) {
        case NODE_NUMBER: case NODE_STRING:
        case NODE_BOOLEAN: case NODE_NULL:
            return 1;
        case NODE_LIST:
            for (int i = 0; i < v->as.list.elements.count; i++)
                if (!import_is_pure_literal(v->as.list.elements.items[i]))
                    return 0;
            return 1;
        default:
            return 0;
    }
}

static ASTNode *import_load_ast(const char *path) {
    ASTNode *cached = epc_load(path);
    if (cached) return cached;

    char *src = read_file(path);
    if (!src) return NULL;

    Lexer lx;
    lexer_init(&lx, src, LEX_EP);
    int count = 0;
    Token *tk = lexer_tokenize_all(&lx, &count);
    if (!tk) { free(src); return NULL; }

    Parser p;
    parser_init(&p, tk, count);
    ASTNode *ast = parser_parse(&p);
    int bad = p.had_error;

    lexer_free_tokens(tk);
    free(src);
    if (bad) { ast_free(ast); return NULL; }

    epc_store(path, ast);
    return ast;
}

static void import_module(Interpreter *interp, const char *raw,
                          const char *base_dir, ASTNode *errnode) {
    char path[4096];
    if (raw[0] == '/') snprintf(path, sizeof path, "%s", raw);
    else               snprintf(path, sizeof path, "%s/%s", base_dir, raw);

    if (!permit_(interp, errnode, perm_check_read(perms_of(interp), path,
                                                  perm_msg, sizeof perm_msg)))
        return;

    char real[4096];
    if (!realpath(path, real)) {
        char msg[4200];
        snprintf(msg, sizeof msg, "cannot import '%s' — module not found", raw);
        runtime_error(interp, errnode, msg);
        return;
    }

    for (int i = 0; i < interp->imported_count; i++)
        if (strcmp(interp->imported_paths[i], real) == 0) return;

    if (interp->import_depth > 128) {
        runtime_error(interp, errnode, "import nesting too deep (cyclic imports?)");
        return;
    }

    if (interp->imported_count >= interp->imported_cap) {
        interp->imported_cap = interp->imported_cap ? interp->imported_cap * 2 : 8;
        interp->imported_paths = realloc(interp->imported_paths,
            (size_t)interp->imported_cap * sizeof(char *));
    }
    interp->imported_paths[interp->imported_count++] = strdup(real);

    ASTNode *mod = import_load_ast(real);
    if (!mod || mod->type != NODE_PROGRAM) {
        char msg[4200];
        snprintf(msg, sizeof msg, "cannot import '%s' — parse failed", raw);
        runtime_error(interp, errnode, msg);
        if (mod) ast_free(mod);
        return;
    }

    interp->imported_asts = realloc(interp->imported_asts,
        (size_t)(interp->imported_ast_count + 1) * sizeof(ASTNode *));
    interp->imported_asts[interp->imported_ast_count++] = mod;

    char moddir_buf[4096];
    snprintf(moddir_buf, sizeof moddir_buf, "%s", real);
    char *moddir = dirname(moddir_buf);

    interp->import_depth++;
    NodeList *body = &mod->as.program.body;
    for (int i = 0; i < body->count; i++) {
        ASTNode *n = body->items[i];
        if (!n) continue;

        if (n->type == NODE_ACTIVITY_DEF) {
            env_set(interp->global, n->as.activity_def.name, val_activity(n));
        } else if (n->type == NODE_SET &&
                   import_is_pure_literal(n->as.assignment.value)) {
            Value *v = eval(interp, n->as.assignment.value, interp->global);
            env_set(interp->global, n->as.assignment.name, v);
        } else if (n->type == NODE_IMPORT) {
            import_module(interp, n->as.import_stmt.source, moddir, n);
        }
    }
    interp->import_depth--;
}

static int db_store_path(Interpreter *interp, const char *table,
                         char *out, size_t outlen) {
    (void)interp;
    char dir[3200];
    const char *root = getenv("EPL_DATA");
    if (root && root[0])
        snprintf(dir, sizeof dir, "%s", root);
    else
        snprintf(dir, sizeof dir, ".epdata");
    mkdir(dir, 0755);

    char safe[256];
    size_t j = 0;
    for (size_t i = 0; table[i] && j < sizeof(safe) - 1; i++) {
        char c = table[i];
        safe[j++] = (isalnum((unsigned char)c) || c == '_' || c == '-') ? c : '_';
    }
    safe[j] = '\0';
    if (safe[0] == '\0') return 0;

    snprintf(out, outlen, "%s/%s.jsonl", dir, safe);
    return 1;
}

static void db_bind_record_fields(Env *env, Value *record) {
    if (!record || record->type != VAL_MAP) return;
    ValueMap *m = record->as.map;
    for (int i = 0; i < m->count; i++)
        env_set(env, m->keys[i], val_copy(m->values[i]));
}

static Value *db_load_all(Interpreter *interp, const char *table) {
    Value *list = val_list();
    char path[3400];
    if (!db_store_path(interp, table, path, sizeof path)) return list;

    FILE *f = fopen(path, "r");
    if (!f) return list;

    char *line = NULL;
    size_t cap = 0;
    ssize_t len;
    while ((len = getline(&line, &cap, f)) != -1) {
        while (len > 0 && (line[len-1] == '\n' || line[len-1] == '\r'))
            line[--len] = '\0';
        int blank = 1;
        for (ssize_t i = 0; i < len; i++)
            if (!isspace((unsigned char)line[i])) { blank = 0; break; }
        if (blank) continue;

        Value *rec = json_parse(line, NULL, 0);
        if (rec && rec->type == VAL_MAP) vallist_push(list->as.list, rec);
        else if (rec) val_free(rec);
    }
    free(line);
    fclose(f);
    return list;
}

static int db_write_all(Interpreter *interp, const char *table, Value *list) {
    char path[3400];
    if (!db_store_path(interp, table, path, sizeof path)) return -1;

    char tmp[3480];
    snprintf(tmp, sizeof tmp, "%s.tmp%d", path, (int)getpid());
    FILE *f = fopen(tmp, "w");
    if (!f) return -1;

    for (int i = 0; i < list->as.list->count; i++) {
        char *js = json_serialize(list->as.list->items[i]);
        if (js) { fputs(js, f); fputc('\n', f); free(js); }
    }
    if (fclose(f) != 0) { remove(tmp); return -1; }
    if (rename(tmp, path) != 0) { remove(tmp); return -1; }
    return 0;
}

static char *db_table_name(Interpreter *interp, ASTNode *table_expr, Env *env) {
    Value *v = eval(interp, table_expr, env);
    if (!v || v->type != VAL_STRING) { val_free(v); return NULL; }
    char *name = strdup(v->as.string);
    val_free(v);
    return name;
}

#define IDX_MAX_COLS   3
#define IDX_BUCKETS 1024

typedef struct IdxEntry {
    char  *key;
    long  *offs;
    int    noff, coff;
    struct IdxEntry *next;
} IdxEntry;

typedef struct {
    char     *column;
    IdxEntry *buckets[IDX_BUCKETS];
    unsigned  used_tick;
    int       usable;
} ColIdx;

typedef struct TableIdx {
    char      *table;
    char       path[3400];
    off_t      size;
    struct timespec mtime;
    ino_t      ino;
    ColIdx    *cols[IDX_MAX_COLS];
    int        ncols;
    struct TableIdx *next;
} TableIdx;

struct DBIndexCache {
    TableIdx *tables;
    unsigned  clock;
};

static unsigned idx_hash(const char *s) {
    unsigned h = 5381;
    for (; *s; s++) h = ((h << 5) + h) ^ (unsigned char)*s;
    return h;
}

static void colidx_reset(ColIdx *col) {
    for (int b = 0; b < IDX_BUCKETS; b++) {
        IdxEntry *e = col->buckets[b];
        while (e) {
            IdxEntry *n = e->next;
            free(e->key); free(e->offs); free(e);
            e = n;
        }
        col->buckets[b] = NULL;
    }
}

static void colidx_free(ColIdx *col) {
    colidx_reset(col);
    free(col->column);
    free(col);
}

static void colidx_add(ColIdx *col, const char *key, long off) {
    unsigned h = idx_hash(key) % IDX_BUCKETS;
    IdxEntry *e = col->buckets[h];
    while (e && strcmp(e->key, key) != 0) e = e->next;
    if (!e) {
        e = calloc(1, sizeof(IdxEntry));
        e->key  = strdup(key);
        e->next = col->buckets[h];
        col->buckets[h] = e;
    }
    if (e->noff >= e->coff) {
        e->coff = e->coff ? e->coff * 2 : 4;
        e->offs = realloc(e->offs, e->coff * sizeof(long));
    }
    e->offs[e->noff++] = off;
}

static IdxEntry *colidx_get(ColIdx *col, const char *key) {
    unsigned h = idx_hash(key) % IDX_BUCKETS;
    IdxEntry *e = col->buckets[h];
    while (e && strcmp(e->key, key) != 0) e = e->next;
    return e;
}

static int idx_value_ok(Value *v) {
    return v && (v->type == VAL_STRING ||
                 (v->type == VAL_NUMBER && v->as.number == (long)v->as.number));
}

static void idx_scan(FILE *f, ColIdx **cols, int ncols, long from) {
    if (fseek(f, from, SEEK_SET) != 0) return;
    char *line = NULL; size_t cap = 0; ssize_t len; long off;
    while ((off = ftell(f)), (len = getline(&line, &cap, f)) != -1) {
        while (len > 0 && (line[len-1] == '\n' || line[len-1] == '\r'))
            line[--len] = '\0';
        if (len == 0) continue;
        Value *rec = json_parse(line, NULL, 0);
        if (!rec || rec->type != VAL_MAP) { if (rec) val_free(rec); continue; }
        for (int c = 0; c < ncols; c++) {
            ColIdx *col = cols[c];
            if (!col->usable) continue;
            Value *cv = valmap_get(rec->as.map, col->column);
            if (!cv) continue;
            if (!idx_value_ok(cv)) {
                colidx_reset(col);
                col->usable = 0;
                continue;
            }
            char *k = val_to_string(cv);
            colidx_add(col, k, off);
            free(k);
        }
        val_free(rec);
    }
    free(line);
}

static TableIdx *table_sync(Interpreter *interp, DBIndexCache *cache,
                            const char *table) {
    char path[3400];
    if (!db_store_path(interp, table, path, sizeof path)) return NULL;

    struct stat st;
    if (stat(path, &st) != 0) return NULL;

    TableIdx *ti = cache->tables;
    while (ti && strcmp(ti->table, table) != 0) ti = ti->next;
    if (!ti) {
        ti = calloc(1, sizeof(TableIdx));
        ti->table = strdup(table);
        snprintf(ti->path, sizeof ti->path, "%s", path);
        ti->size  = -1;
        ti->next  = cache->tables;
        cache->tables = ti;
    }

    int same_file = ti->size >= 0 && ti->ino == st.st_ino;
    if (same_file && ti->size == st.st_size &&
        ti->mtime.tv_sec  == st.st_mtim.tv_sec &&
        ti->mtime.tv_nsec == st.st_mtim.tv_nsec)
        return ti;

    FILE *f = fopen(path, "r");
    if (!f) return ti;

    if (same_file && st.st_size > ti->size) {
        if (ti->ncols > 0) idx_scan(f, ti->cols, ti->ncols, ti->size);
    } else {
        for (int c = 0; c < ti->ncols; c++) {
            colidx_reset(ti->cols[c]);
            ti->cols[c]->usable = 1;
        }
        if (ti->ncols > 0) idx_scan(f, ti->cols, ti->ncols, 0);
    }
    fclose(f);
    ti->size  = st.st_size;
    ti->mtime = st.st_mtim;
    ti->ino   = st.st_ino;
    return ti;
}

static ColIdx *table_get_colidx(DBIndexCache *cache, TableIdx *ti,
                                const char *column) {
    for (int c = 0; c < ti->ncols; c++) {
        if (strcmp(ti->cols[c]->column, column) == 0) {
            ti->cols[c]->used_tick = ++cache->clock;
            return ti->cols[c]->usable ? ti->cols[c] : NULL;
        }
    }

    ColIdx *col;
    if (ti->ncols < IDX_MAX_COLS) {
        col = calloc(1, sizeof(ColIdx));
        ti->cols[ti->ncols++] = col;
    } else {
        int lru = 0;
        for (int c = 1; c < ti->ncols; c++)
            if (ti->cols[c]->used_tick < ti->cols[lru]->used_tick) lru = c;
        col = ti->cols[lru];
        colidx_reset(col);
        free(col->column);
    }
    col->column    = strdup(column);
    col->usable    = 1;
    col->used_tick = ++cache->clock;

    FILE *f = fopen(ti->path, "r");
    if (f) { ColIdx *one[1] = { col }; idx_scan(f, one, 1, 0); fclose(f); }
    return col->usable ? col : NULL;
}

static int node_is_literal(ASTNode *n) {
    return n && (n->type == NODE_STRING ||
                 n->type == NODE_NUMBER || n->type == NODE_BOOLEAN);
}

static Value *db_try_index_query(Interpreter *interp, ASTNode *table_expr,
                                 ASTNode *condition, Env *env) {
    ASTNode *tbl = table_expr;
    if (!tbl || (tbl->type != NODE_STRING && tbl->type != NODE_IDENTIFIER))
        return NULL;

    if (!condition || condition->type != NODE_BINARY_OP) return NULL;
    TokenType op = condition->as.binary_op.operator;
    if (op != TOKEN_IS && op != TOKEN_EQUALS) return NULL;

    ASTNode *sides[2] = { condition->as.binary_op.left,
                          condition->as.binary_op.right };
    const char *column     = NULL;
    ASTNode    *probe_node = NULL;
    for (int s = 0; s < 2; s++) {
        ASTNode *n     = sides[s];
        ASTNode *other = sides[s ^ 1];
        int n_is_column = n && n->type == NODE_IDENTIFIER &&
                          env_get(env, n->as.identifier.name) == NULL;
        int other_is_probe = node_is_literal(other) ||
                             (other && other->type == NODE_IDENTIFIER &&
                              env_get(env, other->as.identifier.name) != NULL);
        if (n_is_column && other_is_probe) {
            column     = n->as.identifier.name;
            probe_node = other;
            break;
        }
    }
    if (!column || !probe_node) return NULL;

    Value *probe = eval(interp, probe_node, env);
    if (!idx_value_ok(probe)) { val_free(probe); return NULL; }

    char *table = db_table_name(interp, tbl, env);
    if (!table) { val_free(probe); return NULL; }

    if (!interp->db_index) interp->db_index = calloc(1, sizeof(DBIndexCache));
    TableIdx *ti = table_sync(interp, interp->db_index, table);
    ColIdx   *col = ti ? table_get_colidx(interp->db_index, ti, column) : NULL;
    if (!col) { val_free(probe); free(table); return NULL; }

    char *key = val_to_string(probe);
    IdxEntry *e = colidx_get(col, key);
    free(key);

    Value *out = val_list();
    if (e) {
        FILE *f = fopen(ti->path, "r");
        if (!f) { val_free(out); val_free(probe); free(table); return NULL; }
        char *line = NULL; size_t cap = 0;
        for (int i = 0; i < e->noff; i++) {
            if (fseek(f, e->offs[i], SEEK_SET) != 0) continue;
            ssize_t len = getline(&line, &cap, f);
            if (len < 0) continue;
            Value *rec = json_parse(line, NULL, 0);
            if (rec && rec->type == VAL_MAP) {
                Value *cv = valmap_get(rec->as.map, column);
                if (cv && val_equals(cv, probe))
                    vallist_push(out->as.list, val_copy(rec));
            }
            if (rec) val_free(rec);
        }
        free(line);
        fclose(f);
    }
    val_free(probe);
    free(table);
    return out;
}

static void db_index_cache_free(DBIndexCache *cache) {
    if (!cache) return;
    TableIdx *ti = cache->tables;
    while (ti) {
        TableIdx *next = ti->next;
        for (int c = 0; c < ti->ncols; c++) colidx_free(ti->cols[c]);
        free(ti->table);
        free(ti);
        ti = next;
    }
    free(cache);
}

static void exec_db_make_table(Interpreter *interp, ASTNode *node, Env *env) {
    char *table = db_table_name(interp, node->as.db_table.table, env);
    if (!table) { runtime_error(interp, node, "make table: table name must be a string"); return; }

    char path[3400];
    if (db_store_path(interp, table, path, sizeof path)) {
        FILE *f = fopen(path, "a");
        if (f) fclose(f);
        else runtime_error(interp, node, "make table: cannot create table store");
    }
    free(table);
}

static void exec_db_insert(Interpreter *interp, ASTNode *node, Env *env) {
    char *table = db_table_name(interp, node->as.db_insert.table, env);
    if (!table) { runtime_error(interp, node, "save into: table name must be a string"); return; }

    Value *rec = val_map();
    NodeList *names = &node->as.db_insert.field_names;
    NodeList *vals  = &node->as.db_insert.field_values;
    for (int i = 0; i < names->count && i < vals->count; i++) {
        const char *key = names->items[i]->as.string.value;
        Value *v = eval(interp, vals->items[i], env);
        valmap_set(rec->as.map, key, v);
    }

    char path[3400];
    if (db_store_path(interp, table, path, sizeof path)) {
        FILE *f = fopen(path, "a");
        if (f) {
            char *js = json_serialize(rec);
            if (js) { fputs(js, f); fputc('\n', f); free(js); }
            fclose(f);
        } else {
            runtime_error(interp, node, "save into: cannot open table store");
        }
    }
    val_free(rec);
    free(table);
}

static void exec_db_delete(Interpreter *interp, ASTNode *node, Env *env) {
    char *table = db_table_name(interp, node->as.db_filter.table, env);
    if (!table) { runtime_error(interp, node, "delete from: table name must be a string"); return; }

    Value *all = db_load_all(interp, table);
    Value *kept = val_list();

    for (int i = 0; i < all->as.list->count; i++) {
        Value *rec = all->as.list->items[i];
        int matches;
        if (!node->as.db_filter.condition) {
            matches = 1;
        } else {
            Env *fe = env_new(env);
            env_set(fe, node->as.db_filter.item_var, val_copy(rec));
            db_bind_record_fields(fe, rec);
            interp->record_scope++;
            Value *passes = eval(interp, node->as.db_filter.condition, fe);
            interp->record_scope--;
            matches = val_is_truthy(passes);
            val_free(passes);
            env_free(fe);
        }
        if (!matches) vallist_push(kept->as.list, val_copy(rec));
    }

    if (db_write_all(interp, table, kept) != 0)
        runtime_error(interp, node, "delete from: cannot rewrite table store");

    val_free(all);
    val_free(kept);
    free(table);
}

static void exec_import(Interpreter *interp, ASTNode *node) {
    char base[4096];
    if (interp->source_file) {
        char tmp[4096];
        snprintf(tmp, sizeof tmp, "%s", interp->source_file);
        snprintf(base, sizeof base, "%s", dirname(tmp));
    } else {
        snprintf(base, sizeof base, ".");
    }
    import_module(interp, node->as.import_stmt.source, base, node);
}

void exec_node(Interpreter *interp, ASTNode *node, Env *env) {
    if (!node) return;
    if (interp->signal.type != SIG_NONE) return;

    switch (node->type) {

        case NODE_DESCRIBE:
            break;

        case NODE_IMPORT:
            exec_import(interp, node);
            break;

        case NODE_DB_MAKE_TABLE:
            exec_db_make_table(interp, node, env);
            break;
        case NODE_DB_INSERT:
            exec_db_insert(interp, node, env);
            break;
        case NODE_DB_DELETE:
            exec_db_delete(interp, node, env);
            break;

        case NODE_LINK:
            exec_link(interp, node, env);
            break;

        case NODE_MAKE_PAGE:
            if (!PERMIT(perm_check_write, node->as.make_page.filename)) break;
            codegen_page(interp, env, node, interp->page_manifest);
            break;

        case NODE_SET: {
            Value *val = eval(interp, node->as.assignment.value, env);
            env_set(env, node->as.assignment.name, val);
            break;
        }

        case NODE_UPDATE: {
            Value *val = eval(interp, node->as.assignment.value, env);
            if (!env_update(env, node->as.assignment.name, val)) {
                const char *name = node->as.assignment.name;
                char msg[512];
                if (strncmp(name, "item ", 5) == 0)
                    snprintf(msg, sizeof msg,
                             "cannot update '%s' — list items can't be changed in place; "
                             "build a new list (add VALUE to LIST appends)", name);
                else
                    snprintf(msg, sizeof msg,
                             "cannot update '%s' — it was never set; create it with: set %s as VALUE",
                             name, name);
                val_free(val);
                runtime_error(interp, node, msg);
            }
            break;
        }

        case NODE_UPDATE_PART: {
            const char *name = node->as.update_part.target;
            Value *target = env_get(env, name);
            if (!target) { unknown_name_error(interp, node, env, name); break; }

            Value *sel = eval(interp, node->as.update_part.selector, env);
            Value *val = eval(interp, node->as.update_part.value, env);
            if (interp->signal.type == SIG_ERROR) { val_free(sel); val_free(val); break; }
            char msg[512];

            if (!node->as.update_part.is_field) {
                double d;
                if (target->type != VAL_LIST) {
                    snprintf(msg, sizeof msg, "'update item' needs a list, but '%s' is %s%s",
                             name, val_type_name(target),
                             target->type == VAL_STRING ? " (text can't be changed in place; build new text)" : "");
                    runtime_error(interp, node, msg);
                } else if (!num_operand(sel, &d) || d != floor(d)) {
                    runtime_error(interp, node, "'update item N' needs a whole number for N");
                } else if (d < 0 || d >= target->as.list->count) {
                    int c = target->as.list->count;
                    snprintf(msg, sizeof msg, "there is no item %d in '%s': it has %d item%s, numbered 0 to %d "
                             "(use add VALUE to %s to append)", (int)d, name, c, c == 1 ? "" : "s", c - 1, name);
                    runtime_error(interp, node, msg);
                } else {
                    val_free(target->as.list->items[(int)d]);
                    target->as.list->items[(int)d] = val;
                    val = NULL;
                }
            } else {
                char *path = val_to_string(sel);
                Value *cur = target;
                char *seg = path, *dot;
                while (cur && (dot = strchr(seg, '.')) != NULL) {
                    *dot = '\0';
                    Value *next = NULL;
                    if (cur->type == VAL_MAP) next = valmap_get(cur->as.map, seg);
                    else if (cur->type == VAL_LIST) {
                        double i;
                        Value tmp = { .type = VAL_STRING };
                        tmp.as.string = seg;
                        if (num_operand(&tmp, &i) && i >= 0 && i < cur->as.list->count)
                            next = cur->as.list->items[(int)i];
                    }
                    if (!next) {
                        snprintf(msg, sizeof msg, "cannot update field: '%s' has no '%s'", name, seg);
                        runtime_error(interp, node, msg);
                    }
                    cur = next;
                    seg = dot + 1;
                }
                if (cur && cur->type != VAL_MAP) {
                    snprintf(msg, sizeof msg, "'update field' needs a record, but %s is %s "
                             "(start one with: set %s as empty record)",
                             cur == target ? name : "that part", val_type_name(cur), name);
                    runtime_error(interp, node, msg);
                } else if (cur) {
                    valmap_set(cur->as.map, seg, val);
                    val = NULL;
                }
                free(path);
            }
            val_free(sel);
            val_free(val);
            break;
        }

        case NODE_SHOW: {
            Value *val = eval(interp, node->as.show.value, env);
            if (interp->signal.type == SIG_ERROR) { val_free(val); break; }
            val_print(val);
            printf("\n");
            val_free(val);
            break;
        }

        case NODE_IF: {
            Value *cond = eval(interp, node->as.if_stmt.condition, env);
            int truthy  = val_is_truthy(cond);
            val_free(cond);

            Env *branch_env = env_new(env);
            if (truthy) {
                exec_block(interp, &node->as.if_stmt.then_body, branch_env);
            } else if (node->as.if_stmt.else_body.count > 0) {
                exec_block(interp, &node->as.if_stmt.else_body, branch_env);
            }
            env_free(branch_env);
            break;
        }

        case NODE_WHILE: {
            while (1) {
                Value *cond = eval(interp, node->as.while_stmt.condition, env);
                int truthy = val_is_truthy(cond);
                val_free(cond);
                if (!truthy) break;

                Env *loop_env = env_new(env);
                exec_block(interp, &node->as.while_stmt.body, loop_env);
                env_free(loop_env);

                if (interp->signal.type == SIG_BREAK) {
                    interp->signal.type = SIG_NONE; break;
                }
                if (interp->signal.type == SIG_CONTINUE) {
                    interp->signal.type = SIG_NONE; continue;
                }
                if (interp->signal.type != SIG_NONE) break;
            }
            break;
        }

        case NODE_REPEAT_INDEFINITELY: {
            while (1) {
                Env *loop_env = env_new(env);
                exec_block(interp, &node->as.repeat_indefinitely.body, loop_env);
                env_free(loop_env);

                if (interp->signal.type == SIG_BREAK) {
                    interp->signal.type = SIG_NONE; break;
                }
                if (interp->signal.type == SIG_CONTINUE) {
                    interp->signal.type = SIG_NONE; continue;
                }
                if (interp->signal.type != SIG_NONE) break;
            }
            break;
        }

        case NODE_REPEAT_TIMES: {
            Value *count_val = eval(interp, node->as.repeat_times.count, env);
            double cd;
            if (!num_operand(count_val, &cd)) {
                arith_type_error(interp, node, "repeat … times", count_val, count_val);
                val_free(count_val);
                break;
            }
            int count = (int)cd;
            val_free(count_val);

            int has_counter = node->as.repeat_times.counter[0] != '\0';
            Env *loop_env = NULL;
            for (int i = 0; i < count; i++) {
                loop_env = loop_pass_scope(loop_env, env, has_counter);
                if (has_counter) {
                    env_set(loop_env,
                        node->as.repeat_times.counter, val_number(i));
                }
                exec_block(interp, &node->as.repeat_times.body, loop_env);

                if (interp->signal.type == SIG_BREAK) {
                    interp->signal.type = SIG_NONE; break;
                }
                if (interp->signal.type == SIG_CONTINUE) {
                    interp->signal.type = SIG_NONE; continue;
                }
                if (interp->signal.type != SIG_NONE) break;
            }
            env_free(loop_env);
            break;
        }

        case NODE_REPEAT_UNTIL: {
            while (1) {
                Value *cond = eval(interp, node->as.repeat_until.condition, env);
                int done = val_is_truthy(cond);
                val_free(cond);
                if (done) break;

                Env *loop_env = env_new(env);
                exec_block(interp, &node->as.repeat_until.body, loop_env);
                env_free(loop_env);

                if (interp->signal.type == SIG_BREAK) {
                    interp->signal.type = SIG_NONE; break;
                }
                if (interp->signal.type == SIG_CONTINUE) {
                    interp->signal.type = SIG_NONE; continue;
                }
                if (interp->signal.type != SIG_NONE) break;
            }
            break;
        }

        case NODE_FOR_EACH: {
            Value *collection = eval(interp, node->as.for_each.collection, env);

            if (collection->type == VAL_STRING) {
                Value *chars = val_list();
                for (const char *c = collection->as.string; *c; ) {
                    int n = utf8_seq_len(c);
                    char ch[8] = {0};
                    memcpy(ch, c, n < 7 ? n : 7);
                    vallist_push(chars->as.list, val_string(ch));
                    c += n;
                }
                val_free(collection);
                collection = chars;
            } else if (collection->type == VAL_MAP) {
                Value *keys = val_list();
                for (int i = 0; i < collection->as.map->count; i++)
                    vallist_push(keys->as.list, val_string(collection->as.map->keys[i]));
                val_free(collection);
                collection = keys;
            } else if (collection->type != VAL_LIST) {
                if (interp->signal.type != SIG_ERROR) {
                    char msg[256];
                    snprintf(msg, sizeof msg, "'for each' needs a list, text or a record, but got %s",
                             val_type_name(collection));
                    runtime_error(interp, node, msg);
                }
                val_free(collection);
                break;
            }

            if (collection->type == VAL_LIST) {
                Env *loop_env = NULL;
                for (int i = 0; i < collection->as.list->count; i++) {
                    loop_env = loop_pass_scope(loop_env, env, 1);
                    env_set(loop_env, node->as.for_each.var,
                        val_copy(collection->as.list->items[i]));
                    exec_block(interp, &node->as.for_each.body, loop_env);

                    if (interp->signal.type == SIG_BREAK) {
                        interp->signal.type = SIG_NONE; break;
                    }
                    if (interp->signal.type == SIG_CONTINUE) {
                        interp->signal.type = SIG_NONE; continue;
                    }
                    if (interp->signal.type != SIG_NONE) break;
                }
                env_free(loop_env);
            }
            val_free(collection);
            break;
        }

        case NODE_FOR_RANGE: {
            Value *from_val = eval(interp, node->as.for_range.from, env);
            Value *to_val   = eval(interp, node->as.for_range.to, env);
            double fd, td;
            if (!num_operand(from_val, &fd) || !num_operand(to_val, &td)) {
                arith_type_error(interp, node, "for … from … to", from_val, to_val);
                val_free(from_val); val_free(to_val);
                break;
            }
            int from = (int)fd;
            int to   = (int)td;
            val_free(from_val); val_free(to_val);

            int step = node->as.for_range.downward ? -1 : 1;
            Env *loop_env = NULL;
            for (int i = from;
                 node->as.for_range.downward ? i >= to : i <= to;
                 i += step) {
                loop_env = loop_pass_scope(loop_env, env, 1);
                env_set(loop_env, node->as.for_range.var, val_number(i));
                exec_block(interp, &node->as.for_range.body, loop_env);

                if (interp->signal.type == SIG_BREAK) {
                    interp->signal.type = SIG_NONE; break;
                }
                if (interp->signal.type == SIG_CONTINUE) {
                    interp->signal.type = SIG_NONE; continue;
                }
                if (interp->signal.type != SIG_NONE) break;
            }
            env_free(loop_env);
            break;
        }

        case NODE_ACTIVITY_DEF: {
            Value *activity = val_activity(node);
            env_set(env, node->as.activity_def.name, activity);
            break;
        }

        case NODE_RETURN: {
            Value *val = eval(interp, node->as.return_stmt.value, env);
            interp->signal.type  = SIG_RETURN;
            interp->signal.value = val;
            break;
        }

        case NODE_BREAK:
            interp->signal.type = SIG_BREAK;
            break;

        case NODE_CONTINUE:
            interp->signal.type = SIG_CONTINUE;
            break;

        case NODE_CHECK: {
            Value *cond = eval(interp, node->as.check_stmt.condition, env);
            if (!val_is_truthy(cond)) {
                runtime_error(interp, node, "check failed: assertion is not true");
            }
            val_free(cond);
            break;
        }

        case NODE_PROVIDE: {
            Value *prompt = eval(interp, node->as.provide_stmt.prompt, env);
            char  *prompt_str = val_to_string(prompt);
            val_free(prompt);

            printf("%s", prompt_str);
            free(prompt_str);
            fflush(stdout);

            char buf[1024];
            if (fgets(buf, sizeof(buf), stdin)) {
                int len = strlen(buf);
                if (len > 0 && buf[len-1] == '\n') buf[len-1] = '\0';

                char *end;
                double num = strtod(buf, &end);
                Value *val;
                if (*end == '\0' && end != buf) {
                    val = val_number(num);
                } else {
                    val = val_string(buf);
                }
                env_set(env, node->as.provide_stmt.var, val);
            } else {
                printf("\n");
                runtime_error(interp, node, "no more input (reached end of input)");
            }
            break;
        }

        case NODE_FAIL_WITH: {
            Value *m = eval(interp, node->as.fail_with.message_expr, env);
            char *s = val_to_string(m);
            runtime_error(interp, node, s);
            free(s); val_free(m);
            break;
        }

        case NODE_ATTEMPT: {
            int prev_error = interp->had_error;
            interp->had_error = 0;

            interp->attempt_depth++;
            exec_block(interp, &node->as.attempt_stmt.try_body, env);
            interp->attempt_depth--;

            if (interp->had_error) {
                interp->had_error    = 0;
                interp->signal.type  = SIG_NONE;
                if (node->as.attempt_stmt.fail_body.count > 0)
                    exec_block(interp, &node->as.attempt_stmt.fail_body, env);
            }
            interp->had_error = prev_error;
            break;
        }

        case NODE_ADD_TO: {
            Value *item = eval(interp, node->as.add_to.item, env);
            Value *list = env_get(env, node->as.add_to.list);
            if (list && list->type == VAL_LIST) {
                vallist_push(list->as.list, item);
            } else {
                val_free(item);
            }
            break;
        }

        case NODE_WAIT: {
            printf("%s", node->as.wait_stmt.message);
            fflush(stdout);
            char buf[256];
            fgets(buf, sizeof(buf), stdin);
            break;
        }

        case NODE_EXECUTE: {
            const char *cmd = node->as.execute_stmt.command;
            if (!PERMIT(perm_check_command,
                        node->as.execute_stmt.conditional ? node->as.execute_stmt.alt_command : cmd))
                break;
            if (node->as.execute_stmt.conditional) {
                #ifdef _WIN32
                    system(cmd);
                #else
                    system(node->as.execute_stmt.alt_command);
                #endif
            } else {
                system(cmd);
            }
            break;
        }

        case NODE_WRITE_FILE:
        case NODE_APPEND_FILE: {
            Value *val = eval(interp, node->as.file_write.value_expr, env);
            Value *path_val = eval(interp, node->as.file_write.path_expr, env);
            char *content = val_to_string(val);
            char *path = val_to_string(path_val);
            val_free(val); val_free(path_val);

            char actual_path[1024];
            perm_expand_home(path, actual_path, sizeof actual_path);
            if (!PERMIT(perm_check_write, actual_path)) {
                free(content); free(path);
                break;
            }

            FILE *f = fopen(actual_path, node->type == NODE_WRITE_FILE ? "w" : "a");
            if (!f) {
                char msg[1152];
                snprintf(msg, sizeof(msg), "cannot %s file '%s': %s",
                         node->type == NODE_WRITE_FILE ? "write" : "append to",
                         actual_path, strerror(errno));
                runtime_error(interp, node, msg);
            } else {
                if (fputs(content, f) == EOF) {
                    char msg[1152];
                    snprintf(msg, sizeof(msg), "error writing to file '%s': %s",
                             actual_path, strerror(errno));
                    runtime_error(interp, node, msg);
                }
                fclose(f);
            }
            free(content); free(path);
            break;
        }

        case NODE_DELETE_FILE: {
            Value *path_val = eval(interp, node->as.file_op.path_expr, env);
            char *path = val_to_string(path_val);
            val_free(path_val);

            char actual_path[1024];
            perm_expand_home(path, actual_path, sizeof actual_path);
            if (!PERMIT(perm_check_write, actual_path)) { free(path); break; }

            if (remove(actual_path) != 0) {
                char msg[1152];
                snprintf(msg, sizeof(msg), "cannot delete file '%s': %s",
                         actual_path, strerror(errno));
                runtime_error(interp, node, msg);
            }
            free(path);
            break;
        }

        case NODE_SLEEP: {
            Value *dur_val = eval(interp, node->as.sleep_stmt.duration_expr, env);
            if (dur_val->type == VAL_NUMBER) {
                sleep((unsigned int)dur_val->as.number);
            }
            val_free(dur_val);
            break;
        }

        case NODE_SERVE_SOCKET: {
            Value *path_val = eval(interp, node->as.serve_socket.path_expr, env);
            char *path = val_to_string(path_val);
            val_free(path_val);

            char actual_path[1024];
            perm_expand_home(path, actual_path, sizeof actual_path);
            if (!PERMIT(perm_check_socket, actual_path)) { free(path); break; }

            unlink(actual_path);

            int server_fd = socket(AF_UNIX, SOCK_STREAM, 0);
            struct sockaddr_un addr;
            addr.sun_family = AF_UNIX;
            strncpy(addr.sun_path, actual_path, sizeof(addr.sun_path) - 1);

            bind(server_fd, (struct sockaddr*)&addr, sizeof(addr));
            chmod(actual_path, 0600);
            listen(server_fd, 16);

            printf("[EPL Daemon] Serving on %s using activity '%s'\n", actual_path, node->as.serve_socket.activity);

            while (1) {
                int client_fd = accept(server_fd, NULL, NULL);
                if (client_fd < 0) continue;

                char buf[16384] = {0};
                int n = read(client_fd, buf, sizeof(buf) - 1);
                if (n > 0) {
                    if (buf[n - 1] == '\n') buf[n - 1] = '\0';

                    int prev_fd = _nova_conn_fd;
                    _nova_conn_fd = client_fd;

                    ValueList args;
                    args.count = 0;
                    args.capacity = 1;
                    args.items = malloc(sizeof(Value*) * 1);
                    args.items[0] = val_string(buf);
                    args.count = 1;

                    _nova_replied = 0;
                    Value *res = call_activity(interp, node->as.serve_socket.activity, &args, env, node);
                    if (!_nova_replied && res && res->type != VAL_NULL) {
                        char *str = val_to_string(res);
                        dprintf(client_fd, "%s\n", str);
                        free(str);
                    }
                    if (res) val_free(res);

                    val_free(args.items[0]);
                    free(args.items);

                    _nova_conn_fd = prev_fd;
                }
                close(client_fd);
            }

            close(server_fd);
            free(path);
            break;
        }

        case NODE_SERVE_PORT:
            serve_http(interp, node, env);
            break;

        case NODE_REPLY_WITH: {
            if (_nova_conn_fd >= 0) {
                Value *val = eval(interp, node->as.reply_with.value_expr, env);
                char *str = val_to_string(val);
                val_free(val);

                dprintf(_nova_conn_fd, "%s\n", str);
                free(str);
                _nova_replied = 1;
            }
            break;
        }

        default:
            break;
    }
}

void exec_block(Interpreter *interp, NodeList *list, Env *env) {
    for (int i = 0; i < list->count; i++) {
        if (interp->signal.type != SIG_NONE) break;
        exec_node(interp, list->items[i], env);
    }
}

void interpreter_run(Interpreter *interp, ASTNode *program) {
    if (!program || program->type != NODE_PROGRAM) return;

    for (int i = 0; i < program->as.program.body.count; i++) {
        ASTNode *node = program->as.program.body.items[i];
        if (node && node->type == NODE_ACTIVITY_DEF) {
            exec_node(interp, node, interp->global);
        }
    }

    static PageManifest mf_storage;
    int page_count = 0;
    for (int i = 0; i < program->as.program.body.count; i++) {
        ASTNode *n = program->as.program.body.items[i];
        if (n && n->type == NODE_MAKE_PAGE) page_count++;
    }
    if (page_count > 1) {
        codegen_collect_pages(program, &mf_storage, interp, interp->global);
        interp->page_manifest = &mf_storage;
    } else {
        interp->page_manifest = NULL;
    }

    static char old_html[64][512];
    int old_html_count = binding_get_html_files(interp->source_file, old_html, 64);

    exec_block(interp, &program->as.program.body, interp->global);

    PageManifest *mf = interp->page_manifest;
    for (int o = 0; o < old_html_count; o++) {
        int still_present = 0;
        if (mf) {
            for (int n = 0; n < mf->count; n++) {
                if (strcmp(old_html[o], mf->pages[n].filename) == 0) {
                    still_present = 1;
                    break;
                }
            }
        } else {
            for (int i = 0; i < program->as.program.body.count; i++) {
                ASTNode *nd = program->as.program.body.items[i];
                if (nd && nd->type == NODE_MAKE_PAGE) {
                    const char *fn = nd->as.make_page.filename;
                    if (fn && strcmp(old_html[o], fn) == 0) {
                        still_present = 1;
                        break;
                    }
                }
            }
        }

        if (!still_present) {
            char why[2048];
            if (!perm_check_write(perms_of(interp), old_html[o], why, sizeof why)) {
                printf("[EPL] Not removing '%s': %s\n", old_html[o], why);
                continue;
            }
            if (remove(old_html[o]) == 0)
                printf("[EPL] Removed:   %s\n", old_html[o]);
            else
                printf("[EPL] Note: could not delete '%s' (already gone?)\n", old_html[o]);
            binding_remove(old_html[o]);
        }
    }
}
