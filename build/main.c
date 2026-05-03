#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
unsigned int __attribute__((stdcall)) SetConsoleOutputCP(unsigned int wCodePageID);
#endif

#include "token.h"
#include "lexer.h"
#include "ast.h"
#include "parser.h"
#include "interpreter.h"
#include "describe.h"

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

    size_t read_size = fread(buf, 1, size, f);
    buf[read_size] = '\0';
    fclose(f);
    return buf;
}


/* ─────────────────────────────────────────────
   print_banner
   ───────────────────────────────────────────── */
void print_banner(void) {
#ifdef _WIN32
    SetConsoleOutputCP(65001);
#endif

    printf("\n");
    printf("  ███████╗██████╗ ██╗     \n");
    printf("  ██╔════╝██╔══██╗██║     \n");
    printf("  █████╗  ██████╔╝██║     \n");
    printf("  ██╔══╝  ██╔═══╝ ██║     \n");
    printf("  ███████╗██║     ███████╗\n");
    printf("  ╚══════╝╚═╝     ╚══════╝\n");
}



/* ─────────────────────────────────────────────
   main
   ───────────────────────────────────────────── */
int main(int argc, char *argv[]) {
    print_banner();

    /* ── Argument check ── */
    if (argc < 2) {
        fprintf(stderr, "Usage: epl <file.EP>\n");
        fprintf(stderr, "       epl --tokens <file.EP>   (print token stream)\n\n");
        return 1;
    }

    /* ── Flag handling ── */
    int print_tokens  = 0;
    int print_ast     = 0;
    int show_describe = 0;
    const char *filepath = NULL;

    for (int i = 1; i < argc; i++) {
        if      (strcmp(argv[i], "--tokens")   == 0) print_tokens   = 1;
        else if (strcmp(argv[i], "--ast")      == 0) print_ast      = 1;
        else if (strcmp(argv[i], "--describe") == 0) show_describe  = 1;
        else filepath = argv[i];
    }

    if (!filepath) {
        fprintf(stderr, "EPL Error: No file specified\n");
        return 1;
    }

    /* ── File extension check ── */
    const char *ext = strrchr(filepath, '.');
    if (!ext) {
        fprintf(stderr, "EPL Warning: File '%s' has no extension (expected .EP)\n", filepath);
    } else {
        /* Case-insensitive compare so .ep and .EP both work */
        char ext_upper[8] = {0};
        for (int i = 0; i < 7 && ext[i]; i++)
            ext_upper[i] = (char)((ext[i] >= 'a' && ext[i] <= 'z') ? ext[i] - 32 : ext[i]);
        if (strcmp(ext_upper, ".EP") != 0 && strcmp(ext_upper, ".EPF") != 0 && strcmp(ext_upper, ".EPDB") != 0)
            fprintf(stderr, "EPL Warning: File '%s' does not have .EP, .EPF, or .EPDB extension\n", filepath);
    }

    /* ── Read source file ── */
    char *source = read_file(filepath);
    if (!source) return 1;

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

    /* ── --tokens debug output ── */
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

    /* ── PARSER STAGE ── */
    {

        Parser parser;
        parser_init(&parser, tokens, count);
        ASTNode *ast = parser_parse(&parser);

        if (parser.had_error) {
            fprintf(stderr, "EPL: Parsing failed with errors.\n");
            lexer_free_tokens(tokens);
            free(source);
            return 1;
        }

        /* ── --ast debug output ── */
        if (print_ast) {
            ast_print(ast, 2);
            ast_free(ast);
            lexer_free_tokens(tokens);
            free(source);
            return 0;
        }

        /* ── --describe: build .EPD semantic index ── */
        if (show_describe) {
            char epd_file[512];
            strncpy(epd_file, filepath, sizeof(epd_file) - 1);
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

        /* ── EXECUTION STAGE ── */
        {
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
