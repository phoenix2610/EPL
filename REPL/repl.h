#ifndef REPL_H
#define REPL_H

#include "interpreter.h"

/* ─────────────────────────────────────────────
   EPL REPL — repl.h
   Interactive Read-Eval-Print Loop.
   Lets users type EPL directly in terminal.
   ───────────────────────────────────────────── */

#define REPL_VERSION    "1.0"
#define REPL_PROMPT     "epl> "
#define REPL_CONTINUE   "...> "     /* for multi-line input          */
#define MAX_INPUT_LEN   4096        /* max chars per input line      */
#define MAX_LINES       128         /* max lines in a multi-line block */

/* ─────────────────────────────────────────────
   REPL STATE
   ───────────────────────────────────────────── */
typedef struct {
    Interpreter interp;         /* persistent interpreter across lines  */
    int         running;        /* REPL loop flag                       */
    int         line_number;    /* tracks how many lines entered        */
} Repl;


/* ─────────────────────────────────────────────
   REPL FUNCTIONS
   ───────────────────────────────────────────── */

/* Start the REPL — blocks until user exits */
void repl_start(void);

/* Initialize REPL state */
void repl_init(Repl *repl);

/* Free REPL state */
void repl_free(Repl *repl);

/* Print the welcome banner */
void repl_banner(void);

/* Read one line or multi-line block from user */
int repl_read(Repl *repl, char *buf, int maxlen);

/* Evaluate and execute a source string */
void repl_eval(Repl *repl, const char *source);

/* Handle special REPL commands like :exit :help :clear */
int repl_handle_command(Repl *repl, const char *input);


#endif /* REPL_H */
