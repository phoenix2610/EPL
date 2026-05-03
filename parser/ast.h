#ifndef AST_H
#define AST_H

#include "token.h"

/* ─────────────────────────────────────────────
   EPL Parser — ast.h
   Defines every AST node type the parser
   can produce. The interpreter walks this tree.
   ───────────────────────────────────────────── */


/* ═════════════════════════════════════════════
   NODE TYPES
   Every possible node in the EPL syntax tree.
   ═════════════════════════════════════════════ */
typedef enum {

    /* ── PROGRAM ── */
    NODE_PROGRAM,           /* root node, holds all statements     */

    /* ── STATEMENTS ── */
    NODE_SET,               /* set x as 10                         */
    NODE_UPDATE,            /* update x as 20                      */
    NODE_SHOW,              /* show "hello"                        */
    NODE_IF,                /* if ... then ... otherwise ...       */
    NODE_WHILE,             /* while condition                     */
    NODE_REPEAT_TIMES,      /* repeat 5 times                      */
    NODE_REPEAT_UNTIL,      /* repeat until condition              */
    NODE_REPEAT_INDEFINITELY, /* repeat indefinitely               */
    NODE_FOR_EACH,          /* for each item in collection         */
    NODE_FOR_RANGE,         /* for i from 0 to 10                  */
    NODE_ACTIVITY_DEF,      /* to add, return x plus y!            */
    NODE_ACTIVITY_CALL,     /* set r by adding x and y             */
    NODE_RETURN,            /* return value                        */
    NODE_CHECK,             /* check that x is greater than 0      */
    NODE_DESCRIBE,          /* describe add as "..."               */
    NODE_PROVIDE,           /* provide choice as user entry from   */
    NODE_ATTEMPT,           /* attempt ... if fails ...            */
    NODE_BREAK,             /* break                               */
    NODE_CONTINUE,          /* continue                            */
    NODE_ADD_TO,            /* add item to list                    */
    NODE_WAIT,              /* wait for enter key                  */
    NODE_EXECUTE,           /* execute system command "cls"        */

    /* ── EXPRESSIONS ── */
    NODE_NUMBER,            /* 42, 19.99                           */
    NODE_STRING,            /* "hello"                             */
    NODE_BOOLEAN,           /* true, false                         */
    NODE_NULL,              /* null                                */
    NODE_IDENTIFIER,        /* salary, name, x                     */
    NODE_LIST,              /* [1, 2, 3]                           */
    NODE_BINARY_OP,         /* x plus y, x greater than y          */
    NODE_UNARY_OP,          /* not x                               */
    NODE_INLINE_IF,         /* "Pass" if score >= 60 otherwise "Fail" */
    NODE_INDEX,             /* item 0 from list                    */

} NodeType;


/* ═════════════════════════════════════════════
   FORWARD DECLARATION
   ASTNode references itself for children.
   ═════════════════════════════════════════════ */
typedef struct ASTNode ASTNode;


/* ═════════════════════════════════════════════
   NODE LIST
   Dynamic array of ASTNode pointers.
   Used for statement blocks, argument lists etc.
   ═════════════════════════════════════════════ */
typedef struct {
    ASTNode **items;
    int       count;
    int       capacity;
} NodeList;


/* ═════════════════════════════════════════════
   AST NODE
   One struct covers all node types via a union.
   The 'type' field tells you which union member
   is active.
   ═════════════════════════════════════════════ */
struct ASTNode {
    NodeType type;
    int      line;      /* source line for error reporting  */
    int      col;       /* source col  for error reporting  */

    union {

        /* NODE_PROGRAM */
        struct {
            NodeList body;          /* all top-level statements     */
        } program;

        /* NODE_SET / NODE_UPDATE */
        struct {
            char     name[MAX_TOKEN_LEN];   /* variable name        */
            ASTNode *value;                 /* expression           */
        } assignment;

        /* NODE_SHOW */
        struct {
            ASTNode *value;         /* expression to display        */
        } show;

        /* NODE_IF */
        struct {
            ASTNode *condition;     /* the test expression          */
            NodeList then_body;     /* statements if true           */
            NodeList else_body;     /* statements if false          */
        } if_stmt;

        /* NODE_WHILE */
        struct {
            ASTNode *condition;
            NodeList body;
        } while_stmt;

        /* NODE_REPEAT_TIMES */
        struct {
            ASTNode *count;         /* how many times               */
            char     counter[MAX_TOKEN_LEN]; /* optional "with i"   */
            NodeList body;
        } repeat_times;

        /* NODE_REPEAT_UNTIL */
        struct {
            ASTNode *condition;
            NodeList body;
        } repeat_until;

        /* NODE_REPEAT_INDEFINITELY */
        struct {
            NodeList body;
        } repeat_indefinitely;

        /* NODE_FOR_EACH */
        struct {
            char     var[MAX_TOKEN_LEN];    /* loop variable        */
            ASTNode *collection;            /* what to iterate      */
            NodeList body;
        } for_each;

        /* NODE_FOR_RANGE */
        struct {
            char     var[MAX_TOKEN_LEN];    /* loop variable        */
            ASTNode *from;                  /* start value          */
            ASTNode *to;                    /* end value            */
            int      downward;             /* "down to" flag        */
            NodeList body;
        } for_range;

        /* NODE_ACTIVITY_DEF */
        struct {
            char     name[MAX_TOKEN_LEN];
            NodeList params;        /* parameter name nodes         */
            NodeList body;
        } activity_def;

        /* NODE_ACTIVITY_CALL */
        struct {
            char     name[MAX_TOKEN_LEN];   /* activity name        */
            NodeList args;                  /* argument expressions */
        } activity_call;

        /* NODE_RETURN */
        struct {
            ASTNode *value;         /* expression to return         */
        } return_stmt;

        /* NODE_CHECK */
        struct {
            ASTNode *condition;     /* check that <condition>       */
        } check_stmt;

        /* NODE_DESCRIBE */
        struct {
            char     name[MAX_TOKEN_LEN];   /* what is described    */
            char     text[MAX_TOKEN_LEN];   /* description string   */
        } describe_stmt;

        /* NODE_PROVIDE */
        struct {
            char     var[MAX_TOKEN_LEN];    /* variable to store in */
            ASTNode *prompt;               /* prompt string         */
        } provide_stmt;

        /* NODE_ATTEMPT */
        struct {
            NodeList try_body;      /* code to attempt              */
            NodeList fail_body;     /* code if fails                */
        } attempt_stmt;

        /* NODE_ADD_TO */
        struct {
            ASTNode *item;          /* what to add                  */
            char     list[MAX_TOKEN_LEN];   /* list name            */
        } add_to;

        /* NODE_WAIT */
        struct {
            char message[MAX_TOKEN_LEN];    /* prompt message       */
        } wait_stmt;

        /* NODE_EXECUTE */
        struct {
            char     command[MAX_TOKEN_LEN];    /* command string   */
            int      conditional;               /* has if windows?  */
            char     alt_command[MAX_TOKEN_LEN];/* otherwise cmd    */
        } execute_stmt;

        /* NODE_NUMBER */
        struct {
            double value;
        } number;

        /* NODE_STRING */
        struct {
            char value[MAX_TOKEN_LEN];
        } string;

        /* NODE_BOOLEAN */
        struct {
            int value;              /* 1 = true, 0 = false          */
        } boolean;

        /* NODE_IDENTIFIER */
        struct {
            char name[MAX_TOKEN_LEN];
        } identifier;

        /* NODE_LIST */
        struct {
            NodeList elements;
        } list;

        /* NODE_BINARY_OP */
        struct {
            TokenType operator;     /* which operation              */
            ASTNode  *left;
            ASTNode  *right;
        } binary_op;

        /* NODE_UNARY_OP */
        struct {
            TokenType operator;
            ASTNode  *operand;
        } unary_op;

        /* NODE_INLINE_IF */
        struct {
            ASTNode *condition;
            ASTNode *then_val;
            ASTNode *else_val;
        } inline_if;

        /* NODE_INDEX */
        struct {
            ASTNode *index;
            char     collection[MAX_TOKEN_LEN];
        } index_expr;

    } as;
};


/* ═════════════════════════════════════════════
   NODE LIST FUNCTIONS
   ═════════════════════════════════════════════ */
void nodelist_init(NodeList *list);
void nodelist_push(NodeList *list, ASTNode *node);
void nodelist_free(NodeList *list);


/* ═════════════════════════════════════════════
   AST NODE FUNCTIONS
   ═════════════════════════════════════════════ */

/* Allocate a new node of given type */
ASTNode *ast_make_node(NodeType type, int line, int col);

/* Recursively free an entire AST */
void ast_free(ASTNode *node);

/* Print AST for debugging (indented tree view) */
void ast_print(ASTNode *node, int indent);

/* Get human readable name for a node type */
const char *ast_node_type_name(NodeType type);


#endif /* AST_H */
