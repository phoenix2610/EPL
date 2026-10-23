#ifndef REPL_H
#define REPL_H

#include "interpreter.h"

#define REPL_VERSION    "1.0"
#define REPL_PROMPT     "epl> "
#define REPL_CONTINUE   "...> "
#define MAX_INPUT_LEN   4096
#define MAX_LINES       128

typedef struct {
    Interpreter interp;
    int         running;
    int         line_number;
} Repl;

void repl_start(void);

void repl_init(Repl *repl);

void repl_free(Repl *repl);

void repl_banner(void);

int repl_read(Repl *repl, char *buf, int maxlen);

void repl_eval(Repl *repl, const char *source);

int repl_handle_command(Repl *repl, const char *input);

#endif
