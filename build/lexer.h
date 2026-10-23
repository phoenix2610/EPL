#ifndef LEXER_H
#define LEXER_H

#include "token.h"

#define LEX_EP   (1u << 0)
#define LEX_EPF  (1u << 1)
#define LEX_ANY  (LEX_EP | LEX_EPF)

typedef unsigned LexMode;

typedef struct {
    const char *source;
    int         start;
    int         current;
    int         line;
    int         col;
    int         length;
    LexMode     mode;
} Lexer;

typedef struct {
    const char *phrase;
    TokenType   type;
    LexMode     modes;
} PhraseEntry;

void lexer_init(Lexer *lexer, const char *source, LexMode mode);

Token lexer_next_token(Lexer *lexer);

Token lexer_peek_token(Lexer *lexer);

Token *lexer_tokenize_all(Lexer *lexer, int *count);

void lexer_free_tokens(Token *tokens);

int  lexer_at_end(Lexer *lexer);

char lexer_peek(Lexer *lexer);

char lexer_peek_next(Lexer *lexer);

char lexer_advance(Lexer *lexer);

void lexer_skip_whitespace(Lexer *lexer);

void lexer_read_word(Lexer *lexer, char *buf, int maxlen);

Token lexer_read_string(Lexer *lexer);

Token lexer_read_number(Lexer *lexer);

Token lexer_read_comment(Lexer *lexer);

Token lexer_read_multiline_comment(Lexer *lexer);

TokenType lexer_match_keyword(const char *word);
TokenType lexer_match_keyword_mode(const char *word, LexMode mode);

TokenType lexer_match_phrase(Lexer *lexer, const char *first_word, char *matched_phrase);

#endif
