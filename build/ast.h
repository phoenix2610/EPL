#ifndef AST_H
#define AST_H

#include "token.h"

/* ─────────────────────────────────────────────
   EPL AST — ast.h   (v1.1)
   Adds: NODE_COLLECTION_OP, NODE_WHERE_FILTER
   ───────────────────────────────────────────── */

#define MAX_TOKEN_LEN 256
#define MAX_NAME_LEN  256

/* ─────────────────────────────────────────────
   Collection operation enum  (v1.1)
   ───────────────────────────────────────────── */
typedef enum {
    COLLOP_AVERAGE,
    COLLOP_TOTAL,
    COLLOP_MAXIMUM,
    COLLOP_MINIMUM,
    COLLOP_LENGTH,
    COLLOP_SORTED,
    COLLOP_REVERSED
} CollectionOp;

/* ─────────────────────────────────────────────
   String operation enum  (v1.2)
   ───────────────────────────────────────────── */
typedef enum {
    STROP_UPPERCASE,
    STROP_LOWERCASE,
    STROP_TRIMMED,
    STROP_SPLIT,
    STROP_JOINED,
    STROP_REPLACED,
    STROP_STARTS_WITH,
    STROP_ENDS_WITH,
    STROP_CONTAINS
} StringOp;

/* ─────────────────────────────────────────────
   Format operation enum  (v1.2)
   ───────────────────────────────────────────── */
typedef enum {
    FMTOP_CURRENCY,
    FMTOP_PERCENTAGE,
    FMTOP_ROUNDED,
    FMTOP_FORMATTED
} FormatOp;

/* ─────────────────────────────────────────────
   Type check enum  (v1.2)
   ───────────────────────────────────────────── */
typedef enum {
    TYPECHECK_NUMBER,
    TYPECHECK_STRING,
    TYPECHECK_BOOLEAN,
    TYPECHECK_LIST,
    TYPECHECK_NULL
} TypeCheckKind;

/* ─────────────────────────────────────────────
   Chart type  (v1.4)
   ───────────────────────────────────────────── */
typedef enum {
    CHART_LINE,
    CHART_BAR,
    CHART_PIE
} ChartType;

/* ─────────────────────────────────────────────
   EPF theme  (v1.4)
   ───────────────────────────────────────────── */
typedef enum {
    THEME_LIGHT,
    THEME_DARK,
    THEME_INDIGO,
    THEME_EMERALD,
    THEME_ROSE,
    THEME_CUSTOM   /* any hex, css name, or multi-word color */
} EPFTheme;

/* ─────────────────────────────────────────────
   Graph node — adjacency list entry  (v1.3)
   ───────────────────────────────────────────── */
#define EPL_GRAPH_MAX_NODES 256
#define EPL_GRAPH_MAX_EDGES 1024

typedef struct {
    char from[MAX_TOKEN_LEN];
    char to[MAX_TOKEN_LEN];
} GraphEdge;

typedef struct {
    char      nodes[EPL_GRAPH_MAX_NODES][MAX_TOKEN_LEN];
    int       node_count;
    GraphEdge edges[EPL_GRAPH_MAX_EDGES];
    int       edge_count;
} EPLGraph;

/* ─────────────────────────────────────────────
   AST node types
   ───────────────────────────────────────────── */
typedef enum {
    /* Program */
    NODE_PROGRAM,

    /* Literals */
    NODE_NUMBER,
    NODE_STRING,
    NODE_BOOLEAN,
    NODE_NULL,
    NODE_LIST,

    /* Variables */
    NODE_IDENTIFIER,

    /* Statements */
    NODE_SET,
    NODE_UPDATE,
    NODE_SHOW,
    NODE_IF,
    NODE_WHILE,
    NODE_REPEAT_INDEFINITELY,
    NODE_REPEAT_TIMES,
    NODE_REPEAT_UNTIL,
    NODE_FOR_EACH,
    NODE_FOR_RANGE,
    NODE_ACTIVITY_DEF,
    NODE_ACTIVITY_CALL,
    NODE_RETURN,
    NODE_BREAK,
    NODE_CONTINUE,
    NODE_CHECK,
    NODE_DESCRIBE,
    NODE_PROVIDE,
    NODE_ATTEMPT,
    NODE_ADD_TO,
    NODE_WAIT,
    NODE_EXECUTE,

    /* Expressions */
    NODE_BINARY_OP,
    NODE_UNARY_OP,
    NODE_INLINE_IF,

    /* v1.1 — Smart Collections */
    NODE_COLLECTION_OP,

    /* v1.1 — Smart Filter */
    NODE_WHERE_FILTER,

    /* v1.2 — String operations */
    NODE_STRING_OP,

    /* v1.2 — Number formatting */
    NODE_FORMAT_OP,

    /* v1.2 — Type checking */
    NODE_TYPE_CHECK,

    /* v1.3 — Connection / Graph */
    NODE_MAKE_GRAPH,       /* set network as graph                        */
    NODE_LINK,             /* link customer to store in network           */
    NODE_ROUTE,            /* set path as route from X to Y in network   */
    NODE_CONNECTED,        /* set reach as connected to X in network      */

    /* v1.4 — EPF frontend nodes */
    NODE_MAKE_PAGE,        /* make page saved as "x.html" styled as "dark" */
    NODE_SECTION_OUT,      /* section "title" ... !                         */
    NODE_ROW,              /* row ... !                                     */
    NODE_HEADING_OUT,      /* heading "text"                                */
    NODE_SUBHEADING,       /* subheading "text"                             */
    NODE_PARAGRAPH,        /* paragraph "text"                              */
    NODE_LABEL_OUT,        /* label "text"                                  */
    NODE_BOLD_OUT,         /* bold "text"                                   */
    NODE_DIVIDER,          /* divider                                       */
    NODE_SPACER,           /* spacer                                        */
    NODE_MAKE_CHART,       /* make line/bar/pie chart ...                   */
    NODE_MAKE_TABLE_OUT,   /* make table from X titled "Y" with headers [.] */
    NODE_MAKE_REPORT,      /* make report from X titled "Y"                 */
    NODE_MAKE_STAT,        /* make stat "label" from value                  */
    NODE_IMPORT,           /* import data from "file.epdb"                  */

    /* v1.5 — List index access */
    NODE_INDEX_ACCESS      /* item N of LIST                                */

} NodeType;

/* Forward declaration */
typedef struct ASTNode ASTNode;

/* ─────────────────────────────────────────────
   Node list (dynamic array of ASTNode *)
   ───────────────────────────────────────────── */
typedef struct {
    ASTNode **items;
    int       count;
    int       capacity;
} NodeList;

void nodelist_init(NodeList *list);
void nodelist_push(NodeList *list, ASTNode *node);
void nodelist_free(NodeList *list);


/* ─────────────────────────────────────────────
   The ASTNode union
   ───────────────────────────────────────────── */
struct ASTNode {
    NodeType type;
    int      line;
    int      end_line;   /* v1.5 — last line of this node's block */
    int      col;

    union {
        /* NODE_PROGRAM */
        struct { NodeList body; } program;

        /* NODE_NUMBER */
        struct { double value; } number;

        /* NODE_STRING */
        struct { char value[MAX_TOKEN_LEN]; } string;

        /* NODE_BOOLEAN */
        struct { int value; } boolean;

        /* NODE_IDENTIFIER */
        struct { char name[MAX_TOKEN_LEN]; } identifier;

        /* NODE_LIST */
        struct { NodeList elements; } list;

        /* NODE_SET / NODE_UPDATE */
        struct {
            char     name[MAX_TOKEN_LEN];
            ASTNode *value;
        } assignment;

        /* NODE_SHOW */
        struct { ASTNode *value; } show;

        /* NODE_IF */
        struct {
            ASTNode *condition;
            NodeList then_body;
            NodeList else_body;
        } if_stmt;

        /* NODE_WHILE */
        struct {
            ASTNode *condition;
            NodeList body;
        } while_stmt;

        /* NODE_REPEAT_INDEFINITELY */
        struct { NodeList body; } repeat_indefinitely;

        /* NODE_REPEAT_TIMES */
        struct {
            ASTNode *count;
            char     counter[MAX_TOKEN_LEN];
            NodeList body;
        } repeat_times;

        /* NODE_REPEAT_UNTIL */
        struct {
            ASTNode *condition;
            NodeList body;
        } repeat_until;

        /* NODE_FOR_EACH */
        struct {
            char     var[MAX_TOKEN_LEN];
            ASTNode *collection;
            NodeList body;
        } for_each;

        /* NODE_FOR_RANGE */
        struct {
            char     var[MAX_TOKEN_LEN];
            ASTNode *from;
            ASTNode *to;
            int      downward;
            NodeList body;
        } for_range;

        /* NODE_ACTIVITY_DEF */
        struct {
            char     name[MAX_TOKEN_LEN];
            NodeList params;
            NodeList body;
        } activity_def;

        /* NODE_ACTIVITY_CALL */
        struct {
            char     name[MAX_TOKEN_LEN];
            NodeList args;
        } activity_call;

        /* NODE_RETURN */
        struct { ASTNode *value; } return_stmt;

        /* NODE_CHECK */
        struct { ASTNode *condition; } check_stmt;

        /* NODE_DESCRIBE */
        struct {
            char name[MAX_TOKEN_LEN];
            char text[MAX_TOKEN_LEN];
        } describe_stmt;

        /* NODE_PROVIDE */
        struct {
            char     var[MAX_TOKEN_LEN];
            ASTNode *prompt;
        } provide_stmt;

        /* NODE_ATTEMPT */
        struct {
            NodeList try_body;
            NodeList fail_body;
        } attempt_stmt;

        /* NODE_ADD_TO */
        struct {
            ASTNode *item;
            char     list[MAX_TOKEN_LEN];
        } add_to;

        /* NODE_WAIT */
        struct { char message[MAX_TOKEN_LEN]; } wait_stmt;

        /* NODE_EXECUTE */
        struct {
            char command[MAX_TOKEN_LEN];
            char alt_command[MAX_TOKEN_LEN];
            int  conditional;
        } execute_stmt;

        /* NODE_BINARY_OP */
        struct {
            TokenType operator;
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

        /* NODE_COLLECTION_OP  (v1.1)
           average of prices
           total of scores
           sorted items
           reversed items
           length of name        */
        struct {
            CollectionOp  op;
            ASTNode      *collection;
        } collection_op;

        /* NODE_WHERE_FILTER  (v1.1) */
        struct {
            ASTNode *collection;
            char     item_var[MAX_TOKEN_LEN];
            ASTNode *condition;
        } where_filter;

        /* NODE_STRING_OP  (v1.2)
           name in uppercase
           sentence split by " "
           parts joined by ", "
           text with "old" replaced by "new"
           name starts with "Ph"              */
        struct {
            StringOp  op;
            ASTNode  *target;      /* the string/list value          */
            ASTNode  *arg1;        /* delimiter / search / precision  */
            ASTNode  *arg2;        /* replacement (for replaced)      */
        } string_op;

        /* NODE_FORMAT_OP  (v1.2)
           price as currency
           pi rounded to 2                    */
        struct {
            FormatOp  op;
            ASTNode  *target;
            ASTNode  *precision;   /* for rounded to N                */
        } format_op;

        /* NODE_TYPE_CHECK  (v1.2)
           x is a number
           name is a string                   */
        struct {
            TypeCheckKind kind;
            ASTNode      *target;
        } type_check;

        /* NODE_MAKE_GRAPH  (v1.3)
           set network as graph             */
        struct {
            char name[MAX_TOKEN_LEN];       /* unused at parse — name in assignment */
        } make_graph;

        /* NODE_LINK  (v1.3)
           link customer to store in network */
        struct {
            char from[MAX_TOKEN_LEN];
            char to[MAX_TOKEN_LEN];
            char graph[MAX_TOKEN_LEN];
        } link_stmt;

        /* NODE_ROUTE  (v1.3)
           route from customer to owner in network */
        struct {
            char from[MAX_TOKEN_LEN];
            char to[MAX_TOKEN_LEN];
            char graph[MAX_TOKEN_LEN];
        } route_stmt;

        /* NODE_CONNECTED  (v1.3)
           connected to customer in network */
        struct {
            char target[MAX_TOKEN_LEN];
            char graph[MAX_TOKEN_LEN];
        } connected_stmt;

        /* NODE_MAKE_PAGE  (v1.4)
           make page saved as "x.html" styled as "dark" */
        struct {
            char     filename[MAX_TOKEN_LEN];
            EPFTheme theme;
            char     color[MAX_TOKEN_LEN];   /* custom accent color */
            NodeList children;
        } make_page;

        /* NODE_SECTION_OUT  (v1.4) */
        struct {
            char     title[MAX_TOKEN_LEN];
            NodeList children;
        } section_out;

        /* NODE_ROW  (v1.4) */
        struct {
            NodeList children;
        } row_out;

        /* NODE_HEADING_OUT / NODE_SUBHEADING /
           NODE_PARAGRAPH / NODE_LABEL_OUT / NODE_BOLD_OUT  (v1.4) */
        struct {
            ASTNode *value;   /* expression or string literal */
        } text_out;

        /* NODE_MAKE_CHART  (v1.4) */
        struct {
            ChartType type;
            char      title[MAX_TOKEN_LEN];
            char      filename[MAX_TOKEN_LEN];  /* saved as — optional */
            ASTNode  *x_data;    /* list expression for x axis labels */
            ASTNode  *y_data;    /* list expression for y axis values  */
            char      x_label[MAX_TOKEN_LEN];
            char      y_label[MAX_TOKEN_LEN];
        } make_chart;

        /* NODE_MAKE_TABLE_OUT  (v1.4) */
        struct {
            char     title[MAX_TOKEN_LEN];
            char     filename[MAX_TOKEN_LEN];
            ASTNode *data;       /* list of lists or list of values */
            ASTNode *headers;    /* list of strings — column headers */
        } make_table_out;

        /* NODE_MAKE_REPORT  (v1.4) */
        struct {
            char     title[MAX_TOKEN_LEN];
            char     filename[MAX_TOKEN_LEN];
            ASTNode *data;
        } make_report;

        /* NODE_MAKE_STAT  (v1.4) */
        struct {
            char     label[MAX_TOKEN_LEN];
            ASTNode *value;
        } make_stat;

        /* NODE_IMPORT  (v1.4) */
        struct {
            char source[MAX_TOKEN_LEN];
            char alias[MAX_TOKEN_LEN];
        } import_stmt;

        /* NODE_INDEX_ACCESS  (v1.5) — item N of LIST */
        struct {
            ASTNode *list;    /* the collection expression */
            ASTNode *index;   /* the index expression      */
        } index_access;

    } as;
};


/* ─────────────────────────────────────────────
   AST constructor and printer
   ───────────────────────────────────────────── */
ASTNode *ast_make_node(NodeType type, int line, int col);
void     ast_free(ASTNode *node);
void     ast_print(ASTNode *node, int indent);

#endif /* AST_H */
