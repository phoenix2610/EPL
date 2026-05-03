#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "token.h"
#include "lexer.h"
#include "ast.h"
#include "parser.h"
#include "interpreter.h"
#include "repl.h"

/* ─────────────────────────────────────────────
   EPL Interpreter — main.c
   Entry point. Reads a .EP file, runs it
   through the lexer, prints the token stream.
   ───────────────────────────────────────────── */

#define MAX_SOURCE_SIZE 1048576   /* 1MB max source file */


/* ─────────────────────────────────────────────
   read_file
   Loads entire file into a heap string.
   Caller is responsible for freeing.
   ───────────────────────────────────────────── */
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

    fread(buf, 1, size, f);
    buf[size] = '\0';
    fclose(f);
    return buf;
}


/* ─────────────────────────────────────────────
   print_banner
   ───────────────────────────────────────────── */
void print_banner(void) {
    printf("\n");
    printf("  ███████╗██████╗ ██╗     \n");
    printf("  ██╔════╝██╔══██╗██║     \n");
    printf("  █████╗  ██████╔╝██║     \n");
    printf("  ██╔══╝  ██╔═══╝ ██║     \n");
    printf("  ███████╗██║     ███████╗\n");
    printf("  ╚══════╝╚═╝     ╚══════╝\n");
    printf("  English Programming Language v1.0\n");
    printf("  Tokenizer Stage\n\n");
}


/* ─────────────────────────────────────────────
   main
   ───────────────────────────────────────────── */
int main(int argc, char *argv[]) {
    print_banner();

    /* ── No file — launch REPL ── */
    if (argc < 2) {
        repl_start();
        return 0;
    }

    /* ── Flag handling ── */
    int print_tokens = 0;
    int print_ast    = 0;
    const char *filepath = NULL;

    for (int i = 1; i < argc; i++) {
        if      (strcmp(argv[i], "--tokens") == 0) print_tokens = 1;
        else if (strcmp(argv[i], "--ast")    == 0) print_ast    = 1;
        else filepath = argv[i];
    }

    if (!filepath) {
        fprintf(stderr, "EPL Error: No file specified\n");
        return 1;
    }

    /* ── File extension check ── */
    const char *ext = strrchr(filepath, '.');
    if (!ext || strcmp(ext, ".EP") != 0) {
        fprintf(stderr, "EPL Warning: File '%s' does not have .EP extension\n", filepath);
    }

    /* ── Read source file ── */
    char *source = read_file(filepath);
    if (!source) return 1;

    printf("  File    : %s\n", filepath);
    printf("  Size    : %zu bytes\n\n", strlen(source));

    /* ── Initialize lexer ── */
    Lexer lexer;
    lexer_init(&lexer, source);

    /* ── Tokenize ── */
    int    count  = 0;
    Token *tokens = lexer_tokenize_all(&lexer, &count);

    if (!tokens) {
        fprintf(stderr, "EPL Error: Tokenization failed\n");
        free(source);
        return 1;
    }

    /* ── Print token stream ── */
    if (print_tokens) {
        printf("  ── TOKEN STREAM ────────────────────────────\n\n");
        for (int i = 0; i < count; i++) {
            /* Skip newlines and comments in default view for cleanliness */
            if (tokens[i].type == TOKEN_NEWLINE) continue;
            if (tokens[i].type == TOKEN_COMMENT) continue;
            if (tokens[i].type == TOKEN_MULTICOMMENT) continue;
            print_token(tokens[i]);
        }
        printf("\n  ── END OF TOKEN STREAM ─────────────────────\n");
    }

    /* ── Summary ── */
    int unknown_count = 0;
    for (int i = 0; i < count; i++) {
        if (tokens[i].type == TOKEN_UNKNOWN) unknown_count++;
    }

    printf("\n  ── SUMMARY ─────────────────────────────────\n");
    printf("  Total tokens : %d\n", count);
    printf("  Unknown      : %d\n", unknown_count);

    if (unknown_count > 0) {
        printf("\n  ── UNKNOWN TOKENS (need attention) ─────────\n");
        for (int i = 0; i < count; i++) {
            if (tokens[i].type == TOKEN_UNKNOWN) {
                printf("  Line %-4d Col %-4d : \"%s\"\n",
                    tokens[i].line,
                    tokens[i].col,
                    tokens[i].value);
            }
        }
    }

    printf("\n  Tokenization complete.\n\n");

    /* ── PARSER STAGE ── */
    if (print_ast || (!print_tokens)) {
        printf("  ── PARSING ─────────────────────────────────\n\n");

        Parser parser;
        parser_init(&parser, tokens, count);
        ASTNode *ast = parser_parse(&parser);

        if (parser.had_error) {
            fprintf(stderr, "\n  EPL: Parsing failed with errors.\n\n");
            lexer_free_tokens(tokens);
            free(source);
            return 1;
        }

        printf("  Parse successful.\n");
        printf("  Top-level statements: %d\n\n",
            ast->as.program.body.count);

        if (print_ast) {
            printf("  ── AST ─────────────────────────────────────\n\n");
            ast_print(ast, 2);
            printf("\n  ── END OF AST ──────────────────────────────\n");
        }

        /* ── EXECUTION STAGE ── */
        if (!print_tokens && !print_ast) {
            Interpreter interp;
            interpreter_init(&interp);
            interpreter_run(&interp, ast);
            interpreter_free(&interp);
        }

        ast_free(ast);
    }

    /* ── Cleanup ── */
    lexer_free_tokens(tokens);
    free(source);
    return 0;
}
