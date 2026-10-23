#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
unsigned int __attribute__((stdcall)) SetConsoleOutputCP(unsigned int wCodePageID);
#endif

#include "repl.h"
#include "lexer.h"
#include "parser.h"
#include "ast.h"
#include "interpreter.h"

static int count_open_blocks(const char *source) {
    static const char *openers[] = {
        "then", "while", "repeat", "indefinitely",
        "attempt", NULL
    };

    int depth = 0;
    char word[256];
    const char *p = source;

    while (*p) {
        if (*p == '"') {
            p++;
            while (*p && *p != '"') p++;
            if (*p) p++;
            continue;
        }
        if (*p == '#') {
            while (*p && *p != '\n') p++;
            continue;
        }

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

            for (int j = 0; openers[j]; j++) {
                if (strcmp(word, openers[j]) == 0) {
                    depth++;
                    break;
                }
            }
            continue;
        }

        if (*p == '!') { depth--; p++; continue; }
        p++;
    }
    return depth;
}

void repl_banner(void) {
#ifdef _WIN32
    SetConsoleOutputCP(65001);
#endif

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

void repl_init(Repl *repl) {
    interpreter_init(&repl->interp, "REPL");
    repl->running     = 1;
    repl->line_number = 0;
}

void repl_free(Repl *repl) {
    interpreter_free(&repl->interp);
}

int repl_read(Repl *repl, char *buf, int maxlen) {
    (void)repl;
    buf[0] = '\0';
    int total_len = 0;
    int open_blocks = 0;

    while (1) {
        if (total_len == 0)
            printf("%s", REPL_PROMPT);
        else
            printf("%s", REPL_CONTINUE);
        fflush(stdout);

        char line[MAX_INPUT_LEN];
        if (!fgets(line, MAX_INPUT_LEN, stdin)) return 0;

        int line_len = strlen(line);
        if (total_len + line_len >= maxlen - 1) break;
        strcat(buf, line);
        total_len += line_len;

        open_blocks = count_open_blocks(buf);

        if (open_blocks <= 0) break;
    }

    return 1;
}

int repl_handle_command(Repl *repl, const char *input) {
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
            for (int i = 0; i < env->capacity; i++) {
                if (!env->names[i]) continue;
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

void repl_eval(Repl *repl, const char *source) {
    if (!source || source[0] == '\0') return;

    Lexer lexer;
    lexer_init(&lexer, source, LEX_EP);
    int    count  = 0;
    Token *tokens = lexer_tokenize_all(&lexer, &count);
    if (!tokens) return;

    Parser parser;
    parser_init(&parser, tokens, count);
    ASTNode *ast = parser_parse(&parser);

    if (parser.had_error) {
        fprintf(stderr, "  Syntax error — check your EPL.\n\n");
        lexer_free_tokens(tokens);
        ast_free(ast);
        return;
    }

    repl->interp.signal.type  = SIG_NONE;
    repl->interp.signal.value = NULL;
    repl->interp.had_error    = 0;

    exec_block(&repl->interp,
               &ast->as.program.body,
               repl->interp.global);

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

void repl_start(void) {
    Repl repl;
    repl_init(&repl);
    repl_banner();

    char input[MAX_INPUT_LEN * MAX_LINES];

    while (repl.running) {
        if (!repl_read(&repl, input, sizeof(input))) {
            printf("\n  EPL out.\n\n");
            break;
        }

        char *trimmed = input;
        while (*trimmed == ' ' || *trimmed == '\t' ||
               *trimmed == '\n' || *trimmed == '\r') trimmed++;
        if (*trimmed == '\0') continue;

        if (trimmed[0] == ':') {
            repl_handle_command(&repl, trimmed);
            continue;
        }

        repl.line_number++;

        repl_eval(&repl, input);
    }

    repl_free(&repl);
}
