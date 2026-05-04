#ifndef PARSER_H
#define PARSER_H

#include "token.h"
#include "lexer.h"
#include "ast.h"

/* ─────────────────────────────────────────────
   EPL Parser — parser.h
   Takes a token stream and builds an AST.
   ───────────────────────────────────────────── */


/* ═════════════════════════════════════════════
   PARSER STRUCT
   Walks the token array produced by the lexer.
   ═════════════════════════════════════════════ */
typedef struct {
    Token  *tokens;         /* full token array from lexer          */
    int     count;          /* total token count                    */
    int     current;        /* current position in token array      */
    int     had_error;      /* error flag — set on first error      */
    int     panic_mode;     /* panic mode — skip until sync point   */
} Parser;


/* ═════════════════════════════════════════════
   PARSER LIFECYCLE
   ═════════════════════════════════════════════ */

/* Initialize parser with token array */
void parser_init(Parser *p, Token *tokens, int count);

/* Parse entire token stream, return root AST node */
ASTNode *parser_parse(Parser *p);


/* ═════════════════════════════════════════════
   STATEMENT PARSERS
   ═════════════════════════════════════════════ */
ASTNode *parse_statement(Parser *p);
ASTNode *parse_set(Parser *p);
ASTNode *parse_update(Parser *p);
ASTNode *parse_show(Parser *p);
ASTNode *parse_if(Parser *p);
ASTNode *parse_while(Parser *p);
ASTNode *parse_repeat(Parser *p);
ASTNode *parse_for(Parser *p);
ASTNode *parse_activity_def(Parser *p);
ASTNode *parse_return(Parser *p);
ASTNode *parse_check(Parser *p);
ASTNode *parse_describe(Parser *p);
ASTNode *parse_provide(Parser *p);
ASTNode *parse_attempt(Parser *p);
ASTNode *parse_add_to(Parser *p);
ASTNode *parse_wait(Parser *p);
ASTNode *parse_execute(Parser *p);
ASTNode *parse_link(Parser *p);   /* v1.3 */

/* EPF Statement Parsers (v1.4) */
ASTNode *parse_text_node(Parser *p, NodeType type);
ASTNode *parse_section(Parser *p);
ASTNode *parse_row(Parser *p);
ASTNode *parse_make_stat(Parser *p);
ASTNode *parse_make_chart(Parser *p);
ASTNode *parse_make_table_out(Parser *p);
ASTNode *parse_make_report(Parser *p);
ASTNode *parse_make_page(Parser *p);

/* 3D Canvas Parsers (v1.6) */
ASTNode *parse_node3d(Parser *p);
ASTNode *parse_core_star(Parser *p);
ASTNode *parse_orbital_ring(Parser *p);
ASTNode *parse_canvas3d(Parser *p);
ASTNode *parse_panel3d(Parser *p);
ASTNode *parse_drawer3d(Parser *p);
/* ═════════════════════════════════════════════
   EXPRESSION PARSERS
   ═════════════════════════════════════════════ */
ASTNode *parse_expression(Parser *p);
ASTNode *parse_inline_if(Parser *p);
ASTNode *parse_logical(Parser *p);
ASTNode *parse_comparison(Parser *p);
ASTNode *parse_additive(Parser *p);
ASTNode *parse_multiplicative(Parser *p);
ASTNode *parse_unary(Parser *p);
ASTNode *parse_primary(Parser *p);
ASTNode *parse_list_literal(Parser *p);


/* ═════════════════════════════════════════════
   BLOCK PARSER
   Reads statements until TOKEN_TERMINATOR (!)
   ═════════════════════════════════════════════ */
void parse_block(Parser *p, NodeList *list);


#endif /* PARSER_H */
