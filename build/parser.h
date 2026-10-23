#ifndef PARSER_H
#define PARSER_H

#include "token.h"
#include "lexer.h"
#include "ast.h"

typedef struct {
    Token  *tokens;
    int     count;
    int     current;
    int     had_error;
    int     panic_mode;
    int     no_times_op;
} Parser;

void parser_init(Parser *p, Token *tokens, int count);

ASTNode *parser_parse(Parser *p);

ASTNode *parse_statement(Parser *p);
ASTNode *parse_set(Parser *p);
ASTNode *parse_update(Parser *p);
ASTNode *parse_show(Parser *p);
ASTNode *parse_if(Parser *p);
ASTNode *parse_while(Parser *p);
ASTNode *parse_repeat(Parser *p);
ASTNode *parse_for(Parser *p);
ASTNode *parse_activity_def(Parser *p);
ASTNode *parse_import(Parser *p);
ASTNode *parse_db_make_table(Parser *p);
ASTNode *parse_db_save(Parser *p);
ASTNode *parse_db_delete(Parser *p);
ASTNode *parse_return(Parser *p);
ASTNode *parse_check(Parser *p);
ASTNode *parse_describe(Parser *p);
ASTNode *parse_provide(Parser *p);
ASTNode *parse_attempt(Parser *p);
ASTNode *parse_add_to(Parser *p);
ASTNode *parse_wait(Parser *p);
ASTNode *parse_execute(Parser *p);
ASTNode *parse_link(Parser *p);

ASTNode *parse_text_node(Parser *p, NodeType type);
ASTNode *parse_section(Parser *p);
ASTNode *parse_row(Parser *p);
ASTNode *parse_make_stat(Parser *p);
ASTNode *parse_make_chart(Parser *p);
ASTNode *parse_make_table_out(Parser *p);
ASTNode *parse_make_report(Parser *p);
ASTNode *parse_make_page(Parser *p);

ASTNode *parse_node3d(Parser *p);
ASTNode *parse_core_star(Parser *p);
ASTNode *parse_orbital_ring(Parser *p);
ASTNode *parse_canvas3d(Parser *p);
ASTNode *parse_panel3d(Parser *p);
ASTNode *parse_drawer3d(Parser *p);

ASTNode *parse_html_element(Parser *p);

ASTNode *parse_use_design(Parser *p);
ASTNode *parse_make_navbar(Parser *p);
ASTNode *parse_nav_link(Parser *p);
ASTNode *parse_action_btn(Parser *p);
ASTNode *parse_make_hero(Parser *p);
ASTNode *parse_overlay_card(Parser *p);
ASTNode *parse_guest_picker(Parser *p);
ASTNode *parse_date_picker(Parser *p);
ASTNode *parse_content_section(Parser *p);
ASTNode *parse_column(Parser *p, int is_right);
ASTNode *parse_eyebrow(Parser *p);
ASTNode *parse_item_list(Parser *p);
ASTNode *parse_list_entry(Parser *p);
ASTNode *parse_feature_image(Parser *p);
ASTNode *parse_price_badge(Parser *p);
ASTNode *parse_make_gallery(Parser *p);
ASTNode *parse_gallery_image(Parser *p);
ASTNode *parse_make_form(Parser *p);
ASTNode *parse_form_field(Parser *p);
ASTNode *parse_submit_btn(Parser *p);
ASTNode *parse_make_modal(Parser *p);
ASTNode *parse_dismiss_btn(Parser *p);
ASTNode *parse_make_footer(Parser *p);

ASTNode *parse_write_file(Parser *p, NodeType kind);
ASTNode *parse_delete_file(Parser *p);
ASTNode *parse_sleep(Parser *p);
ASTNode *parse_serve_socket(Parser *p);
ASTNode *parse_serve_port(Parser *p);
ASTNode *parse_make_button(Parser *p);
ASTNode *parse_reply_with(Parser *p);

ASTNode *parse_expression(Parser *p);
ASTNode *parse_inline_if(Parser *p);
ASTNode *parse_logical(Parser *p);
ASTNode *parse_comparison(Parser *p);
ASTNode *parse_additive(Parser *p);
ASTNode *parse_multiplicative(Parser *p);
ASTNode *parse_unary(Parser *p);
ASTNode *parse_primary(Parser *p);
ASTNode *parse_list_literal(Parser *p);

void parse_block(Parser *p, NodeList *list);

#endif
