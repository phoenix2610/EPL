#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "token.h"

#define POOL_CHUNK (64 * 1024)

static char  **intern_slots = NULL;
static size_t  intern_cap   = 0;
static size_t  intern_count = 0;
static char   *chunk        = NULL;
static size_t  chunk_left   = 0;

static unsigned long intern_hash(const char *s, size_t n) {
    unsigned long h = 1469598103934665603UL;
    for (size_t i = 0; i < n; i++) { h ^= (unsigned char)s[i]; h *= 1099511628211UL; }
    return h;
}

static char *pool_copy(const char *s, size_t n) {
    if (n + 1 > chunk_left) {
        size_t size = n + 1 > POOL_CHUNK ? n + 1 : POOL_CHUNK;
        chunk = malloc(size);
        if (!chunk) { fprintf(stderr, "EPL: out of memory\n"); exit(1); }
        chunk_left = size;
    }
    char *out = chunk;
    memcpy(out, s, n);
    out[n] = '\0';
    chunk += n + 1;
    chunk_left -= n + 1;
    return out;
}

static void intern_grow(void) {
    size_t cap = intern_cap ? intern_cap * 2 : 1024;
    char **slots = calloc(cap, sizeof(char *));
    if (!slots) { fprintf(stderr, "EPL: out of memory\n"); exit(1); }
    for (size_t i = 0; i < intern_cap; i++) {
        char *s = intern_slots[i];
        if (!s) continue;
        size_t j = intern_hash(s, strlen(s)) & (cap - 1);
        while (slots[j]) j = (j + 1) & (cap - 1);
        slots[j] = s;
    }
    free(intern_slots);
    intern_slots = slots;
    intern_cap = cap;
}

const char *token_intern(const char *s) {
    size_t n = strnlen(s, MAX_TOKEN_LEN - 1);
    if ((intern_count + 1) * 2 > intern_cap) intern_grow();
    size_t i = intern_hash(s, n) & (intern_cap - 1);
    while (intern_slots[i]) {
        if (strncmp(intern_slots[i], s, n) == 0 && intern_slots[i][n] == '\0')
            return intern_slots[i];
        i = (i + 1) & (intern_cap - 1);
    }
    intern_slots[i] = pool_copy(s, n);
    intern_count++;
    return intern_slots[i];
}

Token make_token(TokenType type, const char *value, int line, int col) {
    Token t;
    t.type = type;
    t.line = line;
    t.col  = col;

    t.value = token_intern(value ? value : "");
    return t;
}

const char *token_type_name(TokenType type) {
    switch (type) {

        case TOKEN_NUMBER:          return "NUMBER";
        case TOKEN_STRING:          return "STRING";
        case TOKEN_BOOLEAN:         return "BOOLEAN";
        case TOKEN_NULL:            return "NULL";

        case TOKEN_IDENTIFIER:      return "IDENTIFIER";

        case TOKEN_SET:             return "SET";
        case TOKEN_UPDATE:          return "UPDATE";
        case TOKEN_AS:              return "AS";
        case TOKEN_TO:              return "TO";
        case TOKEN_BY:              return "BY";
        case TOKEN_FROM:            return "FROM";
        case TOKEN_WITH:            return "WITH";
        case TOKEN_AND:             return "AND";
        case TOKEN_OR:              return "OR";
        case TOKEN_NOT:             return "NOT";
        case TOKEN_IN:              return "IN";
        case TOKEN_OF:              return "OF";

        case TOKEN_IF:              return "IF";
        case TOKEN_THEN:            return "THEN";
        case TOKEN_OTHERWISE:       return "OTHERWISE";
        case TOKEN_ELSE:            return "ELSE";
        case TOKEN_WHILE:           return "WHILE";
        case TOKEN_REPEAT:          return "REPEAT";
        case TOKEN_UNTIL:           return "UNTIL";
        case TOKEN_FOR:             return "FOR";
        case TOKEN_EACH:            return "EACH";
        case TOKEN_BREAK:           return "BREAK";
        case TOKEN_CONTINUE:        return "CONTINUE";
        case TOKEN_RETURN:          return "RETURN";

        case TOKEN_TIMES:           return "TIMES";
        case TOKEN_INDEFINITELY:    return "INDEFINITELY";
        case TOKEN_DOWN:            return "DOWN";

        case TOKEN_PLUS:            return "PLUS";
        case TOKEN_MINUS:           return "MINUS";
        case TOKEN_TIMES_OP:        return "TIMES(OP)";
        case TOKEN_DIVIDED_BY:      return "DIVIDED_BY";
        case TOKEN_MODULO:          return "MODULO";
        case TOKEN_POWER_OF:        return "POWER_OF";

        case TOKEN_EQUALS:          return "EQUALS";
        case TOKEN_IS:              return "IS";
        case TOKEN_NOT_EQUALS:      return "NOT_EQUALS";
        case TOKEN_IS_NOT:          return "IS_NOT";
        case TOKEN_GREATER_THAN:    return "GREATER_THAN";
        case TOKEN_LESS_THAN:       return "LESS_THAN";
        case TOKEN_AT_LEAST:        return "AT_LEAST";
        case TOKEN_AT_MOST:         return "AT_MOST";

        case TOKEN_IS_ANY_OF:       return "IS_ANY_OF";
        case TOKEN_IS_NONE_OF:      return "IS_NONE_OF";
        case TOKEN_FOR_ALL:         return "FOR_ALL_VALUES";
        case TOKEN_FOR_ANY:         return "FOR_ANY_VALUE";
        case TOKEN_FOR_EVERY:       return "FOR_EVERY_VALUE";
        case TOKEN_RESPECTIVELY:    return "RESPECTIVELY";
        case TOKEN_LENGTH_OF:       return "LENGTH_OF";
        case TOKEN_SIZE_OF:         return "SIZE_OF";
        case TOKEN_KEYS_OF:         return "KEYS_OF";
        case TOKEN_CODE_OF:         return "CODE_OF";
        case TOKEN_CHAR_CODE:       return "CHARACTER_WITH_CODE";
        case TOKEN_SHA256_OF:       return "SHA256_OF";
        case TOKEN_RANDOM_TOKEN:    return "RANDOM_TOKEN_OF";
        case TOKEN_RANDOM_NUMBER:   return "RANDOM_NUMBER_FROM";
        case TOKEN_ITEM:            return "ITEM";
        case TOKEN_ITEMS:           return "ITEMS";

        case TOKEN_SHOW:            return "SHOW";
        case TOKEN_DISPLAY:         return "DISPLAY";
        case TOKEN_PROVIDE:         return "PROVIDE";
        case TOKEN_WAIT:            return "WAIT";
        case TOKEN_USER:            return "USER";
        case TOKEN_INPUT:           return "INPUT";
        case TOKEN_ENTRY:           return "ENTRY";

        case TOKEN_DESCRIBE:        return "DESCRIBE";
        case TOKEN_CHECK:           return "CHECK";
        case TOKEN_THAT:            return "THAT";
        case TOKEN_ATTEMPT:         return "ATTEMPT";
        case TOKEN_IF_FAILS:        return "IF_FAILS";
        case TOKEN_GIVEN_THAT:      return "GIVEN_THAT";

        case TOKEN_ADD:             return "ADD";
        case TOKEN_EMPTY:           return "EMPTY";

        case TOKEN_EXECUTE:         return "EXECUTE";
        case TOKEN_SYSTEM:          return "SYSTEM";
        case TOKEN_COMMAND:         return "COMMAND";
        case TOKEN_WINDOWS:         return "WINDOWS";

        case TOKEN_COMMA:           return "COMMA";
        case TOKEN_LPAREN:          return "LPAREN";
        case TOKEN_RPAREN:          return "RPAREN";
        case TOKEN_LBRACKET:        return "LBRACKET";
        case TOKEN_RBRACKET:        return "RBRACKET";

        case TOKEN_TERMINATOR:      return "TERMINATOR(!)";
        case TOKEN_NEWLINE:         return "NEWLINE";
        case TOKEN_COMMENT:         return "COMMENT";
        case TOKEN_MULTICOMMENT:    return "MULTICOMMENT";
        case TOKEN_EOF:             return "EOF";

        case TOKEN_UNKNOWN:         return "UNKNOWN";

        case TOKEN_TAKING:                 return "TAKING";
        case TOKEN_FAIL_WITH_PH:           return "FAIL_WITH_PH";
        case TOKEN_ERROR_MSG_PH:           return "ERROR_MSG_PH";
        case TOKEN_PARSE_JSON_PH:          return "PARSE_JSON_PH";
        case TOKEN_TO_JSON_PH:             return "TO_JSON_PH";
        case TOKEN_AVERAGE:                return "AVERAGE";
        case TOKEN_TOTAL:                  return "TOTAL";
        case TOKEN_MAXIMUM:                return "MAXIMUM";
        case TOKEN_MINIMUM:                return "MINIMUM";
        case TOKEN_LENGTH:                 return "LENGTH";
        case TOKEN_SORTED:                 return "SORTED";
        case TOKEN_REVERSED:               return "REVERSED";
        case TOKEN_WHERE:                  return "WHERE";
        case TOKEN_UPPERCASE:              return "UPPERCASE";
        case TOKEN_LOWERCASE:              return "LOWERCASE";
        case TOKEN_TRIMMED:                return "TRIMMED";
        case TOKEN_SPLIT:                  return "SPLIT";
        case TOKEN_JOINED:                 return "JOINED";
        case TOKEN_REPLACED:               return "REPLACED";
        case TOKEN_STARTS:                 return "STARTS";
        case TOKEN_ENDS:                   return "ENDS";
        case TOKEN_CONTAINS:               return "CONTAINS";
        case TOKEN_CURRENCY:               return "CURRENCY";
        case TOKEN_PERCENTAGE:             return "PERCENTAGE";
        case TOKEN_ROUNDED:                return "ROUNDED";
        case TOKEN_FORMATTED:              return "FORMATTED";
        case TOKEN_A:                      return "A";
        case TOKEN_NUMBER_KW:              return "NUMBER_KW";
        case TOKEN_STRING_KW:              return "STRING_KW";
        case TOKEN_BOOLEAN_KW:             return "BOOLEAN_KW";
        case TOKEN_LIST_KW:                return "LIST_KW";
        case TOKEN_GRAPH:                  return "GRAPH";
        case TOKEN_LINK:                   return "LINK";
        case TOKEN_ROUTE:                  return "ROUTE";
        case TOKEN_CONNECTED:              return "CONNECTED";
        case TOKEN_NODE_KW:                return "NODE_KW";
        case TOKEN_EDGE_KW:                return "EDGE_KW";
        case TOKEN_MAKE_PAGE:              return "MAKE_PAGE";
        case TOKEN_SECTION_OUT:            return "SECTION_OUT";
        case TOKEN_ROW:                    return "ROW";
        case TOKEN_HEADING_OUT:            return "HEADING_OUT";
        case TOKEN_SUBHEADING:             return "SUBHEADING";
        case TOKEN_PARAGRAPH:              return "PARAGRAPH";
        case TOKEN_LABEL_OUT:              return "LABEL_OUT";
        case TOKEN_BOLD_OUT:               return "BOLD_OUT";
        case TOKEN_DIVIDER:                return "DIVIDER";
        case TOKEN_SPACER:                 return "SPACER";
        case TOKEN_STYLED:                 return "STYLED";
        case TOKEN_TITLED:                 return "TITLED";
        case TOKEN_SAVED:                  return "SAVED";
        case TOKEN_MAKE_CHART:             return "MAKE_CHART";
        case TOKEN_LINE_CHART:             return "LINE_CHART";
        case TOKEN_BAR_CHART:              return "BAR_CHART";
        case TOKEN_PIE_CHART:              return "PIE_CHART";
        case TOKEN_TABLE_OUT:              return "TABLE_OUT";
        case TOKEN_MAKE_REPORT:            return "MAKE_REPORT";
        case TOKEN_MAKE_STAT:              return "MAKE_STAT";
        case TOKEN_X_AXIS:                 return "X_AXIS";
        case TOKEN_Y_AXIS:                 return "Y_AXIS";
        case TOKEN_Z_AXIS:                 return "Z_AXIS";
        case TOKEN_AXIS:                   return "AXIS";
        case TOKEN_WITH_COLOR:             return "WITH_COLOR";
        case TOKEN_HEADERS:                return "HEADERS";
        case TOKEN_IMPORT:                 return "IMPORT";
        case TOKEN_DIV_KW:                 return "DIV_KW";
        case TOKEN_SPAN_KW:                return "SPAN_KW";
        case TOKEN_BUTTON_KW:              return "BUTTON_KW";
        case TOKEN_IMAGE_KW:               return "IMAGE_KW";
        case TOKEN_INPUT_KW:               return "INPUT_KW";
        case TOKEN_ID_KW:                  return "ID_KW";
        case TOKEN_CLASS_KW:               return "CLASS_KW";
        case TOKEN_SRC_KW:                 return "SRC_KW";
        case TOKEN_ONCLICK_KW:             return "ONCLICK_KW";
        case TOKEN_TYPE_KW:                return "TYPE_KW";
        case TOKEN_PLACEHOLDER_KW:         return "PLACEHOLDER_KW";
        case TOKEN_VALUE_KW:               return "VALUE_KW";
        case TOKEN_CANVAS3D:               return "CANVAS3D";
        case TOKEN_ORBITAL:                return "ORBITAL";
        case TOKEN_RING_KW:                return "RING_KW";
        case TOKEN_RADIUS_KW:              return "RADIUS_KW";
        case TOKEN_SPEED_KW:               return "SPEED_KW";
        case TOKEN_NODE_3D:                return "NODE_3D";
        case TOKEN_AT_KW:                  return "AT_KW";
        case TOKEN_LABELED:                return "LABELED";
        case TOKEN_STATUS_KW:              return "STATUS_KW";
        case TOKEN_CORE_KW:                return "CORE_KW";
        case TOKEN_STAR_KW:                return "STAR_KW";
        case TOKEN_BACKGROUND_KW:          return "BACKGROUND_KW";
        case TOKEN_PANEL_KW:               return "PANEL_KW";
        case TOKEN_DRAWER_KW:              return "DRAWER_KW";
        case TOKEN_OFFLINE:                return "OFFLINE";
        case TOKEN_FROM_GRAPH:             return "FROM_GRAPH";
        case TOKEN_VOID_BG:                return "VOID_BG";
        case TOKEN_SPACE_BG:               return "SPACE_BG";
        case TOKEN_MAKE_NAVBAR:            return "MAKE_NAVBAR";
        case TOKEN_MAKE_HERO:              return "MAKE_HERO";
        case TOKEN_MAKE_GALLERY:           return "MAKE_GALLERY";
        case TOKEN_MAKE_FORM:              return "MAKE_FORM";
        case TOKEN_MAKE_MODAL:             return "MAKE_MODAL";
        case TOKEN_MAKE_FOOTER:            return "MAKE_FOOTER";
        case TOKEN_USE_DESIGN:             return "USE_DESIGN";
        case TOKEN_CONTENT_SECTION:        return "CONTENT_SECTION";
        case TOKEN_OVERLAY_CARD:           return "OVERLAY_CARD";
        case TOKEN_NAV_LINK:               return "NAV_LINK";
        case TOKEN_ACTION_BTN:             return "ACTION_BTN";
        case TOKEN_LEFT_COL:               return "LEFT_COL";
        case TOKEN_RIGHT_COL:              return "RIGHT_COL";
        case TOKEN_ITEM_LIST_KW:           return "ITEM_LIST_KW";
        case TOKEN_FEATURE_IMAGE:          return "FEATURE_IMAGE";
        case TOKEN_PRICE_BADGE:            return "PRICE_BADGE";
        case TOKEN_SUBMIT_BTN:             return "SUBMIT_BTN";
        case TOKEN_DISMISS_BTN:            return "DISMISS_BTN";
        case TOKEN_GUEST_PICKER:           return "GUEST_PICKER";
        case TOKEN_DATE_PICKER:            return "DATE_PICKER";
        case TOKEN_OPEN_MODAL:             return "OPEN_MODAL";
        case TOKEN_ON_CLICK:               return "ON_CLICK";
        case TOKEN_SHOW_LOADING:           return "SHOW_LOADING";
        case TOKEN_BRANDED:                return "BRANDED";
        case TOKEN_EYEBROW:                return "EYEBROW";
        case TOKEN_ACCENTED:               return "ACCENTED";
        case TOKEN_DIMMED:                 return "DIMMED";
        case TOKEN_ASPECT:                 return "ASPECT";
        case TOKEN_COLUMNS:                return "COLUMNS";
        case TOKEN_OFFSET:                 return "OFFSET";
        case TOKEN_NUMBERED:               return "NUMBERED";
        case TOKEN_DETAIL_KW:              return "DETAIL_KW";
        case TOKEN_TAGLINE:                return "TAGLINE";
        case TOKEN_ADDRESS_KW:             return "ADDRESS_KW";
        case TOKEN_COPYRIGHT:              return "COPYRIGHT";
        case TOKEN_FIELD:                  return "FIELD";
        case TOKEN_PLACEHOLDER_EPF:        return "PLACEHOLDER_EPF";
        case TOKEN_ENTRY_KW:               return "ENTRY_KW";
        case TOKEN_ICONS_KW:               return "ICONS_KW";
        case TOKEN_FROSTED:                return "FROSTED";
        case TOKEN_READ_KW:                return "READ_KW";
        case TOKEN_WRITE_KW:               return "WRITE_KW";
        case TOKEN_APPEND_KW:              return "APPEND_KW";
        case TOKEN_FILE_KW:                return "FILE_KW";
        case TOKEN_DELETE_KW:              return "DELETE_KW";
        case TOKEN_RUN_KW:                 return "RUN_KW";
        case TOKEN_ENV_KW:                 return "ENV_KW";
        case TOKEN_NOW_KW:                 return "NOW_KW";
        case TOKEN_SLEEP_KW:               return "SLEEP_KW";
        case TOKEN_SOCKET_KW:              return "SOCKET_KW";
        case TOKEN_SERVE_KW:               return "SERVE_KW";
        case TOKEN_POST_KW:                return "POST_KW";
        case TOKEN_FETCH_KW:               return "FETCH_KW";
        case TOKEN_JSON_KW:                return "JSON_KW";
        case TOKEN_REPLY_KW:               return "REPLY_KW";
        case TOKEN_USING_KW:               return "USING_KW";
        case TOKEN_EXISTS_KW:              return "EXISTS_KW";
        case TOKEN_GET_KW:                 return "GET_KW";
        case TOKEN_READ_FILE_PH:           return "READ_FILE_PH";
        case TOKEN_WRITE_FILE_PH:          return "WRITE_FILE_PH";
        case TOKEN_APPEND_FILE_PH:         return "APPEND_FILE_PH";
        case TOKEN_FILE_EXISTS_PH:         return "FILE_EXISTS_PH";
        case TOKEN_DELETE_FILE_PH:         return "DELETE_FILE_PH";
        case TOKEN_GET_ENV_PH:             return "GET_ENV_PH";
        case TOKEN_SEND_JSON_PH:           return "SEND_JSON_PH";
        case TOKEN_POST_JSON_PH:           return "POST_JSON_PH";
        case TOKEN_FIELD_JSON_PH:          return "FIELD_JSON_PH";
        case TOKEN_SERVE_SOCK_PH:          return "SERVE_SOCK_PH";
        case TOKEN_REPLY_WITH_PH:          return "REPLY_WITH_PH";
        case TOKEN_SERVE_PORT_PH:          return "SERVE_PORT_PH";
        case TOKEN_MAKE_BUTTON:            return "MAKE_BUTTON";

        default:                    return "UNRECOGNIZED";
    }
}

void print_token(Token token) {

    if (token.type == TOKEN_NEWLINE) {
        printf("[NEWLINE] (line %d)\n", token.line);
        return;
    }

    if (token.type == TOKEN_COMMENT || token.type == TOKEN_MULTICOMMENT) {
        printf("[%s] \"%s\" (line %d)\n",
            token_type_name(token.type),
            token.value,
            token.line);
        return;
    }

    if (token.type == TOKEN_EOF) {
        printf("[EOF] (line %d)\n", token.line);
        return;
    }

    printf("[%-20s] \"%s\"  (line %d, col %d)\n",
        token_type_name(token.type),
        token.value,
        token.line,
        token.col);
}
