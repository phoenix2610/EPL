#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "repl.h"
#include "lexer.h"
#include "parser.h"
#include "ast.h"
#include "interpreter.h"

/* ─────────────────────────────────────────────
   EPL REPL — repl.c
   Interactive Read-Eval-Print Loop.
   Maintains persistent state across inputs so
   variables and activities survive each line.
   ───────────────────────────────────────────── */


/* ═════════════════════════════════════════════
   BLOCK DETECTION
   Figures out if the current input opens a
   block that needs more lines.
   Counts block openers vs ! terminators.
   ═════════════════════════════════════════════ */
static int count_open_blocks(const char *source) {
    /* Keywords that open a block needing ! to close */
    static const char *openers[] = {
        "then", "while", "repeat", "indefinitely",
        "attempt", NULL
    };

    int depth = 0;
    char word[256];
    const char *p = source;

    while (*p) {
        /* Skip strings */
        if (*p == '"') {
            p++;
            while (*p && *p != '"') p++;
            if (*p) p++;
            continue;
        }
        /* Skip comments */
        if (*p == '#') {
            while (*p && *p != '\n') p++;
            continue;
        }

        /* Read a word */
        if ((*p >= 'a' && *p <= 'z') || (*p >= 'A' && *p <= 'Z')) {
            int i = 0;
            while ((*p >= 'a' && *p <= 'z') ||
                   (*p >= 'A' && *p <= 'Z') ||
                   (*p >= '0' && *p <= '9') ||
                   *p == '_') {
                if (i < 255) word[i++] = *p;
                p++;
            }
            word[i] = '\0';

            /* Check openers */
            for (int j = 0; openers[j]; j++) {
                if (strcmp(word, openers[j]) == 0) {
                    depth++;
                    break;
                }
            }
            /* Activity definition with comma = inline, no block */
            continue;
        }

        /* ! closes a block */
        if (*p == '!') { depth--; p++; continue; }
        p++;
    }
    return depth;
}


/* ═════════════════════════════════════════════
   REPL BANNER
   ═════════════════════════════════════════════ */
void repl_banner(void) {
    printf("\n");
    printf("  ╔═══════════════════════════════════════╗\n");
    printf("  ║  EPL — English Programming Language   ║\n");
    printf("  ║  Version %-5s  Interactive REPL       ║\n", REPL_VERSION);
    printf("  ╚═══════════════════════════════════════╝\n");
    printf("\n");
    printf("  Type EPL code and press Enter to run.\n");
    printf("  Multi-line blocks close with !\n");
    printf("\n");
    printf("  Commands:\n");
    printf("    :help    — show this help\n");
    printf("    :clear   — clear screen\n");
    printf("    :env     — show all variables\n");
    printf("    :exit    — quit the REPL\n");
    printf("\n");
    printf("  Example:\n");
    printf("    epl> set name as \"World\"\n");
    printf("    epl> show \"Hello, \" + name\n");
    printf("    Hello, World\n");
    printf("\n");
}


/* ═════════════════════════════════════════════
   REPL INIT / FREE
   ═════════════════════════════════════════════ */
void repl_init(Repl *repl) {
    interpreter_init(&repl->interp);
    repl->running     = 1;
    repl->line_number = 0;
}

void repl_free(Repl *repl) {
    interpreter_free(&repl->interp);
}


/* ═════════════════════════════════════════════
   REPL READ
   Reads one line. If that line opens a block
   (ends with 'then', 'while', etc.) keeps
   reading with continuation prompt until
   the block is closed with !
   Returns 1 if input was read, 0 on EOF.
   ═════════════════════════════════════════════ */
int repl_read(Repl *repl, char *buf, int maxlen) {
    (void)repl;
    buf[0] = '\0';
    int total_len = 0;
    int open_blocks = 0;

    while (1) {
        /* Print appropriate prompt */
        if (total_len == 0)
            printf("%s", REPL_PROMPT);
        else
            printf("%s", REPL_CONTINUE);
        fflush(stdout);

        /* Read a line */
        char line[MAX_INPUT_LEN];
        if (!fgets(line, MAX_INPUT_LEN, stdin)) return 0;   /* EOF / Ctrl+D */

        /* Append to buffer */
        int line_len = strlen(line);
        if (total_len + line_len >= maxlen - 1) break;
        strcat(buf, line);
        total_len += line_len;

        /* Recount open blocks across full buffer */
        open_blocks = count_open_blocks(buf);

        /* If all blocks are closed, we're done */
        if (open_blocks <= 0) break;
    }

    return 1;
}


/* ═════════════════════════════════════════════
   SPECIAL COMMAND HANDLER
   Returns 1 if input was a command (handled),
   0 if it's regular EPL code.
   ═════════════════════════════════════════════ */
int repl_handle_command(Repl *repl, const char *input) {
    /* Trim leading whitespace */
    while (*input == ' ' || *input == '\t') input++;

    if (strcmp(input, ":exit\n") == 0 ||
        strcmp(input, ":exit")   == 0 ||
        strcmp(input, ":quit\n") == 0 ||
        strcmp(input, ":quit")   == 0) {
        printf("\n  Goodbye, sir. EPL out.\n\n");
        repl->running = 0;
        return 1;
    }

    if (strcmp(input, ":help\n") == 0 ||
        strcmp(input, ":help")   == 0) {
        repl_banner();
        return 1;
    }

    if (strcmp(input, ":clear\n") == 0 ||
        strcmp(input, ":clear")   == 0) {
        #ifdef _WIN32
            system("cls");
        #else
            system("clear");
        #endif
        return 1;
    }

    if (strcmp(input, ":env\n") == 0 ||
        strcmp(input, ":env")   == 0) {
        Env *env = repl->interp.global;
        printf("\n  ── Variables ──────────────────────────\n");
        if (env->count == 0) {
            printf("  (no variables set)\n");
        } else {
            for (int i = 0; i < env->count; i++) {
                char *val_str = val_to_string(env->values[i]);
                printf("  %-20s = %s\n", env->names[i], val_str);
                free(val_str);
            }
        }
        printf("  ────────────────────────────────────────\n\n");
        return 1;
    }

    return 0;
}


/* ═════════════════════════════════════════════
   REPL EVAL
   Tokenizes, parses, and executes a source
   string inside the persistent interpreter.
   ═════════════════════════════════════════════ */
void repl_eval(Repl *repl, const char *source) {
    if (!source || source[0] == '\0') return;

    /* ── Tokenize ── */
    Lexer lexer;
    lexer_init(&lexer, source);
    int    count  = 0;
    Token *tokens = lexer_tokenize_all(&lexer, &count);
    if (!tokens) return;

    /* ── Parse ── */
    Parser parser;
    parser_init(&parser, tokens, count);
    ASTNode *ast = parser_parse(&parser);

    if (parser.had_error) {
        fprintf(stderr, "  Syntax error — check your EPL.\n\n");
        lexer_free_tokens(tokens);
        ast_free(ast);
        return;
    }

    /* ── Execute — reuse persistent global env ── */
    repl->interp.signal.type  = SIG_NONE;
    repl->interp.signal.value = NULL;
    repl->interp.had_error    = 0;

    /* Run statements directly in global env */
    exec_block(&repl->interp,
               &ast->as.program.body,
               repl->interp.global);

    /* If a bare expression produced a return value, show it */
    if (repl->interp.signal.type == SIG_RETURN &&
        repl->interp.signal.value) {
        char *s = val_to_string(repl->interp.signal.value);
        printf("  => %s\n", s);
        free(s);
        val_free(repl->interp.signal.value);
        repl->interp.signal.type  = SIG_NONE;
        repl->interp.signal.value = NULL;
    }

    lexer_free_tokens(tokens);
    ast_free(ast);
}


/* ═════════════════════════════════════════════
   REPL START — MAIN LOOP
   ═════════════════════════════════════════════ */
void repl_start(void) {
    Repl repl;
    repl_init(&repl);
    repl_banner();

    char input[MAX_INPUT_LEN * MAX_LINES];

    while (repl.running) {
        /* Read */
        if (!repl_read(&repl, input, sizeof(input))) {
            /* EOF — Ctrl+D */
            printf("\n  EPL out.\n\n");
            break;
        }

        /* Skip empty input */
        char *trimmed = input;
        while (*trimmed == ' ' || *trimmed == '\t' ||
               *trimmed == '\n' || *trimmed == '\r') trimmed++;
        if (*trimmed == '\0') continue;

        /* Handle special commands */
        if (trimmed[0] == ':') {
            repl_handle_command(&repl, trimmed);
            continue;
        }

        repl.line_number++;

        /* Eval */
        repl_eval(&repl, input);
    }

    repl_free(&repl);
}
