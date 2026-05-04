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

    /* v1.1 — Smart Collection keywords */
    TOKEN_AVERAGE,        /* average  */
    TOKEN_TOTAL,          /* total    */
    TOKEN_MAXIMUM,        /* maximum  */
    TOKEN_MINIMUM,        /* minimum  */
    TOKEN_LENGTH,         /* length   */
    TOKEN_SORTED,         /* sorted   */
    TOKEN_REVERSED,       /* reversed */
    TOKEN_WHERE,          /* where    */

    /* v1.2 — String operations */
    TOKEN_UPPERCASE,       /* uppercase  */
    TOKEN_LOWERCASE,       /* lowercase  */
    TOKEN_TRIMMED,         /* trimmed    */
    TOKEN_SPLIT,           /* split      */
    TOKEN_JOINED,          /* joined     */
    TOKEN_REPLACED,        /* replaced   */
    TOKEN_STARTS,          /* starts     */
    TOKEN_ENDS,            /* ends       */
    TOKEN_CONTAINS,        /* contains   */

    /* v1.2 — Number formatting */
    TOKEN_CURRENCY,        /* currency   */
    TOKEN_PERCENTAGE,      /* percentage */
    TOKEN_ROUNDED,         /* rounded    */
    TOKEN_FORMATTED,       /* formatted  */

    /* v1.2 — Type checking keywords */
    TOKEN_A,               /* a          */
    TOKEN_NUMBER_KW,       /* number     */
    TOKEN_STRING_KW,       /* string     */
    TOKEN_BOOLEAN_KW,      /* boolean    */
    TOKEN_LIST_KW,         /* list       */

    /* v1.3 — Connection / Graph keywords */
    TOKEN_GRAPH,           /* graph                        */
    TOKEN_LINK,            /* link                         */
    TOKEN_ROUTE,           /* route                        */
    TOKEN_CONNECTED,       /* connected                    */
    TOKEN_NODE_KW,         /* node                         */
    TOKEN_EDGE_KW,         /* edge                         */

    /* v1.4 — EPF frontend keywords */
    TOKEN_MAKE_PAGE,       /* page        */
    TOKEN_SECTION_OUT,     /* section     */
    TOKEN_ROW,             /* row         */
    TOKEN_HEADING_OUT,     /* heading     */
    TOKEN_SUBHEADING,      /* subheading  */
    TOKEN_PARAGRAPH,       /* paragraph   */
    TOKEN_LABEL_OUT,       /* label       */
    TOKEN_BOLD_OUT,        /* bold        */
    TOKEN_DIVIDER,         /* divider     */
    TOKEN_SPACER,          /* spacer      */
    TOKEN_STYLED,          /* styled      */
    TOKEN_TITLED,          /* titled      */
    TOKEN_SAVED,           /* saved       */
    TOKEN_MAKE_CHART,      /* chart       */
    TOKEN_LINE_CHART,      /* line        */
    TOKEN_BAR_CHART,       /* bar         */
    TOKEN_PIE_CHART,       /* pie         */
    TOKEN_TABLE_OUT,       /* table       */
    TOKEN_MAKE_REPORT,     /* report      */
    TOKEN_MAKE_STAT,       /* stat        */
    TOKEN_X_AXIS,          /* x           */
    TOKEN_Y_AXIS,          /* y           */
    TOKEN_Z_AXIS,          /* z           */
    TOKEN_AXIS,            /* axis        */
    TOKEN_WITH_COLOR,      /* color       */
    TOKEN_HEADERS,         /* headers     */
    TOKEN_IMPORT,          /* import      */

    /* v1.6 — 3D canvas keywords */
    TOKEN_CANVAS3D,        /* canvas3d   */
    TOKEN_ORBITAL,         /* orbital    */
    TOKEN_RING_KW,         /* ring       */
    TOKEN_RADIUS_KW,       /* radius     */
    TOKEN_SPEED_KW,        /* speed      */
    TOKEN_NODE_3D,         /* node       */
    TOKEN_AT_KW,           /* at         */
    TOKEN_LABELED,         /* labeled    */
    TOKEN_STATUS_KW,       /* status     */
    TOKEN_CORE_KW,         /* core       */
    TOKEN_STAR_KW,         /* star       */
    TOKEN_BACKGROUND_KW,   /* background */
    TOKEN_PANEL_KW,        /* panel      */
    TOKEN_DRAWER_KW,       /* drawer     */
    TOKEN_OFFLINE,         /* offline    */
    TOKEN_FROM_GRAPH,      /* (from graph — multi-word, handled in parser) */
    TOKEN_VOID_BG,         /* void       */
    TOKEN_SPACE_BG,        /* space      */

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
