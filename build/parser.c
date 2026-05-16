#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "parser.h"

/* ─────────────────────────────────────────────
   EPL Parser — parser.c
   Recursive descent parser.
   Reads the token stream and builds an AST.
   ───────────────────────────────────────────── */


/* ═════════════════════════════════════════════
   INTERNAL NAVIGATION HELPERS
   ═════════════════════════════════════════════ */

/* Current token without consuming */
static Token current(Parser *p) {
    return p->tokens[p->current];
}

/* Peek N tokens ahead without consuming */
static Token peek_at(Parser *p, int offset) {
    int idx = p->current + offset;
    if (idx >= p->count) return p->tokens[p->count - 1]; /* EOF */
    return p->tokens[idx];
}

/* Consume and return current token */
static Token advance(Parser *p) {
    Token t = p->tokens[p->current];
    if (p->current < p->count - 1) p->current++;
    return t;
}

/* Check if current token matches type */
static int check(Parser *p, TokenType type) {
    return current(p).type == type;
}

/* Consume token if it matches, return 1 if matched */
static int match(Parser *p, TokenType type) {
    if (check(p, type)) { advance(p); return 1; }
    return 0;
}

/* Skip all newline tokens */
static void skip_newlines(Parser *p) {
    while (check(p, TOKEN_NEWLINE)) advance(p);
}

/* Error reporting */
static void parser_error(Parser *p, const char *msg) {
    if (p->panic_mode) return;
    Token t = current(p);
    fprintf(stderr, "[EPL Error] Line %d, Col %d: %s (got \"%s\")\n",
        t.line, t.col, msg, t.value);
    p->had_error  = 1;
    p->panic_mode = 1;
}

/* Expect a specific token type or error */
static Token expect(Parser *p, TokenType type, const char *msg) {
    if (check(p, type)) return advance(p);
    parser_error(p, msg);
    return current(p);
}

/* Sync after error — skip to next safe point */
static void synchronize(Parser *p) {
    p->panic_mode = 0;
    while (!check(p, TOKEN_EOF)) {
        if (check(p, TOKEN_NEWLINE)) { advance(p); return; }
        if (check(p, TOKEN_TERMINATOR)) { advance(p); return; }
        advance(p);
    }
}

/* Helper to check if a token is a postfix operator (v1.2) */
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

/* Helper to check if a token can be part of an identifier name */
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
            type == TOKEN_REPLACED);
}

/* Collect multi-word identifier name into buf */
/* e.g. "clear screen", "get number" */
static void collect_name(Parser *p, char *buf, int maxlen) {
    buf[0] = '\0';
    int first = 1;
    while (1) {
        TokenType type = current(p).type;
        int is_valid = is_identifier_word(type) || (first && is_postfix_op(type));
        
        /* Special case for v1.2: 'rounded' is an identifier unless followed by 'to' */
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


/* ═════════════════════════════════════════════
   PARSER INIT
   ═════════════════════════════════════════════ */
void parser_init(Parser *p, Token *tokens, int count) {
    p->tokens    = tokens;
    p->count     = count;
    p->current   = 0;
    p->had_error = 0;
    p->panic_mode = 0;
}


/* ═════════════════════════════════════════════
   BLOCK PARSER
   Reads statements until ! or EOF.
   ═════════════════════════════════════════════ */
void parse_block(Parser *p, NodeList *list) {
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
    match(p, TOKEN_TERMINATOR);   /* consume the ! */
}


/* ═════════════════════════════════════════════
   MAIN PARSE ENTRY
   ═════════════════════════════════════════════ */
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


/* ═════════════════════════════════════════════
   STATEMENT DISPATCHER
   ═════════════════════════════════════════════ */
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
        case TOKEN_RETURN:      return parse_return(p);
        case TOKEN_CHECK:       return parse_check(p);
        case TOKEN_DESCRIBE:    return parse_describe(p);
        case TOKEN_PROVIDE:     return parse_provide(p);
        case TOKEN_ATTEMPT:     return parse_attempt(p);
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
        case TOKEN_IMAGE_KW:    return parse_gallery_image(p);
        /* v2.0 EPF Components */
        case TOKEN_USE_DESIGN:      return parse_use_design(p);
        case TOKEN_MAKE_NAVBAR:     return parse_make_navbar(p);
        case TOKEN_MAKE_HERO:       return parse_make_hero(p);
        case TOKEN_CONTENT_SECTION: return parse_content_section(p);
        case TOKEN_MAKE_GALLERY:    return parse_make_gallery(p);
        case TOKEN_MAKE_FORM:       return parse_make_form(p);
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
            parser_error(p, "Unexpected token at start of statement");
            advance(p);
            return NULL;
    }
}


/* ═════════════════════════════════════════════
   COLLECTION OPERATION  (v1.1)
   Parses expressions like:
     average of prices
     total of scores
     maximum of grades
     minimum of temps
     length of items
     sorted prices
     reversed items
   Called from parse_primary when a collection
   keyword is encountered.
   ═════════════════════════════════════════════ */
ASTNode *parse_collection_op(Parser *p) {
    Token t = current(p);
    CollectionOp op;
    int has_of = 1;   /* most ops use "of" — sorted/reversed don't */

    switch (t.type) {
        case TOKEN_AVERAGE:  op = COLLOP_AVERAGE;  break;
        case TOKEN_TOTAL:    op = COLLOP_TOTAL;    break;
        case TOKEN_MAXIMUM:  op = COLLOP_MAXIMUM;  break;
        case TOKEN_MINIMUM:  op = COLLOP_MINIMUM;  break;
        case TOKEN_LENGTH:   op = COLLOP_LENGTH;   break;
        case TOKEN_SORTED:   op = COLLOP_SORTED;   has_of = 0; break;
        case TOKEN_REVERSED: op = COLLOP_REVERSED; has_of = 0; break;
        default:
            parser_error(p, "Unknown collection operation");
            return ast_make_node(NODE_NULL, t.line, t.col);
    }

    advance(p);   /* consume the op keyword */

    /* Consume optional 'of' */
    if (has_of) match(p, TOKEN_OF);

    ASTNode *node = ast_make_node(NODE_COLLECTION_OP, t.line, t.col);
    node->as.collection_op.op         = op;
    node->as.collection_op.collection = parse_primary(p);
    return node;
}


/* ═════════════════════════════════════════════
   STRING OPERATION  (v1.2)
   Called from parse_postfix when "in", "trimmed",
   "split", "joined", "starts", "ends", "contains"
   follow a primary expression.
   ═════════════════════════════════════════════ */
ASTNode *parse_string_op(Parser *p, ASTNode *target) {
    Token t = current(p);
    ASTNode *node = ast_make_node(NODE_STRING_OP, t.line, t.col);
    node->as.string_op.target = target;
    node->as.string_op.arg1   = NULL;
    node->as.string_op.arg2   = NULL;

    /* name in uppercase / name in lowercase */
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
            /* "in" used for membership — not a string op, bail */
            ast_free(node);
            return target;
        }
        return node;
    }

    /* name trimmed */
    if (t.type == TOKEN_TRIMMED) {
        advance(p);
        node->as.string_op.op = STROP_TRIMMED;
        return node;
    }

    /* sentence split by " " */
    if (t.type == TOKEN_SPLIT) {
        advance(p);
        match(p, TOKEN_BY);
        node->as.string_op.op   = STROP_SPLIT;
        node->as.string_op.arg1 = parse_primary(p);
        return node;
    }

    /* parts joined by ", " */
    if (t.type == TOKEN_JOINED) {
        advance(p);
        match(p, TOKEN_BY);
        node->as.string_op.op   = STROP_JOINED;
        node->as.string_op.arg1 = parse_primary(p);
        return node;
    }

    /* text with "old" replaced by "new" */
    if (t.type == TOKEN_WITH) {
        advance(p);
        node->as.string_op.op   = STROP_REPLACED;
        node->as.string_op.arg1 = parse_primary(p);   /* search  */
        match(p, TOKEN_REPLACED);
        match(p, TOKEN_BY);
        node->as.string_op.arg2 = parse_primary(p);   /* replace */
        return node;
    }

    /* name starts with "Ph" */
    if (t.type == TOKEN_STARTS) {
        advance(p);
        match(p, TOKEN_WITH);
        node->as.string_op.op   = STROP_STARTS_WITH;
        node->as.string_op.arg1 = parse_primary(p);
        return node;
    }

    /* name ends with "ix" */
    if (t.type == TOKEN_ENDS) {
        advance(p);
        match(p, TOKEN_WITH);
        node->as.string_op.op   = STROP_ENDS_WITH;
        node->as.string_op.arg1 = parse_primary(p);
        return node;
    }

    /* name contains "oen" */
    if (t.type == TOKEN_CONTAINS) {
        advance(p);
        node->as.string_op.op   = STROP_CONTAINS;
        node->as.string_op.arg1 = parse_primary(p);
        return node;
    }

    /* nothing matched — not a string op */
    ast_free(node);
    return target;
}


/* ═════════════════════════════════════════════
   FORMAT OPERATION  (v1.2)
   Called from parse_postfix when "as" follows
   a primary that resolves to a number.
   price as currency
   ratio as percentage
   pi rounded to 2
   big as formatted
   ═════════════════════════════════════════════ */
ASTNode *parse_format_op(Parser *p, ASTNode *target) {
    Token t = current(p);
    ASTNode *node = ast_make_node(NODE_FORMAT_OP, t.line, t.col);
    node->as.format_op.target    = target;
    node->as.format_op.precision = NULL;

    /* X rounded to N */
    if (t.type == TOKEN_ROUNDED) {
        advance(p);
        match(p, TOKEN_TO);
        node->as.format_op.op        = FMTOP_ROUNDED;
        node->as.format_op.precision = parse_primary(p);
        return node;
    }

    /* X as currency / X as percentage / X as formatted */
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
        /* not a format op — back out */
        p->current--;   /* unconsume TOKEN_AS */
    }

    ast_free(node);
    return target;
}


/* ═════════════════════════════════════════════
   SET STATEMENT
   set <name> as <expression>
   set <name> by <activity_call>
   ═════════════════════════════════════════════ */
ASTNode *parse_set(Parser *p) {
    Token t = advance(p);   /* consume 'set' */
    ASTNode *node = ast_make_node(NODE_SET, t.line, t.col);

    /* Collect variable name (may be multi-word) */
    char name[MAX_TOKEN_LEN] = {0};
    int first = 1;
    while (!check(p, TOKEN_AS) && !check(p, TOKEN_BY) && !check(p, TOKEN_EOF) && !check(p, TOKEN_NEWLINE)) {
        if (!first) strncat(name, " ", MAX_TOKEN_LEN - strlen(name) - 1);
        strncat(name, current(p).value, MAX_TOKEN_LEN - strlen(name) - 1);
        advance(p);
        first = 0;
    }
    strncpy(node->as.assignment.name, name, MAX_TOKEN_LEN - 1);

    if (match(p, TOKEN_AS)) {
        node->as.assignment.value = parse_expression(p);
    } else if (match(p, TOKEN_BY)) {
        /* set r by adding n1 and n2
           set msg by greet
           Strategy: collect words into name until we hit
           a string/number/boolean (those are args),
           or 'and'/'from'/'with' (arg separators) */
        ASTNode *call = ast_make_node(NODE_ACTIVITY_CALL, t.line, t.col);
        nodelist_init(&call->as.activity_call.args);

        char act_name[MAX_TOKEN_LEN] = {0};
        int first = 1;

        /* Collect all words that form the activity name */
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
        strncpy(call->as.activity_call.name, act_name, MAX_TOKEN_LEN - 1);

        /* Collect arguments separated by 'and', 'from', 'with' */
        while (!check(p, TOKEN_NEWLINE)    &&
               !check(p, TOKEN_TERMINATOR) &&
               !check(p, TOKEN_EOF)) {
            match(p, TOKEN_AND);
            match(p, TOKEN_FROM);
            match(p, TOKEN_WITH);
            if (check(p, TOKEN_NEWLINE) ||
                check(p, TOKEN_TERMINATOR) ||
                check(p, TOKEN_EOF)) break;
            ASTNode *arg = parse_primary(p);
            if (arg) nodelist_push(&call->as.activity_call.args, arg);
        }

        node->as.assignment.value = call;
    } else {
        parser_error(p, "Expected 'as' or 'by' after variable name in 'set'");
    }

    return node;
}


/* ═════════════════════════════════════════════
   UPDATE STATEMENT
   update <name> as <expression>
   ═════════════════════════════════════════════ */
ASTNode *parse_update(Parser *p) {
    Token t = advance(p);   /* consume 'update' */
    ASTNode *node = ast_make_node(NODE_UPDATE, t.line, t.col);

    char name[MAX_TOKEN_LEN] = {0};
    int first = 1;
    while (!check(p, TOKEN_AS) && !check(p, TOKEN_BY) && !check(p, TOKEN_EOF) && !check(p, TOKEN_NEWLINE)) {
        if (!first) strncat(name, " ", MAX_TOKEN_LEN - strlen(name) - 1);
        strncat(name, current(p).value, MAX_TOKEN_LEN - strlen(name) - 1);
        advance(p);
        first = 0;
    }
    strncpy(node->as.assignment.name, name, MAX_TOKEN_LEN - 1);

    if (match(p, TOKEN_AS)) {
        node->as.assignment.value = parse_expression(p);
    } else if (match(p, TOKEN_BY)) {
        node->as.assignment.value = parse_expression(p);
    } else {
        parser_error(p, "Expected 'as' or 'by' after variable name in 'update'");
    }

    return node;
}


/* ═════════════════════════════════════════════
   SHOW STATEMENT
   show <expression>
   ═════════════════════════════════════════════ */
ASTNode *parse_show(Parser *p) {
    Token t = advance(p);   /* consume 'show' or 'display' */
    ASTNode *node = ast_make_node(NODE_SHOW, t.line, t.col);
    node->as.show.value = parse_expression(p);
    return node;
}


/* ═════════════════════════════════════════════
   IF STATEMENT
   if <condition> then
       <body>
   !
   if <condition> then
       <body>
   otherwise
       <body>
   !
   ═════════════════════════════════════════════ */
ASTNode *parse_if(Parser *p) {
    Token t = advance(p);   /* consume 'if' */
    ASTNode *node = ast_make_node(NODE_IF, t.line, t.col);

    node->as.if_stmt.condition = parse_expression(p);
    expect(p, TOKEN_THEN, "Expected 'then' after if condition");

    nodelist_init(&node->as.if_stmt.then_body);
    nodelist_init(&node->as.if_stmt.else_body);

    skip_newlines(p);

    /* Read then-body until !, otherwise, or EOF */
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

    /* Optional otherwise block */
    if (match(p, TOKEN_OTHERWISE)) {
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

    match(p, TOKEN_TERMINATOR);   /* consume ! */
    node->end_line = current(p).line;   /* v1.5 */
    return node;
}


/* ═════════════════════════════════════════════
   WHILE STATEMENT
   while <condition>
       <body>
   !
   ═════════════════════════════════════════════ */
ASTNode *parse_while(Parser *p) {
    Token t = advance(p);   /* consume 'while' */
    ASTNode *node = ast_make_node(NODE_WHILE, t.line, t.col);
    node->as.while_stmt.condition = parse_expression(p);
    nodelist_init(&node->as.while_stmt.body);
    parse_block(p, &node->as.while_stmt.body);
    node->end_line = current(p).line;   /* v1.5 */
    return node;
}


/* ═════════════════════════════════════════════
   REPEAT STATEMENT
   Handles three variants:
   repeat 5 times [with i]
   repeat until <condition>
   repeat indefinitely
   ═════════════════════════════════════════════ */
ASTNode *parse_repeat(Parser *p) {
    Token t = advance(p);   /* consume 'repeat' */

    /* repeat indefinitely */
    if (match(p, TOKEN_INDEFINITELY)) {
        ASTNode *node = ast_make_node(NODE_REPEAT_INDEFINITELY, t.line, t.col);
        parse_block(p, &node->as.repeat_indefinitely.body);
        node->end_line = current(p).line;   /* v1.5 */
        return node;
    }

    /* repeat until <condition> */
    if (match(p, TOKEN_UNTIL)) {
        ASTNode *node = ast_make_node(NODE_REPEAT_UNTIL, t.line, t.col);
        node->as.repeat_until.condition = parse_expression(p);
        parse_block(p, &node->as.repeat_until.body);
        node->end_line = current(p).line;   /* v1.5 */
        return node;
    }

    /* repeat <count> times [with i] */
    ASTNode *node = ast_make_node(NODE_REPEAT_TIMES, t.line, t.col);
    node->as.repeat_times.count = parse_expression(p);
    node->as.repeat_times.counter[0] = '\0';

    expect(p, TOKEN_TIMES, "Expected 'times' after repeat count");

    /* Optional: with <counter_var> */
    if (match(p, TOKEN_WITH)) {
        if (check(p, TOKEN_IDENTIFIER)) {
            strncpy(node->as.repeat_times.counter,
                current(p).value, MAX_TOKEN_LEN - 1);
            advance(p);
        }
    }

    parse_block(p, &node->as.repeat_times.body);
    node->end_line = current(p).line;   /* v1.5 */
    return node;
}


/* ═════════════════════════════════════════════
   FOR STATEMENT
   Handles:
   for each <var> in <collection>
   for <var> from <n> to <n>
   for <var> from <n> down to <n>
   ═════════════════════════════════════════════ */
ASTNode *parse_for(Parser *p) {
    Token t = advance(p);   /* consume 'for' */

    /* for each <var> in <collection> */
    if (match(p, TOKEN_EACH)) {
        ASTNode *node = ast_make_node(NODE_FOR_EACH, t.line, t.col);
        node->as.for_each.var[0] = '\0';

        if (check(p, TOKEN_IDENTIFIER)) {
            strncpy(node->as.for_each.var,
                current(p).value, MAX_TOKEN_LEN - 1);
            advance(p);
        }

        expect(p, TOKEN_IN, "Expected 'in' after loop variable");
        node->as.for_each.collection = parse_expression(p);
        parse_block(p, &node->as.for_each.body);
        node->end_line = current(p).line;   /* v1.5 */
        return node;
    }

    /* for <var> from <n> to/down to <n> */
    ASTNode *node = ast_make_node(NODE_FOR_RANGE, t.line, t.col);
    node->as.for_range.downward = 0;
    node->as.for_range.var[0]   = '\0';

    if (check(p, TOKEN_IDENTIFIER)) {
        strncpy(node->as.for_range.var,
            current(p).value, MAX_TOKEN_LEN - 1);
        advance(p);
    }

    expect(p, TOKEN_FROM, "Expected 'from' in for range loop");
    node->as.for_range.from = parse_expression(p);

    /* down to */
    if (match(p, TOKEN_DOWN)) {
        node->as.for_range.downward = 1;
    }

    expect(p, TOKEN_TO, "Expected 'to' in for range loop");
    node->as.for_range.to = parse_expression(p);
    parse_block(p, &node->as.for_range.body);
    node->end_line = current(p).line;   /* v1.5 */
    return node;
}


/* ═════════════════════════════════════════════
   ACTIVITY DEFINITION
   to <name>[, <single-line-body>!]
   to <name>
       <body>
   !
   ═════════════════════════════════════════════ */
ASTNode *parse_activity_def(Parser *p) {
    Token t = advance(p);   /* consume 'to' */
    ASTNode *node = ast_make_node(NODE_ACTIVITY_DEF, t.line, t.col);

    /* Collect activity name — keywords like 'add','subtract' are valid names here */
    char name[MAX_TOKEN_LEN] = {0};
    int first = 1;
    while (!check(p, TOKEN_COMMA)      &&
           !check(p, TOKEN_NEWLINE)    &&
           !check(p, TOKEN_TERMINATOR) &&
           !check(p, TOKEN_EOF)) {
        if (!first) strncat(name, " ", MAX_TOKEN_LEN - strlen(name) - 1);
        strncat(name, current(p).value, MAX_TOKEN_LEN - strlen(name) - 1);
        advance(p);
        first = 0;
    }
    strncpy(node->as.activity_def.name, name, MAX_TOKEN_LEN - 1);

    nodelist_init(&node->as.activity_def.params);
    nodelist_init(&node->as.activity_def.body);

    /* Inline one-liner: to add, return x plus y! */
    if (match(p, TOKEN_COMMA)) {
        ASTNode *stmt = parse_statement(p);
        if (stmt) nodelist_push(&node->as.activity_def.body, stmt);
        match(p, TOKEN_TERMINATOR);
        node->end_line = current(p).line;   /* v1.5 */
        return node;
    }

    /* Multi-line body */
    parse_block(p, &node->as.activity_def.body);
    node->end_line = current(p).line;   /* v1.5 */
    return node;
}


/* ═════════════════════════════════════════════
   RETURN STATEMENT
   return <expression>
   ═════════════════════════════════════════════ */
ASTNode *parse_return(Parser *p) {
    Token t = advance(p);   /* consume 'return' */
    ASTNode *node = ast_make_node(NODE_RETURN, t.line, t.col);
    node->as.return_stmt.value = parse_expression(p);
    return node;
}


/* ═════════════════════════════════════════════
   CHECK STATEMENT
   check that <condition>
   ═════════════════════════════════════════════ */
ASTNode *parse_check(Parser *p) {
    Token t = advance(p);   /* consume 'check' */
    ASTNode *node = ast_make_node(NODE_CHECK, t.line, t.col);
    match(p, TOKEN_THAT);   /* consume optional 'that' */
    node->as.check_stmt.condition = parse_expression(p);
    return node;
}


/* ═════════════════════════════════════════════
   DESCRIBE STATEMENT
   describe <name> as "<text>"
   ═════════════════════════════════════════════ */
ASTNode *parse_describe(Parser *p) {
    Token t = advance(p);   /* consume 'describe' */
    ASTNode *node = ast_make_node(NODE_DESCRIBE, t.line, t.col);

    /* Collect the describe label — allow any token (incl. keywords)
       until we hit 'as', newline, or EOF.  This lets users write
       "describe add as ..." or "describe grade analyzer as ..." */
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
    strncpy(node->as.describe_stmt.name, name, MAX_TOKEN_LEN - 1);

    expect(p, TOKEN_AS, "Expected 'as' in describe statement");

    if (check(p, TOKEN_STRING)) {
        strncpy(node->as.describe_stmt.text,
            current(p).value, MAX_TOKEN_LEN - 1);
        advance(p);
    } else {
        parser_error(p, "Expected string after 'as' in describe");
    }

    return node;
}


/* ═════════════════════════════════════════════
   PROVIDE STATEMENT
   provide <var> as user entry from "<prompt>"
   ═════════════════════════════════════════════ */
ASTNode *parse_provide(Parser *p) {
    Token t = advance(p);   /* consume 'provide' */
    ASTNode *node = ast_make_node(NODE_PROVIDE, t.line, t.col);

    char name[MAX_TOKEN_LEN] = {0};
    collect_name(p, name, MAX_TOKEN_LEN);
    strncpy(node->as.provide_stmt.var, name, MAX_TOKEN_LEN - 1);

    expect(p, TOKEN_AS,    "Expected 'as' in provide statement");
    match(p, TOKEN_USER);  /* consume 'user'  */
    match(p, TOKEN_INPUT); /* consume 'input' if present */
    match(p, TOKEN_ENTRY); /* consume 'entry' if present */
    match(p, TOKEN_FROM);  /* consume 'from'  */

    node->as.provide_stmt.prompt = parse_expression(p);
    return node;
}


/* ═════════════════════════════════════════════
   ATTEMPT STATEMENT
   attempt
       <body>
   if fails then
       <body>
   !
   ═════════════════════════════════════════════ */
ASTNode *parse_attempt(Parser *p) {
    Token t = advance(p);   /* consume 'attempt' */
    ASTNode *node = ast_make_node(NODE_ATTEMPT, t.line, t.col);

    nodelist_init(&node->as.attempt_stmt.try_body);
    nodelist_init(&node->as.attempt_stmt.fail_body);

    skip_newlines(p);

    /* Read try body until 'if fails' or ! or EOF */
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

    /* Optional: if fails then <body> */
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

    match(p, TOKEN_TERMINATOR);
    node->end_line = current(p).line;   /* v1.5 */
    return node;
}

/* ═════════════════════════════════════════════
   ADD TO STATEMENT
   add <expression> to <list_name>
   ═════════════════════════════════════════════ */
ASTNode *parse_add_to(Parser *p) {
    Token t = advance(p);   /* consume 'add' */
    ASTNode *node = ast_make_node(NODE_ADD_TO, t.line, t.col);

    node->as.add_to.item = parse_expression(p);

    expect(p, TOKEN_TO, "Expected 'to' in 'add ... to ...'");

    if (check(p, TOKEN_IDENTIFIER)) {
        strncpy(node->as.add_to.list,
            current(p).value, MAX_TOKEN_LEN - 1);
        advance(p);
    } else {
        parser_error(p, "Expected list name after 'to' in add statement");
    }

    return node;
}


/* ═════════════════════════════════════════════
   LINK STATEMENT  (v1.3)
   link <from> to <to> in <graph>
   ═════════════════════════════════════════════ */
ASTNode *parse_link(Parser *p) {
    Token t = advance(p);   /* consume 'link' */
    ASTNode *node = ast_make_node(NODE_LINK, t.line, t.col);

    /* collect 'from' name */
    char from[MAX_TOKEN_LEN] = {0};
    while (!check(p, TOKEN_TO) && !check(p, TOKEN_EOF) && !check(p, TOKEN_NEWLINE)) {
        if (strlen(from) > 0) strncat(from, " ", MAX_TOKEN_LEN - strlen(from) - 1);
        strncat(from, current(p).value, MAX_TOKEN_LEN - strlen(from) - 1);
        advance(p);
    }
    strncpy(node->as.link_stmt.from, from, MAX_TOKEN_LEN - 1);

    expect(p, TOKEN_TO, "Expected 'to' in link statement");

    /* collect 'to' name */
    char to[MAX_TOKEN_LEN] = {0};
    while (!check(p, TOKEN_IN) && !check(p, TOKEN_EOF) && !check(p, TOKEN_NEWLINE)) {
        if (strlen(to) > 0) strncat(to, " ", MAX_TOKEN_LEN - strlen(to) - 1);
        strncat(to, current(p).value, MAX_TOKEN_LEN - strlen(to) - 1);
        advance(p);
    }
    strncpy(node->as.link_stmt.to, to, MAX_TOKEN_LEN - 1);

    expect(p, TOKEN_IN, "Expected 'in <graph>' in link statement");

    /* graph variable name */
    char graph[MAX_TOKEN_LEN] = {0};
    collect_name(p, graph, MAX_TOKEN_LEN);
    strncpy(node->as.link_stmt.graph, graph, MAX_TOKEN_LEN - 1);

    return node;
}


/* ═════════════════════════════════════════════
   NODE3D  (v1.6)
   node at 0.5 labeled "Task" status "completed"
   ═════════════════════════════════════════════ */
ASTNode *parse_node3d(Parser *p) {
    Token t = advance(p);   /* consume 'node' */
    ASTNode *node = ast_make_node(NODE_NODE3D, t.line, t.col);
    node->as.node3d.angle  = 0.0;
    node->as.node3d.status = NODE3D_PENDING;
    node->as.node3d.label[0]= '\0';
    node->as.node3d.desc[0] = '\0';

    /* at <number> */
    if (check(p, TOKEN_AT_KW)) {
        advance(p);
        if (check(p, TOKEN_NUMBER)) {
            node->as.node3d.angle = atof(current(p).value);
            advance(p);
        }
    }
    /* labeled "text" */
    if (check(p, TOKEN_LABELED)) {
        advance(p);
        if (check(p, TOKEN_STRING)) {
            strncpy(node->as.node3d.label,
                current(p).value, MAX_TOKEN_LEN-1);
            advance(p);
        }
    }
    /* status "completed|inprogress|overdue|pending" */
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


/* ═════════════════════════════════════════════
   CORE STAR  (v1.6)
   core star color "gold" labeled "Project"
   ═════════════════════════════════════════════ */
ASTNode *parse_core_star(Parser *p) {
    Token t = advance(p);   /* consume 'core' */
    match(p, TOKEN_STAR_KW);
    ASTNode *node = ast_make_node(NODE_CORE_STAR, t.line, t.col);
    node->as.core_star.color[0] = '\0';
    node->as.core_star.label[0] = '\0';
    node->as.core_star.desc[0]  = '\0';
    strncpy(node->as.core_star.color, "gold", MAX_TOKEN_LEN-1);

    if (check(p, TOKEN_WITH_COLOR)) {
        advance(p);
        if (check(p, TOKEN_STRING)) {
            strncpy(node->as.core_star.color,
                current(p).value, MAX_TOKEN_LEN-1);
            advance(p);
        }
    }
    if (check(p, TOKEN_LABELED)) {
        advance(p);
        if (check(p, TOKEN_STRING)) {
            strncpy(node->as.core_star.label,
                current(p).value, MAX_TOKEN_LEN-1);
            advance(p);
        }
    }
    return node;
}


/* ═════════════════════════════════════════════
   ORBITAL RING  (v1.6)
   orbital ring radius 62 color "indigo" speed 0.007
       node at 0.5 labeled "Task" status "completed"
   !
   ═════════════════════════════════════════════ */
ASTNode *parse_orbital_ring(Parser *p) {
    Token t = advance(p);   /* consume 'orbital' */
    match(p, TOKEN_RING_KW);
    ASTNode *node = ast_make_node(NODE_ORBITAL_RING, t.line, t.col);
    nodelist_init(&node->as.orbital_ring.nodes);
    node->as.orbital_ring.radius = 100.0;
    node->as.orbital_ring.speed  = 0.003;
    node->as.orbital_ring.tilt_x = 0.0;
    node->as.orbital_ring.tilt_z = 0.0;
    strncpy(node->as.orbital_ring.color, "indigo", MAX_TOKEN_LEN-1);

    /* radius N */
    if (check(p, TOKEN_RADIUS_KW)) {
        advance(p);
        if (check(p, TOKEN_NUMBER)) {
            node->as.orbital_ring.radius = atof(current(p).value);
            advance(p);
        }
    }
    /* color "name" */
    if (check(p, TOKEN_WITH_COLOR)||check(p, TOKEN_STYLED)) {
        advance(p);
        if (check(p, TOKEN_STRING)) {
            strncpy(node->as.orbital_ring.color,
                current(p).value, MAX_TOKEN_LEN-1);
            advance(p);
        }
    }
    /* speed N */
    if (check(p, TOKEN_SPEED_KW)) {
        advance(p);
        if (check(p, TOKEN_NUMBER)) {
            node->as.orbital_ring.speed = atof(current(p).value);
            advance(p);
        }
    }
    /* tilt x N z N (optional) */
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

    /* nodes until ! */
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


/* ═════════════════════════════════════════════
   CANVAS3D  (v1.6)
   canvas3d "name" background "deep space"
       orbital ring ... !
       core star ...
   !
   ═════════════════════════════════════════════ */
ASTNode *parse_canvas3d(Parser *p) {
    Token t = advance(p);   /* consume 'canvas3d' */
    ASTNode *node = ast_make_node(NODE_CANVAS3D, t.line, t.col);
    nodelist_init(&node->as.canvas3d.children);
    node->as.canvas3d.background = BG3D_DEEP_SPACE;
    node->as.canvas3d.offline    = 0;
    node->as.canvas3d.name[0]    = '\0';
    node->as.canvas3d.bg_color[0]= '\0';

    /* optional name string */
    if (check(p, TOKEN_STRING)) {
        strncpy(node->as.canvas3d.name,
            current(p).value, MAX_TOKEN_LEN-1);
        advance(p);
    }

    /* background "preset" */
    if (check(p, TOKEN_BACKGROUND_KW)) {
        advance(p);
        if (check(p, TOKEN_STRING)) {
            const char *bg = current(p).value;
            if      (strstr(bg,"space")) node->as.canvas3d.background = BG3D_DEEP_SPACE;
            else if (strstr(bg,"void"))  node->as.canvas3d.background = BG3D_VOID;
            else if (strstr(bg,"nebula"))node->as.canvas3d.background = BG3D_NEBULA;
            else {
                node->as.canvas3d.background = BG3D_CUSTOM;
                strncpy(node->as.canvas3d.bg_color, bg, MAX_TOKEN_LEN-1);
            }
            advance(p);
        }
    }

    /* offline keyword */
    if (check(p, TOKEN_OFFLINE)) {
        node->as.canvas3d.offline = 1;
        advance(p);
    }

    /* children until ! */
    while (!check(p, TOKEN_TERMINATOR) && !check(p, TOKEN_EOF)) {
        if (check(p, TOKEN_NEWLINE)) { advance(p); continue; }
        ASTNode *child = parse_statement(p);
        if (child) nodelist_push(&node->as.canvas3d.children, child);
    }
    match(p, TOKEN_TERMINATOR);
    node->end_line = current(p).line;
    return node;
}


/* ═════════════════════════════════════════════
   PANEL3D  (v1.6)
   panel "right"
       heading "Title"
       make stat "Active" from 12
   !
   ═════════════════════════════════════════════ */
ASTNode *parse_panel3d(Parser *p) {
    Token t = advance(p);   /* consume 'panel' */
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


/* ═════════════════════════════════════════════
   DRAWER3D  (v1.6)
   drawer "right" titled "All Tasks"
       make table from tasks ...
   !
   ═════════════════════════════════════════════ */
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
            strncpy(node->as.drawer3d.title,
                current(p).value, MAX_TOKEN_LEN-1);
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


/* ═════════════════════════════════════════════
   TEXT NODE  (v1.4)
   heading "text" / paragraph "text" / etc.
   ═════════════════════════════════════════════ */
ASTNode *parse_text_node(Parser *p, NodeType type) {
    Token t = advance(p);
    ASTNode *node = ast_make_node(type, t.line, t.col);
    node->as.text_out.value = parse_expression(p);
    return node;
}


/* ═════════════════════════════════════════════
   SECTION  (v1.4)
   section "title" ... !
   ═════════════════════════════════════════════ */
ASTNode *parse_section(Parser *p) {
    Token t = advance(p);
    ASTNode *node = ast_make_node(NODE_SECTION_OUT, t.line, t.col);
    nodelist_init(&node->as.section_out.children);
    node->as.section_out.title[0] = '\0';

    /* optional title string */
    if (check(p, TOKEN_STRING)) {
        strncpy(node->as.section_out.title,
            current(p).value, MAX_TOKEN_LEN - 1);
        advance(p);
    }

    /* consume children until ! */
    while (!check(p, TOKEN_TERMINATOR) && !check(p, TOKEN_EOF)) {
        if (check(p, TOKEN_NEWLINE)) { advance(p); continue; }
        ASTNode *child = parse_statement(p);
        if (child) nodelist_push(&node->as.section_out.children, child);
    }
    match(p, TOKEN_TERMINATOR);
    node->end_line = current(p).line;   /* v1.5 */
    return node;
}

/* ═════════════════════════════════════════════
   ROW  (v1.4)
   row ... !
   ═════════════════════════════════════════════ */
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
    node->end_line = current(p).line;   /* v1.5 */
    return node;
}

/* ═════════════════════════════════════════════
   MAKE STAT  (v1.4)
   make stat "label" from value
   ═════════════════════════════════════════════ */
ASTNode *parse_make_stat(Parser *p) {
    Token t = advance(p);   /* consume 'stat' */
    ASTNode *node = ast_make_node(NODE_MAKE_STAT, t.line, t.col);
    node->as.make_stat.label[0] = '\0';

    /* label string */
    if (check(p, TOKEN_STRING)) {
        strncpy(node->as.make_stat.label,
            current(p).value, MAX_TOKEN_LEN - 1);
        advance(p);
    }

    match(p, TOKEN_FROM);
    node->as.make_stat.value = parse_expression(p);
    return node;
}


/* ═════════════════════════════════════════════
   MAKE CHART  (v1.4)
   make line chart for x axis as months and y axis as revenue titled "T"
   ═════════════════════════════════════════════ */
ASTNode *parse_make_chart(Parser *p) {
    Token t = advance(p);   /* consume chart type token */
    ASTNode *node = ast_make_node(NODE_MAKE_CHART, t.line, t.col);

    /* determine chart type */
    if (t.type == TOKEN_LINE_CHART)     node->as.make_chart.type = CHART_LINE;
    else if (t.type == TOKEN_BAR_CHART) node->as.make_chart.type = CHART_BAR;
    else                                node->as.make_chart.type = CHART_PIE;

    /* consume 'chart' keyword if present */
    if (check(p, TOKEN_MAKE_CHART)) advance(p);

    node->as.make_chart.title[0]    = '\0';
    node->as.make_chart.filename[0] = '\0';
    node->as.make_chart.x_label[0]  = '\0';
    node->as.make_chart.y_label[0]  = '\0';
    node->as.make_chart.x_data      = NULL;
    node->as.make_chart.y_data      = NULL;

    /* pie chart: make pie chart from labels and values */
    if (node->as.make_chart.type == CHART_PIE) {
        match(p, TOKEN_FROM);
        node->as.make_chart.x_data = parse_primary(p);
        match(p, TOKEN_AND);
        node->as.make_chart.y_data = parse_primary(p);
    } else {
        /* for x axis as <var> and y axis as <var> */
        match(p, TOKEN_FOR);
        match(p, TOKEN_X_AXIS);
        match(p, TOKEN_AXIS);
        match(p, TOKEN_AS);
        char xname[MAX_TOKEN_LEN] = {0};
        collect_name(p, xname, MAX_TOKEN_LEN);
        strncpy(node->as.make_chart.x_label, xname, MAX_TOKEN_LEN - 1);
        node->as.make_chart.x_data = ast_make_node(NODE_IDENTIFIER,
            t.line, t.col);
        strncpy(node->as.make_chart.x_data->as.identifier.name,
            xname, MAX_TOKEN_LEN - 1);

        match(p, TOKEN_AND);
        match(p, TOKEN_Y_AXIS);
        match(p, TOKEN_AXIS);
        match(p, TOKEN_AS);
        char yname[MAX_TOKEN_LEN] = {0};
        collect_name(p, yname, MAX_TOKEN_LEN);
        strncpy(node->as.make_chart.y_label, yname, MAX_TOKEN_LEN - 1);
        node->as.make_chart.y_data = ast_make_node(NODE_IDENTIFIER,
            t.line, t.col);
        strncpy(node->as.make_chart.y_data->as.identifier.name,
            yname, MAX_TOKEN_LEN - 1);
    }

    /* titled "..." */
    if (check(p, TOKEN_TITLED)) {
        advance(p);
        if (check(p, TOKEN_STRING)) {
            strncpy(node->as.make_chart.title,
                current(p).value, MAX_TOKEN_LEN - 1);
            advance(p);
        }
    }

    return node;
}


/* ═════════════════════════════════════════════
   MAKE TABLE  (v1.4)
   make table from data titled "T" with headers [...]
   ═════════════════════════════════════════════ */
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
            strncpy(node->as.make_table_out.title,
                current(p).value, MAX_TOKEN_LEN - 1);
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


/* ═════════════════════════════════════════════
   MAKE REPORT  (v1.4)
   make report from data titled "T"
   ═════════════════════════════════════════════ */
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
            strncpy(node->as.make_report.title,
                current(p).value, MAX_TOKEN_LEN - 1);
            advance(p);
        }
    }

    return node;
}


/* ═════════════════════════════════════════════
   MAKE PAGE  (v1.4)
   make page saved as "x.html" styled as "dark"
       ...children...
   !
   ═════════════════════════════════════════════ */
ASTNode *parse_make_page(Parser *p) {
    Token t = advance(p);   /* consume 'page' */
    ASTNode *node = ast_make_node(NODE_MAKE_PAGE, t.line, t.col);
    nodelist_init(&node->as.make_page.children);
    node->as.make_page.filename[0] = '\0';
    node->as.make_page.color[0]    = '\0';
    node->as.make_page.theme       = THEME_LIGHT;

    /* saved as "filename.html" */
    if (check(p, TOKEN_SAVED)) {
        advance(p);
        match(p, TOKEN_AS);
        if (check(p, TOKEN_STRING)) {
            strncpy(node->as.make_page.filename,
                current(p).value, MAX_TOKEN_LEN - 1);
            advance(p);
        }
    }

    /* styled as "dark" / "light" / "indigo" / "emerald" / "rose" */
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
                /* Any unrecognised string → custom full-palette color.
                   CSS accepts: hex (#ff69b4), named (hotpink), or descriptive
                   multi-word names resolved by the codegen lookup table. */
                node->as.make_page.theme = THEME_CUSTOM;
                strncpy(node->as.make_page.color, theme, MAX_TOKEN_LEN - 1);
            }
            advance(p);
        }
    }

    /* with color "hex" */
    if (check(p, TOKEN_WITH)) {
        advance(p);
        if (check(p, TOKEN_WITH_COLOR)) {
            advance(p);
            if (check(p, TOKEN_STRING)) {
                strncpy(node->as.make_page.color,
                    current(p).value, MAX_TOKEN_LEN - 1);
                advance(p);
            }
        }
    }

    /* children until ! */
    while (!check(p, TOKEN_TERMINATOR) && !check(p, TOKEN_EOF)) {
        if (check(p, TOKEN_NEWLINE)) { advance(p); continue; }
        ASTNode *child = parse_statement(p);
        if (child) nodelist_push(&node->as.make_page.children, child);
    }
    match(p, TOKEN_TERMINATOR);
    node->end_line = current(p).line;   /* v1.5 */
    return node;
}

/* ═════════════════════════════════════════════
   WAIT STATEMENT
   wait for enter key with message "<msg>"
   ═════════════════════════════════════════════ */
ASTNode *parse_wait(Parser *p) {
    Token t = advance(p);   /* consume 'wait' */
    ASTNode *node = ast_make_node(NODE_WAIT, t.line, t.col);
    node->as.wait_stmt.message[0] = '\0';

    /* Consume: for enter key with message */
    while (!check(p, TOKEN_STRING) &&
           !check(p, TOKEN_NEWLINE) &&
           !check(p, TOKEN_TERMINATOR) &&
           !check(p, TOKEN_EOF)) {
        advance(p);
    }

    if (check(p, TOKEN_STRING)) {
        strncpy(node->as.wait_stmt.message,
            current(p).value, MAX_TOKEN_LEN - 1);
        advance(p);
    }

    return node;
}


/* ═════════════════════════════════════════════
   EXECUTE STATEMENT
   execute system command "cmd"
   execute system command "cls" if windows otherwise "clear"
   ═════════════════════════════════════════════ */
ASTNode *parse_execute(Parser *p) {
    Token t = advance(p);   /* consume 'execute' */
    ASTNode *node = ast_make_node(NODE_EXECUTE, t.line, t.col);
    node->as.execute_stmt.conditional = 0;
    node->as.execute_stmt.command[0]  = '\0';

    /* consume: system command */
    match(p, TOKEN_SYSTEM);
    match(p, TOKEN_COMMAND);

    if (check(p, TOKEN_STRING)) {
        strncpy(node->as.execute_stmt.command,
            current(p).value, MAX_TOKEN_LEN - 1);
        advance(p);
    } else {
        parser_error(p, "Expected command string in execute statement");
        return node;
    }

    /* Optional: if windows otherwise "clear" */
    if (match(p, TOKEN_IF)) {
        match(p, TOKEN_WINDOWS);
        node->as.execute_stmt.conditional = 1;
        match(p, TOKEN_OTHERWISE);
        if (check(p, TOKEN_STRING)) {
            strncpy(node->as.execute_stmt.alt_command,
                current(p).value, MAX_TOKEN_LEN - 1);
            advance(p);
        }
    }

    return node;
}


/* ═════════════════════════════════════════════
   EXPRESSION PARSER
   Handles the full precedence chain +
   inline-if expressions.
   ═════════════════════════════════════════════ */

/* Top level — checks for inline if and where filter */
ASTNode *parse_expression(Parser *p) {
    ASTNode *left = parse_logical(p);

    /* Where filter: <collection> where <var> <condition>
       e.g.  people where age is greater than 18
             scores where score at least 60             */
    if (check(p, TOKEN_WHERE)) {
        Token wt = advance(p);   /* consume 'where' */
        ASTNode *node = ast_make_node(NODE_WHERE_FILTER, wt.line, wt.col);
        node->as.where_filter.collection = left;

        /* The next identifier is the item variable name.
           We collect it but DO NOT permanently advance the parser,
           because the condition expression needs to parse it as its left operand. */
        int saved_pos = p->current;
        if (is_identifier_word(current(p).type)) {
            char name[MAX_TOKEN_LEN] = {0};
            collect_name(p, name, MAX_TOKEN_LEN);
            strncpy(node->as.where_filter.item_var, name, MAX_TOKEN_LEN - 1);
            p->current = saved_pos; /* restore */
        } else {
            /* No explicit var — use "item" as default */
            strncpy(node->as.where_filter.item_var, "item", MAX_TOKEN_LEN - 1);
        }

        node->as.where_filter.condition = parse_logical(p);
        return node;
    }

    /* Inline if: <value> if <condition> otherwise <value> */
    if (check(p, TOKEN_IF)) {
        int line = current(p).line;
        int col  = current(p).col;
        advance(p);   /* consume 'if' */

        ASTNode *node = ast_make_node(NODE_INLINE_IF, line, col);
        node->as.inline_if.then_val  = left;
        node->as.inline_if.condition = parse_logical(p);

        expect(p, TOKEN_OTHERWISE, "Expected 'otherwise' in inline if");
        node->as.inline_if.else_val  = parse_expression(p);
        return node;
    }

    /* String/value concatenation with + */
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

/* Logical: and, or */
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

/* Comparison: equals, is, greater than, less than, etc. */
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

        /* ── Type check: X is a number / is a string / is a list ── */
        if (check(p, TOKEN_IS)) {
            /* peek two ahead for "a" + type keyword */
            int saved = p->current;
            advance(p);  /* consume 'is' */
            if (check(p, TOKEN_A)) {
                advance(p);  /* consume 'a' */
                TypeCheckKind kind;
                int matched = 1;
                Token kw = current(p);
                if      (kw.type == TOKEN_NUMBER_KW)  kind = TYPECHECK_NUMBER;
                else if (kw.type == TOKEN_STRING_KW)  kind = TYPECHECK_STRING;
                else if (kw.type == TOKEN_BOOLEAN_KW) kind = TYPECHECK_BOOLEAN;
                else if (kw.type == TOKEN_LIST_KW)    kind = TYPECHECK_LIST;
                else matched = 0;

                if (matched) {
                    advance(p);   /* consume type keyword */
                    ASTNode *node = ast_make_node(NODE_TYPE_CHECK, kw.line, kw.col);
                    node->as.type_check.kind   = kind;
                    node->as.type_check.target = left;
                    left = node;
                    continue;
                }
            }
            /* not a type check — restore position */
            p->current = saved;
        }

        Token op = advance(p);
        ASTNode *node = ast_make_node(NODE_BINARY_OP, op.line, op.col);
        node->as.binary_op.operator = op.type;
        node->as.binary_op.left     = left;

        /* is any of / is none of — collect comma-separated values into list */
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

/* Additive: plus, minus */
ASTNode *parse_additive(Parser *p) {
    ASTNode *left = parse_multiplicative(p);

    while (check(p, TOKEN_PLUS) || check(p, TOKEN_MINUS)) {
        Token op = advance(p);
        ASTNode *right = parse_multiplicative(p);
        ASTNode *node  = ast_make_node(NODE_BINARY_OP, op.line, op.col);
        node->as.binary_op.operator = op.type;
        node->as.binary_op.left     = left;
        node->as.binary_op.right    = right;
        left = node;
    }
    return left;
}

/* Multiplicative: times, divided by, modulo, power of */
ASTNode *parse_multiplicative(Parser *p) {
    ASTNode *left = parse_unary(p);

    while (check(p, TOKEN_TIMES_OP)  ||
           check(p, TOKEN_TIMES)     ||
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

/* Unary: not */
ASTNode *parse_unary(Parser *p) {
    if (check(p, TOKEN_NOT)) {
        Token op = advance(p);
        ASTNode *node = ast_make_node(NODE_UNARY_OP, op.line, op.col);
        node->as.unary_op.operator = TOKEN_NOT;
        node->as.unary_op.operand  = parse_unary(p);
        return node;
    }

    ASTNode *node = parse_primary(p);

    /* ── Postfix: string operations ── */
    while (check(p, TOKEN_IN)       ||
           check(p, TOKEN_TRIMMED)  ||
           check(p, TOKEN_SPLIT)    ||
           check(p, TOKEN_JOINED)   ||
           check(p, TOKEN_WITH)     ||
           check(p, TOKEN_STARTS)   ||
           check(p, TOKEN_ENDS)     ||
           check(p, TOKEN_CONTAINS)) {
        ASTNode *result = parse_string_op(p, node);
        if (result == node) break;   /* no match, stop */
        node = result;
    }

    /* ── Postfix: format operations ── */
    if (check(p, TOKEN_ROUNDED) || check(p, TOKEN_AS)) {
        ASTNode *result = parse_format_op(p, node);
        if (result != node) node = result;
    }

    return node;
}

/* Primary: literals, identifiers, lists, parens */
ASTNode *parse_primary(Parser *p) {
    Token t = current(p);

    /* Number */
    if (t.type == TOKEN_NUMBER) {
        advance(p);
        ASTNode *node = ast_make_node(NODE_NUMBER, t.line, t.col);
        node->as.number.value = atof(t.value);
        return node;
    }

    /* String */
    if (t.type == TOKEN_STRING) {
        advance(p);
        ASTNode *node = ast_make_node(NODE_STRING, t.line, t.col);
        strncpy(node->as.string.value, t.value, MAX_TOKEN_LEN - 1);
        return node;
    }

    /* Boolean */
    if (t.type == TOKEN_BOOLEAN) {
        advance(p);
        ASTNode *node = ast_make_node(NODE_BOOLEAN, t.line, t.col);
        node->as.boolean.value = (strcmp(t.value, "true") == 0) ? 1 : 0;
        return node;
    }

    /* Null / empty */
    if (t.type == TOKEN_NULL || t.type == TOKEN_EMPTY) {
        advance(p);
        return ast_make_node(NODE_NULL, t.line, t.col);
    }

    /* List literal */
    if (t.type == TOKEN_LBRACKET) {
        return parse_list_literal(p);
    }

    /* Parenthesised expression */
    if (t.type == TOKEN_LPAREN) {
        advance(p);
        ASTNode *expr = parse_expression(p);
        expect(p, TOKEN_RPAREN, "Expected ')' to close expression");
        return expr;
    }

    /* item N of LIST — v1.5 index access */
    if (t.type == TOKEN_ITEM) {
        advance(p);   /* consume 'item' */
        ASTNode *node = ast_make_node(NODE_INDEX_ACCESS, t.line, t.col);
        node->as.index_access.index = parse_additive(p);   /* the index */
        match(p, TOKEN_OF);                                  /* consume 'of' */
        node->as.index_access.list  = parse_primary(p);    /* the collection */
        return node;
    }

    /* Graph ops  (v1.3) */

    if (t.type == TOKEN_GRAPH) {
        advance(p);
        return ast_make_node(NODE_MAKE_GRAPH, t.line, t.col);
    }

    if (t.type == TOKEN_ROUTE) {
        advance(p);   /* consume 'route' */
        ASTNode *node = ast_make_node(NODE_ROUTE, t.line, t.col);

        match(p, TOKEN_FROM);

        /* collect from-name */
        char from[MAX_TOKEN_LEN] = {0};
        while (!check(p, TOKEN_TO) && !check(p, TOKEN_EOF) && !check(p, TOKEN_NEWLINE)) {
            if (strlen(from) > 0) strncat(from, " ", MAX_TOKEN_LEN - strlen(from) - 1);
            strncat(from, current(p).value, MAX_TOKEN_LEN - strlen(from) - 1);
            advance(p);
        }
        strncpy(node->as.route_stmt.from, from, MAX_TOKEN_LEN - 1);

        expect(p, TOKEN_TO, "Expected 'to' in route expression");

        /* collect to-name */
        char to[MAX_TOKEN_LEN] = {0};
        while (!check(p, TOKEN_IN) && !check(p, TOKEN_EOF) && !check(p, TOKEN_NEWLINE)) {
            if (strlen(to) > 0) strncat(to, " ", MAX_TOKEN_LEN - strlen(to) - 1);
            strncat(to, current(p).value, MAX_TOKEN_LEN - strlen(to) - 1);
            advance(p);
        }
        strncpy(node->as.route_stmt.to, to, MAX_TOKEN_LEN - 1);

        expect(p, TOKEN_IN, "Expected 'in <graph>' in route expression");

        char graph[MAX_TOKEN_LEN] = {0};
        collect_name(p, graph, MAX_TOKEN_LEN);
        strncpy(node->as.route_stmt.graph, graph, MAX_TOKEN_LEN - 1);

        return node;
    }

    if (t.type == TOKEN_CONNECTED) {
        advance(p);   /* consume 'connected' */
        ASTNode *node = ast_make_node(NODE_CONNECTED, t.line, t.col);

        match(p, TOKEN_TO);

        /* collect target name */
        char target[MAX_TOKEN_LEN] = {0};
        while (!check(p, TOKEN_IN) && !check(p, TOKEN_EOF) && !check(p, TOKEN_NEWLINE)) {
            if (strlen(target) > 0) strncat(target, " ", MAX_TOKEN_LEN - strlen(target) - 1);
            strncat(target, current(p).value, MAX_TOKEN_LEN - strlen(target) - 1);
            advance(p);
        }
        strncpy(node->as.connected_stmt.target, target, MAX_TOKEN_LEN - 1);

        expect(p, TOKEN_IN, "Expected 'in <graph>' in connected expression");

        char graph[MAX_TOKEN_LEN] = {0};
        collect_name(p, graph, MAX_TOKEN_LEN);
        strncpy(node->as.connected_stmt.graph, graph, MAX_TOKEN_LEN - 1);

        return node;
    }

    /* Smart Collection ops  (v1.1)
       average of X / total of X / sorted X / etc. */
    if (t.type == TOKEN_AVERAGE  ||
        t.type == TOKEN_TOTAL    ||
        t.type == TOKEN_MAXIMUM  ||
        t.type == TOKEN_MINIMUM  ||
        t.type == TOKEN_LENGTH   ||
        t.type == TOKEN_SORTED   ||
        t.type == TOKEN_REVERSED) {
        return parse_collection_op(p);
    }

    /* Identifier — variable or activity call */
    if (is_identifier_word(t.type) || is_postfix_op(t.type)) {
        char name[MAX_TOKEN_LEN] = {0};
        collect_name(p, name, MAX_TOKEN_LEN);
        ASTNode *node = ast_make_node(NODE_IDENTIFIER, t.line, t.col);
        strncpy(node->as.identifier.name, name, MAX_TOKEN_LEN - 1);
        return node;
    }

    /* Anything else — return null node, don't crash */
    return ast_make_node(NODE_NULL, t.line, t.col);
}

/* List literal: [1, 2, 3] */
ASTNode *parse_list_literal(Parser *p) {
    Token t = advance(p);   /* consume '[' */
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

/* ═════════════════════════════════════════════
   HTML DOM ELEMENT PARSER (v1.7)
   Handles:
     div id "main" class "container"
         ...
     !
     image src "test.png" class "logo"
   ═════════════════════════════════════════════ */
ASTNode *parse_html_element(Parser *p) {
    Token t = advance(p);
    ASTNode *node = ast_make_node(NODE_HTML_ELEMENT, t.line, t.col);
    
    strncpy(node->as.html_element.tag, t.value, MAX_TOKEN_LEN - 1);
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

    /* Parse optional attributes */
    while (!check(p, TOKEN_TERMINATOR) && !check(p, TOKEN_EOF) && !check(p, TOKEN_NEWLINE)) {
        if (match(p, TOKEN_ID_KW)) {
            Token str = expect(p, TOKEN_STRING, "Expected string after 'id'");
            strncpy(node->as.html_element.id_attr, str.value, MAX_TOKEN_LEN - 1);
        } else if (match(p, TOKEN_CLASS_KW)) {
            Token str = expect(p, TOKEN_STRING, "Expected string after 'class'");
            strncpy(node->as.html_element.class_attr, str.value, MAX_TOKEN_LEN - 1);
        } else if (match(p, TOKEN_SRC_KW)) {
            Token str = expect(p, TOKEN_STRING, "Expected string after 'src'");
            strncpy(node->as.html_element.src_attr, str.value, MAX_TOKEN_LEN - 1);
        } else if (match(p, TOKEN_TYPE_KW)) {
            Token str = expect(p, TOKEN_STRING, "Expected string after 'type'");
            strncpy(node->as.html_element.type_attr, str.value, MAX_TOKEN_LEN - 1);
        } else if (match(p, TOKEN_ONCLICK_KW)) {
            Token str = expect(p, TOKEN_STRING, "Expected string after 'onclick'");
            strncpy(node->as.html_element.onclick_attr, str.value, MAX_TOKEN_LEN - 1);
        } else if (match(p, TOKEN_PLACEHOLDER_KW)) {
            Token str = expect(p, TOKEN_STRING, "Expected string after 'placeholder'");
            strncpy(node->as.html_element.placeholder_attr, str.value, MAX_TOKEN_LEN - 1);
        } else if (match(p, TOKEN_VALUE_KW)) {
            Token str = expect(p, TOKEN_STRING, "Expected string after 'value'");
            strncpy(node->as.html_element.value_attr, str.value, MAX_TOKEN_LEN - 1);
        } else if (check(p, TOKEN_STRING) || check(p, TOKEN_IDENTIFIER) || check(p, TOKEN_NUMBER)) {
            /* Inner text content expressed as expression inline */
            node->as.html_element.text_content = parse_expression(p);
        } else {
            break; /* unknown token, let loop or block parser handle */
        }
    }

    if (!node->as.html_element.is_self_closing) {
        parse_block(p, &node->as.html_element.children);
    } else {
        skip_newlines(p);
        match(p, TOKEN_TERMINATOR); /* optional terminator */
    }
    
    return node;
}

/* ═══════════════════════════════════════════
   EPF v2.0 COMPONENT PARSERS
   ═══════════════════════════════════════════ */

/* use design system "name"
     primary color "X"
     secondary color "X"
     background "X"
     fonts headline "X" body "X"
     icons "X"
   ! */
ASTNode *parse_use_design(Parser *p) {
    Token t = advance(p); /* consume TOKEN_USE_DESIGN */
    ASTNode *node = ast_make_node(NODE_USE_DESIGN, t.line, t.col);
    node->as.use_design.name[0] = '\0';
    node->as.use_design.primary[0] = '\0';
    node->as.use_design.secondary[0] = '\0';
    node->as.use_design.background[0] = '\0';
    node->as.use_design.headline_font[0] = '\0';
    node->as.use_design.body_font[0] = '\0';
    node->as.use_design.icon_set[0] = '\0';

    if (check(p, TOKEN_STRING))
        strncpy(node->as.use_design.name, advance(p).value, MAX_TOKEN_LEN-1);

    skip_newlines(p);
    while (!check(p, TOKEN_TERMINATOR) && !check(p, TOKEN_EOF)) {
        skip_newlines(p);
        if (check(p, TOKEN_TERMINATOR) || check(p, TOKEN_EOF)) break;
        if (current(p).type == TOKEN_WITH_COLOR) { /* "color" */
            advance(p);
            Token val = expect(p, TOKEN_STRING, "Expected color string");
            /* figure out which color by checking previous word */
            if (strlen(node->as.use_design.primary) == 0)
                strncpy(node->as.use_design.primary, val.value, MAX_TOKEN_LEN-1);
            else
                strncpy(node->as.use_design.secondary, val.value, MAX_TOKEN_LEN-1);
        } else if (current(p).type == TOKEN_BACKGROUND_KW) {
            advance(p);
            Token val = expect(p, TOKEN_STRING, "Expected background color");
            strncpy(node->as.use_design.background, val.value, MAX_TOKEN_LEN-1);
        } else if (strcmp(current(p).value, "fonts") == 0 || strcmp(current(p).value, "headline") == 0) {
            advance(p); /* "fonts" or "headline" */
            match(p, TOKEN_HEADING_OUT); /* "headline" if already consumed */
            if (check(p, TOKEN_STRING))
                strncpy(node->as.use_design.headline_font, advance(p).value, MAX_TOKEN_LEN-1);
            match(p, TOKEN_MAKE_REPORT); /* skip "body" keyword if present */
            if (check(p, TOKEN_STRING))
                strncpy(node->as.use_design.body_font, advance(p).value, MAX_TOKEN_LEN-1);
        } else if (current(p).type == TOKEN_ICONS_KW) {
            advance(p);
            if (check(p, TOKEN_STRING))
                strncpy(node->as.use_design.icon_set, advance(p).value, MAX_TOKEN_LEN-1);
        } else {
            advance(p); /* skip unknown modifiers */
        }
        skip_newlines(p);
    }
    match(p, TOKEN_TERMINATOR);
    return node;
}

/* make navbar branded as "X" styled as "Y"
     nav link "label" at "href"
     action button "label" on click open modal "id"
   ! */
ASTNode *parse_make_navbar(Parser *p) {
    Token t = advance(p);
    ASTNode *node = ast_make_node(NODE_MAKE_NAVBAR, t.line, t.col);
    node->as.make_navbar.brand[0] = '\0';
    node->as.make_navbar.style[0] = '\0';
    nodelist_init(&node->as.make_navbar.children);

    match(p, TOKEN_BRANDED); match(p, TOKEN_AS);
    if (check(p, TOKEN_STRING))
        strncpy(node->as.make_navbar.brand, advance(p).value, MAX_TOKEN_LEN-1);
    match(p, TOKEN_STYLED); match(p, TOKEN_AS);
    if (check(p, TOKEN_STRING))
        strncpy(node->as.make_navbar.style, advance(p).value, MAX_TOKEN_LEN-1);

    parse_block(p, &node->as.make_navbar.children);
    return node;
}

/* nav link "label" at "href" */
ASTNode *parse_nav_link(Parser *p) {
    Token t = advance(p);
    ASTNode *node = ast_make_node(NODE_NAV_LINK, t.line, t.col);
    node->as.nav_link.label[0] = '\0';
    node->as.nav_link.href[0] = '\0';
    if (check(p, TOKEN_STRING))
        strncpy(node->as.nav_link.label, advance(p).value, MAX_TOKEN_LEN-1);
    match(p, TOKEN_AT_KW);
    if (check(p, TOKEN_STRING))
        strncpy(node->as.nav_link.href, advance(p).value, MAX_TOKEN_LEN-1);
    return node;
}

/* action button "label" on click open modal "id" */
ASTNode *parse_action_btn(Parser *p) {
    Token t = advance(p);
    ASTNode *node = ast_make_node(NODE_ACTION_BTN, t.line, t.col);
    node->as.action_btn.label[0] = '\0';
    node->as.action_btn.modal_id[0] = '\0';
    if (check(p, TOKEN_STRING))
        strncpy(node->as.action_btn.label, advance(p).value, MAX_TOKEN_LEN-1);
    if (check(p, TOKEN_ON_CLICK)) {
        advance(p);
        if (check(p, TOKEN_OPEN_MODAL)) {
            advance(p);
            if (check(p, TOKEN_STRING))
                strncpy(node->as.action_btn.modal_id, advance(p).value, MAX_TOKEN_LEN-1);
        }
    }
    return node;
}

/* make hero titled "X" accented "Y" subtitled "Z"
     background image "url" dimmed to N
     overlay card "title" ... !
   ! */
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
        strncpy(node->as.make_hero.title, advance(p).value, MAX_TOKEN_LEN-1);
    if (check(p, TOKEN_ACCENTED)) {
        advance(p);
        if (check(p, TOKEN_STRING))
            strncpy(node->as.make_hero.accented, advance(p).value, MAX_TOKEN_LEN-1);
    }
    if (check(p, TOKEN_SUBHEADING)) {
        advance(p);
        if (check(p, TOKEN_STRING))
            strncpy(node->as.make_hero.subtitle, advance(p).value, MAX_TOKEN_LEN-1);
    }

    skip_newlines(p);
    while (!check(p, TOKEN_TERMINATOR) && !check(p, TOKEN_EOF)) {
        skip_newlines(p);
        if (check(p, TOKEN_TERMINATOR) || check(p, TOKEN_EOF)) break;
        if (check(p, TOKEN_BACKGROUND_KW)) {
            advance(p); /* "background" */
            advance(p); /* "image" */
            if (check(p, TOKEN_STRING))
                strncpy(node->as.make_hero.bg_image, advance(p).value, MAX_TOKEN_LEN-1);
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

/* overlay card "title" ... ! */
ASTNode *parse_overlay_card(Parser *p) {
    Token t = advance(p);
    ASTNode *node = ast_make_node(NODE_OVERLAY_CARD, t.line, t.col);
    node->as.overlay_card.title[0] = '\0';
    nodelist_init(&node->as.overlay_card.children);
    if (check(p, TOKEN_STRING))
        strncpy(node->as.overlay_card.title, advance(p).value, MAX_TOKEN_LEN-1);
    parse_block(p, &node->as.overlay_card.children);
    return node;
}

/* guest picker default N */
ASTNode *parse_guest_picker(Parser *p) {
    Token t = advance(p);
    ASTNode *node = ast_make_node(NODE_GUEST_PICKER, t.line, t.col);
    node->as.guest_picker.default_count = 2;
    /* consume optional "default" */
    if (strcmp(current(p).value, "default") == 0) advance(p);
    if (check(p, TOKEN_NUMBER))
        node->as.guest_picker.default_count = (int)atof(advance(p).value);
    return node;
}

/* date picker label "X" */
ASTNode *parse_date_picker(Parser *p) {
    Token t = advance(p);
    ASTNode *node = ast_make_node(NODE_DATE_PICKER, t.line, t.col);
    node->as.date_picker.label[0] = '\0';
    match(p, TOKEN_LABEL_OUT);
    if (check(p, TOKEN_STRING))
        strncpy(node->as.date_picker.label, advance(p).value, MAX_TOKEN_LEN-1);
    return node;
}

/* make content section id "X"
     left column ... !
     right column ... !
   ! */
ASTNode *parse_content_section(Parser *p) {
    Token t = advance(p);
    ASTNode *node = ast_make_node(NODE_CONTENT_SECTION, t.line, t.col);
    node->as.content_section.id[0] = '\0';
    node->as.content_section.left_col = NULL;
    node->as.content_section.right_col = NULL;

    match(p, TOKEN_ID_KW);
    if (check(p, TOKEN_STRING))
        strncpy(node->as.content_section.id, advance(p).value, MAX_TOKEN_LEN-1);

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

/* left column / right column ... ! */
ASTNode *parse_column(Parser *p, int is_right) {
    Token t = advance(p);
    ASTNode *node = ast_make_node(NODE_COLUMN, t.line, t.col);
    node->as.column.is_right = is_right;
    nodelist_init(&node->as.column.children);
    parse_block(p, &node->as.column.children);
    return node;
}

/* eyebrow "text" */
ASTNode *parse_eyebrow(Parser *p) {
    Token t = advance(p);
    ASTNode *node = ast_make_node(NODE_EYEBROW, t.line, t.col);
    node->as.eyebrow.text[0] = '\0';
    if (check(p, TOKEN_STRING))
        strncpy(node->as.eyebrow.text, advance(p).value, MAX_TOKEN_LEN-1);
    return node;
}

/* item list
     entry "label" numbered N detail "x" [offset]
     ...
   ! */
ASTNode *parse_item_list(Parser *p) {
    Token t = advance(p);
    ASTNode *node = ast_make_node(NODE_ITEM_LIST, t.line, t.col);
    nodelist_init(&node->as.item_list.entries);
    parse_block(p, &node->as.item_list.entries);
    return node;
}

/* entry "label" numbered N detail "desc" [offset] */
ASTNode *parse_list_entry(Parser *p) {
    Token t = advance(p);
    ASTNode *node = ast_make_node(NODE_LIST_ENTRY, t.line, t.col);
    node->as.list_entry.label[0] = '\0';
    node->as.list_entry.number = 0;
    node->as.list_entry.detail[0] = '\0';
    node->as.list_entry.offset = 0;

    if (check(p, TOKEN_STRING))
        strncpy(node->as.list_entry.label, advance(p).value, MAX_TOKEN_LEN-1);
    if (check(p, TOKEN_NUMBERED)) {
        advance(p);
        if (check(p, TOKEN_NUMBER))
            node->as.list_entry.number = (int)atof(advance(p).value);
    }
    if (check(p, TOKEN_DETAIL_KW)) {
        advance(p);
        if (check(p, TOKEN_STRING))
            strncpy(node->as.list_entry.detail, advance(p).value, MAX_TOKEN_LEN-1);
    }
    if (check(p, TOKEN_OFFSET))
        { advance(p); node->as.list_entry.offset = 1; }
    return node;
}

/* feature image "src" aspect "4/5" hover "reveal color" */
ASTNode *parse_feature_image(Parser *p) {
    Token t = advance(p);
    ASTNode *node = ast_make_node(NODE_FEATURE_IMAGE, t.line, t.col);
    node->as.feature_image.src[0] = '\0';
    node->as.feature_image.aspect[0] = '\0';
    node->as.feature_image.hover_mode[0] = '\0';

    if (check(p, TOKEN_STRING))
        strncpy(node->as.feature_image.src, advance(p).value, MAX_TOKEN_LEN-1);
    if (check(p, TOKEN_ASPECT)) {
        advance(p);
        if (check(p, TOKEN_STRING))
            strncpy(node->as.feature_image.aspect, advance(p).value, MAX_TOKEN_LEN-1);
    }
    /* consume "hover" keyword then string */
    if (strcmp(current(p).value, "hover") == 0) {
        advance(p);
        if (check(p, TOKEN_STRING))
            strncpy(node->as.feature_image.hover_mode, advance(p).value, MAX_TOKEN_LEN-1);
    }
    return node;
}

/* price badge "$285" label "per guest" */
ASTNode *parse_price_badge(Parser *p) {
    Token t = advance(p);
    ASTNode *node = ast_make_node(NODE_PRICE_BADGE, t.line, t.col);
    node->as.price_badge.price[0] = '\0';
    node->as.price_badge.label[0] = '\0';
    if (check(p, TOKEN_STRING))
        strncpy(node->as.price_badge.price, advance(p).value, MAX_TOKEN_LEN-1);
    match(p, TOKEN_LABEL_OUT);
    if (check(p, TOKEN_STRING))
        strncpy(node->as.price_badge.label, advance(p).value, MAX_TOKEN_LEN-1);
    return node;
}

/* make gallery titled "X" subtitled "Y" columns N
     image "url" [offset top N]
     ...
   ! */
ASTNode *parse_make_gallery(Parser *p) {
    Token t = advance(p);
    ASTNode *node = ast_make_node(NODE_MAKE_GALLERY, t.line, t.col);
    node->as.make_gallery.title[0] = '\0';
    node->as.make_gallery.subtitle[0] = '\0';
    node->as.make_gallery.columns = 4;
    nodelist_init(&node->as.make_gallery.images);

    match(p, TOKEN_TITLED);
    if (check(p, TOKEN_STRING))
        strncpy(node->as.make_gallery.title, advance(p).value, MAX_TOKEN_LEN-1);
    if (check(p, TOKEN_SUBHEADING)) {
        advance(p);
        if (check(p, TOKEN_STRING))
            strncpy(node->as.make_gallery.subtitle, advance(p).value, MAX_TOKEN_LEN-1);
    }
    if (check(p, TOKEN_COLUMNS)) {
        advance(p);
        if (check(p, TOKEN_NUMBER))
            node->as.make_gallery.columns = (int)atof(advance(p).value);
    }
    parse_block(p, &node->as.make_gallery.images);
    return node;
}

/* image "url" [offset top N] — used inside gallery blocks */
ASTNode *parse_gallery_image(Parser *p) {
    Token t = advance(p);
    ASTNode *node = ast_make_node(NODE_GALLERY_IMAGE, t.line, t.col);
    node->as.gallery_image.src[0] = '\0';
    node->as.gallery_image.offset_top = 0;
    if (check(p, TOKEN_STRING))
        strncpy(node->as.gallery_image.src, advance(p).value, MAX_TOKEN_LEN-1);
    if (check(p, TOKEN_OFFSET)) {
        advance(p); /* "offset" */
        /* consume optional "top" */
        if (strcmp(current(p).value, "top") == 0) advance(p);
        if (check(p, TOKEN_NUMBER))
            node->as.gallery_image.offset_top = (int)atof(advance(p).value);
    }
    return node;
}

/* make form "title" eyebrow "subtitle"
     field text "label" placeholder "x"
     field email "label" placeholder "x"
     field textarea "label" placeholder "x"
     submit button "label" on click open modal "id"
   ! */
ASTNode *parse_make_form(Parser *p) {
    Token t = advance(p);
    ASTNode *node = ast_make_node(NODE_MAKE_FORM, t.line, t.col);
    node->as.make_form.title[0] = '\0';
    node->as.make_form.eyebrow[0] = '\0';
    nodelist_init(&node->as.make_form.children);

    if (check(p, TOKEN_STRING))
        strncpy(node->as.make_form.title, advance(p).value, MAX_TOKEN_LEN-1);
    if (check(p, TOKEN_EYEBROW)) {
        advance(p);
        if (check(p, TOKEN_STRING))
            strncpy(node->as.make_form.eyebrow, advance(p).value, MAX_TOKEN_LEN-1);
    }
    parse_block(p, &node->as.make_form.children);
    return node;
}

/* field text "Full Name" placeholder "Mr. Julian Thorne" */
ASTNode *parse_form_field(Parser *p) {
    Token t = advance(p); /* "field" */
    ASTNode *node = ast_make_node(NODE_FORM_FIELD, t.line, t.col);
    node->as.form_field.field_type[0] = '\0';
    node->as.form_field.field_label[0] = '\0';
    node->as.form_field.placeholder[0] = '\0';
    /* next token is the type: text, email, textarea */
    if (!check(p, TOKEN_EOF) && !check(p, TOKEN_NEWLINE))
        strncpy(node->as.form_field.field_type, advance(p).value, MAX_TOKEN_LEN-1);
    if (check(p, TOKEN_STRING))
        strncpy(node->as.form_field.field_label, advance(p).value, MAX_TOKEN_LEN-1);
    if (check(p, TOKEN_PLACEHOLDER_KW) || check(p, TOKEN_PLACEHOLDER_EPF)) {
        advance(p);
        if (check(p, TOKEN_STRING))
            strncpy(node->as.form_field.placeholder, advance(p).value, MAX_TOKEN_LEN-1);
    }
    return node;
}

/* submit button "label" on click open modal "id" */
ASTNode *parse_submit_btn(Parser *p) {
    Token t = advance(p);
    ASTNode *node = ast_make_node(NODE_SUBMIT_BTN, t.line, t.col);
    node->as.submit_btn.label[0] = '\0';
    node->as.submit_btn.modal_id[0] = '\0';
    if (check(p, TOKEN_STRING))
        strncpy(node->as.submit_btn.label, advance(p).value, MAX_TOKEN_LEN-1);
    if (check(p, TOKEN_ON_CLICK)) {
        advance(p);
        if (check(p, TOKEN_OPEN_MODAL)) {
            advance(p);
            if (check(p, TOKEN_STRING))
                strncpy(node->as.submit_btn.modal_id, advance(p).value, MAX_TOKEN_LEN-1);
        }
    }
    return node;
}

/* make modal id "booking" icon "check_circle" color "primary"
     heading "Sanctuary Secured"
     paragraph "Your reservation..."
     dismiss button "Return to Oasis"
   ! */
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
        strncpy(node->as.make_modal.modal_id, advance(p).value, MAX_TOKEN_LEN-1);
    if (strcmp(current(p).value, "icon") == 0) {
        advance(p);
        if (check(p, TOKEN_STRING))
            strncpy(node->as.make_modal.icon, advance(p).value, MAX_TOKEN_LEN-1);
    }
    if (check(p, TOKEN_WITH_COLOR)) {
        advance(p);
        if (check(p, TOKEN_STRING))
            strncpy(node->as.make_modal.color, advance(p).value, MAX_TOKEN_LEN-1);
    }

    skip_newlines(p);
    while (!check(p, TOKEN_TERMINATOR) && !check(p, TOKEN_EOF)) {
        skip_newlines(p);
        if (check(p, TOKEN_TERMINATOR) || check(p, TOKEN_EOF)) break;
        if (check(p, TOKEN_HEADING_OUT)) {
            advance(p);
            if (check(p, TOKEN_STRING))
                strncpy(node->as.make_modal.heading, advance(p).value, MAX_TOKEN_LEN-1);
        } else if (check(p, TOKEN_PARAGRAPH)) {
            advance(p);
            if (check(p, TOKEN_STRING))
                strncpy(node->as.make_modal.body_text, advance(p).value, MAX_TOKEN_LEN-1);
        } else if (check(p, TOKEN_DISMISS_BTN)) {
            advance(p);
            if (check(p, TOKEN_STRING))
                strncpy(node->as.make_modal.dismiss_label, advance(p).value, MAX_TOKEN_LEN-1);
        } else {
            advance(p);
        }
        skip_newlines(p);
    }
    match(p, TOKEN_TERMINATOR);
    return node;
}

/* dismiss button "label" */
ASTNode *parse_dismiss_btn(Parser *p) {
    Token t = advance(p);
    ASTNode *node = ast_make_node(NODE_DISMISS_BTN, t.line, t.col);
    node->as.make_modal.dismiss_label[0] = '\0';
    if (check(p, TOKEN_STRING))
        strncpy(node->as.make_modal.dismiss_label, advance(p).value, MAX_TOKEN_LEN-1);
    return node;
}

/* make footer branded as "Green Desert"
     tagline "..."
     contact heading "Visit Us"
     address "2424 Obsidian..."
     copyright "2024 Green Desert"
   ! */
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
        strncpy(node->as.make_footer.brand, advance(p).value, MAX_TOKEN_LEN-1);

    skip_newlines(p);
    while (!check(p, TOKEN_TERMINATOR) && !check(p, TOKEN_EOF)) {
        skip_newlines(p);
        if (check(p, TOKEN_TERMINATOR) || check(p, TOKEN_EOF)) break;
        if (check(p, TOKEN_TAGLINE)) {
            advance(p);
            if (check(p, TOKEN_STRING))
                strncpy(node->as.make_footer.tagline, advance(p).value, MAX_TOKEN_LEN-1);
        } else if (strcmp(current(p).value, "contact") == 0) {
            advance(p);
            match(p, TOKEN_HEADING_OUT);
            if (check(p, TOKEN_STRING))
                strncpy(node->as.make_footer.contact_heading, advance(p).value, MAX_TOKEN_LEN-1);
        } else if (check(p, TOKEN_ADDRESS_KW)) {
            advance(p);
            if (check(p, TOKEN_STRING))
                strncpy(node->as.make_footer.address, advance(p).value, MAX_TOKEN_LEN-1);
        } else if (check(p, TOKEN_COPYRIGHT)) {
            advance(p);
            if (check(p, TOKEN_STRING))
                strncpy(node->as.make_footer.copyright, advance(p).value, MAX_TOKEN_LEN-1);
        } else {
            advance(p);
        }
        skip_newlines(p);
    }
    match(p, TOKEN_TERMINATOR);
    return node;
}

/* parse_field — dispatched when TOKEN_FIELD is seen inside a form block */
/* Note: parse_statement default case won't hit TOKEN_FIELD, so the form block
   parser calls parse_form_field directly when it encounters it. Added here for
   completeness so the linker resolves the symbol. */
