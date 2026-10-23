#define _POSIX_C_SOURCE 200809L
#define _DEFAULT_SOURCE 1
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/stat.h>
#include <dirent.h>
#include <libgen.h>
#include <limits.h>
#include <unistd.h>

#ifdef _WIN32
unsigned int __attribute__((stdcall)) SetConsoleOutputCP(unsigned int wCodePageID);
#endif

#include "token.h"
#include "lexer.h"
#include "ast.h"
#include "parser.h"
#include "interpreter.h"
#include "describe.h"
#include "epc.h"
#include "permissions.h"
#include "actions.h"

static int show_permissions(const char *filepath) {
    Permissions p;
    char err[2048];
    if (perm_load(&p, filepath, err, sizeof err) != 0) {
        fprintf(stderr, "[EPL Permissions] %s\n", err);
        return 2;
    }
    printf("Rules: %s\n", p.from_file ? p.file : "defaults (no permissions.txt)");
    if (p.everything) printf("  EVERYTHING is allowed (allow everything)\n");
    printf("  read files in:  ");
    for (int i = 0; i < p.read_dirs.count; i++) printf("%s%s", i ? ", " : "", p.read_dirs.items[i]);
    printf("\n  write files in: ");
    for (int i = 0; i < p.write_dirs.count; i++) printf("%s%s", i ? ", " : "", p.write_dirs.items[i]);
    printf("\n  run commands:   ");
    if (p.any_command) printf("ANY command");
    else if (!p.programs.count) printf("none");
    for (int i = 0; i < p.programs.count; i++) printf("%s%s", i ? ", " : "", p.programs.items[i]);
    printf("\n  network:        ");
    if (!p.hosts.count) printf("none");
    for (int i = 0; i < p.hosts.count; i++) printf("%s%s", i ? ", " : "", p.hosts.items[i]);
    printf("\n  sockets in:     ");
    if (!p.socket_dirs.count) printf("none");
    for (int i = 0; i < p.socket_dirs.count; i++) printf("%s%s", i ? ", " : "", p.socket_dirs.items[i]);
    printf("\n  serve on ports: ");
    if (!p.ports.count) printf("none");
    for (int i = 0; i < p.ports.count; i++) printf("%s%s", i ? ", " : "", p.ports.items[i]);
    printf("\n  environment:    ");
    if (p.any_env) printf("ALL variables");
    else if (!p.env_names.count) printf("none");
    for (int i = 0; i < p.env_names.count; i++) printf("%s%s", i ? ", " : "", p.env_names.items[i]);
    printf("\n");
    perm_free(&p);
    return 0;
}

#define MAX_SOURCE_SIZE 1048576

char *read_file(const char *path) {
    FILE *f = fopen(path, "r");
    if (!f) {
        fprintf(stderr, "EPL Error: Cannot open file '%s'\n", path);
        return NULL;
    }

    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    rewind(f);

    if (size > MAX_SOURCE_SIZE) {
        fprintf(stderr, "EPL Error: File '%s' exceeds 1MB limit\n", path);
        fclose(f);
        return NULL;
    }

    char *buf = malloc(size + 1);
    if (!buf) {
        fprintf(stderr, "EPL Error: Out of memory reading file\n");
        fclose(f);
        return NULL;
    }

    size_t read_size = fread(buf, 1, size, f);
    buf[read_size] = '\0';
    fclose(f);
    return buf;
}

static LexMode lex_mode_for(const char *path) {
    const char *ext = strrchr(path, '.');
    if (!ext) return LEX_EP;
    char up[8] = {0};
    for (int i = 0; i < 7 && ext[i]; i++)
        up[i] = (char)((ext[i] >= 'a' && ext[i] <= 'z') ? ext[i] - 32 : ext[i]);
    return strcmp(up, ".EPF") == 0 ? LEX_EPF : LEX_EP;
}

static ASTNode *ep_parse_file(const char *path, int use_cache, int write_cache) {
    if (use_cache) {
        ASTNode *cached = epc_load(path);
        if (cached) return cached;
    }

    char *source = read_file(path);
    if (!source) return NULL;

    Lexer lexer;
    lexer_init(&lexer, source, lex_mode_for(path));
    int count = 0;
    Token *tokens = lexer_tokenize_all(&lexer, &count);
    if (!tokens) { free(source); return NULL; }

    Parser parser;
    parser_init(&parser, tokens, count);
    ASTNode *ast = parser_parse(&parser);

    if (parser.had_error) {
        ast_free(ast);
        lexer_free_tokens(tokens);
        free(source);
        return NULL;
    }
    if (write_cache) epc_store(path, ast);

    lexer_free_tokens(tokens);
    free(source);
    return ast;
}

static int count_lines(const char *path) {
    FILE *f = fopen(path, "r");
    if (!f) return 0;
    int lines = 0, c, last = '\n';
    while ((c = fgetc(f)) != EOF) { if (c == '\n') lines++; last = c; }
    if (last != '\n') lines++;
    fclose(f);
    return lines;
}

static int is_epl_source(const char *name) {
    const char *dot = strrchr(name, '.');
    if (!dot) return 0;
    return strcasecmp(dot, ".EP")  == 0 ||
           strcasecmp(dot, ".EPF") == 0 ||
           strcasecmp(dot, ".EPDB") == 0;
}

static void ws_walk(const char *dir, const char *root,
                    EPDModule **mods, int *n, int *cap, int use_cache,
                    CallGraph *cg);

static int ws_cmp(const void *a, const void *b) {
    return strcmp(((const EPDModule *)a)->path, ((const EPDModule *)b)->path);
}

static int run_workspace_index(const char *root, int use_cache) {
    EPDModule *mods = NULL;
    int n = 0, cap = 0;
    CallGraph cg = {0};
    ws_walk(root, root, &mods, &n, &cap, use_cache, &cg);

    if (n > 1) qsort(mods, (size_t)n, sizeof(EPDModule), ws_cmp);

    if (n == 0) {
        fprintf(stderr, "EPL: no .EP/.EPF/.EPDB files found under '%s'\n", root);
        free(mods);
        callgraph_free(&cg);
        return 1;
    }

    char out[600];
    snprintf(out, sizeof out, "%s/PROJECT.EPD", root);
    epd_write_workspace(out, root, mods, n, &cg);

    fprintf(stderr, "[EPL] Workspace index: %s  (%d modules, %d call sites)\n",
            out, n, cg.count);

    for (int i = 0; i < n; i++) epd_free(mods[i].index);
    free(mods);
    callgraph_free(&cg);
    return 0;
}

void print_banner(void) {
#ifdef _WIN32
    SetConsoleOutputCP(65001);
#endif

    fprintf(stderr, "\n");
    fprintf(stderr, "  ███████╗██████╗ ██╗     \n");
    fprintf(stderr, "  ██╔════╝██╔══██╗██║     \n");
    fprintf(stderr, "  █████╗  ██████╔╝██║     \n");
    fprintf(stderr, "  ██╔══╝  ██╔═══╝ ██║     \n");
    fprintf(stderr, "  ███████╗██║     ███████╗\n");
    fprintf(stderr, "  ╚══════╝╚═╝     ╚══════╝\n");
}

static void collect_imports(ASTNode *program, const char *importer_full,
                            const char *root, const char *importer_rel,
                            CallGraph *cg) {
    if (!program || program->type != NODE_PROGRAM) return;

    char dirbuf[PATH_MAX];
    snprintf(dirbuf, sizeof dirbuf, "%s", importer_full);
    char *importer_dir = dirname(dirbuf);

    char abs_root[PATH_MAX];
    if (!realpath(root, abs_root))
        snprintf(abs_root, sizeof abs_root, "%s", root);
    size_t rootlen = strlen(abs_root);
    NodeList *body = &program->as.program.body;
    for (int i = 0; i < body->count; i++) {
        ASTNode *node = body->items[i];
        if (!node || node->type != NODE_IMPORT) continue;

        const char *raw = node->as.import_stmt.source;
        char joined[PATH_MAX];
        if (raw[0] == '/') snprintf(joined, sizeof joined, "%s", raw);
        else               snprintf(joined, sizeof joined, "%s/%s", importer_dir, raw);

        char real[PATH_MAX];
        char to_rel[512];
        if (realpath(joined, real)) {
            const char *rel = real;
            if (strncmp(real, abs_root, rootlen) == 0) {
                rel = real + rootlen;
                while (*rel == '/') rel++;
            }
            snprintf(to_rel, sizeof to_rel, "%s", rel);
        } else {
            snprintf(to_rel, sizeof to_rel, "%s", raw);
        }
        callgraph_add_import(cg, importer_rel, to_rel);
    }
}

static void ws_walk(const char *dir, const char *root,
                    EPDModule **mods, int *n, int *cap, int use_cache,
                    CallGraph *cg) {
    DIR *d = opendir(dir);
    if (!d) return;

    struct dirent *de;
    while ((de = readdir(d)) != NULL) {
        if (de->d_name[0] == '.') continue;
        if (strcmp(de->d_name, "node_modules") == 0) continue;

        char full[1024];
        snprintf(full, sizeof full, "%s/%s", dir, de->d_name);

        struct stat st;
        if (stat(full, &st) != 0) continue;

        if (S_ISDIR(st.st_mode)) {
            ws_walk(full, root, mods, n, cap, use_cache, cg);
            continue;
        }
        if (!S_ISREG(st.st_mode) || !is_epl_source(de->d_name)) continue;

        ASTNode *ast = ep_parse_file(full, use_cache, use_cache);
        if (!ast) {
            fprintf(stderr, "  [skip] %s (parse failed)\n", full);
            continue;
        }

        if (*n >= *cap) {
            *cap = *cap ? *cap * 2 : 16;
            *mods = realloc(*mods, (size_t)(*cap) * sizeof(EPDModule));
        }
        EPDModule *m = &(*mods)[*n];
        const char *rel = full;
        size_t rl = strlen(root);
        if (strncmp(full, root, rl) == 0) {
            rel = full + rl;
            while (*rel == '/') rel++;
        }
        snprintf(m->path, sizeof m->path, "%s", rel);
        m->lines = count_lines(full);
        m->index = epd_build(ast, m->path);
        (*n)++;

        callgraph_collect(ast, m->path, cg);
        collect_imports(ast, full, root, m->path, cg);

        ast_free(ast);
    }
    closedir(d);
}

int main(int argc, char *argv[]) {
    if (isatty(STDERR_FILENO)) print_banner();

    if (argc < 2) {
        fprintf(stderr, "Usage: epl <file.EP>\n");
        fprintf(stderr, "       epl --tokens <file.EP>   (print token stream)\n");
        fprintf(stderr, "       epl --check <file.EP>    (parse only; exit 0 if valid)\n");
        fprintf(stderr, "       epl --permissions <file.EP>  (what the program may touch)\n\n");
        return 1;
    }

    int print_tokens  = 0;
    int print_ast     = 0;
    int show_describe = 0;
    int use_cache     = 1;
    int show_cache_status = 0;
    int check_only    = 0;
    int show_perms    = 0;
    const char *filepath = NULL;

    int do_index = 0;
    for (int i = 1; i < argc; i++) {
        if      (strcmp(argv[i], "--tokens")   == 0) print_tokens   = 1;
        else if (strcmp(argv[i], "--ast")      == 0) print_ast      = 1;
        else if (strcmp(argv[i], "--describe") == 0) show_describe  = 1;
        else if (strcmp(argv[i], "--index")    == 0) do_index       = 1;
        else if (strcmp(argv[i], "--no-cache") == 0) use_cache      = 0;
        else if (strcmp(argv[i], "--cache-status") == 0) show_cache_status = 1;
        else if (strcmp(argv[i], "--check")    == 0) { check_only = 1; use_cache = 0; }
        else if (strcmp(argv[i], "--permissions") == 0) show_perms = 1;
        else filepath = argv[i];
    }

    { const char *e = getenv("EPL_NO_CACHE"); if (e && *e && strcmp(e, "0") != 0) use_cache = 0; }

    if (!filepath) {
        fprintf(stderr, "EPL Error: No file specified\n");
        return 1;
    }
    if (show_perms) return show_permissions(filepath);

    {
        struct stat pst;
        int is_dir = (stat(filepath, &pst) == 0 && S_ISDIR(pst.st_mode));
        if (do_index || (show_describe && is_dir)) {
            if (!is_dir) {
                fprintf(stderr, "EPL Error: --index expects a directory, got '%s'\n", filepath);
                return 1;
            }
            return run_workspace_index(filepath, use_cache);
        }
    }

    LexMode lex_mode = LEX_EP;
    const char *ext = strrchr(filepath, '.');
    if (!ext) {
        fprintf(stderr, "EPL Warning: File '%s' has no extension (expected .EP)\n", filepath);
    } else {
        char ext_upper[8] = {0};
        for (int i = 0; i < 7 && ext[i]; i++)
            ext_upper[i] = (char)((ext[i] >= 'a' && ext[i] <= 'z') ? ext[i] - 32 : ext[i]);
        if (strcmp(ext_upper, ".EPF") == 0)
            lex_mode = LEX_EPF;
        else if (strcmp(ext_upper, ".EP") != 0 && strcmp(ext_upper, ".EPDB") != 0)
            fprintf(stderr, "EPL Warning: File '%s' does not have .EP, .EPF, or .EPDB extension\n", filepath);
    }

    char   *source = NULL;
    Token  *tokens = NULL;
    int     count  = 0;
    ASTNode *ast   = NULL;
    int      from_cache = 0;

    if (use_cache && !print_tokens && !print_ast) {
        ast = epc_load(filepath);
        if (ast) from_cache = 1;
    }

    if (!ast) {
        source = read_file(filepath);
        if (!source) return 1;

        Lexer lexer;
        lexer_init(&lexer, source, lex_mode);
        tokens = lexer_tokenize_all(&lexer, &count);

        if (!tokens) {
            fprintf(stderr, "EPL Error: Tokenization failed\n");
            free(source);
            return 1;
        }

        if (print_tokens) {
            for (int i = 0; i < count; i++) {
                if (tokens[i].type == TOKEN_NEWLINE)     continue;
                if (tokens[i].type == TOKEN_COMMENT)     continue;
                if (tokens[i].type == TOKEN_MULTICOMMENT)continue;
                print_token(tokens[i]);
            }
            lexer_free_tokens(tokens);
            free(source);
            return 0;
        }

        Parser parser;
        parser_init(&parser, tokens, count);
        ast = parser_parse(&parser);

        if (parser.had_error) {
            fprintf(stderr, "EPL: Parsing failed with errors.\n");
            ast_free(ast);
            lexer_free_tokens(tokens);
            free(source);
            return 1;
        }

        if (use_cache && !print_ast)
            epc_store(filepath, ast);
    }

    if (!print_ast && !show_describe &&
        (lex_mode == LEX_EPF ? actions_check_page(filepath, ast)
                             : actions_check_server(filepath, ast))) {
        ast_free(ast);
        lexer_free_tokens(tokens);
        free(source);
        return 1;
    }

    if (check_only) {
        ast_free(ast);
        lexer_free_tokens(tokens);
        free(source);
        return 0;
    }

    if (print_ast) {
        ast_print(ast, 2);
        ast_free(ast);
        lexer_free_tokens(tokens);
        free(source);
        return 0;
    }

    if (show_describe) {
        char epd_file[512];
        strncpy(epd_file, filepath, sizeof(epd_file) - 1);
        epd_file[sizeof(epd_file) - 1] = '\0';
        char *dot = strrchr(epd_file, '.');
        if (dot) strcpy(dot, ".EPD");
        else strncat(epd_file, ".EPD",
                     sizeof(epd_file) - strlen(epd_file) - 1);

        EPDIndex *idx = epd_build(ast, filepath);
        epd_write(idx, filepath, epd_file);
        epd_free(idx);

        ast_free(ast);
        lexer_free_tokens(tokens);
        free(source);
        return 0;
    }

    if (show_cache_status)
        fprintf(stderr, "[EPL] %s\n",
                from_cache ? "AST loaded from .EPC cache"
                           : "parsed from source (.EPC written)");

    Permissions perms;
    char perm_err[2048];
    if (perm_load(&perms, filepath, perm_err, sizeof perm_err) != 0) {
        fprintf(stderr, "[EPL Permissions] %s\n", perm_err);
        ast_free(ast);
        lexer_free_tokens(tokens);
        free(source);
        return 2;
    }

    int failed;
    {
        Interpreter interp;
        interpreter_init(&interp, filepath);
        interp.perms = &perms;
        interpreter_run(&interp, ast);
        failed = interp.had_error;
        interpreter_free(&interp);
    }
    perm_free(&perms);

    ast_free(ast);

    lexer_free_tokens(tokens);
    free(source);
    return failed ? 1 : 0;
}
