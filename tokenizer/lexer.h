#ifndef LEXER_H
#define LEXER_H

#include "token.h"

/* ─────────────────────────────────────────────
   EPL Tokenizer — lexer.h
   Defines the Lexer struct and all functions
   the lexer exposes to the rest of the system.
   ───────────────────────────────────────────── */


/* ─────────────────────────────────────────────
   LEXER STRUCT
   The lexer holds everything it needs to walk
   through source code character by character.

   source   → the full .EP file loaded as string
   start    → start of current token being read
   current  → current character position
   line     → current line number (for errors)
   col      → current column number (for errors)
   ───────────────────────────────────────────── */
typedef struct {
    const char *source;     /* full source string           */
    int         start;      /* start of current token       */
    int         current;    /* current read position        */
    int         line;       /* current line number          */
    int         col;        /* current column               */
    int         length;     /* total source length          */
} Lexer;


/* ─────────────────────────────────────────────
   PHRASE TABLE ENTRY
   Used for multi-word keyword matching.
   Maps a phrase string to its TokenType.
   ───────────────────────────────────────────── */
typedef struct {
    const char *phrase;     /* e.g. "is any of"             */
    TokenType   type;       /* e.g. TOKEN_IS_ANY_OF         */
} PhraseEntry;


/* ─────────────────────────────────────────────
   LEXER FUNCTIONS (implemented in lexer.c)
   ───────────────────────────────────────────── */

/* Initialize a lexer with source code string */
void lexer_init(Lexer *lexer, const char *source);

/* Get the next token from the source */
Token lexer_next_token(Lexer *lexer);

/* Peek at the next token without consuming it */
Token lexer_peek_token(Lexer *lexer);

/* Tokenize entire source, returns NULL-terminated Token array */
/* Caller is responsible for freeing the returned array        */
Token *lexer_tokenize_all(Lexer *lexer, int *count);

/* Free a token array produced by lexer_tokenize_all */
void lexer_free_tokens(Token *tokens);


/* ─────────────────────────────────────────────
   INTERNAL HELPERS (used within lexer.c only)
   Declared here for transparency/testing.
   ───────────────────────────────────────────── */

/* Is the lexer at end of source? */
int  lexer_at_end(Lexer *lexer);

/* Peek at current char without consuming */
char lexer_peek(Lexer *lexer);

/* Peek one char ahead without consuming */
char lexer_peek_next(Lexer *lexer);

/* Consume and return current char */
char lexer_advance(Lexer *lexer);

/* Skip whitespace (spaces, tabs) — NOT newlines */
void lexer_skip_whitespace(Lexer *lexer);

/* Read a full word (letters, digits, underscores) */
void lexer_read_word(Lexer *lexer, char *buf, int maxlen);

/* Read a string literal including quotes */
Token lexer_read_string(Lexer *lexer);

/* Read a number literal (int or float) */
Token lexer_read_number(Lexer *lexer);

/* Read a single line comment */
Token lexer_read_comment(Lexer *lexer);

/* Read a multi-line comment ### ... ### */
Token lexer_read_multiline_comment(Lexer *lexer);

/* Match a word against single-word keywords */
TokenType lexer_match_keyword(const char *word);

/* Match current position against multi-word phrases */
TokenType lexer_match_phrase(Lexer *lexer, const char *first_word, char *matched_phrase);


#endif /* LEXER_H */
