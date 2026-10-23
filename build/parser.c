#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include "parser.h"

static Token current(Parser *p) {
    return p->tokens[p->current];
}

static Token peek_at(Parser *p, int offset) {
    int idx = p->current + offset;
    if (idx >= p->count) return p->tokens[p->count - 1];
    return p->tokens[idx];
}

static Token advance(Parser *p) {
    Token t = p->tokens[p->current];
    if (p->current < p->count - 1) p->current++;
    return t;
}

static int check(Parser *p, TokenType type) {
    return current(p).type == type;
}

static int match(Parser *p, TokenType type) {
    if (check(p, type)) { advance(p); return 1; }
    return 0;
}

static void skip_newlines(Parser *p) {
    while (check(p, TOKEN_NEWLINE)) advance(p);
}

static void parser_error(Parser *p, const char *msg) {
    if (p->panic_mode) return;
    Token t = current(p);
    fprintf(stderr, "[EPL Error] Line %d, Col %d: %s (got \"%s\")\n",
        t.line, t.col, msg, t.value);
    p->had_error  = 1;
    p->panic_mode = 1;
}

static Token expect(Parser *p, TokenType type, const char *msg) {
    if (check(p, type)) return advance(p);
    parser_error(p, msg);
    return current(p);
}

static void synchronize(Parser *p) {
    p->panic_mode = 0;
    while (!check(p, TOKEN_EOF)) {
        if (check(p, TOKEN_NEWLINE)) { advance(p); return; }
        if (check(p, TOKEN_TERMINATOR)) { advance(p); return; }
        advance(p);
    }
}

static int is_postfix_op(TokenType type) {
    return (type == TOKEN_IN ||
            type == TOKEN_TRIMMED ||
            type == TOKEN_SPLIT ||
            type == TOKEN_JOINED ||
            type == TOKEN_WITH ||
            type == TOKEN_STARTS ||
            type == TOKEN_ENDS ||
            type == TOKEN_CONTAINS ||
            type == TOKEN_ROUNDED ||
            type == TOKEN_AS ||
            type == TOKEN_IS);
}

static int is_identifier_word(TokenType type) {
    return (type == TOKEN_IDENTIFIER ||
            type == TOKEN_TOTAL ||
            type == TOKEN_GRAPH ||
            type == TOKEN_ROUTE ||
            type == TOKEN_CONNECTED ||
            type == TOKEN_AVERAGE ||
            type == TOKEN_MAXIMUM ||
            type == TOKEN_MINIMUM ||
            type == TOKEN_LENGTH ||
            type == TOKEN_SORTED ||
            type == TOKEN_REVERSED ||
            type == TOKEN_NUMBER_KW ||
            type == TOKEN_PERCENTAGE ||
            type == TOKEN_CURRENCY ||
            type == TOKEN_FORMATTED ||
            type == TOKEN_UPPERCASE ||
            type == TOKEN_LOWERCASE ||
            type == TOKEN_STRING_KW ||
            type == TOKEN_LIST_KW ||
            type == TOKEN_BOOLEAN_KW ||
            type == TOKEN_A ||
            type == TOKEN_REPLACED ||
            type == TOKEN_X_AXIS ||
            type == TOKEN_Y_AXIS ||
            type == TOKEN_Z_AXIS);
}

static void collect_name(Parser *p, char *buf, int maxlen) {
    buf[0] = '\0';
    int first = 1;
    while (1) {
        TokenType type = current(p).type;
        int is_valid = is_identifier_word(type) || (first && is_postfix_op(type));

        if (type == TOKEN_ROUNDED && peek_at(p, 1).type != TOKEN_TO) {
            is_valid = 1;
        }

        if (!is_valid) break;

        if (!first) strncat(buf, " ", maxlen - strlen(buf) - 1);
        strncat(buf, current(p).value, maxlen - strlen(buf) - 1);
        advance(p);
        first = 0;
    }
}

void parser_init(Parser *p, Token *tokens, int count) {
    p->tokens    = tokens;
    p->count     = count;
    p->current   = 0;
    p->had_error = 0;
    p->panic_mode = 0;
    p->no_times_op = 0;
}

static void close_block(Parser *p, int open_line) {
    if (match(p, TOKEN_TERMINATOR)) return;
    if (check(p, TOKEN_EOF) && !p->panic_mode) {
        fprintf(stderr, "[EPL Error] Line %d: block opened here is never closed "
                        "with '!' (reached end of file)\n", open_line);
        p->had_error = 1;
    }
}

void parse_block(Parser *p, NodeList *list) {
    int open_line = p->current > 0 ? p->tokens[p->current - 1].line : 1;
    nodelist_init(list);
    skip_newlines(p);
    while (!check(p, TOKEN_TERMINATOR) &&
           !check(p, TOKEN_EOF)) {
        skip_newlines(p);
        if (check(p, TOKEN_TERMINATOR) || check(p, TOKEN_EOF)) break;
        ASTNode *stmt = parse_statement(p);
        if (stmt) nodelist_push(list, stmt);
        if (p->panic_mode) synchronize(p);
        skip_newlines(p);
    }
    close_block(p, open_line);
}

ASTNode *parser_parse(Parser *p) {
    ASTNode *program = ast_make_node(NODE_PROGRAM, 1, 1);
    nodelist_init(&program->as.program.body);

    skip_newlines(p);
    while (!check(p, TOKEN_EOF)) {
        skip_newlines(p);
        if (check(p, TOKEN_EOF)) break;
        ASTNode *stmt = parse_statement(p);
        if (stmt) nodelist_push(&program->as.program.body, stmt);
        if (p->panic_mode) synchronize(p);
        skip_newlines(p);
    }
    return program;
}

static const struct { const char *word; const char *use; } STATEMENT_HINTS[] = {
    { "print",    "show VALUE" },        { "echo",     "show VALUE" },
    { "puts",     "show VALUE" },        { "console",  "show VALUE" },
    { "printf",   "show VALUE" },        { "write",    "show VALUE, or: write TEXT to file \"PATH\"" },
    { "let",      "set NAME as VALUE" }, { "var",      "set NAME as VALUE" },
    { "const",    "set NAME as VALUE" }, { "val",      "set NAME as VALUE" },
    { "def",      "to NAME taking A, B   (then the body, then a line with just !)" },
    { "function", "to NAME taking A, B   (then the body, then a line with just !)" },
    { "func",     "to NAME taking A, B   (then the body, then a line with just !)" },
    { "fn",       "to NAME taking A, B   (then the body, then a line with just !)" },
    { "elif",     "otherwise if CONDITION then" },
    { "elseif",   "otherwise if CONDITION then" },
    { "end",      "! on its own line to close a block" },
    { "endif",    "! on its own line to close a block" },
    { "fi",       "! on its own line to close a block" },
    { "done",     "! on its own line to close a block" },
    { "call",     "set RESULT by NAME with ARG and ARG" },
    { "loop",     "repeat N times   or   for each ITEM in LIST" },
    { "try",      "attempt   (then the code, then: if fails then ... !)" },
    { "catch",    "if fails then" },
    { "except",   "if fails then" },
    { "raise",    "fail with \"MESSAGE\"" },
    { "throw",    "fail with \"MESSAGE\"" },
    { "append",   "add VALUE to LIST" },
    { "push",     "add VALUE to LIST" },
    { NULL, NULL }
};

static void statement_start_error(Parser *p, Token t) {
    char msg[600];
    if (p->current > 0) {
        Token prev = p->tokens[p->current - 1];
        if (prev.type != TOKEN_NEWLINE && prev.type != TOKEN_TERMINATOR &&
            prev.line == t.line) {
            snprintf(msg, sizeof msg,
                     "I can't read \"%s\" here: the statement before it on this line "
                     "was already complete. Write one statement per line", t.value);
            parser_error(p, msg);
            return;
        }
    }
    for (int i = 0; STATEMENT_HINTS[i].word; i++) {
        if (strcmp(t.value, STATEMENT_HINTS[i].word) == 0) {
            snprintf(msg, sizeof msg, "EPL has no \"%s\" statement; write: %s",
                     t.value, STATEMENT_HINTS[i].use);
            parser_error(p, msg);
            return;
        }
    }
    if (strcmp(t.value, "}") == 0 || strcmp(t.value, "{") == 0) {
        parser_error(p, "EPL has no braces; blocks end with ! on its own line");
        return;
    }
    Token next = p->current + 1 < p->count ? p->tokens[p->current + 1] : t;
    if (t.type == TOKEN_IDENTIFIER && next.line == t.line &&
        (strcmp(next.value, "=") == 0 || strcmp(next.value, "+=") == 0)) {
        snprintf(msg, sizeof msg, "EPL has no \"=\"; write: set %s as VALUE "
                 "(or update %s as VALUE to change it)", t.value, t.value);
        parser_error(p, msg);
        return;
    }
    if (t.type == TOKEN_IDENTIFIER) {
        snprintf(msg, sizeof msg,
                 "a statement can't start with the name \"%s\". To call an activity write: "
                 "set RESULT by %s with ARG; to change a variable: update %s as VALUE",
                 t.value, t.value, t.value);
        parser_error(p, msg);
        return;
    }
    snprintf(msg, sizeof msg,
             "I can't start a statement with \"%s\". Statements start with a word "
             "such as set, show, if, for, repeat, while, to, update, add or attempt", t.value);
    parser_error(p, msg);
}

ASTNode *parse_statement(Parser *p) {
    skip_newlines(p);
    Token t = current(p);

    switch (t.type) {
        case TOKEN_SET:         return parse_set(p);
        case TOKEN_UPDATE:      return parse_update(p);
        case TOKEN_SHOW:
        case TOKEN_DISPLAY:     return parse_show(p);
        case TOKEN_IF:          return parse_if(p);
        case TOKEN_WHILE:       return parse_while(p);
        case TOKEN_REPEAT:      return parse_repeat(p);
        case TOKEN_FOR:         return parse_for(p);
        case TOKEN_TO:          return parse_activity_def(p);
        case TOKEN_IMPORT:      return parse_import(p);
        case TOKEN_DB_MAKE_TABLE: return parse_db_make_table(p);
        case TOKEN_DB_SAVE:     return parse_db_save(p);
        case TOKEN_DB_DELETE:   return parse_db_delete(p);
        case TOKEN_RETURN:      return parse_return(p);
        case TOKEN_CHECK:       return parse_check(p);
        case TOKEN_DESCRIBE:    return parse_describe(p);
        case TOKEN_PROVIDE:     return parse_provide(p);
        case TOKEN_ATTEMPT:     return parse_attempt(p);
        case TOKEN_FAIL_WITH_PH: {
            Token ft = advance(p);
            ASTNode *fn = ast_make_node(NODE_FAIL_WITH, ft.line, ft.col);
            fn->as.fail_with.message_expr = parse_expression(p);
            return fn;
        }
        case TOKEN_ADD:         return parse_add_to(p);
        case TOKEN_WAIT:        return parse_wait(p);
        case TOKEN_EXECUTE:     return parse_execute(p);
        case TOKEN_LINK:        return parse_link(p);
        case TOKEN_MAKE_PAGE:   return parse_make_page(p);
        case TOKEN_CANVAS3D:    return parse_canvas3d(p);
        case TOKEN_ORBITAL:     return parse_orbital_ring(p);
        case TOKEN_NODE_3D:     return parse_node3d(p);
        case TOKEN_CORE_KW:     return parse_core_star(p);
        case TOKEN_PANEL_KW:    return parse_panel3d(p);
        case TOKEN_DRAWER_KW:   return parse_drawer3d(p);
        case TOKEN_HEADING_OUT: return parse_text_node(p, NODE_HEADING_OUT);
        case TOKEN_SUBHEADING:  return parse_text_node(p, NODE_SUBHEADING);
        case TOKEN_PARAGRAPH:   return parse_text_node(p, NODE_PARAGRAPH);
        case TOKEN_LABEL_OUT:   return parse_text_node(p, NODE_LABEL_OUT);
        case TOKEN_BOLD_OUT:    return parse_text_node(p, NODE_BOLD_OUT);
        case TOKEN_DIVIDER:     { advance(p); return ast_make_node(NODE_DIVIDER, current(p).line, current(p).col); }
        case TOKEN_SPACER:      { advance(p); return ast_make_node(NODE_SPACER,  current(p).line, current(p).col); }
        case TOKEN_MAKE_CHART:
        case TOKEN_LINE_CHART:
        case TOKEN_BAR_CHART:
        case TOKEN_PIE_CHART:   return parse_make_chart(p);
        case TOKEN_TABLE_OUT:   return parse_make_table_out(p);
        case TOKEN_MAKE_REPORT: return parse_make_report(p);
        case TOKEN_MAKE_STAT:   return parse_make_stat(p);
        case TOKEN_SECTION_OUT: return parse_section(p);
        case TOKEN_ROW:         return parse_row(p);
        case TOKEN_DIV_KW:
        case TOKEN_SPAN_KW:
        case TOKEN_BUTTON_KW:
        case TOKEN_INPUT_KW:    return parse_html_element(p);
        case TOKEN_IMAGE_KW:
            switch (peek_at(p, 1).type) {
                case TOKEN_SRC_KW:
                case TOKEN_ID_KW:
                case TOKEN_CLASS_KW:
                case TOKEN_TYPE_KW:
                case TOKEN_ONCLICK_KW:
                case TOKEN_PLACEHOLDER_KW:
                case TOKEN_VALUE_KW:
                    return parse_html_element(p);
                default:
                    return parse_gallery_image(p);
            }
        case TOKEN_USE_DESIGN:      return parse_use_design(p);
        case TOKEN_MAKE_NAVBAR:     return parse_make_navbar(p);
        case TOKEN_MAKE_HERO:       return parse_make_hero(p);
        case TOKEN_CONTENT_SECTION: return parse_content_section(p);
        case TOKEN_MAKE_GALLERY:    return parse_make_gallery(p);
        case TOKEN_MAKE_FORM:       return parse_make_form(p);
        case TOKEN_MAKE_BUTTON:     return parse_make_button(p);
        case TOKEN_MAKE_MODAL:      return parse_make_modal(p);
        case TOKEN_MAKE_FOOTER:     return parse_make_footer(p);
        case TOKEN_OVERLAY_CARD:    return parse_overlay_card(p);
        case TOKEN_LEFT_COL:        return parse_column(p, 0);
        case TOKEN_RIGHT_COL:       return parse_column(p, 1);
        case TOKEN_ITEM_LIST_KW:    return parse_item_list(p);
        case TOKEN_FEATURE_IMAGE:   return parse_feature_image(p);
        case TOKEN_PRICE_BADGE:     return parse_price_badge(p);
        case TOKEN_EYEBROW:         return parse_eyebrow(p);
        case TOKEN_NAV_LINK:        return parse_nav_link(p);
        case TOKEN_ACTION_BTN:      return parse_action_btn(p);
        case TOKEN_GUEST_PICKER:    return parse_guest_picker(p);
        case TOKEN_DATE_PICKER:     return parse_date_picker(p);
        case TOKEN_ENTRY:
        case TOKEN_ENTRY_KW:        return parse_list_entry(p);
        case TOKEN_SUBMIT_BTN:      return parse_submit_btn(p);
        case TOKEN_DISMISS_BTN:     return parse_dismiss_btn(p);
        case TOKEN_FIELD:           return parse_form_field(p);
        case TOKEN_WRITE_KW:        return parse_write_file(p, NODE_WRITE_FILE);
        case TOKEN_APPEND_KW:       return parse_write_file(p, NODE_APPEND_FILE);
        case TOKEN_DELETE_FILE_PH:
        case TOKEN_DELETE_KW:       return parse_delete_file(p);
        case TOKEN_SLEEP_KW:        return parse_sleep(p);
        case TOKEN_SERVE_SOCK_PH:   return parse_serve_socket(p);
        case TOKEN_SERVE_PORT_PH:   return parse_serve_port(p);
        case TOKEN_REPLY_WITH_PH:   return parse_reply_with(p);
        case TOKEN_BREAK: {
            ASTNode *n = ast_make_node(NODE_BREAK, t.line, t.col);
            advance(p);
            return n;
        }
        case TOKEN_CONTINUE: {
            ASTNode *n = ast_make_node(NODE_CONTINUE, t.line, t.col);
            advance(p);
            return n;
        }
        case TOKEN_COMMENT:
        case TOKEN_MULTICOMMENT:
        case TOKEN_NEWLINE:
            advance(p);
            return NULL;
        default:
            statement_start_error(p, t);
            advance(p);
            return NULL;
    }
}

ASTNode *parse_collection_op(Parser *p) {
    Token t = current(p);
    CollectionOp op;
    int has_of = 1;

    switch (t.type) {
        case TOKEN_AVERAGE:  op = COLLOP_AVERAGE;  break;
        case TOKEN_TOTAL:    op = COLLOP_TOTAL;    break;
        case TOKEN_MAXIMUM:  op = COLLOP_MAXIMUM;  break;
        case TOKEN_MINIMUM:  op = COLLOP_MINIMUM;  break;
        case TOKEN_LENGTH:   op = COLLOP_LENGTH;   break;
        case TOKEN_SIZE_OF:  op = COLLOP_LENGTH;   has_of = 0; break;
        case TOKEN_SORTED:   op = COLLOP_SORTED;   has_of = 0; break;
        case TOKEN_REVERSED: op = COLLOP_REVERSED; has_of = 0; break;
        case TOKEN_KEYS_OF:  op = COLLOP_KEYS;     has_of = 0; break;
        case TOKEN_CODE_OF:  op = COLLOP_CODE;     has_of = 0; break;
        case TOKEN_CHAR_CODE: op = COLLOP_CHARACTER; has_of = 0; break;
        case TOKEN_SHA256_OF: op = COLLOP_SHA256;  has_of = 0; break;
        case TOKEN_RANDOM_TOKEN: op = COLLOP_RANDOM_TOKEN; has_of = 0; break;
        default:
            parser_error(p, "Unknown collection operation");
            return ast_make_node(NODE_NULL, t.line, t.col);
    }

    advance(p);

    if (has_of) match(p, TOKEN_OF);

    ASTNode *node = ast_make_node(NODE_COLLECTION_OP, t.line, t.col);
    node->as.collection_op.op         = op;
    node->as.collection_op.collection = parse_primary(p);
    return node;
}

ASTNode *parse_string_op(Parser *p, ASTNode *target) {
    Token t = current(p);
    ASTNode *node = ast_make_node(NODE_STRING_OP, t.line, t.col);
    node->as.string_op.target = target;
    node->as.string_op.arg1   = NULL;
    node->as.string_op.arg2   = NULL;

    if (t.type == TOKEN_IN) {
        advance(p);
        Token kw = current(p);
        if (kw.type == TOKEN_UPPERCASE) {
            advance(p);
            node->as.string_op.op = STROP_UPPERCASE;
        } else if (kw.type == TOKEN_LOWERCASE) {
            advance(p);
            node->as.string_op.op = STROP_LOWERCASE;
        } else {
            ast_free(node);
            return target;
        }
        return node;
    }

    if (t.type == TOKEN_TRIMMED) {
        advance(p);
        node->as.string_op.op = STROP_TRIMMED;
        return node;
    }

    if (t.type == TOKEN_SPLIT) {
        advance(p);
        match(p, TOKEN_BY);
        node->as.string_op.op   = STROP_SPLIT;
        node->as.string_op.arg1 = parse_primary(p);
        return node;
    }

    if (t.type == TOKEN_JOINED) {
        advance(p);
        match(p, TOKEN_BY);
        node->as.string_op.op   = STROP_JOINED;
        node->as.string_op.arg1 = parse_primary(p);
        return node;
    }

    if (t.type == TOKEN_WITH) {
        advance(p);
        node->as.string_op.op   = STROP_REPLACED;
        node->as.string_op.arg1 = parse_primary(p);
        match(p, TOKEN_REPLACED);
        match(p, TOKEN_BY);
        node->as.string_op.arg2 = parse_primary(p);
        return node;
    }

    if (t.type == TOKEN_STARTS) {
        advance(p);
        match(p, TOKEN_WITH);
        node->as.string_op.op   = STROP_STARTS_WITH;
        node->as.string_op.arg1 = parse_primary(p);
        return node;
    }

    if (t.type == TOKEN_ENDS) {
        advance(p);
        match(p, TOKEN_WITH);
        node->as.string_op.op   = STROP_ENDS_WITH;
        node->as.string_op.arg1 = parse_primary(p);
        return node;
    }

    if (t.type == TOKEN_CONTAINS) {
        advance(p);
        node->as.string_op.op   = STROP_CONTAINS;
        node->as.string_op.arg1 = parse_primary(p);
        return node;
    }

    ast_free(node);
    return target;
}

ASTNode *parse_format_op(Parser *p, ASTNode *target) {
    Token t = current(p);
    ASTNode *node = ast_make_node(NODE_FORMAT_OP, t.line, t.col);
    node->as.format_op.target    = target;
    node->as.format_op.precision = NULL;

    if (t.type == TOKEN_ROUNDED) {
        advance(p);
        match(p, TOKEN_TO);
        node->as.format_op.op        = FMTOP_ROUNDED;
        node->as.format_op.precision = parse_primary(p);
        return node;
    }

    if (t.type == TOKEN_AS) {
        advance(p);
        Token kw = current(p);
        if (kw.type == TOKEN_CURRENCY) {
            advance(p);
            node->as.format_op.op = FMTOP_CURRENCY;
            return node;
        }
        if (kw.type == TOKEN_PERCENTAGE) {
            advance(p);
            node->as.format_op.op = FMTOP_PERCENTAGE;
            return node;
        }
        if (kw.type == TOKEN_FORMATTED) {
            advance(p);
            node->as.format_op.op = FMTOP_FORMATTED;
            return node;
        }
        p->current--;
    }

    ast_free(node);
    return target;
}

ASTNode *parse_set(Parser *p) {
    Token t = advance(p);
    ASTNode *node = ast_make_node(NODE_SET, t.line, t.col);

    char name[MAX_TOKEN_LEN] = {0};
    int first = 1;
    while (!check(p, TOKEN_AS) && !check(p, TOKEN_BY) && !check(p, TOKEN_EOF) && !check(p, TOKEN_NEWLINE)) {
        if (!first) strncat(name, " ", MAX_TOKEN_LEN - strlen(name) - 1);
        strncat(name, current(p).value, MAX_TOKEN_LEN - strlen(name) - 1);
        advance(p);
        first = 0;
    }
    AST_SET_STR(node->as.assignment.name, name);

    if (match(p, TOKEN_AS)) {
        node->as.assignment.value = parse_expression(p);
    } else if (match(p, TOKEN_BY)) {
        ASTNode *call = ast_make_node(NODE_ACTIVITY_CALL, t.line, t.col);
        nodelist_init(&call->as.activity_call.args);

        char act_name[MAX_TOKEN_LEN] = {0};
        int first = 1;

        while (!check(p, TOKEN_NEWLINE)    &&
               !check(p, TOKEN_TERMINATOR) &&
               !check(p, TOKEN_EOF)        &&
               !check(p, TOKEN_AND)        &&
               !check(p, TOKEN_FROM)       &&
               !check(p, TOKEN_WITH)       &&
               !check(p, TOKEN_STRING)     &&
               !check(p, TOKEN_NUMBER)     &&
               !check(p, TOKEN_BOOLEAN)) {

            if (!first) strncat(act_name, " ",
                MAX_TOKEN_LEN - strlen(act_name) - 1);
            strncat(act_name, current(p).value,
                MAX_TOKEN_LEN - strlen(act_name) - 1);
            advance(p);
            first = 0;
        }
        AST_SET_STR(call->as.activity_call.name, act_name);

        while (!check(p, TOKEN_NEWLINE)    &&
               !check(p, TOKEN_TERMINATOR) &&
               !check(p, TOKEN_EOF)) {
            match(p, TOKEN_AND);
            match(p, TOKEN_FROM);
            match(p, TOKEN_WITH);
            if (check(p, TOKEN_NEWLINE) ||
                check(p, TOKEN_TERMINATOR) ||
                check(p, TOKEN_EOF)) break;
            int before = p->current;
            ASTNode *arg = parse_additive(p);
            if (p->current == before) {
                parser_error(p, "Unexpected word in activity arguments");
                break;
            }
            if (arg) nodelist_push(&call->as.activity_call.args, arg);
        }

        node->as.assignment.value = call;
    } else {
        parser_error(p, "Expected 'as' or 'by' after variable name in 'set'");
    }

    return node;
}

ASTNode *parse_update(Parser *p) {
    Token t = advance(p);

    if (check(p, TOKEN_ITEM) || check(p, TOKEN_FIELD)) {
        int is_field = check(p, TOKEN_FIELD);
        advance(p);
        ASTNode *node = ast_make_node(NODE_UPDATE_PART, t.line, t.col);
        node->as.update_part.is_field = is_field;
        node->as.update_part.selector = is_field ? parse_primary(p) : parse_additive(p);
        if (!match(p, TOKEN_OF) && !match(p, TOKEN_FROM))
            parser_error(p, is_field ? "Expected 'of' in 'update field \"name\" of RECORD as VALUE'"
                                     : "Expected 'of' in 'update item N of LIST as VALUE'");
        char name[MAX_TOKEN_LEN] = {0};
        while (!check(p, TOKEN_AS) && !check(p, TOKEN_EOF) && !check(p, TOKEN_NEWLINE)) {
            if (name[0]) strncat(name, " ", MAX_TOKEN_LEN - strlen(name) - 1);
            strncat(name, current(p).value, MAX_TOKEN_LEN - strlen(name) - 1);
            advance(p);
        }
        AST_SET_STR(node->as.update_part.target, name);
        expect(p, TOKEN_AS, "Expected 'as' before the new value");
        node->as.update_part.value = parse_expression(p);
        return node;
    }

    ASTNode *node = ast_make_node(NODE_UPDATE, t.line, t.col);

    char name[MAX_TOKEN_LEN] = {0};
    int first = 1;
    while (!check(p, TOKEN_AS) && !check(p, TOKEN_BY) && !check(p, TOKEN_EOF) && !check(p, TOKEN_NEWLINE)) {
        if (!first) strncat(name, " ", MAX_TOKEN_LEN - strlen(name) - 1);
        strncat(name, current(p).value, MAX_TOKEN_LEN - strlen(name) - 1);
        advance(p);
        first = 0;
    }
    AST_SET_STR(node->as.assignment.name, name);

    if (match(p, TOKEN_AS)) {
        node->as.assignment.value = parse_expression(p);
    } else if (match(p, TOKEN_BY)) {
        node->as.assignment.value = parse_expression(p);
    } else {
        parser_error(p, "Expected 'as' or 'by' after variable name in 'update'");
    }

    return node;
}

ASTNode *parse_show(Parser *p) {
    Token t = advance(p);
    ASTNode *node = ast_make_node(NODE_SHOW, t.line, t.col);
    node->as.show.value = parse_expression(p);
    return node;
}

ASTNode *parse_if(Parser *p) {
    Token t = advance(p);
    ASTNode *node = ast_make_node(NODE_IF, t.line, t.col);

    node->as.if_stmt.condition = parse_expression(p);
    expect(p, TOKEN_THEN, "Expected 'then' after if condition");

    nodelist_init(&node->as.if_stmt.then_body);
    nodelist_init(&node->as.if_stmt.else_body);

    skip_newlines(p);

    while (!check(p, TOKEN_TERMINATOR) &&
           !check(p, TOKEN_OTHERWISE)  &&
           !check(p, TOKEN_EOF)) {
        skip_newlines(p);
        if (check(p, TOKEN_TERMINATOR) ||
            check(p, TOKEN_OTHERWISE)  ||
            check(p, TOKEN_EOF)) break;
        ASTNode *stmt = parse_statement(p);
        if (stmt) nodelist_push(&node->as.if_stmt.then_body, stmt);
        if (p->panic_mode) synchronize(p);
        skip_newlines(p);
    }

    if (match(p, TOKEN_OTHERWISE)) {
        if (check(p, TOKEN_IF)) {
            ASTNode *chained = parse_if(p);
            nodelist_push(&node->as.if_stmt.else_body, chained);
            node->end_line = chained->end_line;
            return node;
        }
        skip_newlines(p);
        while (!check(p, TOKEN_TERMINATOR) && !check(p, TOKEN_EOF)) {
            skip_newlines(p);
            if (check(p, TOKEN_TERMINATOR) || check(p, TOKEN_EOF)) break;
            ASTNode *stmt = parse_statement(p);
            if (stmt) nodelist_push(&node->as.if_stmt.else_body, stmt);
            if (p->panic_mode) synchronize(p);
            skip_newlines(p);
        }
    }

    close_block(p, t.line);
    node->end_line = current(p).line;
    return node;
}

ASTNode *parse_while(Parser *p) {
    Token t = advance(p);
    ASTNode *node = ast_make_node(NODE_WHILE, t.line, t.col);
    node->as.while_stmt.condition = parse_expression(p);
    nodelist_init(&node->as.while_stmt.body);
    parse_block(p, &node->as.while_stmt.body);
    node->end_line = current(p).line;
    return node;
}

ASTNode *parse_repeat(Parser *p) {
    Token t = advance(p);

    if (match(p, TOKEN_INDEFINITELY)) {
        ASTNode *node = ast_make_node(NODE_REPEAT_INDEFINITELY, t.line, t.col);
        parse_block(p, &node->as.repeat_indefinitely.body);
        node->end_line = current(p).line;
        return node;
    }

    if (match(p, TOKEN_UNTIL)) {
        ASTNode *node = ast_make_node(NODE_REPEAT_UNTIL, t.line, t.col);
        node->as.repeat_until.condition = parse_expression(p);
        parse_block(p, &node->as.repeat_until.body);
        node->end_line = current(p).line;
        return node;
    }

    ASTNode *node = ast_make_node(NODE_REPEAT_TIMES, t.line, t.col);
    p->no_times_op = 1;
    node->as.repeat_times.count = parse_expression(p);
    p->no_times_op = 0;
    node->as.repeat_times.counter[0] = '\0';

    expect(p, TOKEN_TIMES, "Expected 'times' after repeat count");

    if (match(p, TOKEN_WITH)) {
        if (check(p, TOKEN_IDENTIFIER)) {
            AST_SET_STR(node->as.repeat_times.counter,
                current(p).value);
            advance(p);
        }
    }

    parse_block(p, &node->as.repeat_times.body);
    node->end_line = current(p).line;
    return node;
}

ASTNode *parse_for(Parser *p) {
    Token t = advance(p);

    if (match(p, TOKEN_EACH)) {
        ASTNode *node = ast_make_node(NODE_FOR_EACH, t.line, t.col);
        node->as.for_each.var[0] = '\0';

        if (check(p, TOKEN_IDENTIFIER)) {
            AST_SET_STR(node->as.for_each.var,
                current(p).value);
            advance(p);
        }

        expect(p, TOKEN_IN, "Expected 'in' after loop variable");
        node->as.for_each.collection = parse_expression(p);
        parse_block(p, &node->as.for_each.body);
        node->end_line = current(p).line;
        return node;
    }

    ASTNode *node = ast_make_node(NODE_FOR_RANGE, t.line, t.col);
    node->as.for_range.downward = 0;
    node->as.for_range.var[0]   = '\0';

    if (check(p, TOKEN_IDENTIFIER)) {
        AST_SET_STR(node->as.for_range.var,
            current(p).value);
        advance(p);
    }

    expect(p, TOKEN_FROM, "Expected 'from': write for I from 1 to 10 for a counted loop, or for each ITEM in LIST to loop over a list");
    node->as.for_range.from = parse_expression(p);

    if (match(p, TOKEN_DOWN)) {
        node->as.for_range.downward = 1;
    }

    expect(p, TOKEN_TO, "Expected 'to' in for range loop");
    node->as.for_range.to = parse_expression(p);
    parse_block(p, &node->as.for_range.body);
    node->end_line = current(p).line;
    return node;
}

ASTNode *parse_activity_def(Parser *p) {
    Token t = advance(p);
    ASTNode *node = ast_make_node(NODE_ACTIVITY_DEF, t.line, t.col);

    char name[MAX_TOKEN_LEN] = {0};
    int first = 1;
    while (!check(p, TOKEN_COMMA)      &&
           !check(p, TOKEN_TAKING)     &&
           !check(p, TOKEN_NEWLINE)    &&
           !check(p, TOKEN_TERMINATOR) &&
           !check(p, TOKEN_EOF)) {
        if (!first) strncat(name, " ", MAX_TOKEN_LEN - strlen(name) - 1);
        strncat(name, current(p).value, MAX_TOKEN_LEN - strlen(name) - 1);
        advance(p);
        first = 0;
    }
    AST_SET_STR(node->as.activity_def.name, name);

    nodelist_init(&node->as.activity_def.params);
    nodelist_init(&node->as.activity_def.body);

    if (match(p, TOKEN_TAKING)) {
        for (;;) {
            if (!is_identifier_word(current(p).type)) break;

            Token param = advance(p);
            ASTNode *pn = ast_make_node(NODE_IDENTIFIER, param.line, param.col);
            AST_SET_STR(pn->as.identifier.name, param.value);
            nodelist_push(&node->as.activity_def.params, pn);

            if (check(p, TOKEN_COMMA) &&
                is_identifier_word(peek_at(p, 1).type)) {
                advance(p);
                continue;
            }
            break;
        }
    }

    if (match(p, TOKEN_COMMA)) {
        ASTNode *stmt = parse_statement(p);
        if (stmt) nodelist_push(&node->as.activity_def.body, stmt);
        match(p, TOKEN_TERMINATOR);
        node->end_line = current(p).line;
        return node;
    }

    parse_block(p, &node->as.activity_def.body);
    node->end_line = current(p).line;
    return node;
}

ASTNode *parse_return(Parser *p) {
    Token t = advance(p);
    ASTNode *node = ast_make_node(NODE_RETURN, t.line, t.col);
    node->as.return_stmt.value = parse_expression(p);
    return node;
}

ASTNode *parse_import(Parser *p) {
    Token t = advance(p);
    ASTNode *node = ast_make_node(NODE_IMPORT, t.line, t.col);
    node->as.import_stmt.source[0] = '\0';
    node->as.import_stmt.alias[0]  = '\0';

    if (check(p, TOKEN_STRING)) {
        AST_SET_STR(node->as.import_stmt.source, current(p).value);
        advance(p);
    } else {
        parser_error(p, "import expects a quoted module path, e.g. import \"utils.EP\"");
        return node;
    }

    if (check(p, TOKEN_AS)) {
        advance(p);
        if (check(p, TOKEN_IDENTIFIER)) {
            AST_SET_STR(node->as.import_stmt.alias, current(p).value);
            advance(p);
        }
    }
    return node;
}

ASTNode *parse_db_make_table(Parser *p) {
    Token t = advance(p);
    ASTNode *node = ast_make_node(NODE_DB_MAKE_TABLE, t.line, t.col);
    node->as.db_table.table = parse_primary(p);
    return node;
}

ASTNode *parse_db_save(Parser *p) {
    Token t = advance(p);
    ASTNode *node = ast_make_node(NODE_DB_INSERT, t.line, t.col);
    node->as.db_insert.table = parse_primary(p);
    nodelist_init(&node->as.db_insert.field_names);
    nodelist_init(&node->as.db_insert.field_values);

    if (!match(p, TOKEN_WITH)) {
        parser_error(p, "save into \"T\" expects 'with field as value, ...'");
        return node;
    }

    do {
        char fname[MAX_TOKEN_LEN] = {0};
        collect_name(p, fname, MAX_TOKEN_LEN);
        ASTNode *fn = ast_make_node(NODE_STRING, current(p).line, current(p).col);
        AST_SET_STR(fn->as.string.value, fname);

        if (!match(p, TOKEN_AS)) {
            parser_error(p, "expected 'as' between field name and value");
            ast_free(fn);
            return node;
        }
        ASTNode *val = parse_expression(p);

        nodelist_push(&node->as.db_insert.field_names,  fn);
        nodelist_push(&node->as.db_insert.field_values, val);
    } while (match(p, TOKEN_COMMA));

    return node;
}

ASTNode *parse_db_delete(Parser *p) {
    Token t = advance(p);
    ASTNode *node = ast_make_node(NODE_DB_DELETE, t.line, t.col);
    node->as.db_filter.table     = parse_primary(p);
    node->as.db_filter.condition = NULL;
    node->as.db_filter.item_var[0] = '\0';

    if (check(p, TOKEN_WHERE)) {
        advance(p);
        int saved = p->current;
        if (is_identifier_word(current(p).type)) {
            char name[MAX_TOKEN_LEN] = {0};
            collect_name(p, name, MAX_TOKEN_LEN);
            AST_SET_STR(node->as.db_filter.item_var, name);
            p->current = saved;
        } else {
            AST_SET_STR(node->as.db_filter.item_var, "item");
        }
        node->as.db_filter.condition = parse_logical(p);
    }
    return node;
}

ASTNode *parse_check(Parser *p) {
    Token t = advance(p);
    ASTNode *node = ast_make_node(NODE_CHECK, t.line, t.col);
    match(p, TOKEN_THAT);
    node->as.check_stmt.condition = parse_expression(p);
    return node;
}

ASTNode *parse_describe(Parser *p) {
    Token t = advance(p);
    ASTNode *node = ast_make_node(NODE_DESCRIBE, t.line, t.col);

    char name[MAX_TOKEN_LEN] = {0};
    int first = 1;
    while (!check(p, TOKEN_AS)      &&
           !check(p, TOKEN_NEWLINE) &&
           !check(p, TOKEN_EOF)) {
        if (!first) strncat(name, " ", MAX_TOKEN_LEN - strlen(name) - 1);
        strncat(name, current(p).value, MAX_TOKEN_LEN - strlen(name) - 1);
        advance(p);
        first = 0;
    }
    AST_SET_STR(node->as.describe_stmt.name, name);

    expect(p, TOKEN_AS, "Expected 'as' in describe statement");

    if (check(p, TOKEN_STRING)) {
        AST_SET_STR(node->as.describe_stmt.text,
            current(p).value);
        advance(p);
    } else {
        parser_error(p, "Expected string after 'as' in describe");
    }

    return node;
}

static void skip_filler_word(Parser *p, const char *word) {
    if (strcasecmp(current(p).value, word) == 0) advance(p);
}

ASTNode *parse_provide(Parser *p) {
    Token t = advance(p);
    ASTNode *node = ast_make_node(NODE_PROVIDE, t.line, t.col);

    char name[MAX_TOKEN_LEN] = {0};
    collect_name(p, name, MAX_TOKEN_LEN);
    AST_SET_STR(node->as.provide_stmt.var, name);

    expect(p, TOKEN_AS,    "Expected 'as' in provide statement");
    skip_filler_word(p, "user");
    skip_filler_word(p, "input");
    skip_filler_word(p, "entry");
    match(p, TOKEN_FROM);

    node->as.provide_stmt.prompt = parse_expression(p);
    return node;
}

ASTNode *parse_attempt(Parser *p) {
    Token t = advance(p);
    ASTNode *node = ast_make_node(NODE_ATTEMPT, t.line, t.col);

    nodelist_init(&node->as.attempt_stmt.try_body);
    nodelist_init(&node->as.attempt_stmt.fail_body);

    skip_newlines(p);

    while (!check(p, TOKEN_IF_FAILS) &&
           !check(p, TOKEN_TERMINATOR) &&
           !check(p, TOKEN_EOF)) {
        skip_newlines(p);
        if (check(p, TOKEN_IF_FAILS) ||
            check(p, TOKEN_TERMINATOR) ||
            check(p, TOKEN_EOF)) break;
        ASTNode *stmt = parse_statement(p);
        if (stmt) nodelist_push(&node->as.attempt_stmt.try_body, stmt);
        if (p->panic_mode) synchronize(p);
        skip_newlines(p);
    }

    if (match(p, TOKEN_IF_FAILS)) {
        match(p, TOKEN_THEN);
        skip_newlines(p);
        while (!check(p, TOKEN_TERMINATOR) && !check(p, TOKEN_EOF)) {
            skip_newlines(p);
            if (check(p, TOKEN_TERMINATOR) || check(p, TOKEN_EOF)) break;
            ASTNode *stmt = parse_statement(p);
            if (stmt) nodelist_push(&node->as.attempt_stmt.fail_body, stmt);
            if (p->panic_mode) synchronize(p);
            skip_newlines(p);
        }
    }

    close_block(p, t.line);
    node->end_line = current(p).line;
    return node;
}

ASTNode *parse_add_to(Parser *p) {
    Token t = advance(p);
    ASTNode *node = ast_make_node(NODE_ADD_TO, t.line, t.col);

    node->as.add_to.item = parse_expression(p);

    expect(p, TOKEN_TO, "Expected 'to' in 'add ... to ...'");

    if (check(p, TOKEN_IDENTIFIER)) {
        AST_SET_STR(node->as.add_to.list,
            current(p).value);
        advance(p);
    } else {
        parser_error(p, "Expected list name after 'to' in add statement");
    }

    return node;
}

ASTNode *parse_link(Parser *p) {
    Token t = advance(p);
    ASTNode *node = ast_make_node(NODE_LINK, t.line, t.col);

    char from[MAX_TOKEN_LEN] = {0};
    while (!check(p, TOKEN_TO) && !check(p, TOKEN_EOF) && !check(p, TOKEN_NEWLINE)) {
        if (strlen(from) > 0) strncat(from, " ", MAX_TOKEN_LEN - strlen(from) - 1);
        strncat(from, current(p).value, MAX_TOKEN_LEN - strlen(from) - 1);
        advance(p);
    }
    AST_SET_STR(node->as.link_stmt.from, from);

    expect(p, TOKEN_TO, "Expected 'to' in link statement");

    char to[MAX_TOKEN_LEN] = {0};
    while (!check(p, TOKEN_IN) && !check(p, TOKEN_EOF) && !check(p, TOKEN_NEWLINE)) {
        if (strlen(to) > 0) strncat(to, " ", MAX_TOKEN_LEN - strlen(to) - 1);
        strncat(to, current(p).value, MAX_TOKEN_LEN - strlen(to) - 1);
        advance(p);
    }
    AST_SET_STR(node->as.link_stmt.to, to);

    expect(p, TOKEN_IN, "Expected 'in <graph>' in link statement");

    char graph[MAX_TOKEN_LEN] = {0};
    collect_name(p, graph, MAX_TOKEN_LEN);
    AST_SET_STR(node->as.link_stmt.graph, graph);

    return node;
}

ASTNode *parse_node3d(Parser *p) {
    Token t = advance(p);
    ASTNode *node = ast_make_node(NODE_NODE3D, t.line, t.col);
    node->as.node3d.angle  = 0.0;
    node->as.node3d.status = NODE3D_PENDING;
    node->as.node3d.label[0]= '\0';
    node->as.node3d.desc[0] = '\0';

    if (check(p, TOKEN_AT_KW)) {
        advance(p);
        if (check(p, TOKEN_NUMBER)) {
            node->as.node3d.angle = atof(current(p).value);
            advance(p);
        }
    }
    if (check(p, TOKEN_LABELED)) {
        advance(p);
        if (check(p, TOKEN_STRING)) {
            AST_SET_STR(node->as.node3d.label,
                current(p).value);
            advance(p);
        }
    }
    if (check(p, TOKEN_STATUS_KW)) {
        advance(p);
        if (check(p, TOKEN_STRING)) {
            const char *s = current(p).value;
            if      (strcmp(s,"completed")==0)  node->as.node3d.status = NODE3D_COMPLETED;
            else if (strcmp(s,"inprogress")==0)  node->as.node3d.status = NODE3D_INPROGRESS;
            else if (strcmp(s,"overdue")==0)     node->as.node3d.status = NODE3D_OVERDUE;
            else                                 node->as.node3d.status = NODE3D_PENDING;
            advance(p);
        }
    }
    return node;
}

ASTNode *parse_core_star(Parser *p) {
    Token t = advance(p);
    match(p, TOKEN_STAR_KW);
    ASTNode *node = ast_make_node(NODE_CORE_STAR, t.line, t.col);
    node->as.core_star.color[0] = '\0';
    node->as.core_star.label[0] = '\0';
    node->as.core_star.desc[0]  = '\0';
    AST_SET_STR(node->as.core_star.color, "gold");

    if (check(p, TOKEN_WITH_COLOR)) {
        advance(p);
        if (check(p, TOKEN_STRING)) {
            AST_SET_STR(node->as.core_star.color,
                current(p).value);
            advance(p);
        }
    }
    if (check(p, TOKEN_LABELED)) {
        advance(p);
        if (check(p, TOKEN_STRING)) {
            AST_SET_STR(node->as.core_star.label,
                current(p).value);
            advance(p);
        }
    }
    return node;
}

ASTNode *parse_orbital_ring(Parser *p) {
    Token t = advance(p);
    match(p, TOKEN_RING_KW);
    ASTNode *node = ast_make_node(NODE_ORBITAL_RING, t.line, t.col);
    nodelist_init(&node->as.orbital_ring.nodes);
    node->as.orbital_ring.radius = 100.0;
    node->as.orbital_ring.speed  = 0.003;
    node->as.orbital_ring.tilt_x = 0.0;
    node->as.orbital_ring.tilt_z = 0.0;
    AST_SET_STR(node->as.orbital_ring.color, "indigo");

    if (check(p, TOKEN_RADIUS_KW)) {
        advance(p);
        if (check(p, TOKEN_NUMBER)) {
            node->as.orbital_ring.radius = atof(current(p).value);
            advance(p);
        }
    }
    if (check(p, TOKEN_WITH_COLOR)||check(p, TOKEN_STYLED)) {
        advance(p);
        if (check(p, TOKEN_STRING)) {
            AST_SET_STR(node->as.orbital_ring.color,
                current(p).value);
            advance(p);
        }
    }
    if (check(p, TOKEN_SPEED_KW)) {
        advance(p);
        if (check(p, TOKEN_NUMBER)) {
            node->as.orbital_ring.speed = atof(current(p).value);
            advance(p);
        }
    }
    if (check(p, TOKEN_X_AXIS)) {
        advance(p);
        if (check(p, TOKEN_NUMBER)) {
            node->as.orbital_ring.tilt_x = atof(current(p).value);
            advance(p);
        }
    }
    if (check(p, TOKEN_Z_AXIS)||check(p, TOKEN_Y_AXIS)) {
        advance(p);
        if (check(p, TOKEN_NUMBER)) {
            node->as.orbital_ring.tilt_z = atof(current(p).value);
            advance(p);
        }
    }

    while (!check(p, TOKEN_TERMINATOR) && !check(p, TOKEN_EOF)) {
        if (check(p, TOKEN_NEWLINE)) { advance(p); continue; }
        if (check(p, TOKEN_NODE_3D)) {
            ASTNode *nd = parse_node3d(p);
            if (nd) nodelist_push(&node->as.orbital_ring.nodes, nd);
        } else {
            advance(p);
        }
    }
    match(p, TOKEN_TERMINATOR);
    node->end_line = current(p).line;
    return node;
}

ASTNode *parse_canvas3d(Parser *p) {
    Token t = advance(p);
    ASTNode *node = ast_make_node(NODE_CANVAS3D, t.line, t.col);
    nodelist_init(&node->as.canvas3d.children);
    node->as.canvas3d.background = BG3D_DEEP_SPACE;
    node->as.canvas3d.offline    = 0;
    node->as.canvas3d.name[0]    = '\0';
    node->as.canvas3d.bg_color[0]= '\0';

    if (check(p, TOKEN_STRING)) {
        AST_SET_STR(node->as.canvas3d.name,
            current(p).value);
        advance(p);
    }

    if (check(p, TOKEN_BACKGROUND_KW)) {
        advance(p);
        if (check(p, TOKEN_STRING)) {
            const char *bg = current(p).value;
            if      (strstr(bg,"space")) node->as.canvas3d.background = BG3D_DEEP_SPACE;
            else if (strstr(bg,"void"))  node->as.canvas3d.background = BG3D_VOID;
            else if (strstr(bg,"nebula"))node->as.canvas3d.background = BG3D_NEBULA;
            else {
                node->as.canvas3d.background = BG3D_CUSTOM;
                AST_SET_STR(node->as.canvas3d.bg_color, bg);
            }
            advance(p);
        }
    }

    if (check(p, TOKEN_OFFLINE)) {
        node->as.canvas3d.offline = 1;
        advance(p);
    }

    while (!check(p, TOKEN_TERMINATOR) && !check(p, TOKEN_EOF)) {
        if (check(p, TOKEN_NEWLINE)) { advance(p); continue; }
        ASTNode *child = parse_statement(p);
        if (child) nodelist_push(&node->as.canvas3d.children, child);
    }
    match(p, TOKEN_TERMINATOR);
    node->end_line = current(p).line;
    return node;
}

ASTNode *parse_panel3d(Parser *p) {
    Token t = advance(p);
    ASTNode *node = ast_make_node(NODE_PANEL3D, t.line, t.col);
    nodelist_init(&node->as.panel3d.children);
    node->as.panel3d.position = PANEL_RIGHT;

    if (check(p, TOKEN_STRING)) {
        const char *pos = current(p).value;
        if      (strcmp(pos,"top")==0)    node->as.panel3d.position = PANEL_TOP;
        else if (strcmp(pos,"left")==0)   node->as.panel3d.position = PANEL_LEFT;
        else if (strcmp(pos,"bottom")==0) node->as.panel3d.position = PANEL_BOTTOM;
        else                              node->as.panel3d.position = PANEL_RIGHT;
        advance(p);
    }

    while (!check(p, TOKEN_TERMINATOR) && !check(p, TOKEN_EOF)) {
        if (check(p, TOKEN_NEWLINE)) { advance(p); continue; }
        ASTNode *child = parse_statement(p);
        if (child) nodelist_push(&node->as.panel3d.children, child);
    }
    match(p, TOKEN_TERMINATOR);
    node->end_line = current(p).line;
    return node;
}

ASTNode *parse_drawer3d(Parser *p) {
    Token t = advance(p);
    ASTNode *node = ast_make_node(NODE_DRAWER3D, t.line, t.col);
    nodelist_init(&node->as.drawer3d.children);
    node->as.drawer3d.position = PANEL_RIGHT;
    node->as.drawer3d.title[0] = '\0';

    if (check(p, TOKEN_STRING)) {
        const char *pos = current(p).value;
        if (strcmp(pos,"left")==0) node->as.drawer3d.position = PANEL_LEFT;
        advance(p);
    }
    if (check(p, TOKEN_TITLED)) {
        advance(p);
        if (check(p, TOKEN_STRING)) {
            AST_SET_STR(node->as.drawer3d.title,
                current(p).value);
            advance(p);
        }
    }

    while (!check(p, TOKEN_TERMINATOR) && !check(p, TOKEN_EOF)) {
        if (check(p, TOKEN_NEWLINE)) { advance(p); continue; }
        ASTNode *child = parse_statement(p);
        if (child) nodelist_push(&node->as.drawer3d.children, child);
    }
    match(p, TOKEN_TERMINATOR);
    node->end_line = current(p).line;
    return node;
}

ASTNode *parse_text_node(Parser *p, NodeType type) {
    Token t = advance(p);
    ASTNode *node = ast_make_node(type, t.line, t.col);
    node->as.text_out.value = parse_expression(p);
    return node;
}

ASTNode *parse_section(Parser *p) {
    Token t = advance(p);
    ASTNode *node = ast_make_node(NODE_SECTION_OUT, t.line, t.col);
    nodelist_init(&node->as.section_out.children);
    node->as.section_out.title[0] = '\0';

    if (check(p, TOKEN_STRING)) {
        AST_SET_STR(node->as.section_out.title,
            current(p).value);
        advance(p);
    }

    while (!check(p, TOKEN_TERMINATOR) && !check(p, TOKEN_EOF)) {
        if (check(p, TOKEN_NEWLINE)) { advance(p); continue; }
        ASTNode *child = parse_statement(p);
        if (child) nodelist_push(&node->as.section_out.children, child);
    }
    match(p, TOKEN_TERMINATOR);
    node->end_line = current(p).line;
    return node;
}

ASTNode *parse_row(Parser *p) {
    Token t = advance(p);
    ASTNode *node = ast_make_node(NODE_ROW, t.line, t.col);
    nodelist_init(&node->as.row_out.children);

    while (!check(p, TOKEN_TERMINATOR) && !check(p, TOKEN_EOF)) {
        if (check(p, TOKEN_NEWLINE)) { advance(p); continue; }
        ASTNode *child = parse_statement(p);
        if (child) nodelist_push(&node->as.row_out.children, child);
    }
    match(p, TOKEN_TERMINATOR);
    node->end_line = current(p).line;
    return node;
}

ASTNode *parse_make_stat(Parser *p) {
    Token t = advance(p);
    ASTNode *node = ast_make_node(NODE_MAKE_STAT, t.line, t.col);
    node->as.make_stat.label[0] = '\0';

    if (check(p, TOKEN_STRING)) {
        AST_SET_STR(node->as.make_stat.label,
            current(p).value);
        advance(p);
    }

    match(p, TOKEN_FROM);
    node->as.make_stat.value = parse_expression(p);
    return node;
}

ASTNode *parse_make_chart(Parser *p) {
    Token t = advance(p);
    ASTNode *node = ast_make_node(NODE_MAKE_CHART, t.line, t.col);

    if (t.type == TOKEN_LINE_CHART)     node->as.make_chart.type = CHART_LINE;
    else if (t.type == TOKEN_BAR_CHART) node->as.make_chart.type = CHART_BAR;
    else                                node->as.make_chart.type = CHART_PIE;

    if (check(p, TOKEN_MAKE_CHART)) advance(p);

    node->as.make_chart.title[0]    = '\0';
    node->as.make_chart.filename[0] = '\0';
    node->as.make_chart.x_label[0]  = '\0';
    node->as.make_chart.y_label[0]  = '\0';
    node->as.make_chart.x_data      = NULL;
    node->as.make_chart.y_data      = NULL;

    if (node->as.make_chart.type == CHART_PIE) {
        match(p, TOKEN_FROM);
        node->as.make_chart.x_data = parse_primary(p);
        match(p, TOKEN_AND);
        node->as.make_chart.y_data = parse_primary(p);
    } else {
        match(p, TOKEN_FOR);
        match(p, TOKEN_X_AXIS);
        match(p, TOKEN_AXIS);
        match(p, TOKEN_AS);
        char xname[MAX_TOKEN_LEN] = {0};
        collect_name(p, xname, MAX_TOKEN_LEN);
        AST_SET_STR(node->as.make_chart.x_label, xname);
        node->as.make_chart.x_data = ast_make_node(NODE_IDENTIFIER,
            t.line, t.col);
        AST_SET_STR(node->as.make_chart.x_data->as.identifier.name,
            xname);

        match(p, TOKEN_AND);
        match(p, TOKEN_Y_AXIS);
        match(p, TOKEN_AXIS);
        match(p, TOKEN_AS);
        char yname[MAX_TOKEN_LEN] = {0};
        collect_name(p, yname, MAX_TOKEN_LEN);
        AST_SET_STR(node->as.make_chart.y_label, yname);
        node->as.make_chart.y_data = ast_make_node(NODE_IDENTIFIER,
            t.line, t.col);
        AST_SET_STR(node->as.make_chart.y_data->as.identifier.name,
            yname);
    }

    if (check(p, TOKEN_TITLED)) {
        advance(p);
        if (check(p, TOKEN_STRING)) {
            AST_SET_STR(node->as.make_chart.title,
                current(p).value);
            advance(p);
        }
    }

    return node;
}

ASTNode *parse_make_table_out(Parser *p) {
    Token t = advance(p);
    ASTNode *node = ast_make_node(NODE_MAKE_TABLE_OUT, t.line, t.col);
    node->as.make_table_out.title[0]    = '\0';
    node->as.make_table_out.filename[0] = '\0';
    node->as.make_table_out.headers     = NULL;

    match(p, TOKEN_FROM);
    node->as.make_table_out.data = parse_primary(p);

    if (check(p, TOKEN_TITLED)) {
        advance(p);
        if (check(p, TOKEN_STRING)) {
            AST_SET_STR(node->as.make_table_out.title,
                current(p).value);
            advance(p);
        }
    }

    if (check(p, TOKEN_WITH)) {
        advance(p);
        match(p, TOKEN_HEADERS);
        node->as.make_table_out.headers = parse_primary(p);
    }

    return node;
}

ASTNode *parse_make_report(Parser *p) {
    Token t = advance(p);
    ASTNode *node = ast_make_node(NODE_MAKE_REPORT, t.line, t.col);
    node->as.make_report.title[0]    = '\0';
    node->as.make_report.filename[0] = '\0';

    match(p, TOKEN_FROM);
    node->as.make_report.data = parse_primary(p);

    if (check(p, TOKEN_TITLED)) {
        advance(p);
        if (check(p, TOKEN_STRING)) {
            AST_SET_STR(node->as.make_report.title,
                current(p).value);
            advance(p);
        }
    }

    return node;
}

ASTNode *parse_make_page(Parser *p) {
    Token t = advance(p);
    ASTNode *node = ast_make_node(NODE_MAKE_PAGE, t.line, t.col);
    nodelist_init(&node->as.make_page.children);
    node->as.make_page.filename[0] = '\0';
    node->as.make_page.color[0]    = '\0';
    node->as.make_page.theme       = THEME_LIGHT;

    if (check(p, TOKEN_SAVED)) {
        advance(p);
        match(p, TOKEN_AS);
        if (check(p, TOKEN_STRING)) {
            AST_SET_STR(node->as.make_page.filename,
                current(p).value);
            advance(p);
        }
    }

    if (check(p, TOKEN_STYLED)) {
        advance(p);
        match(p, TOKEN_AS);
        if (check(p, TOKEN_STRING)) {
            const char *theme = current(p).value;
            if      (strcmp(theme, "dark")    == 0) node->as.make_page.theme = THEME_DARK;
            else if (strcmp(theme, "indigo")  == 0) node->as.make_page.theme = THEME_INDIGO;
            else if (strcmp(theme, "emerald") == 0) node->as.make_page.theme = THEME_EMERALD;
            else if (strcmp(theme, "rose")    == 0) node->as.make_page.theme = THEME_ROSE;
            else if (strcmp(theme, "light")   == 0) node->as.make_page.theme = THEME_LIGHT;
            else {
                node->as.make_page.theme = THEME_CUSTOM;
                AST_SET_STR(node->as.make_page.color, theme);
            }
            advance(p);
        }
    }

    if (check(p, TOKEN_WITH)) {
        advance(p);
        if (check(p, TOKEN_WITH_COLOR)) {
            advance(p);
            if (check(p, TOKEN_STRING)) {
                AST_SET_STR(node->as.make_page.color,
                    current(p).value);
                advance(p);
            }
        }
    }

    while (!check(p, TOKEN_TERMINATOR) && !check(p, TOKEN_EOF)) {
        if (check(p, TOKEN_NEWLINE)) { advance(p); continue; }
        ASTNode *child = parse_statement(p);
        if (child) nodelist_push(&node->as.make_page.children, child);
    }
    match(p, TOKEN_TERMINATOR);
    node->end_line = current(p).line;
    return node;
}

ASTNode *parse_wait(Parser *p) {
    Token t = advance(p);
    ASTNode *node = ast_make_node(NODE_WAIT, t.line, t.col);
    node->as.wait_stmt.message[0] = '\0';

    while (!check(p, TOKEN_STRING) &&
           !check(p, TOKEN_NEWLINE) &&
           !check(p, TOKEN_TERMINATOR) &&
           !check(p, TOKEN_EOF)) {
        advance(p);
    }

    if (check(p, TOKEN_STRING)) {
        AST_SET_STR(node->as.wait_stmt.message,
            current(p).value);
        advance(p);
    }

    return node;
}

ASTNode *parse_execute(Parser *p) {
    Token t = advance(p);
    ASTNode *node = ast_make_node(NODE_EXECUTE, t.line, t.col);
    node->as.execute_stmt.conditional = 0;
    node->as.execute_stmt.command[0]  = '\0';

    match(p, TOKEN_SYSTEM);
    match(p, TOKEN_COMMAND);

    if (check(p, TOKEN_STRING)) {
        AST_SET_STR(node->as.execute_stmt.command,
            current(p).value);
        advance(p);
    } else {
        parser_error(p, "Expected command string in execute statement");
        return node;
    }

    if (match(p, TOKEN_IF)) {
        match(p, TOKEN_WINDOWS);
        node->as.execute_stmt.conditional = 1;
        match(p, TOKEN_OTHERWISE);
        if (check(p, TOKEN_STRING)) {
            AST_SET_STR(node->as.execute_stmt.alt_command,
                current(p).value);
            advance(p);
        }
    }

    return node;
}

ASTNode *parse_write_file(Parser *p, NodeType kind) {
    Token t = advance(p);
    ASTNode *node = ast_make_node(kind, t.line, t.col);
    node->as.file_write.value_expr = parse_expression(p);
    match(p, TOKEN_TO);
    match(p, TOKEN_FILE_KW);
    node->as.file_write.path_expr = parse_primary(p);
    return node;
}

ASTNode *parse_delete_file(Parser *p) {
    Token t = advance(p);
    if (t.type == TOKEN_DELETE_KW) match(p, TOKEN_FILE_KW);
    ASTNode *node = ast_make_node(NODE_DELETE_FILE, t.line, t.col);
    node->as.file_op.path_expr = parse_primary(p);
    return node;
}

ASTNode *parse_sleep(Parser *p) {
    Token t = advance(p);
    ASTNode *node = ast_make_node(NODE_SLEEP, t.line, t.col);
    node->as.sleep_stmt.duration_expr = parse_primary(p);
    return node;
}

ASTNode *parse_serve_socket(Parser *p) {
    Token t = advance(p);
    ASTNode *node = ast_make_node(NODE_SERVE_SOCKET, t.line, t.col);
    node->as.serve_socket.path_expr = parse_primary(p);
    match(p, TOKEN_USING_KW);
    char name[MAX_TOKEN_LEN] = {0};
    collect_name(p, name, MAX_TOKEN_LEN);
    AST_SET_STR(node->as.serve_socket.activity, name);
    return node;
}

ASTNode *parse_serve_port(Parser *p) {
    Token t = advance(p);
    ASTNode *node = ast_make_node(NODE_SERVE_PORT, t.line, t.col);
    nodelist_init(&node->as.serve_port.offered);
    node->as.serve_port.port_expr = parse_primary(p);

    if (!(check(p, TOKEN_IDENTIFIER) &&
          strcmp(current(p).value, "offering") == 0)) {
        parser_error(p, "write the activities a page may call: "
                        "serve on port 8080 offering sell, revenue");
        return node;
    }
    advance(p);
    for (;;) {
        Token at = current(p);
        char name[MAX_TOKEN_LEN] = {0};
        collect_name(p, name, MAX_TOKEN_LEN);
        if (!name[0]) {
            parser_error(p, "expected an activity name after 'offering'");
            return node;
        }
        ASTNode *id = ast_make_node(NODE_IDENTIFIER, at.line, at.col);
        AST_SET_STR(id->as.identifier.name, name);
        nodelist_push(&node->as.serve_port.offered, id);
        if (!match(p, TOKEN_COMMA) && !match(p, TOKEN_AND)) break;
    }
    return node;
}

ASTNode *parse_reply_with(Parser *p) {
    Token t = advance(p);
    ASTNode *node = ast_make_node(NODE_REPLY_WITH, t.line, t.col);
    node->as.reply_with.value_expr = parse_expression(p);
    return node;
}

ASTNode *parse_expression(Parser *p) {
    ASTNode *left = parse_logical(p);

    if (check(p, TOKEN_WHERE)) {
        Token wt = advance(p);
        ASTNode *node = ast_make_node(NODE_WHERE_FILTER, wt.line, wt.col);
        node->as.where_filter.collection = left;

        int saved_pos = p->current;
        if (is_identifier_word(current(p).type)) {
            char name[MAX_TOKEN_LEN] = {0};
            collect_name(p, name, MAX_TOKEN_LEN);
            AST_SET_STR(node->as.where_filter.item_var, name);
            p->current = saved_pos;
        } else {
            AST_SET_STR(node->as.where_filter.item_var, "item");
        }

        node->as.where_filter.condition = parse_logical(p);
        return node;
    }

    if (check(p, TOKEN_IF)) {
        int line = current(p).line;
        int col  = current(p).col;
        advance(p);

        ASTNode *node = ast_make_node(NODE_INLINE_IF, line, col);
        node->as.inline_if.then_val  = left;
        node->as.inline_if.condition = parse_logical(p);

        expect(p, TOKEN_OTHERWISE, "Expected 'otherwise' in inline if");
        node->as.inline_if.else_val  = parse_expression(p);
        return node;
    }

    while (check(p, TOKEN_PLUS)) {
        Token op = advance(p);
        ASTNode *right = parse_logical(p);
        ASTNode *bin = ast_make_node(NODE_BINARY_OP, op.line, op.col);
        bin->as.binary_op.operator = TOKEN_PLUS;
        bin->as.binary_op.left  = left;
        bin->as.binary_op.right = right;
        left = bin;
    }

    return left;
}

ASTNode *parse_logical(Parser *p) {
    ASTNode *left = parse_comparison(p);

    while (check(p, TOKEN_AND) || check(p, TOKEN_OR)) {
        Token op = advance(p);
        ASTNode *right = parse_comparison(p);
        ASTNode *node  = ast_make_node(NODE_BINARY_OP, op.line, op.col);
        node->as.binary_op.operator = op.type;
        node->as.binary_op.left     = left;
        node->as.binary_op.right    = right;
        left = node;
    }
    return left;
}

ASTNode *parse_comparison(Parser *p) {
    ASTNode *left = parse_additive(p);

    while (check(p, TOKEN_EQUALS)       ||
           check(p, TOKEN_IS)           ||
           check(p, TOKEN_IS_NOT)       ||
           check(p, TOKEN_NOT_EQUALS)   ||
           check(p, TOKEN_GREATER_THAN) ||
           check(p, TOKEN_LESS_THAN)    ||
           check(p, TOKEN_AT_LEAST)     ||
           check(p, TOKEN_AT_MOST)      ||
           check(p, TOKEN_IS_ANY_OF)    ||
           check(p, TOKEN_IS_NONE_OF)   ||
           check(p, TOKEN_IN)) {

        if ((check(p, TOKEN_IS) || check(p, TOKEN_IS_NOT)) &&
            peek_at(p, 1).type == TOKEN_EMPTY &&
            peek_at(p, 2).type != TOKEN_LIST_KW &&
            !(peek_at(p, 2).type == TOKEN_IDENTIFIER &&
              strcmp(peek_at(p, 2).value, "record") == 0)) {
            Token op = advance(p);
            advance(p);
            ASTNode *chk = ast_make_node(NODE_TYPE_CHECK, op.line, op.col);
            chk->as.type_check.kind   = TYPECHECK_EMPTY;
            chk->as.type_check.target = left;
            if (op.type == TOKEN_IS_NOT) {
                ASTNode *neg = ast_make_node(NODE_UNARY_OP, op.line, op.col);
                neg->as.unary_op.operator = TOKEN_NOT;
                neg->as.unary_op.operand  = chk;
                left = neg;
            } else {
                left = chk;
            }
            continue;
        }

        if (check(p, TOKEN_IS)) {
            int saved = p->current;
            advance(p);
            if (check(p, TOKEN_A)) {
                advance(p);
                TypeCheckKind kind;
                int matched = 1;
                Token kw = current(p);
                if      (kw.type == TOKEN_NUMBER_KW)  kind = TYPECHECK_NUMBER;
                else if (kw.type == TOKEN_STRING_KW)  kind = TYPECHECK_STRING;
                else if (kw.type == TOKEN_BOOLEAN_KW) kind = TYPECHECK_BOOLEAN;
                else if (kw.type == TOKEN_LIST_KW)    kind = TYPECHECK_LIST;
                else matched = 0;

                if (matched) {
                    advance(p);
                    ASTNode *node = ast_make_node(NODE_TYPE_CHECK, kw.line, kw.col);
                    node->as.type_check.kind   = kind;
                    node->as.type_check.target = left;
                    left = node;
                    continue;
                }
            }
            p->current = saved;
        }

        Token op = advance(p);
        ASTNode *node = ast_make_node(NODE_BINARY_OP, op.line, op.col);
        node->as.binary_op.operator = op.type;
        node->as.binary_op.left     = left;

        if (op.type == TOKEN_IS_ANY_OF || op.type == TOKEN_IS_NONE_OF) {
            ASTNode *list_node = ast_make_node(NODE_LIST, op.line, op.col);
            nodelist_init(&list_node->as.list.elements);

            do {
                match(p, TOKEN_COMMA);
                ASTNode *val = parse_primary(p);
                if (val) nodelist_push(&list_node->as.list.elements, val);
            } while (check(p, TOKEN_COMMA));

            node->as.binary_op.right = list_node;
        } else {
            node->as.binary_op.right = parse_additive(p);
        }

        left = node;
    }
    return left;
}

static int is_format_postfix(Parser *p) {
    if (check(p, TOKEN_ROUNDED)) return 1;
    if (!check(p, TOKEN_AS)) return 0;
    TokenType next = peek_at(p, 1).type;
    return next == TOKEN_CURRENCY || next == TOKEN_PERCENTAGE ||
           next == TOKEN_FORMATTED;
}

static ASTNode *fold_additive(ASTNode **operands, Token *ops, int from, int to) {
    ASTNode *left = operands[from];
    for (int i = from + 1; i <= to; i++) {
        ASTNode *node = ast_make_node(NODE_BINARY_OP, ops[i].line, ops[i].col);
        node->as.binary_op.operator = ops[i].type;
        node->as.binary_op.left     = left;
        node->as.binary_op.right    = operands[i];
        left = node;
    }
    return left;
}

#define MAX_ADDITIVE_OPERANDS 64
ASTNode *parse_additive(Parser *p) {
    ASTNode *operands[MAX_ADDITIVE_OPERANDS];
    Token    ops[MAX_ADDITIVE_OPERANDS];
    int n = 0;
    operands[n++] = parse_multiplicative(p);

    while (check(p, TOKEN_PLUS) || check(p, TOKEN_MINUS)) {
        Token op = advance(p);
        ASTNode *right = parse_multiplicative(p);
        if (n == MAX_ADDITIVE_OPERANDS) {
            operands[0] = fold_additive(operands, ops, 0, n - 1);
            n = 1;
        }
        ops[n] = op;
        operands[n++] = right;
    }

    if (!is_format_postfix(p))
        return fold_additive(operands, ops, 0, n - 1);

    int start = 0;
    for (int i = n - 1; i >= 0; i--) {
        if (operands[i]->type == NODE_STRING) { start = i + 1; break; }
    }
    if (start >= n) start = 0;

    ASTNode *tail = fold_additive(operands, ops, start, n - 1);
    while (is_format_postfix(p)) {
        ASTNode *formatted = parse_format_op(p, tail);
        if (formatted == tail) break;
        tail = formatted;
    }
    if (start == 0) return tail;

    ASTNode *head = fold_additive(operands, ops, 0, start - 1);
    ASTNode *node = ast_make_node(NODE_BINARY_OP, ops[start].line, ops[start].col);
    node->as.binary_op.operator = ops[start].type;
    node->as.binary_op.left     = head;
    node->as.binary_op.right    = tail;
    return node;
}

ASTNode *parse_multiplicative(Parser *p) {
    ASTNode *left = parse_unary(p);

    while ((check(p, TOKEN_TIMES_OP) && !p->no_times_op) ||
           (check(p, TOKEN_TIMES)    && !p->no_times_op) ||
           check(p, TOKEN_DIVIDED_BY)||
           check(p, TOKEN_MODULO)    ||
           check(p, TOKEN_POWER_OF)) {
        Token op = advance(p);
        ASTNode *right = parse_unary(p);
        ASTNode *node  = ast_make_node(NODE_BINARY_OP, op.line, op.col);
        node->as.binary_op.operator = op.type;
        node->as.binary_op.left     = left;
        node->as.binary_op.right    = right;
        left = node;
    }
    return left;
}

ASTNode *parse_unary(Parser *p) {
    if (check(p, TOKEN_MINUS)) {
        Token op = advance(p);
        ASTNode *node = ast_make_node(NODE_UNARY_OP, op.line, op.col);
        node->as.unary_op.operator = TOKEN_MINUS;
        node->as.unary_op.operand  = parse_unary(p);
        return node;
    }
    if (check(p, TOKEN_NOT)) {
        Token op = advance(p);
        ASTNode *node = ast_make_node(NODE_UNARY_OP, op.line, op.col);
        node->as.unary_op.operator = TOKEN_NOT;
        node->as.unary_op.operand  = parse_unary(p);
        return node;
    }

    ASTNode *node = parse_primary(p);

    while (check(p, TOKEN_IN)       ||
           check(p, TOKEN_TRIMMED)  ||
           check(p, TOKEN_SPLIT)    ||
           check(p, TOKEN_JOINED)   ||
           check(p, TOKEN_WITH)     ||
           check(p, TOKEN_STARTS)   ||
           check(p, TOKEN_ENDS)     ||
           check(p, TOKEN_CONTAINS)) {
        ASTNode *result = parse_string_op(p, node);
        if (result == node) break;
        node = result;
    }

    return node;
}

ASTNode *parse_primary(Parser *p) {
    Token t = current(p);

    if (t.type == TOKEN_DB_RECORDS) {
        advance(p);
        ASTNode *node = ast_make_node(NODE_DB_QUERY, t.line, t.col);
        node->as.db_table.table = parse_primary(p);
        return node;
    }

    if (t.type == TOKEN_DB_COUNT) {
        advance(p);
        ASTNode *node = ast_make_node(NODE_DB_COUNT, t.line, t.col);
        node->as.db_filter.table     = parse_primary(p);
        node->as.db_filter.condition = NULL;
        node->as.db_filter.item_var[0] = '\0';
        if (check(p, TOKEN_WHERE)) {
            advance(p);
            int saved = p->current;
            if (is_identifier_word(current(p).type)) {
                char name[MAX_TOKEN_LEN] = {0};
                collect_name(p, name, MAX_TOKEN_LEN);
                AST_SET_STR(node->as.db_filter.item_var, name);
                p->current = saved;
            } else {
                AST_SET_STR(node->as.db_filter.item_var, "item");
            }
            node->as.db_filter.condition = parse_logical(p);
        }
        return node;
    }

    if (t.type == TOKEN_NUMBER) {
        advance(p);
        ASTNode *node = ast_make_node(NODE_NUMBER, t.line, t.col);
        node->as.number.value = atof(t.value);
        return node;
    }

    if (t.type == TOKEN_STRING) {
        advance(p);
        ASTNode *node = ast_make_node(NODE_STRING, t.line, t.col);
        AST_SET_STR(node->as.string.value, t.value);
        return node;
    }

    if (t.type == TOKEN_BOOLEAN) {
        advance(p);
        ASTNode *node = ast_make_node(NODE_BOOLEAN, t.line, t.col);
        node->as.boolean.value = (strcmp(t.value, "true") == 0) ? 1 : 0;
        return node;
    }

    if (t.type == TOKEN_EMPTY) {
        advance(p);
        if (check(p, TOKEN_LIST_KW)) {
            advance(p);
            ASTNode *list = ast_make_node(NODE_LIST, t.line, t.col);
            nodelist_init(&list->as.list.elements);
            return list;
        }
        if (check(p, TOKEN_IDENTIFIER) && strcmp(current(p).value, "record") == 0) {
            advance(p);
            ASTNode *text = ast_make_node(NODE_STRING, t.line, t.col);
            AST_SET_STR(text->as.string.value, "{}");
            ASTNode *rec = ast_make_node(NODE_PARSE_JSON, t.line, t.col);
            rec->as.json_convert.value_expr = text;
            return rec;
        }
        return ast_make_node(NODE_NULL, t.line, t.col);
    }
    if (t.type == TOKEN_NULL) {
        advance(p);
        return ast_make_node(NODE_NULL, t.line, t.col);
    }

    if (t.type == TOKEN_LBRACKET) {
        return parse_list_literal(p);
    }

    if (t.type == TOKEN_LPAREN) {
        advance(p);
        ASTNode *expr = parse_expression(p);
        expect(p, TOKEN_RPAREN, "Expected ')' to close expression");
        return expr;
    }

    if (t.type == TOKEN_ITEM) {
        advance(p);
        ASTNode *node = ast_make_node(NODE_INDEX_ACCESS, t.line, t.col);
        node->as.index_access.index = parse_additive(p);
        match(p, TOKEN_OF);
        node->as.index_access.list  = parse_primary(p);
        return node;
    }

    if (t.type == TOKEN_GRAPH) {
        advance(p);
        return ast_make_node(NODE_MAKE_GRAPH, t.line, t.col);
    }

    if (t.type == TOKEN_ROUTE) {
        advance(p);
        ASTNode *node = ast_make_node(NODE_ROUTE, t.line, t.col);

        match(p, TOKEN_FROM);

        char from[MAX_TOKEN_LEN] = {0};
        while (!check(p, TOKEN_TO) && !check(p, TOKEN_EOF) && !check(p, TOKEN_NEWLINE)) {
            if (strlen(from) > 0) strncat(from, " ", MAX_TOKEN_LEN - strlen(from) - 1);
            strncat(from, current(p).value, MAX_TOKEN_LEN - strlen(from) - 1);
            advance(p);
        }
        AST_SET_STR(node->as.route_stmt.from, from);

        expect(p, TOKEN_TO, "Expected 'to' in route expression");

        char to[MAX_TOKEN_LEN] = {0};
        while (!check(p, TOKEN_IN) && !check(p, TOKEN_EOF) && !check(p, TOKEN_NEWLINE)) {
            if (strlen(to) > 0) strncat(to, " ", MAX_TOKEN_LEN - strlen(to) - 1);
            strncat(to, current(p).value, MAX_TOKEN_LEN - strlen(to) - 1);
            advance(p);
        }
        AST_SET_STR(node->as.route_stmt.to, to);

        expect(p, TOKEN_IN, "Expected 'in <graph>' in route expression");

        char graph[MAX_TOKEN_LEN] = {0};
        collect_name(p, graph, MAX_TOKEN_LEN);
        AST_SET_STR(node->as.route_stmt.graph, graph);

        return node;
    }

    if (t.type == TOKEN_CONNECTED) {
        advance(p);
        ASTNode *node = ast_make_node(NODE_CONNECTED, t.line, t.col);

        match(p, TOKEN_TO);

        char target[MAX_TOKEN_LEN] = {0};
        while (!check(p, TOKEN_IN) && !check(p, TOKEN_EOF) && !check(p, TOKEN_NEWLINE)) {
            if (strlen(target) > 0) strncat(target, " ", MAX_TOKEN_LEN - strlen(target) - 1);
            strncat(target, current(p).value, MAX_TOKEN_LEN - strlen(target) - 1);
            advance(p);
        }
        AST_SET_STR(node->as.connected_stmt.target, target);

        expect(p, TOKEN_IN, "Expected 'in <graph>' in connected expression");

        char graph[MAX_TOKEN_LEN] = {0};
        collect_name(p, graph, MAX_TOKEN_LEN);
        AST_SET_STR(node->as.connected_stmt.graph, graph);

        return node;
    }

    if (t.type == TOKEN_AVERAGE  ||
        t.type == TOKEN_TOTAL    ||
        t.type == TOKEN_MAXIMUM  ||
        t.type == TOKEN_MINIMUM  ||
        t.type == TOKEN_LENGTH) {
        if (peek_at(p, 1).type == TOKEN_OF)
            return parse_collection_op(p);
    }
    else if (t.type == TOKEN_SIZE_OF  ||
             t.type == TOKEN_SORTED   ||
             t.type == TOKEN_REVERSED ||
             t.type == TOKEN_KEYS_OF  ||
             t.type == TOKEN_CODE_OF  ||
             t.type == TOKEN_CHAR_CODE ||
             t.type == TOKEN_SHA256_OF ||
             t.type == TOKEN_RANDOM_TOKEN) {
        return parse_collection_op(p);
    }

    if (t.type == TOKEN_RANDOM_NUMBER) {
        advance(p);
        ASTNode *node = ast_make_node(NODE_BINARY_OP, t.line, t.col);
        node->as.binary_op.operator = TOKEN_RANDOM_NUMBER;
        node->as.binary_op.left = parse_additive(p);
        expect(p, TOKEN_TO, "Expected 'to' in 'random number from A to B'");
        node->as.binary_op.right = parse_additive(p);
        return node;
    }

    if (t.type == TOKEN_READ_FILE_PH || t.type == TOKEN_READ_KW) {
        advance(p);
        if (t.type == TOKEN_READ_KW) match(p, TOKEN_FILE_KW);
        ASTNode *node = ast_make_node(NODE_READ_FILE, t.line, t.col);
        node->as.file_op.path_expr = parse_primary(p);
        return node;
    }

    if (t.type == TOKEN_FILE_EXISTS_PH) {
        advance(p);
        ASTNode *node = ast_make_node(NODE_FILE_EXISTS, t.line, t.col);
        node->as.file_op.path_expr = parse_primary(p);
        return node;
    }

    if (t.type == TOKEN_RUN_KW) {
        advance(p);
        ASTNode *node = ast_make_node(NODE_RUN_CMD, t.line, t.col);
        node->as.run_cmd.cmd_expr = parse_primary(p);
        return node;
    }

    if (t.type == TOKEN_GET_ENV_PH) {
        advance(p);
        ASTNode *node = ast_make_node(NODE_GET_ENV, t.line, t.col);
        node->as.get_env.var_expr = parse_primary(p);
        return node;
    }

    if (t.type == TOKEN_NOW_KW) {
        advance(p);
        return ast_make_node(NODE_NOW, t.line, t.col);
    }

    if (t.type == TOKEN_ERROR_MSG_PH) {
        advance(p);
        return ast_make_node(NODE_ERROR_MSG, t.line, t.col);
    }

    if (t.type == TOKEN_PARSE_JSON_PH || t.type == TOKEN_TO_JSON_PH) {
        advance(p);
        ASTNode *n = ast_make_node(
            t.type == TOKEN_PARSE_JSON_PH ? NODE_PARSE_JSON : NODE_TO_JSON,
            t.line, t.col);
        n->as.json_convert.value_expr = parse_unary(p);
        return n;
    }

    if (t.type == TOKEN_SEND_JSON_PH) {
        advance(p);
        ASTNode *node = ast_make_node(NODE_SEND_SOCKET, t.line, t.col);
        node->as.send_socket.json_expr = parse_primary(p);
        match(p, TOKEN_TO);
        match(p, TOKEN_SOCKET_KW);
        node->as.send_socket.path_expr = parse_primary(p);
        return node;
    }

    if (t.type == TOKEN_POST_JSON_PH) {
        advance(p);
        ASTNode *node = ast_make_node(NODE_POST_HTTP, t.line, t.col);
        node->as.post_http.json_expr = parse_primary(p);
        match(p, TOKEN_TO);
        node->as.post_http.url_expr = parse_primary(p);
        return node;
    }

    if (t.type == TOKEN_FIELD) {
        advance(p);
        ASTNode *node = ast_make_node(NODE_JSON_FIELD, t.line, t.col);
        node->as.json_field.key_expr  = parse_primary(p);
        match(p, TOKEN_FROM);
        match(p, TOKEN_JSON_KW);
        node->as.json_field.json_expr = parse_primary(p);
        return node;
    }

    if (is_identifier_word(t.type) || is_postfix_op(t.type)) {
        char name[MAX_TOKEN_LEN] = {0};
        collect_name(p, name, MAX_TOKEN_LEN);
        ASTNode *node = ast_make_node(NODE_IDENTIFIER, t.line, t.col);
        AST_SET_STR(node->as.identifier.name, name);
        return node;
    }

    return ast_make_node(NODE_NULL, t.line, t.col);
}

ASTNode *parse_list_literal(Parser *p) {
    Token t = advance(p);
    ASTNode *node = ast_make_node(NODE_LIST, t.line, t.col);
    nodelist_init(&node->as.list.elements);

    while (!check(p, TOKEN_RBRACKET) && !check(p, TOKEN_EOF)) {
        ASTNode *elem = parse_expression(p);
        if (elem) nodelist_push(&node->as.list.elements, elem);
        if (!match(p, TOKEN_COMMA)) break;
    }

    expect(p, TOKEN_RBRACKET, "Expected ']' to close list");
    return node;
}

ASTNode *parse_html_element(Parser *p) {
    Token t = advance(p);
    ASTNode *node = ast_make_node(NODE_HTML_ELEMENT, t.line, t.col);

    AST_SET_STR(node->as.html_element.tag, t.value);
    node->as.html_element.id_attr[0] = '\0';
    node->as.html_element.class_attr[0] = '\0';
    node->as.html_element.src_attr[0] = '\0';
    node->as.html_element.type_attr[0] = '\0';
    node->as.html_element.onclick_attr[0] = '\0';
    node->as.html_element.placeholder_attr[0] = '\0';
    node->as.html_element.value_attr[0] = '\0';
    node->as.html_element.text_content = NULL;
    nodelist_init(&node->as.html_element.children);
    node->as.html_element.is_self_closing = 0;

    if (t.type == TOKEN_IMAGE_KW || t.type == TOKEN_INPUT_KW) {
        node->as.html_element.is_self_closing = 1;
    }

    while (!check(p, TOKEN_TERMINATOR) && !check(p, TOKEN_EOF) && !check(p, TOKEN_NEWLINE)) {
        if (match(p, TOKEN_ID_KW)) {
            Token str = expect(p, TOKEN_STRING, "Expected string after 'id'");
            AST_SET_STR(node->as.html_element.id_attr, str.value);
        } else if (match(p, TOKEN_CLASS_KW)) {
            Token str = expect(p, TOKEN_STRING, "Expected string after 'class'");
            AST_SET_STR(node->as.html_element.class_attr, str.value);
        } else if (match(p, TOKEN_SRC_KW)) {
            Token str = expect(p, TOKEN_STRING, "Expected string after 'src'");
            AST_SET_STR(node->as.html_element.src_attr, str.value);
        } else if (match(p, TOKEN_TYPE_KW)) {
            Token str = expect(p, TOKEN_STRING, "Expected string after 'type'");
            AST_SET_STR(node->as.html_element.type_attr, str.value);
        } else if (match(p, TOKEN_ONCLICK_KW)) {
            Token str = expect(p, TOKEN_STRING, "Expected string after 'onclick'");
            AST_SET_STR(node->as.html_element.onclick_attr, str.value);
        } else if (match(p, TOKEN_PLACEHOLDER_KW)) {
            Token str = expect(p, TOKEN_STRING, "Expected string after 'placeholder'");
            AST_SET_STR(node->as.html_element.placeholder_attr, str.value);
        } else if (match(p, TOKEN_VALUE_KW)) {
            Token str = expect(p, TOKEN_STRING, "Expected string after 'value'");
            AST_SET_STR(node->as.html_element.value_attr, str.value);
        } else if (check(p, TOKEN_STRING) || check(p, TOKEN_IDENTIFIER) || check(p, TOKEN_NUMBER)) {
            node->as.html_element.text_content = parse_expression(p);
        } else {
            break;
        }
    }

    if (!node->as.html_element.is_self_closing) {
        parse_block(p, &node->as.html_element.children);
    } else {
        skip_newlines(p);
        match(p, TOKEN_TERMINATOR);
    }

    return node;
}

ASTNode *parse_use_design(Parser *p) {
    Token t = advance(p);
    ASTNode *node = ast_make_node(NODE_USE_DESIGN, t.line, t.col);
    node->as.use_design.name[0] = '\0';
    node->as.use_design.primary[0] = '\0';
    node->as.use_design.secondary[0] = '\0';
    node->as.use_design.background[0] = '\0';
    node->as.use_design.headline_font[0] = '\0';
    node->as.use_design.body_font[0] = '\0';
    node->as.use_design.icon_set[0] = '\0';

    if (check(p, TOKEN_STRING))
        AST_SET_STR(node->as.use_design.name, advance(p).value);

    skip_newlines(p);
    while (!check(p, TOKEN_TERMINATOR) && !check(p, TOKEN_EOF)) {
        skip_newlines(p);
        if (check(p, TOKEN_TERMINATOR) || check(p, TOKEN_EOF)) break;
        if (current(p).type == TOKEN_WITH_COLOR) {
            advance(p);
            Token val = expect(p, TOKEN_STRING, "Expected color string");
            if (strlen(node->as.use_design.primary) == 0)
                AST_SET_STR(node->as.use_design.primary, val.value);
            else
                AST_SET_STR(node->as.use_design.secondary, val.value);
        } else if (current(p).type == TOKEN_BACKGROUND_KW) {
            advance(p);
            Token val = expect(p, TOKEN_STRING, "Expected background color");
            AST_SET_STR(node->as.use_design.background, val.value);
        } else if (strcmp(current(p).value, "fonts") == 0 || strcmp(current(p).value, "headline") == 0) {
            advance(p);
            match(p, TOKEN_HEADING_OUT);
            if (check(p, TOKEN_STRING))
                AST_SET_STR(node->as.use_design.headline_font, advance(p).value);
            match(p, TOKEN_MAKE_REPORT);
            if (check(p, TOKEN_STRING))
                AST_SET_STR(node->as.use_design.body_font, advance(p).value);
        } else if (current(p).type == TOKEN_ICONS_KW) {
            advance(p);
            if (check(p, TOKEN_STRING))
                AST_SET_STR(node->as.use_design.icon_set, advance(p).value);
        } else {
            advance(p);
        }
        skip_newlines(p);
    }
    match(p, TOKEN_TERMINATOR);
    return node;
}

ASTNode *parse_make_navbar(Parser *p) {
    Token t = advance(p);
    ASTNode *node = ast_make_node(NODE_MAKE_NAVBAR, t.line, t.col);
    node->as.make_navbar.brand[0] = '\0';
    node->as.make_navbar.style[0] = '\0';
    nodelist_init(&node->as.make_navbar.children);

    match(p, TOKEN_BRANDED); match(p, TOKEN_AS);
    if (check(p, TOKEN_STRING))
        AST_SET_STR(node->as.make_navbar.brand, advance(p).value);
    match(p, TOKEN_STYLED); match(p, TOKEN_AS);
    if (check(p, TOKEN_STRING))
        AST_SET_STR(node->as.make_navbar.style, advance(p).value);

    parse_block(p, &node->as.make_navbar.children);
    return node;
}

ASTNode *parse_nav_link(Parser *p) {
    Token t = advance(p);
    ASTNode *node = ast_make_node(NODE_NAV_LINK, t.line, t.col);
    node->as.nav_link.label[0] = '\0';
    node->as.nav_link.href[0] = '\0';
    if (check(p, TOKEN_STRING))
        AST_SET_STR(node->as.nav_link.label, advance(p).value);
    match(p, TOKEN_AT_KW);
    if (check(p, TOKEN_STRING))
        AST_SET_STR(node->as.nav_link.href, advance(p).value);
    return node;
}

ASTNode *parse_action_btn(Parser *p) {
    Token t = advance(p);
    ASTNode *node = ast_make_node(NODE_ACTION_BTN, t.line, t.col);
    node->as.action_btn.label[0] = '\0';
    node->as.action_btn.modal_id[0] = '\0';
    if (check(p, TOKEN_STRING))
        AST_SET_STR(node->as.action_btn.label, advance(p).value);
    if (check(p, TOKEN_ON_CLICK)) {
        advance(p);
        if (check(p, TOKEN_OPEN_MODAL)) {
            advance(p);
            if (check(p, TOKEN_STRING))
                AST_SET_STR(node->as.action_btn.modal_id, advance(p).value);
        }
    }
    return node;
}

ASTNode *parse_make_hero(Parser *p) {
    Token t = advance(p);
    ASTNode *node = ast_make_node(NODE_MAKE_HERO, t.line, t.col);
    node->as.make_hero.title[0] = '\0';
    node->as.make_hero.accented[0] = '\0';
    node->as.make_hero.subtitle[0] = '\0';
    node->as.make_hero.bg_image[0] = '\0';
    node->as.make_hero.bg_dim = 40;
    node->as.make_hero.overlay_card = NULL;

    match(p, TOKEN_TITLED);
    if (check(p, TOKEN_STRING))
        AST_SET_STR(node->as.make_hero.title, advance(p).value);
    if (check(p, TOKEN_ACCENTED)) {
        advance(p);
        if (check(p, TOKEN_STRING))
            AST_SET_STR(node->as.make_hero.accented, advance(p).value);
    }
    if (check(p, TOKEN_SUBHEADING)) {
        advance(p);
        if (check(p, TOKEN_STRING))
            AST_SET_STR(node->as.make_hero.subtitle, advance(p).value);
    }

    skip_newlines(p);
    while (!check(p, TOKEN_TERMINATOR) && !check(p, TOKEN_EOF)) {
        skip_newlines(p);
        if (check(p, TOKEN_TERMINATOR) || check(p, TOKEN_EOF)) break;
        if (check(p, TOKEN_BACKGROUND_KW)) {
            advance(p);
            advance(p);
            if (check(p, TOKEN_STRING))
                AST_SET_STR(node->as.make_hero.bg_image, advance(p).value);
            if (check(p, TOKEN_DIMMED)) {
                advance(p); match(p, TOKEN_TO);
                if (check(p, TOKEN_NUMBER))
                    node->as.make_hero.bg_dim = (int)atof(advance(p).value);
            }
        } else if (check(p, TOKEN_OVERLAY_CARD)) {
            node->as.make_hero.overlay_card = parse_overlay_card(p);
        } else {
            advance(p);
        }
        skip_newlines(p);
    }
    match(p, TOKEN_TERMINATOR);
    return node;
}

ASTNode *parse_overlay_card(Parser *p) {
    Token t = advance(p);
    ASTNode *node = ast_make_node(NODE_OVERLAY_CARD, t.line, t.col);
    node->as.overlay_card.title[0] = '\0';
    nodelist_init(&node->as.overlay_card.children);
    if (check(p, TOKEN_STRING))
        AST_SET_STR(node->as.overlay_card.title, advance(p).value);
    parse_block(p, &node->as.overlay_card.children);
    return node;
}

ASTNode *parse_guest_picker(Parser *p) {
    Token t = advance(p);
    ASTNode *node = ast_make_node(NODE_GUEST_PICKER, t.line, t.col);
    node->as.guest_picker.default_count = 2;
    if (strcmp(current(p).value, "default") == 0) advance(p);
    if (check(p, TOKEN_NUMBER))
        node->as.guest_picker.default_count = (int)atof(advance(p).value);
    return node;
}

ASTNode *parse_date_picker(Parser *p) {
    Token t = advance(p);
    ASTNode *node = ast_make_node(NODE_DATE_PICKER, t.line, t.col);
    node->as.date_picker.label[0] = '\0';
    match(p, TOKEN_LABEL_OUT);
    if (check(p, TOKEN_STRING))
        AST_SET_STR(node->as.date_picker.label, advance(p).value);
    return node;
}

ASTNode *parse_content_section(Parser *p) {
    Token t = advance(p);
    ASTNode *node = ast_make_node(NODE_CONTENT_SECTION, t.line, t.col);
    node->as.content_section.id[0] = '\0';
    node->as.content_section.left_col = NULL;
    node->as.content_section.right_col = NULL;

    match(p, TOKEN_ID_KW);
    if (check(p, TOKEN_STRING))
        AST_SET_STR(node->as.content_section.id, advance(p).value);

    skip_newlines(p);
    while (!check(p, TOKEN_TERMINATOR) && !check(p, TOKEN_EOF)) {
        skip_newlines(p);
        if (check(p, TOKEN_TERMINATOR) || check(p, TOKEN_EOF)) break;
        if (check(p, TOKEN_LEFT_COL))
            node->as.content_section.left_col = parse_column(p, 0);
        else if (check(p, TOKEN_RIGHT_COL))
            node->as.content_section.right_col = parse_column(p, 1);
        else
            advance(p);
        skip_newlines(p);
    }
    match(p, TOKEN_TERMINATOR);
    return node;
}

ASTNode *parse_column(Parser *p, int is_right) {
    Token t = advance(p);
    ASTNode *node = ast_make_node(NODE_COLUMN, t.line, t.col);
    node->as.column.is_right = is_right;
    nodelist_init(&node->as.column.children);
    parse_block(p, &node->as.column.children);
    return node;
}

ASTNode *parse_eyebrow(Parser *p) {
    Token t = advance(p);
    ASTNode *node = ast_make_node(NODE_EYEBROW, t.line, t.col);
    node->as.eyebrow.text[0] = '\0';
    if (check(p, TOKEN_STRING))
        AST_SET_STR(node->as.eyebrow.text, advance(p).value);
    return node;
}

ASTNode *parse_item_list(Parser *p) {
    Token t = advance(p);
    ASTNode *node = ast_make_node(NODE_ITEM_LIST, t.line, t.col);
    nodelist_init(&node->as.item_list.entries);
    parse_block(p, &node->as.item_list.entries);
    return node;
}

ASTNode *parse_list_entry(Parser *p) {
    Token t = advance(p);
    ASTNode *node = ast_make_node(NODE_LIST_ENTRY, t.line, t.col);
    node->as.list_entry.label[0] = '\0';
    node->as.list_entry.number = 0;
    node->as.list_entry.detail[0] = '\0';
    node->as.list_entry.offset = 0;

    if (check(p, TOKEN_STRING))
        AST_SET_STR(node->as.list_entry.label, advance(p).value);
    if (check(p, TOKEN_NUMBERED)) {
        advance(p);
        if (check(p, TOKEN_NUMBER))
            node->as.list_entry.number = (int)atof(advance(p).value);
    }
    if (check(p, TOKEN_DETAIL_KW)) {
        advance(p);
        if (check(p, TOKEN_STRING))
            AST_SET_STR(node->as.list_entry.detail, advance(p).value);
    }
    if (check(p, TOKEN_OFFSET))
        { advance(p); node->as.list_entry.offset = 1; }
    return node;
}

ASTNode *parse_feature_image(Parser *p) {
    Token t = advance(p);
    ASTNode *node = ast_make_node(NODE_FEATURE_IMAGE, t.line, t.col);
    node->as.feature_image.src[0] = '\0';
    node->as.feature_image.aspect[0] = '\0';
    node->as.feature_image.hover_mode[0] = '\0';

    if (check(p, TOKEN_STRING))
        AST_SET_STR(node->as.feature_image.src, advance(p).value);
    if (check(p, TOKEN_ASPECT)) {
        advance(p);
        if (check(p, TOKEN_STRING))
            AST_SET_STR(node->as.feature_image.aspect, advance(p).value);
    }
    if (strcmp(current(p).value, "hover") == 0) {
        advance(p);
        if (check(p, TOKEN_STRING))
            AST_SET_STR(node->as.feature_image.hover_mode, advance(p).value);
    }
    return node;
}

ASTNode *parse_price_badge(Parser *p) {
    Token t = advance(p);
    ASTNode *node = ast_make_node(NODE_PRICE_BADGE, t.line, t.col);
    node->as.price_badge.price[0] = '\0';
    node->as.price_badge.label[0] = '\0';
    if (check(p, TOKEN_STRING))
        AST_SET_STR(node->as.price_badge.price, advance(p).value);
    match(p, TOKEN_LABEL_OUT);
    if (check(p, TOKEN_STRING))
        AST_SET_STR(node->as.price_badge.label, advance(p).value);
    return node;
}

ASTNode *parse_make_gallery(Parser *p) {
    Token t = advance(p);
    ASTNode *node = ast_make_node(NODE_MAKE_GALLERY, t.line, t.col);
    node->as.make_gallery.title[0] = '\0';
    node->as.make_gallery.subtitle[0] = '\0';
    node->as.make_gallery.columns = 4;
    nodelist_init(&node->as.make_gallery.images);

    match(p, TOKEN_TITLED);
    if (check(p, TOKEN_STRING))
        AST_SET_STR(node->as.make_gallery.title, advance(p).value);
    if (check(p, TOKEN_SUBHEADING)) {
        advance(p);
        if (check(p, TOKEN_STRING))
            AST_SET_STR(node->as.make_gallery.subtitle, advance(p).value);
    }
    if (check(p, TOKEN_COLUMNS)) {
        advance(p);
        if (check(p, TOKEN_NUMBER))
            node->as.make_gallery.columns = (int)atof(advance(p).value);
    }
    parse_block(p, &node->as.make_gallery.images);
    return node;
}

ASTNode *parse_gallery_image(Parser *p) {
    Token t = advance(p);
    ASTNode *node = ast_make_node(NODE_GALLERY_IMAGE, t.line, t.col);
    node->as.gallery_image.src[0] = '\0';
    node->as.gallery_image.offset_top = 0;
    if (check(p, TOKEN_STRING))
        AST_SET_STR(node->as.gallery_image.src, advance(p).value);
    if (check(p, TOKEN_OFFSET)) {
        advance(p);
        if (strcmp(current(p).value, "top") == 0) advance(p);
        if (check(p, TOKEN_NUMBER))
            node->as.gallery_image.offset_top = (int)atof(advance(p).value);
    }
    return node;
}

static void parse_action_after(Parser *p, ASTNode *node) {
    advance(p);
    char words[256] = {0};
    while (!check(p, TOKEN_NEWLINE) && !check(p, TOKEN_TERMINATOR) &&
           !check(p, TOKEN_EOF)) {
        Token w = advance(p);
        if (w.type == TOKEN_STRING) {
            AST_SET_STR(node->as.action_form.go_to, w.value);
            continue;
        }
        strncat(words, " ", sizeof words - strlen(words) - 1);
        strncat(words, w.value, sizeof words - strlen(words) - 1);
    }
    if (strcmp(words, " show the reply") == 0 || strcmp(words, " show reply") == 0)
        node->as.action_form.after = ACTION_AFTER_SHOW;
    else if (strcmp(words, " refresh the page") == 0 || strcmp(words, " refresh page") == 0)
        node->as.action_form.after = ACTION_AFTER_REFRESH;
    else if (strcmp(words, " go to") == 0 && node->as.action_form.go_to[0])
        node->as.action_form.after = ACTION_AFTER_GO;
    else
        parser_error(p, "after an action write one of: then show the reply / "
                        "then refresh the page / then go to \"page.html\"");
}

static ASTNode *parse_action_form(Parser *p, Token t, const char *label) {
    ASTNode *node = ast_make_node(NODE_ACTION_FORM, t.line, t.col);
    nodelist_init(&node->as.action_form.inputs);
    AST_SET_STR(node->as.action_form.label, label);
    advance(p);
    char name[MAX_TOKEN_LEN] = {0};
    collect_name(p, name, MAX_TOKEN_LEN);
    if (!name[0]) { parser_error(p, "expected an activity name after 'for'"); return node; }
    AST_SET_STR(node->as.action_form.activity, name);
    if (check(p, TOKEN_THEN)) parse_action_after(p, node);

    int open_line = t.line;
    skip_newlines(p);
    while (!check(p, TOKEN_TERMINATOR) && !check(p, TOKEN_EOF)) {
        if (check(p, TOKEN_INPUT_KW)) {
            Token it = advance(p);
            ASTNode *in = ast_make_node(NODE_ACTION_INPUT, it.line, it.col);
            if (!check(p, TOKEN_STRING)) {
                parser_error(p, "write the parameter it fills: input \"qty\" labeled \"Quantity\"");
                ast_free(in);
                return node;
            }
            AST_SET_STR(in->as.action_input.name, advance(p).value);
            AST_SET_STR(in->as.action_input.label, in->as.action_input.name);
            if (match(p, TOKEN_LABELED) && check(p, TOKEN_STRING))
                AST_SET_STR(in->as.action_input.label, advance(p).value);
            nodelist_push(&node->as.action_form.inputs, in);
        } else if (check(p, TOKEN_THEN)) {
            parse_action_after(p, node);
        } else {
            parser_error(p, "a form holds lines like input \"qty\" labeled \"Quantity\" "
                            "and one 'then …' line");
            return node;
        }
        if (p->panic_mode) return node;
        skip_newlines(p);
    }
    close_block(p, open_line);
    node->end_line = p->tokens[p->current > 0 ? p->current - 1 : 0].line;
    return node;
}

ASTNode *parse_make_button(Parser *p) {
    Token t = advance(p);
    ASTNode *node = ast_make_node(NODE_ACTION_BUTTON, t.line, t.col);
    nodelist_init(&node->as.action_form.inputs);
    if (!check(p, TOKEN_STRING)) {
        parser_error(p, "write the button text: make button \"Refresh\" for revenue");
        return node;
    }
    AST_SET_STR(node->as.action_form.label, advance(p).value);
    if (!match(p, TOKEN_FOR)) {
        parser_error(p, "say which activity it calls: make button \"Refresh\" for revenue");
        return node;
    }
    char name[MAX_TOKEN_LEN] = {0};
    collect_name(p, name, MAX_TOKEN_LEN);
    if (!name[0]) { parser_error(p, "expected an activity name after 'for'"); return node; }
    AST_SET_STR(node->as.action_form.activity, name);
    if (check(p, TOKEN_THEN)) parse_action_after(p, node);
    return node;
}

ASTNode *parse_make_form(Parser *p) {
    Token t = advance(p);
    if (check(p, TOKEN_FOR))
        return parse_action_form(p, t, "Send");
    if (check(p, TOKEN_STRING) && peek_at(p, 1).type == TOKEN_FOR) {
        Token lbl = advance(p);
        return parse_action_form(p, t, lbl.value);
    }
    ASTNode *node = ast_make_node(NODE_MAKE_FORM, t.line, t.col);
    node->as.make_form.title[0] = '\0';
    node->as.make_form.eyebrow[0] = '\0';
    nodelist_init(&node->as.make_form.children);

    if (check(p, TOKEN_STRING))
        AST_SET_STR(node->as.make_form.title, advance(p).value);
    if (check(p, TOKEN_EYEBROW)) {
        advance(p);
        if (check(p, TOKEN_STRING))
            AST_SET_STR(node->as.make_form.eyebrow, advance(p).value);
    }
    parse_block(p, &node->as.make_form.children);
    return node;
}

ASTNode *parse_form_field(Parser *p) {
    Token t = advance(p);
    ASTNode *node = ast_make_node(NODE_FORM_FIELD, t.line, t.col);
    node->as.form_field.field_type[0] = '\0';
    node->as.form_field.field_label[0] = '\0';
    node->as.form_field.placeholder[0] = '\0';
    if (!check(p, TOKEN_EOF) && !check(p, TOKEN_NEWLINE))
        AST_SET_STR(node->as.form_field.field_type, advance(p).value);
    if (check(p, TOKEN_STRING))
        AST_SET_STR(node->as.form_field.field_label, advance(p).value);
    if (check(p, TOKEN_PLACEHOLDER_KW) || check(p, TOKEN_PLACEHOLDER_EPF)) {
        advance(p);
        if (check(p, TOKEN_STRING))
            AST_SET_STR(node->as.form_field.placeholder, advance(p).value);
    }
    return node;
}

ASTNode *parse_submit_btn(Parser *p) {
    Token t = advance(p);
    ASTNode *node = ast_make_node(NODE_SUBMIT_BTN, t.line, t.col);
    node->as.submit_btn.label[0] = '\0';
    node->as.submit_btn.modal_id[0] = '\0';
    if (check(p, TOKEN_STRING))
        AST_SET_STR(node->as.submit_btn.label, advance(p).value);
    if (check(p, TOKEN_ON_CLICK)) {
        advance(p);
        if (check(p, TOKEN_OPEN_MODAL)) {
            advance(p);
            if (check(p, TOKEN_STRING))
                AST_SET_STR(node->as.submit_btn.modal_id, advance(p).value);
        }
    }
    return node;
}

ASTNode *parse_make_modal(Parser *p) {
    Token t = advance(p);
    ASTNode *node = ast_make_node(NODE_MAKE_MODAL, t.line, t.col);
    node->as.make_modal.modal_id[0] = '\0';
    node->as.make_modal.icon[0] = '\0';
    node->as.make_modal.color[0] = '\0';
    node->as.make_modal.heading[0] = '\0';
    node->as.make_modal.body_text[0] = '\0';
    node->as.make_modal.dismiss_label[0] = '\0';

    match(p, TOKEN_ID_KW);
    if (check(p, TOKEN_STRING))
        AST_SET_STR(node->as.make_modal.modal_id, advance(p).value);
    if (strcmp(current(p).value, "icon") == 0) {
        advance(p);
        if (check(p, TOKEN_STRING))
            AST_SET_STR(node->as.make_modal.icon, advance(p).value);
    }
    if (check(p, TOKEN_WITH_COLOR)) {
        advance(p);
        if (check(p, TOKEN_STRING))
            AST_SET_STR(node->as.make_modal.color, advance(p).value);
    }

    skip_newlines(p);
    while (!check(p, TOKEN_TERMINATOR) && !check(p, TOKEN_EOF)) {
        skip_newlines(p);
        if (check(p, TOKEN_TERMINATOR) || check(p, TOKEN_EOF)) break;
        if (check(p, TOKEN_HEADING_OUT)) {
            advance(p);
            if (check(p, TOKEN_STRING))
                AST_SET_STR(node->as.make_modal.heading, advance(p).value);
        } else if (check(p, TOKEN_PARAGRAPH)) {
            advance(p);
            if (check(p, TOKEN_STRING))
                AST_SET_STR(node->as.make_modal.body_text, advance(p).value);
        } else if (check(p, TOKEN_DISMISS_BTN)) {
            advance(p);
            if (check(p, TOKEN_STRING))
                AST_SET_STR(node->as.make_modal.dismiss_label, advance(p).value);
        } else {
            advance(p);
        }
        skip_newlines(p);
    }
    match(p, TOKEN_TERMINATOR);
    return node;
}

ASTNode *parse_dismiss_btn(Parser *p) {
    Token t = advance(p);
    ASTNode *node = ast_make_node(NODE_DISMISS_BTN, t.line, t.col);
    node->as.make_modal.dismiss_label[0] = '\0';
    if (check(p, TOKEN_STRING))
        AST_SET_STR(node->as.make_modal.dismiss_label, advance(p).value);
    return node;
}

ASTNode *parse_make_footer(Parser *p) {
    Token t = advance(p);
    ASTNode *node = ast_make_node(NODE_MAKE_FOOTER, t.line, t.col);
    node->as.make_footer.brand[0] = '\0';
    node->as.make_footer.tagline[0] = '\0';
    node->as.make_footer.contact_heading[0] = '\0';
    node->as.make_footer.address[0] = '\0';
    node->as.make_footer.copyright[0] = '\0';

    match(p, TOKEN_BRANDED); match(p, TOKEN_AS);
    if (check(p, TOKEN_STRING))
        AST_SET_STR(node->as.make_footer.brand, advance(p).value);

    skip_newlines(p);
    while (!check(p, TOKEN_TERMINATOR) && !check(p, TOKEN_EOF)) {
        skip_newlines(p);
        if (check(p, TOKEN_TERMINATOR) || check(p, TOKEN_EOF)) break;
        if (check(p, TOKEN_TAGLINE)) {
            advance(p);
            if (check(p, TOKEN_STRING))
                AST_SET_STR(node->as.make_footer.tagline, advance(p).value);
        } else if (strcmp(current(p).value, "contact") == 0) {
            advance(p);
            match(p, TOKEN_HEADING_OUT);
            if (check(p, TOKEN_STRING))
                AST_SET_STR(node->as.make_footer.contact_heading, advance(p).value);
        } else if (check(p, TOKEN_ADDRESS_KW)) {
            advance(p);
            if (check(p, TOKEN_STRING))
                AST_SET_STR(node->as.make_footer.address, advance(p).value);
        } else if (check(p, TOKEN_COPYRIGHT)) {
            advance(p);
            if (check(p, TOKEN_STRING))
                AST_SET_STR(node->as.make_footer.copyright, advance(p).value);
        } else {
            advance(p);
        }
        skip_newlines(p);
    }
    match(p, TOKEN_TERMINATOR);
    return node;
}

