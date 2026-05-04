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
