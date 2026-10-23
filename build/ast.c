#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ast.h"

void nodelist_init(NodeList *list) {
    list->items    = NULL;
    list->count    = 0;
    list->capacity = 0;
}

void nodelist_push(NodeList *list, ASTNode *node) {
    if (list->count >= list->capacity) {
        list->capacity = list->capacity == 0 ? 8 : list->capacity * 2;
        list->items = realloc(list->items, list->capacity * sizeof(ASTNode *));
    }
    list->items[list->count++] = node;
}

void nodelist_free(NodeList *list) {
    for (int i = 0; i < list->count; i++)
        ast_free(list->items[i]);
    free(list->items);
    list->items    = NULL;
    list->count    = 0;
    list->capacity = 0;
}

void ast_str_set(char *dst, size_t cap, const char *src, const char *field) {
    if (!src) { dst[0] = '\0'; return; }
    size_t n = strlen(src);
    if (n >= cap) {
        fprintf(stderr,
            "[EPL Warning] value truncated to %zu chars in %s (was %zu)\n",
            cap - 1, field, n);
    }
    strncpy(dst, src, cap - 1);
    dst[cap - 1] = '\0';
}

#include <stddef.h>
#include "ast_sizes.inc"

ASTNode *ast_make_node(NodeType type, int line, int col) {
    size_t size = ((unsigned)type < AST_NODE_SIZE_COUNT) ? AST_NODE_SIZE[type]
                                                         : sizeof(ASTNode);
    ASTNode *node = calloc(1, size);
    if (!node) {
        fprintf(stderr, "EPL: out of memory allocating AST node\n");
        exit(1);
    }
    node->type     = type;
    node->line     = line;
    node->end_line = line;
    node->col      = col;
    return node;
}

const char *ast_node_type_name(NodeType type) {
    switch (type) {
        case NODE_PROGRAM:              return "PROGRAM";
        case NODE_SET:                  return "SET";
        case NODE_UPDATE:               return "UPDATE";
        case NODE_SHOW:                 return "SHOW";
        case NODE_IF:                   return "IF";
        case NODE_WHILE:                return "WHILE";
        case NODE_REPEAT_TIMES:         return "REPEAT_TIMES";
        case NODE_REPEAT_UNTIL:         return "REPEAT_UNTIL";
        case NODE_REPEAT_INDEFINITELY:  return "REPEAT_INDEFINITELY";
        case NODE_FOR_EACH:             return "FOR_EACH";
        case NODE_FOR_RANGE:            return "FOR_RANGE";
        case NODE_ACTIVITY_DEF:         return "ACTIVITY_DEF";
        case NODE_ACTIVITY_CALL:        return "ACTIVITY_CALL";
        case NODE_RETURN:               return "RETURN";
        case NODE_CHECK:                return "CHECK";
        case NODE_DESCRIBE:             return "DESCRIBE";
        case NODE_PROVIDE:              return "PROVIDE";
        case NODE_ATTEMPT:              return "ATTEMPT";
        case NODE_BREAK:                return "BREAK";
        case NODE_CONTINUE:             return "CONTINUE";
        case NODE_ADD_TO:               return "ADD_TO";
        case NODE_WAIT:                 return "WAIT";
        case NODE_EXECUTE:              return "EXECUTE";
        case NODE_NUMBER:               return "NUMBER";
        case NODE_STRING:               return "STRING";
        case NODE_BOOLEAN:              return "BOOLEAN";
        case NODE_NULL:                 return "NULL";
        case NODE_IDENTIFIER:           return "IDENTIFIER";
        case NODE_LIST:                 return "LIST";
        case NODE_BINARY_OP:            return "BINARY_OP";
        case NODE_UNARY_OP:             return "UNARY_OP";
        case NODE_INLINE_IF:            return "INLINE_IF";
        case NODE_COLLECTION_OP:        return "COLLECTION_OP";
        case NODE_WHERE_FILTER:         return "WHERE_FILTER";
        case NODE_SERVE_PORT:           return "SERVE_PORT";
        case NODE_ACTION_FORM:          return "ACTION_FORM";
        case NODE_ACTION_BUTTON:        return "ACTION_BUTTON";
        case NODE_ACTION_INPUT:         return "ACTION_INPUT";
        default:                        return "UNKNOWN_NODE";
    }
}

void ast_free(ASTNode *node) {
    if (!node) return;

    switch (node->type) {
        case NODE_PROGRAM:
            nodelist_free(&node->as.program.body);
            break;
        case NODE_SET:
        case NODE_UPDATE:
            ast_free(node->as.assignment.value);
            break;
        case NODE_SHOW:
            ast_free(node->as.show.value);
            break;
        case NODE_IF:
            ast_free(node->as.if_stmt.condition);
            nodelist_free(&node->as.if_stmt.then_body);
            nodelist_free(&node->as.if_stmt.else_body);
            break;
        case NODE_WHILE:
            ast_free(node->as.while_stmt.condition);
            nodelist_free(&node->as.while_stmt.body);
            break;
        case NODE_REPEAT_TIMES:
            ast_free(node->as.repeat_times.count);
            nodelist_free(&node->as.repeat_times.body);
            break;
        case NODE_REPEAT_UNTIL:
            ast_free(node->as.repeat_until.condition);
            nodelist_free(&node->as.repeat_until.body);
            break;
        case NODE_REPEAT_INDEFINITELY:
            nodelist_free(&node->as.repeat_indefinitely.body);
            break;
        case NODE_FOR_EACH:
            ast_free(node->as.for_each.collection);
            nodelist_free(&node->as.for_each.body);
            break;
        case NODE_FOR_RANGE:
            ast_free(node->as.for_range.from);
            ast_free(node->as.for_range.to);
            nodelist_free(&node->as.for_range.body);
            break;
        case NODE_ACTIVITY_DEF:
            nodelist_free(&node->as.activity_def.params);
            nodelist_free(&node->as.activity_def.body);
            break;
        case NODE_ACTIVITY_CALL:
            nodelist_free(&node->as.activity_call.args);
            break;
        case NODE_RETURN:
            ast_free(node->as.return_stmt.value);
            break;
        case NODE_CHECK:
            ast_free(node->as.check_stmt.condition);
            break;
        case NODE_PROVIDE:
            ast_free(node->as.provide_stmt.prompt);
            break;
        case NODE_ATTEMPT:
            nodelist_free(&node->as.attempt_stmt.try_body);
            nodelist_free(&node->as.attempt_stmt.fail_body);
            break;
        case NODE_ADD_TO:
            ast_free(node->as.add_to.item);
            break;
        case NODE_LIST:
            nodelist_free(&node->as.list.elements);
            break;
        case NODE_BINARY_OP:
            ast_free(node->as.binary_op.left);
            ast_free(node->as.binary_op.right);
            break;
        case NODE_UNARY_OP:
            ast_free(node->as.unary_op.operand);
            break;
        case NODE_INLINE_IF:
            ast_free(node->as.inline_if.condition);
            ast_free(node->as.inline_if.then_val);
            ast_free(node->as.inline_if.else_val);
            break;
        case NODE_UPDATE_PART:
            ast_free(node->as.update_part.selector);
            ast_free(node->as.update_part.value);
            break;
        case NODE_SERVE_PORT:
            ast_free(node->as.serve_port.port_expr);
            nodelist_free(&node->as.serve_port.offered);
            break;
        case NODE_ACTION_FORM:
        case NODE_ACTION_BUTTON:
            nodelist_free(&node->as.action_form.inputs);
            break;
        default:
            break;
    }
    free(node);
}

static void indent(int level) {
    for (int i = 0; i < level; i++) printf("  ");
}

void ast_print(ASTNode *node, int depth) {
    if (!node) return;
    indent(depth);

    switch (node->type) {

        case NODE_PROGRAM:
            printf("[PROGRAM] (%d statements)\n", node->as.program.body.count);
            for (int i = 0; i < node->as.program.body.count; i++)
                ast_print(node->as.program.body.items[i], depth + 1);
            break;

        case NODE_SET:
            printf("[SET] \"%s\"\n", node->as.assignment.name);
            ast_print(node->as.assignment.value, depth + 1);
            break;

        case NODE_UPDATE:
            printf("[UPDATE] \"%s\"\n", node->as.assignment.name);
            ast_print(node->as.assignment.value, depth + 1);
            break;

        case NODE_SHOW:
            printf("[SHOW]\n");
            ast_print(node->as.show.value, depth + 1);
            break;

        case NODE_IF:
            printf("[IF]\n");
            indent(depth + 1); printf("condition:\n");
            ast_print(node->as.if_stmt.condition, depth + 2);
            indent(depth + 1); printf("then (%d stmts):\n",
                node->as.if_stmt.then_body.count);
            for (int i = 0; i < node->as.if_stmt.then_body.count; i++)
                ast_print(node->as.if_stmt.then_body.items[i], depth + 2);
            if (node->as.if_stmt.else_body.count > 0) {
                indent(depth + 1); printf("otherwise (%d stmts):\n",
                    node->as.if_stmt.else_body.count);
                for (int i = 0; i < node->as.if_stmt.else_body.count; i++)
                    ast_print(node->as.if_stmt.else_body.items[i], depth + 2);
            }
            break;

        case NODE_WHILE:
            printf("[WHILE]\n");
            indent(depth + 1); printf("condition:\n");
            ast_print(node->as.while_stmt.condition, depth + 2);
            indent(depth + 1); printf("body (%d stmts):\n",
                node->as.while_stmt.body.count);
            for (int i = 0; i < node->as.while_stmt.body.count; i++)
                ast_print(node->as.while_stmt.body.items[i], depth + 2);
            break;

        case NODE_REPEAT_TIMES:
            printf("[REPEAT_TIMES] counter=\"%s\"\n",
                node->as.repeat_times.counter);
            indent(depth + 1); printf("count:\n");
            ast_print(node->as.repeat_times.count, depth + 2);
            indent(depth + 1); printf("body (%d stmts):\n",
                node->as.repeat_times.body.count);
            for (int i = 0; i < node->as.repeat_times.body.count; i++)
                ast_print(node->as.repeat_times.body.items[i], depth + 2);
            break;

        case NODE_REPEAT_INDEFINITELY:
            printf("[REPEAT_INDEFINITELY]\n");
            for (int i = 0; i < node->as.repeat_indefinitely.body.count; i++)
                ast_print(node->as.repeat_indefinitely.body.items[i], depth+1);
            break;

        case NODE_REPEAT_UNTIL:
            printf("[REPEAT_UNTIL]\n");
            indent(depth + 1); printf("condition:\n");
            ast_print(node->as.repeat_until.condition, depth + 2);
            for (int i = 0; i < node->as.repeat_until.body.count; i++)
                ast_print(node->as.repeat_until.body.items[i], depth + 1);
            break;

        case NODE_FOR_EACH:
            printf("[FOR_EACH] var=\"%s\"\n", node->as.for_each.var);
            indent(depth + 1); printf("collection:\n");
            ast_print(node->as.for_each.collection, depth + 2);
            indent(depth + 1); printf("body (%d stmts):\n",
                node->as.for_each.body.count);
            for (int i = 0; i < node->as.for_each.body.count; i++)
                ast_print(node->as.for_each.body.items[i], depth + 2);
            break;

        case NODE_FOR_RANGE:
            printf("[FOR_RANGE] var=\"%s\" %s\n",
                node->as.for_range.var,
                node->as.for_range.downward ? "(down)" : "");
            indent(depth + 1); printf("from:\n");
            ast_print(node->as.for_range.from, depth + 2);
            indent(depth + 1); printf("to:\n");
            ast_print(node->as.for_range.to, depth + 2);
            indent(depth + 1); printf("body (%d stmts):\n",
                node->as.for_range.body.count);
            for (int i = 0; i < node->as.for_range.body.count; i++)
                ast_print(node->as.for_range.body.items[i], depth + 2);
            break;

        case NODE_ACTIVITY_DEF:
            printf("[ACTIVITY_DEF] name=\"%s\" params=%d\n",
                node->as.activity_def.name,
                node->as.activity_def.params.count);
            indent(depth + 1); printf("body (%d stmts):\n",
                node->as.activity_def.body.count);
            for (int i = 0; i < node->as.activity_def.body.count; i++)
                ast_print(node->as.activity_def.body.items[i], depth + 2);
            break;

        case NODE_ACTIVITY_CALL:
            printf("[ACTIVITY_CALL] name=\"%s\" args=%d\n",
                node->as.activity_call.name,
                node->as.activity_call.args.count);
            for (int i = 0; i < node->as.activity_call.args.count; i++)
                ast_print(node->as.activity_call.args.items[i], depth + 1);
            break;

        case NODE_RETURN:
            printf("[RETURN]\n");
            ast_print(node->as.return_stmt.value, depth + 1);
            break;

        case NODE_CHECK:
            printf("[CHECK]\n");
            ast_print(node->as.check_stmt.condition, depth + 1);
            break;

        case NODE_DESCRIBE:
            printf("[DESCRIBE] \"%s\" → \"%s\"\n",
                node->as.describe_stmt.name,
                node->as.describe_stmt.text);
            break;

        case NODE_PROVIDE:
            printf("[PROVIDE] var=\"%s\"\n", node->as.provide_stmt.var);
            ast_print(node->as.provide_stmt.prompt, depth + 1);
            break;

        case NODE_ATTEMPT:
            printf("[ATTEMPT]\n");
            indent(depth + 1); printf("try (%d stmts):\n",
                node->as.attempt_stmt.try_body.count);
            for (int i = 0; i < node->as.attempt_stmt.try_body.count; i++)
                ast_print(node->as.attempt_stmt.try_body.items[i], depth + 2);
            if (node->as.attempt_stmt.fail_body.count > 0) {
                indent(depth + 1); printf("fails (%d stmts):\n",
                    node->as.attempt_stmt.fail_body.count);
                for (int i = 0; i < node->as.attempt_stmt.fail_body.count; i++)
                    ast_print(node->as.attempt_stmt.fail_body.items[i], depth+2);
            }
            break;

        case NODE_BREAK:
            printf("[BREAK]\n");
            break;

        case NODE_CONTINUE:
            printf("[CONTINUE]\n");
            break;

        case NODE_ADD_TO:
            printf("[ADD_TO] list=\"%s\"\n", node->as.add_to.list);
            ast_print(node->as.add_to.item, depth + 1);
            break;

        case NODE_WAIT:
            printf("[WAIT] message=\"%s\"\n", node->as.wait_stmt.message);
            break;

        case NODE_EXECUTE:
            printf("[EXECUTE] cmd=\"%s\"%s\n",
                node->as.execute_stmt.command,
                node->as.execute_stmt.conditional ? " (conditional)" : "");
            if (node->as.execute_stmt.conditional)
                indent(depth+1),
                printf("alt=\"%s\"\n", node->as.execute_stmt.alt_command);
            break;

        case NODE_NUMBER:
            printf("[NUMBER] %g\n", node->as.number.value);
            break;

        case NODE_STRING:
            printf("[STRING] \"%s\"\n", node->as.string.value);
            break;

        case NODE_BOOLEAN:
            printf("[BOOLEAN] %s\n", node->as.boolean.value ? "true" : "false");
            break;

        case NODE_NULL:
            printf("[NULL]\n");
            break;

        case NODE_IDENTIFIER:
            printf("[IDENTIFIER] \"%s\"\n", node->as.identifier.name);
            break;

        case NODE_LIST:
            printf("[LIST] (%d elements)\n", node->as.list.elements.count);
            for (int i = 0; i < node->as.list.elements.count; i++)
                ast_print(node->as.list.elements.items[i], depth + 1);
            break;

        case NODE_BINARY_OP:
            printf("[BINARY_OP] op=%s\n", token_type_name(node->as.binary_op.operator));
            indent(depth + 1); printf("left:\n");
            ast_print(node->as.binary_op.left,  depth + 2);
            indent(depth + 1); printf("right:\n");
            ast_print(node->as.binary_op.right, depth + 2);
            break;

        case NODE_UNARY_OP:
            printf("[UNARY_OP] op=%s\n", token_type_name(node->as.unary_op.operator));
            ast_print(node->as.unary_op.operand, depth + 1);
            break;

        case NODE_INLINE_IF:
            printf("[INLINE_IF]\n");
            indent(depth + 1); printf("condition:\n");
            ast_print(node->as.inline_if.condition, depth + 2);
            indent(depth + 1); printf("then:\n");
            ast_print(node->as.inline_if.then_val,  depth + 2);
            indent(depth + 1); printf("otherwise:\n");
            ast_print(node->as.inline_if.else_val,  depth + 2);
            break;

        case NODE_COLLECTION_OP:
            printf("[COLLECTION_OP] op=%d\n", node->as.collection_op.op);
            indent(depth + 1); printf("collection:\n");
            ast_print(node->as.collection_op.collection, depth + 2);
            break;

        case NODE_WHERE_FILTER:
            printf("[WHERE_FILTER] item_var=\"%s\"\n",
                node->as.where_filter.item_var);
            indent(depth + 1); printf("collection:\n");
            ast_print(node->as.where_filter.collection, depth + 2);
            indent(depth + 1); printf("condition:\n");
            ast_print(node->as.where_filter.condition, depth + 2);
            break;

        case NODE_SERVE_PORT:
            printf("[SERVE_PORT] offering %d\n", node->as.serve_port.offered.count);
            ast_print(node->as.serve_port.port_expr, depth + 1);
            for (int i = 0; i < node->as.serve_port.offered.count; i++)
                ast_print(node->as.serve_port.offered.items[i], depth + 1);
            break;

        case NODE_ACTION_FORM:
        case NODE_ACTION_BUTTON:
            printf("[%s] for \"%s\" label=\"%s\" after=%d\n",
                ast_node_type_name(node->type), node->as.action_form.activity,
                node->as.action_form.label, node->as.action_form.after);
            for (int i = 0; i < node->as.action_form.inputs.count; i++)
                ast_print(node->as.action_form.inputs.items[i], depth + 1);
            break;

        case NODE_ACTION_INPUT:
            printf("[ACTION_INPUT] \"%s\" label=\"%s\"\n",
                node->as.action_input.name, node->as.action_input.label);
            break;

        default:
            printf("[UNKNOWN NODE type=%d]\n", node->type);
            break;
    }
}
