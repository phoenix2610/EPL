#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "lexer.h"
#include "token.h"

/* ─────────────────────────────────────────────
   EPL Tokenizer — lexer.c
   The heart of the tokenizer.
   Walks source code char by char and emits
   a clean stream of labeled Token structs.
   ───────────────────────────────────────────── */


/* ─────────────────────────────────────────────
   PHRASE TABLE
   All multi-word EPL keywords in one place.
   Sorted longest-first so greedy match works —
   "is any of" matched before "is any" or "is".
   ───────────────────────────────────────────── */
static PhraseEntry phrase_table[] = {
    { "is any of",          TOKEN_IS_ANY_OF      },
    { "is none of",         TOKEN_IS_NONE_OF     },
    { "is not",             TOKEN_IS_NOT         },
    { "not equals",         TOKEN_NOT_EQUALS     },
    { "greater than",       TOKEN_GREATER_THAN   },
    { "less than",          TOKEN_LESS_THAN      },
    { "at least",           TOKEN_AT_LEAST       },
    { "at most",            TOKEN_AT_MOST        },
    { "divided by",         TOKEN_DIVIDED_BY     },
    { "power of",           TOKEN_POWER_OF       },
    { "length of",          TOKEN_LENGTH_OF      },
    { "size of",            TOKEN_SIZE_OF        },
    { "for all values",     TOKEN_FOR_ALL        },
    { "for any value",      TOKEN_FOR_ANY        },
    { "for every value",    TOKEN_FOR_EVERY      },
    { "if fails",           TOKEN_IF_FAILS       },
    { "given that",         TOKEN_GIVEN_THAT     },
    { NULL,                 TOKEN_UNKNOWN        }   /* sentinel */
};


/* ─────────────────────────────────────────────
   SINGLE WORD KEYWORD TABLE
   Maps single words to their token types.
   ───────────────────────────────────────────── */
typedef struct {
    const char *word;
    TokenType   type;
} KeywordEntry;

static KeywordEntry keyword_table[] = {
    /* Core */
    { "set",            TOKEN_SET           },
    { "update",         TOKEN_UPDATE        },
    { "as",             TOKEN_AS            },
    { "to",             TOKEN_TO            },
    { "by",             TOKEN_BY            },
    { "from",           TOKEN_FROM          },
    { "with",           TOKEN_WITH          },
    { "and",            TOKEN_AND           },
    { "or",             TOKEN_OR            },
    { "not",            TOKEN_NOT           },
    { "in",             TOKEN_IN            },
    { "of",             TOKEN_OF            },
    /* Control flow */
    { "if",             TOKEN_IF            },
    { "then",           TOKEN_THEN          },
    { "otherwise",      TOKEN_OTHERWISE     },
    { "else",           TOKEN_ELSE          },
    { "while",          TOKEN_WHILE         },
    { "repeat",         TOKEN_REPEAT        },
    { "until",          TOKEN_UNTIL         },
    { "for",            TOKEN_FOR           },
    { "each",           TOKEN_EACH          },
    { "break",          TOKEN_BREAK         },
    { "continue",       TOKEN_CONTINUE      },
    { "return",         TOKEN_RETURN        },
    /* Loop */
    { "times",          TOKEN_TIMES         },
    { "indefinitely",   TOKEN_INDEFINITELY  },
    { "down",           TOKEN_DOWN          },
    /* Arithmetic */
    { "plus",           TOKEN_PLUS          },
    { "minus",          TOKEN_MINUS         },
    { "times",          TOKEN_TIMES_OP      },
    { "modulo",         TOKEN_MODULO        },
    /* Comparison */
    { "equals",         TOKEN_EQUALS        },
    { "is",             TOKEN_IS            },
    /* Advanced */
    { "respectively",   TOKEN_RESPECTIVELY  },
    { "item",           TOKEN_ITEM          },
    { "items",          TOKEN_ITEMS         },
    /* Output/Input */
    { "show",           TOKEN_SHOW          },
    { "display",        TOKEN_DISPLAY       },
    { "provide",        TOKEN_PROVIDE       },
    { "wait",           TOKEN_WAIT          },
    { "user",           TOKEN_USER          },
    { "input",          TOKEN_INPUT         },
    { "entry",          TOKEN_ENTRY         },
    /* Activity */
    { "describe",       TOKEN_DESCRIBE      },
    { "check",          TOKEN_CHECK         },
    { "that",           TOKEN_THAT          },
    { "attempt",        TOKEN_ATTEMPT       },
    /* Collection */
    { "add",            TOKEN_ADD           },
    { "empty",          TOKEN_EMPTY         },
    /* Booleans / Null */
    { "true",           TOKEN_BOOLEAN       },
    { "false",          TOKEN_BOOLEAN       },
    { "null",           TOKEN_NULL          },
    /* System */
    { "execute",        TOKEN_EXECUTE       },
    { "system",         TOKEN_SYSTEM        },
    { "command",        TOKEN_COMMAND       },
    { "windows",        TOKEN_WINDOWS       },
    { NULL,             TOKEN_UNKNOWN       }   /* sentinel */
};


/* ═════════════════════════════════════════════
   LEXER INIT
   ═════════════════════════════════════════════ */
void lexer_init(Lexer *lexer, const char *source) {
    lexer->source  = source;
    lexer->start   = 0;
    lexer->current = 0;
    lexer->line    = 1;
    lexer->col     = 1;
    lexer->length  = (int)strlen(source);
}


/* ═════════════════════════════════════════════
   BASIC NAVIGATION
   ═════════════════════════════════════════════ */

int lexer_at_end(Lexer *lexer) {
    return lexer->current >= lexer->length;
}

char lexer_peek(Lexer *lexer) {
    if (lexer_at_end(lexer)) return '\0';
    return lexer->source[lexer->current];
}

char lexer_peek_next(Lexer *lexer) {
    if (lexer->current + 1 >= lexer->length) return '\0';
    return lexer->source[lexer->current + 1];
}

char lexer_advance(Lexer *lexer) {
    char c = lexer->source[lexer->current++];
    if (c == '\n') {
        lexer->line++;
        lexer->col = 1;
    } else {
        lexer->col++;
    }
    return c;
}

void lexer_skip_whitespace(Lexer *lexer) {
    while (!lexer_at_end(lexer)) {
        char c = lexer_peek(lexer);
        if (c == ' ' || c == '\t' || c == '\r') {
            lexer_advance(lexer);
        } else {
            break;
        }
    }
}


/* ═════════════════════════════════════════════
   WORD READER
   Reads a full alphabetic word into buf.
   Stops at space, newline, punctuation.
   ═════════════════════════════════════════════ */
void lexer_read_word(Lexer *lexer, char *buf, int maxlen) {
    int i = 0;
    while (!lexer_at_end(lexer) && i < maxlen - 1) {
        char c = lexer_peek(lexer);
        if (isalpha(c) || c == '_') {
            buf[i++] = lexer_advance(lexer);
        } else {
            break;
        }
    }
    buf[i] = '\0';
}


/* ═════════════════════════════════════════════
   STRING READER
   Reads "..." including escape sequences.
   ═════════════════════════════════════════════ */
Token lexer_read_string(Lexer *lexer) {
    int    line = lexer->line;
    int    col  = lexer->col;
    char   buf[MAX_TOKEN_LEN];
    int    i = 0;

    lexer_advance(lexer);   /* consume opening " */

    while (!lexer_at_end(lexer)) {
        char c = lexer_peek(lexer);

        if (c == '"') {
            lexer_advance(lexer);   /* consume closing " */
            break;
        }

        /* Basic escape sequences */
        if (c == '\\') {
            lexer_advance(lexer);
            char esc = lexer_advance(lexer);
            switch (esc) {
                case 'n':  buf[i++] = '\n'; break;
                case 't':  buf[i++] = '\t'; break;
                case '"':  buf[i++] = '"';  break;
                case '\\': buf[i++] = '\\'; break;
                default:   buf[i++] = esc;  break;
            }
        } else {
            buf[i++] = lexer_advance(lexer);
        }

        if (i >= MAX_TOKEN_LEN - 1) break;   /* safety cap */
    }

    buf[i] = '\0';
    return make_token(TOKEN_STRING, buf, line, col);
}


/* ═════════════════════════════════════════════
   NUMBER READER
   Reads integers and floats.
   ═════════════════════════════════════════════ */
Token lexer_read_number(Lexer *lexer) {
    int  line = lexer->line;
    int  col  = lexer->col;
    char buf[MAX_TOKEN_LEN];
    int  i = 0;
    int  has_dot = 0;

    while (!lexer_at_end(lexer)) {
        char c = lexer_peek(lexer);
        if (isdigit(c)) {
            buf[i++] = lexer_advance(lexer);
        } else if (c == '.' && !has_dot && isdigit(lexer_peek_next(lexer))) {
            has_dot = 1;
            buf[i++] = lexer_advance(lexer);
        } else {
            break;
        }
        if (i >= MAX_TOKEN_LEN - 1) break;
    }

    buf[i] = '\0';
    return make_token(TOKEN_NUMBER, buf, line, col);
}


/* ═════════════════════════════════════════════
   COMMENT READERS
   ═════════════════════════════════════════════ */
Token lexer_read_comment(Lexer *lexer) {
    int  line = lexer->line;
    int  col  = lexer->col;
    char buf[MAX_TOKEN_LEN];
    int  i = 0;

    /* Check for ### multi-line comment */
    if (lexer->current + 2 < lexer->length &&
        lexer->source[lexer->current]     == '#' &&
        lexer->source[lexer->current + 1] == '#') {
        return lexer_read_multiline_comment(lexer);
    }

    /* Single line — consume # and read to end of line */
    lexer_advance(lexer);   /* consume # */
    while (!lexer_at_end(lexer) && lexer_peek(lexer) != '\n') {
        if (i < MAX_TOKEN_LEN - 1)
            buf[i++] = lexer_advance(lexer);
        else
            lexer_advance(lexer);
    }
    buf[i] = '\0';

    /* Trim leading space from comment text */
    char *text = buf;
    while (*text == ' ') text++;

    return make_token(TOKEN_COMMENT, text, line, col);
}

Token lexer_read_multiline_comment(Lexer *lexer) {
    int  line = lexer->line;
    int  col  = lexer->col;
    char buf[MAX_TOKEN_LEN];
    int  i = 0;

    /* Consume opening ### */
    lexer_advance(lexer);
    lexer_advance(lexer);
    lexer_advance(lexer);

    /* Read until closing ### */
    while (!lexer_at_end(lexer)) {
        if (lexer_peek(lexer) == '#' &&
            lexer->current + 2 < lexer->length &&
            lexer->source[lexer->current + 1] == '#' &&
            lexer->source[lexer->current + 2] == '#') {
            lexer_advance(lexer);
            lexer_advance(lexer);
            lexer_advance(lexer);
            break;
        }
        if (i < MAX_TOKEN_LEN - 1)
            buf[i++] = lexer_advance(lexer);
        else
            lexer_advance(lexer);
    }
    buf[i] = '\0';
    return make_token(TOKEN_MULTICOMMENT, buf, line, col);
}


/* ═════════════════════════════════════════════
   SINGLE WORD KEYWORD MATCHER
   Returns the TokenType for a known keyword
   or TOKEN_IDENTIFIER if not found.
   ═════════════════════════════════════════════ */
TokenType lexer_match_keyword(const char *word) {
    for (int i = 0; keyword_table[i].word != NULL; i++) {
        if (strcmp(keyword_table[i].word, word) == 0) {
            return keyword_table[i].type;
        }
    }
    return TOKEN_IDENTIFIER;
}


/* ═════════════════════════════════════════════
   MULTI-WORD PHRASE MATCHER
   Given the first word already read, tries to
   match against all known multi-word phrases.

   Saves and restores lexer position if no match.
   Writes matched phrase text into matched_phrase.
   Returns TOKEN_UNKNOWN if no phrase matched.
   ═════════════════════════════════════════════ */
TokenType lexer_match_phrase(Lexer *lexer, const char *first_word, char *matched_phrase) {
    /* Save current lexer state so we can backtrack */
    int saved_current = lexer->current;
    int saved_line    = lexer->line;
    int saved_col     = lexer->col;

    /* Build a working buffer starting with first word */
    char buf[MAX_TOKEN_LEN];
    strncpy(buf, first_word, MAX_TOKEN_LEN - 1);
    buf[MAX_TOKEN_LEN - 1] = '\0';

    TokenType best_match     = TOKEN_UNKNOWN;
    int       best_current   = saved_current;
    int       best_line      = saved_line;
    int       best_col       = saved_col;
    char      best_phrase[MAX_TOKEN_LEN];
    best_phrase[0] = '\0';

    /* Try to extend phrase word by word */
    int attempts = 0;
    while (attempts < 4) {   /* max 4 extra words in a phrase */
        attempts++;

        /* Skip whitespace between words */
        lexer_skip_whitespace(lexer);
        if (lexer_at_end(lexer) || lexer_peek(lexer) == '\n') break;

        /* Read next word */
        char next_word[MAX_TOKEN_LEN];
        if (!isalpha(lexer_peek(lexer))) break;
        lexer_read_word(lexer, next_word, MAX_TOKEN_LEN);

        /* Append to buf with space */
        strncat(buf, " ", MAX_TOKEN_LEN - strlen(buf) - 1);
        strncat(buf, next_word, MAX_TOKEN_LEN - strlen(buf) - 1);

        /* Check if buf matches any phrase */
        for (int i = 0; phrase_table[i].phrase != NULL; i++) {
            if (strcmp(phrase_table[i].phrase, buf) == 0) {
                /* Found a match — record it but keep going (greedy) */
                best_match   = phrase_table[i].type;
                best_current = lexer->current;
                best_line    = lexer->line;
                best_col     = lexer->col;
                strncpy(best_phrase, buf, MAX_TOKEN_LEN - 1);
                break;
            }
        }
    }

    if (best_match != TOKEN_UNKNOWN) {
        /* Restore to position AFTER the matched phrase */
        lexer->current = best_current;
        lexer->line    = best_line;
        lexer->col     = best_col;
        strncpy(matched_phrase, best_phrase, MAX_TOKEN_LEN - 1);
        return best_match;
    }

    /* No phrase matched — restore to right after first word */
    lexer->current = saved_current;
    lexer->line    = saved_line;
    lexer->col     = saved_col;
    return TOKEN_UNKNOWN;
}


/* ═════════════════════════════════════════════
   NEXT TOKEN — THE MAIN ENGINE
   Called repeatedly to produce one token at
   a time from the source.
   ═════════════════════════════════════════════ */
Token lexer_next_token(Lexer *lexer) {
    lexer_skip_whitespace(lexer);

    if (lexer_at_end(lexer))
        return make_token(TOKEN_EOF, "", lexer->line, lexer->col);

    lexer->start = lexer->current;
    int  line    = lexer->line;
    int  col     = lexer->col;
    char c       = lexer_peek(lexer);

    /* ── NEWLINE ── */
    if (c == '\n') {
        lexer_advance(lexer);
        return make_token(TOKEN_NEWLINE, "\\n", line, col);
    }

    /* ── COMMENT ── */
    if (c == '#') {
        return lexer_read_comment(lexer);
    }

    /* ── STRING ── */
    if (c == '"') {
        return lexer_read_string(lexer);
    }

    /* ── NUMBER ── */
    if (isdigit(c)) {
        return lexer_read_number(lexer);
    }

    /* ── SINGLE CHAR DELIMITERS ── */
    if (c == '!') { lexer_advance(lexer); return make_token(TOKEN_TERMINATOR, "!", line, col); }
    if (c == ',') { lexer_advance(lexer); return make_token(TOKEN_COMMA,       ",", line, col); }
    if (c == '(') { lexer_advance(lexer); return make_token(TOKEN_LPAREN,      "(", line, col); }
    if (c == ')') { lexer_advance(lexer); return make_token(TOKEN_RPAREN,      ")", line, col); }
    if (c == '[') { lexer_advance(lexer); return make_token(TOKEN_LBRACKET,    "[", line, col); }
    if (c == ']') { lexer_advance(lexer); return make_token(TOKEN_RBRACKET,    "]", line, col); }
    if (c == '+') { lexer_advance(lexer); return make_token(TOKEN_PLUS,        "+", line, col); }
    if (c == '-') { lexer_advance(lexer); return make_token(TOKEN_MINUS,       "-", line, col); }

    /* ── WORDS — keywords, phrases, identifiers ── */
    if (isalpha(c) || c == '_') {
        char word[MAX_TOKEN_LEN];
        lexer_read_word(lexer, word, MAX_TOKEN_LEN);

        /* First try multi-word phrase match */
        char matched_phrase[MAX_TOKEN_LEN];
        TokenType phrase_type = lexer_match_phrase(lexer, word, matched_phrase);

        if (phrase_type != TOKEN_UNKNOWN) {
            return make_token(phrase_type, matched_phrase, line, col);
        }

        /* Then try single-word keyword match */
        TokenType kw_type = lexer_match_keyword(word);
        return make_token(kw_type, word, line, col);
    }

    /* ── UNKNOWN CHARACTER ── */
    char unknown[2] = { c, '\0' };
    lexer_advance(lexer);
    return make_token(TOKEN_UNKNOWN, unknown, line, col);
}


/* ═════════════════════════════════════════════
   PEEK TOKEN
   Returns next token without consuming it.
   Saves and restores full lexer state.
   ═════════════════════════════════════════════ */
Token lexer_peek_token(Lexer *lexer) {
    int saved_current = lexer->current;
    int saved_line    = lexer->line;
    int saved_col     = lexer->col;
    int saved_start   = lexer->start;

    Token t = lexer_next_token(lexer);

    lexer->current = saved_current;
    lexer->line    = saved_line;
    lexer->col     = saved_col;
    lexer->start   = saved_start;

    return t;
}


/* ═════════════════════════════════════════════
   TOKENIZE ALL
   Runs the lexer to completion, returns a
   heap-allocated array of all tokens.
   Last token is always TOKEN_EOF.
   Caller must free with lexer_free_tokens().
   ═════════════════════════════════════════════ */
Token *lexer_tokenize_all(Lexer *lexer, int *count) {
    int     capacity = 256;
    int     size     = 0;
    Token  *tokens   = malloc(capacity * sizeof(Token));

    if (!tokens) {
        fprintf(stderr, "EPL Lexer: out of memory\n");
        *count = 0;
        return NULL;
    }

    while (1) {
        Token t = lexer_next_token(lexer);

        /* Grow array if needed */
        if (size >= capacity) {
            capacity *= 2;
            Token *resized = realloc(tokens, capacity * sizeof(Token));
            if (!resized) {
                fprintf(stderr, "EPL Lexer: out of memory during realloc\n");
                break;
            }
            tokens = resized;
        }

        tokens[size++] = t;
        if (t.type == TOKEN_EOF) break;
    }

    *count = size;
    return tokens;
}


/* ═════════════════════════════════════════════
   FREE TOKENS
   ═════════════════════════════════════════════ */
void lexer_free_tokens(Token *tokens) {
    free(tokens);
}
