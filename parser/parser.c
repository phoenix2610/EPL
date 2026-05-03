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

/* Collect multi-word identifier name into buf */
/* e.g. "clear screen", "get number" */
static void collect_name(Parser *p, char *buf, int maxlen) {
    buf[0] = '\0';
    int first = 1;
    while (check(p, TOKEN_IDENTIFIER) ||
           (check(p, TOKEN_AS) && first == 0)) {
        if (!first) strncat(buf, " ", maxlen - strlen(buf) - 1);
        strncat(buf, current(p).value, maxlen - strlen(buf) - 1);
        advance(p);
        first = 0;
        /* Stop before keywords that aren't part of name */
        if (!check(p, TOKEN_IDENTIFIER)) break;
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
   SET STATEMENT
   set <name> as <expression>
   set <name> by <activity_call>
   ═════════════════════════════════════════════ */
ASTNode *parse_set(Parser *p) {
    Token t = advance(p);   /* consume 'set' */
    ASTNode *node = ast_make_node(NODE_SET, t.line, t.col);

    /* Collect variable name (may be multi-word) */
    char name[MAX_TOKEN_LEN] = {0};
    collect_name(p, name, MAX_TOKEN_LEN);
    strncpy(node->as.assignment.name, name, MAX_TOKEN_LEN - 1);

    if (match(p, TOKEN_AS)) {
        node->as.assignment.value = parse_expression(p);
    } else if (match(p, TOKEN_BY)) {
        /* set r by adding n1 and n2 */
        node->as.assignment.value = parse_expression(p);
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
    collect_name(p, name, MAX_TOKEN_LEN);
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
        return node;
    }

    /* repeat until <condition> */
    if (match(p, TOKEN_UNTIL)) {
        ASTNode *node = ast_make_node(NODE_REPEAT_UNTIL, t.line, t.col);
        node->as.repeat_until.condition = parse_expression(p);
        parse_block(p, &node->as.repeat_until.body);
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
        return node;
    }

    /* Multi-line body */
    parse_block(p, &node->as.activity_def.body);
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

    char name[MAX_TOKEN_LEN] = {0};
    collect_name(p, name, MAX_TOKEN_LEN);
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

/* Top level — checks for inline if */
ASTNode *parse_expression(Parser *p) {
    ASTNode *left = parse_logical(p);

    /* Inline if: <value> if <condition> otherwise <value> */
    if (check(p, TOKEN_IF)) {
        int line = current(p).line;
        int col  = current(p).col;
        advance(p);   /* consume 'if' */

        ASTNode *node = ast_make_node(NODE_INLINE_IF, line, col);
        node->as.inline_if.then_val  = left;
        node->as.inline_if.condition = parse_logical(p);

        expect(p, TOKEN_OTHERWISE, "Expected 'otherwise' in inline if");
        node->as.inline_if.else_val  = parse_logical(p);
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
    return parse_primary(p);
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

    /* Identifier — variable or activity call */
    if (t.type == TOKEN_IDENTIFIER) {
        advance(p);
        ASTNode *node = ast_make_node(NODE_IDENTIFIER, t.line, t.col);
        strncpy(node->as.identifier.name, t.value, MAX_TOKEN_LEN - 1);
        return node;
    }

    /* Keywords used as identifiers in expressions
       e.g. "getting input", "get number", activity calls */
    if (t.type != TOKEN_EOF        &&
        t.type != TOKEN_TERMINATOR &&
        t.type != TOKEN_NEWLINE    &&
        t.type != TOKEN_COMMA      &&
        t.type != TOKEN_RBRACKET   &&
        t.type != TOKEN_RPAREN     &&
        t.type != TOKEN_IF         &&
        t.type != TOKEN_THEN       &&
        t.type != TOKEN_OTHERWISE  &&
        t.type != TOKEN_AND        &&
        t.type != TOKEN_OR         &&
        t.type != TOKEN_PLUS       &&
        t.type != TOKEN_EQUALS     &&
        t.type != TOKEN_IS         &&
        t.type != TOKEN_IS_NOT     &&
        t.type != TOKEN_NOT_EQUALS &&
        t.type != TOKEN_GREATER_THAN &&
        t.type != TOKEN_LESS_THAN  &&
        t.type != TOKEN_AT_LEAST   &&
        t.type != TOKEN_AT_MOST    &&
        t.type != TOKEN_IS_ANY_OF  &&
        t.type != TOKEN_IS_NONE_OF &&
        t.type != TOKEN_IN) {
        advance(p);
        ASTNode *node = ast_make_node(NODE_IDENTIFIER, t.line, t.col);
        strncpy(node->as.identifier.name, t.value, MAX_TOKEN_LEN - 1);
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
