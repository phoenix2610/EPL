#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "lexer.h"
#include "token.h"

static PhraseEntry phrase_table[] = {
    { "is any of",             TOKEN_IS_ANY_OF,       LEX_ANY },
    { "is none of",            TOKEN_IS_NONE_OF,      LEX_ANY },
    { "is not",                TOKEN_IS_NOT,          LEX_ANY },
    { "is greater than",       TOKEN_GREATER_THAN,    LEX_ANY },
    { "is less than",          TOKEN_LESS_THAN,       LEX_ANY },
    { "not equals",            TOKEN_NOT_EQUALS,      LEX_ANY },
    { "greater than",          TOKEN_GREATER_THAN,    LEX_ANY },
    { "less than",             TOKEN_LESS_THAN,       LEX_ANY },
    { "at least",              TOKEN_AT_LEAST,        LEX_ANY },
    { "at most",               TOKEN_AT_MOST,         LEX_ANY },
    { "divided by",            TOKEN_DIVIDED_BY,      LEX_ANY },
    { "power of",              TOKEN_POWER_OF,        LEX_ANY },
    { "size of",               TOKEN_SIZE_OF,         LEX_ANY },
    { "keys of",               TOKEN_KEYS_OF,         LEX_ANY },
    { "code of",               TOKEN_CODE_OF,         LEX_ANY },
    { "character with code",   TOKEN_CHAR_CODE,       LEX_ANY },
    { "sha256 of",             TOKEN_SHA256_OF,       LEX_ANY },
    { "random token of",       TOKEN_RANDOM_TOKEN,    LEX_ANY },
    { "random number from",    TOKEN_RANDOM_NUMBER,   LEX_ANY },
    { "for all values",        TOKEN_FOR_ALL,         LEX_ANY },
    { "for any value",         TOKEN_FOR_ANY,         LEX_ANY },
    { "for every value",       TOKEN_FOR_EVERY,       LEX_ANY },
    { "if fails",              TOKEN_IF_FAILS,        LEX_ANY },
    { "given that",            TOKEN_GIVEN_THAT,      LEX_ANY },
    { "error message",         TOKEN_ERROR_MSG_PH,    LEX_ANY },
    { "parse json",            TOKEN_PARSE_JSON_PH,   LEX_ANY },
    { "to json",               TOKEN_TO_JSON_PH,      LEX_ANY },
    { "fail with",             TOKEN_FAIL_WITH_PH,    LEX_ANY },
    { "make line chart",       TOKEN_LINE_CHART,      LEX_EPF },
    { "make bar chart",        TOKEN_BAR_CHART,       LEX_EPF },
    { "make pie chart",        TOKEN_PIE_CHART,       LEX_EPF },
    { "make table",            TOKEN_TABLE_OUT,       LEX_EPF },
    { "make report",           TOKEN_MAKE_REPORT,     LEX_EPF },
    { "make stat",             TOKEN_MAKE_STAT,       LEX_EPF },
    { "make page",             TOKEN_MAKE_PAGE,       LEX_EPF },
    { "make canvas3d",         TOKEN_CANVAS3D,        LEX_EPF },
    { "use design system",     TOKEN_USE_DESIGN,      LEX_EPF },
    { "make navbar",           TOKEN_MAKE_NAVBAR,     LEX_EPF },
    { "make hero",             TOKEN_MAKE_HERO,       LEX_EPF },
    { "make gallery",          TOKEN_MAKE_GALLERY,    LEX_EPF },
    { "make form",             TOKEN_MAKE_FORM,       LEX_EPF },
    { "make button",           TOKEN_MAKE_BUTTON,     LEX_EPF },
    { "make modal",            TOKEN_MAKE_MODAL,      LEX_EPF },
    { "make footer",           TOKEN_MAKE_FOOTER,     LEX_EPF },
    { "content section",       TOKEN_CONTENT_SECTION, LEX_EPF },
    { "overlay card",          TOKEN_OVERLAY_CARD,    LEX_EPF },
    { "nav link",              TOKEN_NAV_LINK,        LEX_EPF },
    { "action button",         TOKEN_ACTION_BTN,      LEX_EPF },
    { "left column",           TOKEN_LEFT_COL,        LEX_EPF },
    { "right column",          TOKEN_RIGHT_COL,       LEX_EPF },
    { "item list",             TOKEN_ITEM_LIST_KW,    LEX_EPF },
    { "feature image",         TOKEN_FEATURE_IMAGE,   LEX_EPF },
    { "price badge",           TOKEN_PRICE_BADGE,     LEX_EPF },
    { "submit button",         TOKEN_SUBMIT_BTN,      LEX_EPF },
    { "dismiss button",        TOKEN_DISMISS_BTN,     LEX_EPF },
    { "guest picker",          TOKEN_GUEST_PICKER,    LEX_EPF },
    { "date picker",           TOKEN_DATE_PICKER,     LEX_EPF },
    { "open modal",            TOKEN_OPEN_MODAL,      LEX_EPF },
    { "on click",              TOKEN_ON_CLICK,        LEX_EPF },
    { "show loading",          TOKEN_SHOW_LOADING,    LEX_EPF },
    { "serve on socket",       TOKEN_SERVE_SOCK_PH,   LEX_ANY },
    { "serve on port",         TOKEN_SERVE_PORT_PH,   LEX_EP  },
    { "append to file",        TOKEN_APPEND_FILE_PH,  LEX_ANY },
    { "write to file",         TOKEN_WRITE_FILE_PH,   LEX_ANY },
    { "field from json",       TOKEN_FIELD_JSON_PH,   LEX_ANY },
    { "file exists",           TOKEN_FILE_EXISTS_PH,  LEX_ANY },
    { "delete file",           TOKEN_DELETE_FILE_PH,  LEX_ANY },
    { "read file",             TOKEN_READ_FILE_PH,    LEX_ANY },
    { "reply with",            TOKEN_REPLY_WITH_PH,   LEX_ANY },
    { "send json",             TOKEN_SEND_JSON_PH,    LEX_ANY },
    { "post json",             TOKEN_POST_JSON_PH,    LEX_ANY },
    { "get env",               TOKEN_GET_ENV_PH,      LEX_ANY },

    { "count records in",      TOKEN_DB_COUNT,        LEX_ANY },
    { "records in",            TOKEN_DB_RECORDS,      LEX_ANY },
    { "make table",            TOKEN_DB_MAKE_TABLE,   LEX_EP  },
    { "save into",             TOKEN_DB_SAVE,         LEX_EP  },
    { "delete from",           TOKEN_DB_DELETE,       LEX_EP  },

    { NULL,                TOKEN_UNKNOWN        }
};

typedef struct {
    const char *word;
    TokenType   type;
    LexMode     modes;
} KeywordEntry;

static KeywordEntry keyword_table[] = {
    { "set",           TOKEN_SET,             LEX_ANY },
    { "update",        TOKEN_UPDATE,          LEX_ANY },
    { "as",            TOKEN_AS,              LEX_ANY },
    { "to",            TOKEN_TO,              LEX_ANY },
    { "by",            TOKEN_BY,              LEX_ANY },
    { "from",          TOKEN_FROM,            LEX_ANY },
    { "with",          TOKEN_WITH,            LEX_ANY },
    { "and",           TOKEN_AND,             LEX_ANY },
    { "or",            TOKEN_OR,              LEX_ANY },
    { "not",           TOKEN_NOT,             LEX_ANY },
    { "in",            TOKEN_IN,              LEX_ANY },
    { "of",            TOKEN_OF,              LEX_ANY },
    { "if",            TOKEN_IF,              LEX_ANY },
    { "then",          TOKEN_THEN,            LEX_ANY },
    { "otherwise",     TOKEN_OTHERWISE,       LEX_ANY },
    { "else",          TOKEN_ELSE,            LEX_ANY },
    { "while",         TOKEN_WHILE,           LEX_ANY },
    { "repeat",        TOKEN_REPEAT,          LEX_ANY },
    { "until",         TOKEN_UNTIL,           LEX_ANY },
    { "for",           TOKEN_FOR,             LEX_ANY },
    { "each",          TOKEN_EACH,            LEX_ANY },
    { "break",         TOKEN_BREAK,           LEX_ANY },
    { "continue",      TOKEN_CONTINUE,        LEX_ANY },
    { "return",        TOKEN_RETURN,          LEX_ANY },
    { "times",         TOKEN_TIMES,           LEX_ANY },
    { "indefinitely",  TOKEN_INDEFINITELY,    LEX_ANY },
    { "down",          TOKEN_DOWN,            LEX_ANY },
    { "plus",          TOKEN_PLUS,            LEX_ANY },
    { "minus",         TOKEN_MINUS,           LEX_ANY },
    { "times",         TOKEN_TIMES_OP,        LEX_ANY },
    { "modulo",        TOKEN_MODULO,          LEX_ANY },
    { "equals",        TOKEN_EQUALS,          LEX_ANY },
    { "is",            TOKEN_IS,              LEX_ANY },
    { "item",          TOKEN_ITEM,            LEX_ANY },
    { "show",          TOKEN_SHOW,            LEX_ANY },
    { "display",       TOKEN_DISPLAY,         LEX_ANY },
    { "provide",       TOKEN_PROVIDE,         LEX_ANY },
    { "wait",          TOKEN_WAIT,            LEX_ANY },
    { "entry",         TOKEN_ENTRY,           LEX_EPF },
    { "describe",      TOKEN_DESCRIBE,        LEX_ANY },
    { "check",         TOKEN_CHECK,           LEX_ANY },
    { "that",          TOKEN_THAT,            LEX_ANY },
    { "attempt",       TOKEN_ATTEMPT,         LEX_ANY },
    { "taking",        TOKEN_TAKING,          LEX_ANY },
    { "add",           TOKEN_ADD,             LEX_ANY },
    { "average",       TOKEN_AVERAGE,         LEX_ANY },
    { "empty",         TOKEN_EMPTY,           LEX_ANY },
    { "length",        TOKEN_LENGTH,          LEX_ANY },
    { "maximum",       TOKEN_MAXIMUM,         LEX_ANY },
    { "minimum",       TOKEN_MINIMUM,         LEX_ANY },
    { "reversed",      TOKEN_REVERSED,        LEX_ANY },
    { "sorted",        TOKEN_SORTED,          LEX_ANY },
    { "total",         TOKEN_TOTAL,           LEX_ANY },
    { "where",         TOKEN_WHERE,           LEX_ANY },
    { "true",          TOKEN_BOOLEAN,         LEX_ANY },
    { "false",         TOKEN_BOOLEAN,         LEX_ANY },
    { "null",          TOKEN_NULL,            LEX_ANY },
    { "execute",       TOKEN_EXECUTE,         LEX_ANY },
    { "system",        TOKEN_SYSTEM,          LEX_ANY },
    { "command",       TOKEN_COMMAND,         LEX_ANY },
    { "windows",       TOKEN_WINDOWS,         LEX_ANY },
    { "connected",     TOKEN_CONNECTED,       LEX_ANY },
    { "graph",         TOKEN_GRAPH,           LEX_ANY },
    { "link",          TOKEN_LINK,            LEX_ANY },
    { "node",          TOKEN_NODE_3D,         LEX_EPF },
    { "route",         TOKEN_ROUTE,           LEX_ANY },
    { "axis",          TOKEN_AXIS,            LEX_EPF },
    { "bar",           TOKEN_BAR_CHART,       LEX_EPF },
    { "bold",          TOKEN_BOLD_OUT,        LEX_EPF },
    { "chart",         TOKEN_MAKE_CHART,      LEX_EPF },
    { "color",         TOKEN_WITH_COLOR,      LEX_EPF },
    { "divider",       TOKEN_DIVIDER,         LEX_EPF },
    { "heading",       TOKEN_HEADING_OUT,     LEX_EPF },
    { "headers",       TOKEN_HEADERS,         LEX_EPF },
    { "import",        TOKEN_IMPORT,          LEX_ANY },
    { "label",         TOKEN_LABEL_OUT,       LEX_EPF },
    { "line",          TOKEN_LINE_CHART,      LEX_EPF },
    { "page",          TOKEN_MAKE_PAGE,       LEX_EPF },
    { "paragraph",     TOKEN_PARAGRAPH,       LEX_EPF },
    { "pie",           TOKEN_PIE_CHART,       LEX_EPF },
    { "report",        TOKEN_MAKE_REPORT,     LEX_EPF },
    { "row",           TOKEN_ROW,             LEX_EPF },
    { "saved",         TOKEN_SAVED,           LEX_EPF },
    { "section",       TOKEN_SECTION_OUT,     LEX_EPF },
    { "spacer",        TOKEN_SPACER,          LEX_EPF },
    { "stat",          TOKEN_MAKE_STAT,       LEX_EPF },
    { "styled",        TOKEN_STYLED,          LEX_EPF },
    { "subheading",    TOKEN_SUBHEADING,      LEX_EPF },
    { "table",         TOKEN_TABLE_OUT,       LEX_EPF },
    { "titled",        TOKEN_TITLED,          LEX_EPF },
    { "x",             TOKEN_X_AXIS,          LEX_EPF },
    { "y",             TOKEN_Y_AXIS,          LEX_EPF },
    { "z",             TOKEN_Z_AXIS,          LEX_EPF },
    { "a",             TOKEN_A,               LEX_ANY },
    { "boolean",       TOKEN_BOOLEAN_KW,      LEX_ANY },
    { "contains",      TOKEN_CONTAINS,        LEX_ANY },
    { "currency",      TOKEN_CURRENCY,        LEX_ANY },
    { "ends",          TOKEN_ENDS,            LEX_ANY },
    { "formatted",     TOKEN_FORMATTED,       LEX_ANY },
    { "joined",        TOKEN_JOINED,          LEX_ANY },
    { "list",          TOKEN_LIST_KW,         LEX_ANY },
    { "lowercase",     TOKEN_LOWERCASE,       LEX_ANY },
    { "number",        TOKEN_NUMBER_KW,       LEX_ANY },
    { "percentage",    TOKEN_PERCENTAGE,      LEX_ANY },
    { "replaced",      TOKEN_REPLACED,        LEX_ANY },
    { "rounded",       TOKEN_ROUNDED,         LEX_ANY },
    { "split",         TOKEN_SPLIT,           LEX_ANY },
    { "starts",        TOKEN_STARTS,          LEX_ANY },
    { "string",        TOKEN_STRING_KW,       LEX_ANY },
    { "trimmed",       TOKEN_TRIMMED,         LEX_ANY },
    { "uppercase",     TOKEN_UPPERCASE,       LEX_ANY },
    { "at",            TOKEN_AT_KW,           LEX_EPF },
    { "background",    TOKEN_BACKGROUND_KW,   LEX_EPF },
    { "canvas3d",      TOKEN_CANVAS3D,        LEX_EPF },
    { "core",          TOKEN_CORE_KW,         LEX_EPF },
    { "drawer",        TOKEN_DRAWER_KW,       LEX_EPF },
    { "labeled",       TOKEN_LABELED,         LEX_EPF },
    { "labelled",      TOKEN_LABELED,         LEX_EPF },
    { "offline",       TOKEN_OFFLINE,         LEX_EPF },
    { "orbital",       TOKEN_ORBITAL,         LEX_EPF },
    { "panel",         TOKEN_PANEL_KW,        LEX_EPF },
    { "radius",        TOKEN_RADIUS_KW,       LEX_EPF },
    { "ring",          TOKEN_RING_KW,         LEX_EPF },
    { "speed",         TOKEN_SPEED_KW,        LEX_EPF },
    { "star",          TOKEN_STAR_KW,         LEX_EPF },
    { "status",        TOKEN_STATUS_KW,       LEX_EPF },
    { "button",        TOKEN_BUTTON_KW,       LEX_EPF },
    { "class",         TOKEN_CLASS_KW,        LEX_EPF },
    { "div",           TOKEN_DIV_KW,          LEX_EPF },
    { "id",            TOKEN_ID_KW,           LEX_EPF },
    { "image",         TOKEN_IMAGE_KW,        LEX_EPF },
    { "input",         TOKEN_INPUT_KW,        LEX_EPF },
    { "onclick",       TOKEN_ONCLICK_KW,      LEX_EPF },
    { "placeholder",   TOKEN_PLACEHOLDER_KW,  LEX_EPF },
    { "span",          TOKEN_SPAN_KW,         LEX_EPF },
    { "src",           TOKEN_SRC_KW,          LEX_EPF },
    { "type",          TOKEN_TYPE_KW,         LEX_EPF },
    { "value",         TOKEN_VALUE_KW,        LEX_EPF },
    { "branded",       TOKEN_BRANDED,         LEX_ANY },
    { "eyebrow",       TOKEN_EYEBROW,         LEX_ANY },
    { "accented",      TOKEN_ACCENTED,        LEX_ANY },
    { "dimmed",        TOKEN_DIMMED,          LEX_ANY },
    { "aspect",        TOKEN_ASPECT,          LEX_ANY },
    { "columns",       TOKEN_COLUMNS,         LEX_ANY },
    { "offset",        TOKEN_OFFSET,          LEX_ANY },
    { "numbered",      TOKEN_NUMBERED,        LEX_ANY },
    { "detail",        TOKEN_DETAIL_KW,       LEX_ANY },
    { "tagline",       TOKEN_TAGLINE,         LEX_ANY },
    { "address",       TOKEN_ADDRESS_KW,      LEX_ANY },
    { "copyright",     TOKEN_COPYRIGHT,       LEX_ANY },
    { "field",         TOKEN_FIELD,           LEX_ANY },
    { "entry",         TOKEN_ENTRY_KW,        LEX_EPF },
    { "icons",         TOKEN_ICONS_KW,        LEX_ANY },
    { "frosted",       TOKEN_FROSTED,         LEX_ANY },
    { "read",          TOKEN_READ_KW,         LEX_ANY },
    { "write",         TOKEN_WRITE_KW,        LEX_ANY },
    { "append",        TOKEN_APPEND_KW,       LEX_ANY },
    { "file",          TOKEN_FILE_KW,         LEX_ANY },
    { "delete",        TOKEN_DELETE_KW,       LEX_ANY },
    { "run",           TOKEN_RUN_KW,          LEX_ANY },
    { "now",           TOKEN_NOW_KW,          LEX_ANY },
    { "sleep",         TOKEN_SLEEP_KW,        LEX_ANY },
    { "socket",        TOKEN_SOCKET_KW,       LEX_ANY },
    { "json",          TOKEN_JSON_KW,         LEX_ANY },
    { "using",         TOKEN_USING_KW,        LEX_ANY },
    { NULL,             TOKEN_UNKNOWN       }
};

void lexer_init(Lexer *lexer, const char *source, LexMode mode) {
    lexer->mode = mode;
    lexer->source  = source;
    lexer->start   = 0;
    lexer->current = 0;
    lexer->line    = 1;
    lexer->col     = 1;
    lexer->length  = (int)strlen(source);
}

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

void lexer_read_word(Lexer *lexer, char *buf, int maxlen) {
    int i = 0;
    while (!lexer_at_end(lexer) && i < maxlen - 1) {
        char c = lexer_peek(lexer);
        if (isalpha(c) || c == '_' || (i > 0 && isdigit(c))) {
            buf[i++] = lexer_advance(lexer);
        } else {
            break;
        }
    }
    buf[i] = '\0';
}

Token lexer_read_string(Lexer *lexer) {
    int    line = lexer->line;
    int    col  = lexer->col;
    char   buf[MAX_TOKEN_LEN];
    int    i = 0;

    lexer_advance(lexer);

    while (!lexer_at_end(lexer)) {
        char c = lexer_peek(lexer);

        if (c == '"') {
            lexer_advance(lexer);
            break;
        }

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

        if (i >= MAX_TOKEN_LEN - 1) {
            fprintf(stderr,
                "[EPL Error] Line %d: string literal exceeds %d characters\n",
                line, MAX_TOKEN_LEN - 1);
            break;
        }
    }

    buf[i] = '\0';
    return make_token(TOKEN_STRING, buf, line, col);
}

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

Token lexer_read_comment(Lexer *lexer) {
    int  line = lexer->line;
    int  col  = lexer->col;
    char buf[MAX_TOKEN_LEN];
    int  i = 0;

    if (lexer->current + 2 < lexer->length &&
        lexer->source[lexer->current]     == '#' &&
        lexer->source[lexer->current + 1] == '#') {
        return lexer_read_multiline_comment(lexer);
    }

    lexer_advance(lexer);
    while (!lexer_at_end(lexer) && lexer_peek(lexer) != '\n') {
        if (i < MAX_TOKEN_LEN - 1)
            buf[i++] = lexer_advance(lexer);
        else
            lexer_advance(lexer);
    }
    buf[i] = '\0';

    char *text = buf;
    while (*text == ' ') text++;

    return make_token(TOKEN_COMMENT, text, line, col);
}

Token lexer_read_multiline_comment(Lexer *lexer) {
    int  line = lexer->line;
    int  col  = lexer->col;
    char buf[MAX_TOKEN_LEN];
    int  i = 0;

    lexer_advance(lexer);
    lexer_advance(lexer);
    lexer_advance(lexer);

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

#define LEX_HASH_CAP 2048

typedef struct {
    const char *key;
    int first_kw;
    int first_phrase;
    int is_prefix;
} LexSlot;

static LexSlot lex_slots[LEX_HASH_CAP];
static int     kw_next[512];
static int     phrase_next[256];
static char    prefix_store[256][MAX_TOKEN_LEN / 4];
static int     lex_index_ready = 0;

static unsigned lex_hash(const char *s) {
    unsigned h = 2166136261u;
    while (*s) { h ^= (unsigned char)*s++; h *= 16777619u; }
    return h;
}

static LexSlot *lex_slot(const char *key, int create) {
    unsigned i = lex_hash(key) & (LEX_HASH_CAP - 1);
    while (lex_slots[i].key) {
        if (strcmp(lex_slots[i].key, key) == 0) return &lex_slots[i];
        i = (i + 1) & (LEX_HASH_CAP - 1);
    }
    if (!create) return NULL;
    lex_slots[i].key = key;
    lex_slots[i].first_kw = lex_slots[i].first_phrase = -1;
    return &lex_slots[i];
}

static void lex_build_index(void) {
    for (int i = 0; keyword_table[i].word; i++) {
        kw_next[i] = -1;
        LexSlot *sl = lex_slot(keyword_table[i].word, 1);
        if (sl->first_kw < 0) { sl->first_kw = i; continue; }
        int j = sl->first_kw;
        while (kw_next[j] >= 0) j = kw_next[j];
        kw_next[j] = i;
    }
    int np = 0;
    for (int i = 0; phrase_table[i].phrase; i++) {
        phrase_next[i] = -1;
        LexSlot *sl = lex_slot(phrase_table[i].phrase, 1);
        if (sl->first_phrase < 0) sl->first_phrase = i;
        else {
            int j = sl->first_phrase;
            while (phrase_next[j] >= 0) j = phrase_next[j];
            phrase_next[j] = i;
        }
        const char *ph = phrase_table[i].phrase;
        for (const char *sp = strchr(ph, ' '); sp; sp = strchr(sp + 1, ' ')) {
            size_t n = (size_t)(sp - ph);
            if (n >= sizeof prefix_store[0]) break;
            char tmp[sizeof prefix_store[0]];
            memcpy(tmp, ph, n);
            tmp[n] = '\0';
            LexSlot *ps = lex_slot(tmp, 0);
            if (!ps) {
                if (np == (int)(sizeof prefix_store / sizeof prefix_store[0])) break;
                memcpy(prefix_store[np], tmp, n + 1);
                ps = lex_slot(prefix_store[np++], 1);
            }
            ps->is_prefix = 1;
        }
    }
    lex_index_ready = 1;
}

TokenType lexer_match_keyword_mode(const char *word, LexMode mode) {
    if (!lex_index_ready) lex_build_index();
    LexSlot *sl = lex_slot(word, 0);
    if (!sl) return TOKEN_IDENTIFIER;
    for (int i = sl->first_kw; i >= 0; i = kw_next[i])
        if (keyword_table[i].modes & mode) return keyword_table[i].type;
    return TOKEN_IDENTIFIER;
}

TokenType lexer_match_keyword(const char *word) {
    return lexer_match_keyword_mode(word, LEX_ANY);
}

TokenType lexer_match_phrase(Lexer *lexer, const char *first_word, char *matched_phrase) {
    int saved_current = lexer->current;
    int saved_line    = lexer->line;
    int saved_col     = lexer->col;

    char buf[MAX_TOKEN_LEN];
    strncpy(buf, first_word, MAX_TOKEN_LEN - 1);
    buf[MAX_TOKEN_LEN - 1] = '\0';

    if (!lex_index_ready) lex_build_index();
    {
        LexSlot *sl = lex_slot(buf, 0);
        if (!sl || !sl->is_prefix) return TOKEN_UNKNOWN;
    }

    TokenType best_match     = TOKEN_UNKNOWN;
    int       best_current   = saved_current;
    int       best_line      = saved_line;
    int       best_col       = saved_col;
    char      best_phrase[MAX_TOKEN_LEN];
    best_phrase[0] = '\0';

    int attempts = 0;
    while (attempts < 4) {
        attempts++;

        lexer_skip_whitespace(lexer);
        if (lexer_at_end(lexer) || lexer_peek(lexer) == '\n') break;

        char next_word[MAX_TOKEN_LEN];
        if (!isalpha(lexer_peek(lexer))) break;
        lexer_read_word(lexer, next_word, MAX_TOKEN_LEN);

        strncat(buf, " ", MAX_TOKEN_LEN - strlen(buf) - 1);
        strncat(buf, next_word, MAX_TOKEN_LEN - strlen(buf) - 1);

        LexSlot *sl = lex_slot(buf, 0);
        if (!sl) break;
        for (int i = sl->first_phrase; i >= 0; i = phrase_next[i]) {
            if (!(phrase_table[i].modes & lexer->mode)) continue;
            best_match   = phrase_table[i].type;
            best_current = lexer->current;
            best_line    = lexer->line;
            best_col     = lexer->col;
            strncpy(best_phrase, buf, MAX_TOKEN_LEN - 1);
            break;
        }
        if (!sl->is_prefix) break;
    }

    if (best_match != TOKEN_UNKNOWN) {
        lexer->current = best_current;
        lexer->line    = best_line;
        lexer->col     = best_col;
        strncpy(matched_phrase, best_phrase, MAX_TOKEN_LEN - 1);
        return best_match;
    }

    lexer->current = saved_current;
    lexer->line    = saved_line;
    lexer->col     = saved_col;
    return TOKEN_UNKNOWN;
}

Token lexer_next_token(Lexer *lexer) {
    lexer_skip_whitespace(lexer);

    if (lexer_at_end(lexer))
        return make_token(TOKEN_EOF, "", lexer->line, lexer->col);

    lexer->start = lexer->current;
    int  line    = lexer->line;
    int  col     = lexer->col;
    char c       = lexer_peek(lexer);

    if (c == '\n') {
        lexer_advance(lexer);
        return make_token(TOKEN_NEWLINE, "\\n", line, col);
    }

    if (c == '#') {
        return lexer_read_comment(lexer);
    }
    if (c == '-' && lexer->current + 1 < lexer->length &&
        lexer->source[lexer->current + 1] == '-') {
        lexer_advance(lexer);
        return lexer_read_comment(lexer);
    }

    if (c == '"') {
        return lexer_read_string(lexer);
    }

    if (isdigit(c)) {
        return lexer_read_number(lexer);
    }

    if (c == '!') { lexer_advance(lexer); return make_token(TOKEN_TERMINATOR, "!", line, col); }
    if (c == ',') { lexer_advance(lexer); return make_token(TOKEN_COMMA,       ",", line, col); }
    if (c == '(') { lexer_advance(lexer); return make_token(TOKEN_LPAREN,      "(", line, col); }
    if (c == ')') { lexer_advance(lexer); return make_token(TOKEN_RPAREN,      ")", line, col); }
    if (c == '[') { lexer_advance(lexer); return make_token(TOKEN_LBRACKET,    "[", line, col); }
    if (c == ']') { lexer_advance(lexer); return make_token(TOKEN_RBRACKET,    "]", line, col); }
    if (c == '+') { lexer_advance(lexer); return make_token(TOKEN_PLUS,        "+", line, col); }
    if (c == '-') { lexer_advance(lexer); return make_token(TOKEN_MINUS,       "-", line, col); }

    if (isalpha(c) || c == '_') {
        char word[MAX_TOKEN_LEN];
        lexer_read_word(lexer, word, MAX_TOKEN_LEN);

        char matched_phrase[MAX_TOKEN_LEN];
        TokenType phrase_type = lexer_match_phrase(lexer, word, matched_phrase);

        if (phrase_type != TOKEN_UNKNOWN) {
            return make_token(phrase_type, matched_phrase, line, col);
        }

        TokenType kw_type = lexer_match_keyword_mode(word, lexer->mode);
        return make_token(kw_type, word, line, col);
    }

    char unknown[2] = { c, '\0' };
    lexer_advance(lexer);
    return make_token(TOKEN_UNKNOWN, unknown, line, col);
}

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

void lexer_free_tokens(Token *tokens) {
    free(tokens);
}
