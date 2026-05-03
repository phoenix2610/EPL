#ifndef TOKEN_H
#define TOKEN_H

/* ─────────────────────────────────────────────
   EPL Tokenizer — token.h
   Defines every token type the lexer can emit.
   ───────────────────────────────────────────── */

/* Max length of any token's raw text value */
#define MAX_TOKEN_LEN 256

/* ─────────────────────────────────────────────
   TOKEN TYPES
   Grouped by category for readability.
   ───────────────────────────────────────────── */
typedef enum {

    /* ── LITERALS ── */
    TOKEN_NUMBER,           /* 42, 19.99                  */
    TOKEN_STRING,           /* "hello world"               */
    TOKEN_BOOLEAN,          /* true, false                 */
    TOKEN_NULL,             /* null                        */

    /* ── IDENTIFIER ── */
    TOKEN_IDENTIFIER,       /* salary, name, running       */

    /* ── CORE KEYWORDS ── */
    TOKEN_SET,              /* set                         */
    TOKEN_UPDATE,           /* update                      */
    TOKEN_AS,               /* as                          */
    TOKEN_TO,               /* to (activity definition)    */
    TOKEN_BY,               /* by (set result by adding)   */
    TOKEN_FROM,             /* from                        */
    TOKEN_WITH,             /* with                        */
    TOKEN_AND,              /* and                         */
    TOKEN_OR,               /* or                          */
    TOKEN_NOT,              /* not                         */
    TOKEN_IN,               /* in                          */
    TOKEN_OF,               /* of                          */

    /* ── CONTROL FLOW ── */
    TOKEN_IF,               /* if                          */
    TOKEN_THEN,             /* then                        */
    TOKEN_OTHERWISE,        /* otherwise                   */
    TOKEN_ELSE,             /* else                        */
    TOKEN_WHILE,            /* while                       */
    TOKEN_REPEAT,           /* repeat                      */
    TOKEN_UNTIL,            /* until                       */
    TOKEN_FOR,              /* for                         */
    TOKEN_EACH,             /* each                        */
    TOKEN_BREAK,            /* break                       */
    TOKEN_CONTINUE,         /* continue                    */
    TOKEN_RETURN,           /* return                      */

    /* ── LOOP VARIANTS ── */
    TOKEN_TIMES,            /* times (repeat 5 times)      */
    TOKEN_INDEFINITELY,     /* indefinitely                */
    TOKEN_DOWN,             /* down (for i from 10 down to)*/

    /* ── ARITHMETIC OPERATORS ── */
    TOKEN_PLUS,             /* plus                        */
    TOKEN_MINUS,            /* minus                       */
    TOKEN_TIMES_OP,         /* times (as operator)         */
    TOKEN_DIVIDED_BY,       /* divided by   [MULTI]        */
    TOKEN_MODULO,           /* modulo                      */
    TOKEN_POWER_OF,         /* power of     [MULTI]        */

    /* ── COMPARISON OPERATORS ── */
    TOKEN_EQUALS,           /* equals                      */
    TOKEN_IS,               /* is                          */
    TOKEN_NOT_EQUALS,       /* not equals   [MULTI]        */
    TOKEN_IS_NOT,           /* is not       [MULTI]        */
    TOKEN_GREATER_THAN,     /* greater than [MULTI]        */
    TOKEN_LESS_THAN,        /* less than    [MULTI]        */
    TOKEN_AT_LEAST,         /* at least     [MULTI]        */
    TOKEN_AT_MOST,          /* at most      [MULTI]        */

    /* ── ADVANCED OPERATORS ── */
    TOKEN_IS_ANY_OF,        /* is any of    [MULTI]        */
    TOKEN_IS_NONE_OF,       /* is none of   [MULTI]        */
    TOKEN_FOR_ALL,          /* for all values [MULTI]      */
    TOKEN_FOR_ANY,          /* for any value  [MULTI]      */
    TOKEN_FOR_EVERY,        /* for every value [MULTI]     */
    TOKEN_RESPECTIVELY,     /* respectively                */
    TOKEN_LENGTH_OF,        /* length of    [MULTI]        */
    TOKEN_SIZE_OF,          /* size of      [MULTI]        */
    TOKEN_ITEM,             /* item                        */
    TOKEN_ITEMS,            /* items                       */

    /* ── OUTPUT / INPUT ── */
    TOKEN_SHOW,             /* show                        */
    TOKEN_DISPLAY,          /* display                     */
    TOKEN_PROVIDE,          /* provide                     */
    TOKEN_WAIT,             /* wait                        */
    TOKEN_USER,             /* user                        */
    TOKEN_INPUT,            /* input                       */
    TOKEN_ENTRY,            /* entry                       */

    /* ── ACTIVITY KEYWORDS ── */
    TOKEN_DESCRIBE,         /* describe                    */
    TOKEN_CHECK,            /* check                       */
    TOKEN_THAT,             /* that (check that x > 0)     */
    TOKEN_ATTEMPT,          /* attempt                     */
    TOKEN_IF_FAILS,         /* if fails     [MULTI]        */
    TOKEN_GIVEN_THAT,       /* given that   [MULTI]        */

    /* ── COLLECTION ── */
    TOKEN_ADD,              /* add (add x to list)         */
    TOKEN_EMPTY,            /* empty                       */

    /* ── SYSTEM ── */
    TOKEN_EXECUTE,          /* execute                     */
    TOKEN_SYSTEM,           /* system                      */
    TOKEN_COMMAND,          /* command                     */
    TOKEN_WINDOWS,          /* windows                     */

    /* ── DELIMITERS ── */
    TOKEN_COMMA,            /* ,                           */
    TOKEN_LPAREN,           /* (                           */
    TOKEN_RPAREN,           /* )                           */
    TOKEN_LBRACKET,         /* [                           */
    TOKEN_RBRACKET,         /* ]                           */

    /* ── STRUCTURE ── */
    TOKEN_TERMINATOR,       /* !  (block end)              */
    TOKEN_NEWLINE,          /* \n                          */
    TOKEN_COMMENT,          /* # single line comment       */
    TOKEN_MULTICOMMENT,     /* ### multi line comment ###  */
    TOKEN_EOF,              /* end of file                 */

    /* ── ERROR ── */
    TOKEN_UNKNOWN           /* unrecognized token          */

} TokenType;


/* ─────────────────────────────────────────────
   TOKEN STRUCT
   Every token carries:
   - its type (what it is)
   - its raw value (what it literally says)
   - its position (where in source for errors)
   ───────────────────────────────────────────── */
typedef struct {
    TokenType   type;
    char        value[MAX_TOKEN_LEN];   /* raw text of the token    */
    int         line;                   /* line number in source    */
    int         col;                    /* column number in source  */
} Token;


/* ─────────────────────────────────────────────
   UTILITY FUNCTIONS (implemented in token.c)
   ───────────────────────────────────────────── */

/* Create a token */
Token make_token(TokenType type, const char *value, int line, int col);

/* Get human-readable name of a token type (for debugging) */
const char *token_type_name(TokenType type);

/* Print a token to stdout (for debugging) */
void print_token(Token token);


#endif /* TOKEN_H */
