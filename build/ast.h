#ifndef AST_H
#define AST_H

#include "token.h"

/* ─────────────────────────────────────────────
   EPL AST — ast.h   (v1.1)
   Adds: NODE_COLLECTION_OP, NODE_WHERE_FILTER
   ───────────────────────────────────────────── */

#define MAX_TOKEN_LEN 4096
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
   3D Background presets  (v1.6)
   ───────────────────────────────────────────── */
typedef enum {
    BG3D_DEEP_SPACE,    /* dark blue/purple star field  */
    BG3D_VOID,          /* pure black minimal           */
    BG3D_NEBULA,        /* colorful particle clouds     */
    BG3D_CUSTOM         /* custom hex color             */
} Background3D;

/* ─────────────────────────────────────────────
   Node status (mirrors interpreter status)
   ───────────────────────────────────────────── */
typedef enum {
    NODE3D_COMPLETED,
    NODE3D_INPROGRESS,
    NODE3D_OVERDUE,
    NODE3D_PENDING
} Node3DStatus;

/* ─────────────────────────────────────────────
   Panel / Drawer position
   ───────────────────────────────────────────── */
typedef enum {
    PANEL_TOP, PANEL_RIGHT, PANEL_BOTTOM, PANEL_LEFT
} PanelPosition;

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
    NODE_INDEX_ACCESS,     /* item N of LIST                                */

    /* v1.6 — 3D Canvas */
    NODE_CANVAS3D,         /* canvas3d "name" background "x" ... ! */
    NODE_ORBITAL_RING,     /* orbital ring radius N color "x" speed N ... ! */
    NODE_NODE3D,           /* node at N labeled "x" status "x" */
    NODE_CORE_STAR,        /* core star color "x" labeled "x" */
    NODE_FROM_GRAPH,       /* from graph <varname> */
    NODE_PANEL3D,          /* panel "position" ... ! */
    NODE_DRAWER3D,         /* drawer "position" titled "x" ... ! */

    /* v1.7 — HTML/DOM Interpreter */
    NODE_HTML_ELEMENT,     /* generic html element */

    /* v2.0 — EPF High-Level Component System */
    NODE_USE_DESIGN,       /* use design system "name" ... !               */
    NODE_MAKE_NAVBAR,      /* make navbar branded as "X" styled as "Y" ... ! */
    NODE_NAV_LINK,         /* nav link "label" at "href"                   */
    NODE_ACTION_BTN,       /* action button "label" on click open modal "id" */
    NODE_MAKE_HERO,        /* make hero titled "X" accented "Y" ... !      */
    NODE_OVERLAY_CARD,     /* overlay card "title" ... !                   */
    NODE_GUEST_PICKER,     /* guest picker default N                       */
    NODE_DATE_PICKER,      /* date picker label "X"                        */
    NODE_CONTENT_SECTION,  /* make content section id "X" ... !            */
    NODE_COLUMN,           /* left column / right column ... !             */
    NODE_EYEBROW,          /* eyebrow "text"                               */
    NODE_ITEM_LIST,        /* item list ... !                              */
    NODE_LIST_ENTRY,       /* entry "label" numbered N detail "x" [offset] */
    NODE_FEATURE_IMAGE,    /* feature image "src" aspect "X" hover "Y"    */
    NODE_PRICE_BADGE,      /* price badge "$X" label "Y"                  */
    NODE_MAKE_GALLERY,     /* make gallery titled "X" columns N ... !      */
    NODE_GALLERY_IMAGE,    /* image "src" [offset top N]                  */
    NODE_MAKE_FORM,        /* make form "title" eyebrow "X" ... !         */
    NODE_FORM_FIELD,       /* field text/email/textarea "label" placeholder "x" */
    NODE_SUBMIT_BTN,       /* submit button "label" on click open modal "id" */
    NODE_MAKE_MODAL,       /* make modal id "x" icon "y" color "z" ... !  */
    NODE_DISMISS_BTN,      /* dismiss button "label"                       */
    NODE_MAKE_FOOTER,      /* make footer branded as "X" ... !            */

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

        /* NODE_CANVAS3D  (v1.6) */
        struct {
            char         name[MAX_TOKEN_LEN];
            Background3D background;
            char         bg_color[MAX_TOKEN_LEN];
            int          offline;        /* 1 = embed JS inline */
            NodeList     children;       /* orbital rings, core, from_graph */
        } canvas3d;

        /* NODE_ORBITAL_RING  (v1.6) */
        struct {
            double       radius;
            char         color[MAX_TOKEN_LEN];
            double       speed;
            double       tilt_x;
            double       tilt_z;
            NodeList     nodes;          /* NODE_NODE3D children */
        } orbital_ring;

        /* NODE_NODE3D  (v1.6) */
        struct {
            double       angle;          /* position on ring 0..2pi */
            char         label[MAX_TOKEN_LEN];
            Node3DStatus status;
            char         desc[MAX_TOKEN_LEN];
        } node3d;

        /* NODE_CORE_STAR  (v1.6) */
        struct {
            char color[MAX_TOKEN_LEN];
            char label[MAX_TOKEN_LEN];
            char desc[MAX_TOKEN_LEN];
        } core_star;

        /* NODE_FROM_GRAPH  (v1.6) */
        struct {
            char graph_var[MAX_TOKEN_LEN];
        } from_graph;

        /* NODE_PANEL3D  (v1.6) */
        struct {
            PanelPosition position;
            NodeList      children;      /* heading, stat, etc. */
        } panel3d;

        /* NODE_DRAWER3D  (v1.6) */
        struct {
            PanelPosition position;
            char          title[MAX_TOKEN_LEN];
            NodeList      children;
        } drawer3d;

        /* NODE_HTML_ELEMENT  (v1.7) */
        struct {
            char     tag[MAX_TOKEN_LEN];
            char     id_attr[MAX_TOKEN_LEN];
            char     class_attr[MAX_TOKEN_LEN];
            char     src_attr[MAX_TOKEN_LEN];
            char     type_attr[MAX_TOKEN_LEN];
            char     onclick_attr[MAX_TOKEN_LEN];
            char     placeholder_attr[MAX_TOKEN_LEN];
            char     value_attr[MAX_TOKEN_LEN];
            int      is_self_closing;
            ASTNode *text_content;  /* optional inner text expression */
            NodeList children;
        } html_element;

        /* ── v2.0 EPF Component Structs ── */

        /* NODE_USE_DESIGN */
        struct {
            char name[MAX_TOKEN_LEN];       /* "ethereal oasis" etc. */
            char primary[MAX_TOKEN_LEN];    /* hex or css color */
            char secondary[MAX_TOKEN_LEN];
            char background[MAX_TOKEN_LEN];
            char headline_font[MAX_TOKEN_LEN];
            char body_font[MAX_TOKEN_LEN];
            char icon_set[MAX_TOKEN_LEN];   /* "Material Symbols" */
        } use_design;

        /* NODE_MAKE_NAVBAR */
        struct {
            char     brand[MAX_TOKEN_LEN];  /* "Green Desert" */
            char     style[MAX_TOKEN_LEN];  /* "frosted" etc. */
            NodeList children;              /* NODE_NAV_LINK, NODE_ACTION_BTN */
        } make_navbar;

        /* NODE_NAV_LINK */
        struct {
            char label[MAX_TOKEN_LEN];
            char href[MAX_TOKEN_LEN];
        } nav_link;

        /* NODE_ACTION_BTN */
        struct {
            char label[MAX_TOKEN_LEN];
            char modal_id[MAX_TOKEN_LEN];   /* from "on click open modal X" */
        } action_btn;

        /* NODE_MAKE_HERO */
        struct {
            char     title[MAX_TOKEN_LEN];
            char     accented[MAX_TOKEN_LEN];   /* word to italicize */
            char     subtitle[MAX_TOKEN_LEN];
            char     bg_image[MAX_TOKEN_LEN];
            int      bg_dim;                    /* 0-100 */
            ASTNode *overlay_card;              /* optional NODE_OVERLAY_CARD */
        } make_hero;

        /* NODE_OVERLAY_CARD */
        struct {
            char     title[MAX_TOKEN_LEN];
            NodeList children;              /* pickers, action btns */
        } overlay_card;

        /* NODE_GUEST_PICKER */
        struct {
            int default_count;              /* e.g. 2 */
        } guest_picker;

        /* NODE_DATE_PICKER */
        struct {
            char label[MAX_TOKEN_LEN];      /* "Select Evening" */
        } date_picker;

        /* NODE_CONTENT_SECTION */
        struct {
            char     id[MAX_TOKEN_LEN];
            ASTNode *left_col;
            ASTNode *right_col;
        } content_section;

        /* NODE_COLUMN */
        struct {
            int      is_right;   /* 0=left, 1=right */
            NodeList children;
        } column;

        /* NODE_EYEBROW */
        struct {
            char text[MAX_TOKEN_LEN];
        } eyebrow;

        /* NODE_ITEM_LIST */
        struct {
            NodeList entries;   /* NODE_LIST_ENTRY children */
        } item_list;

        /* NODE_LIST_ENTRY */
        struct {
            char label[MAX_TOKEN_LEN];
            int  number;            /* display number e.g. 01 */
            char detail[MAX_TOKEN_LEN];
            int  offset;            /* 1 = apply md:translate-x-12 */
        } list_entry;

        /* NODE_FEATURE_IMAGE */
        struct {
            char src[MAX_TOKEN_LEN];
            char aspect[MAX_TOKEN_LEN];     /* "4/5" "16/9" */
            char hover_mode[MAX_TOKEN_LEN]; /* "reveal color" "zoom" */
        } feature_image;

        /* NODE_PRICE_BADGE */
        struct {
            char price[MAX_TOKEN_LEN];  /* "$285" */
            char label[MAX_TOKEN_LEN];  /* "per guest" */
        } price_badge;

        /* NODE_MAKE_GALLERY */
        struct {
            char     title[MAX_TOKEN_LEN];
            char     subtitle[MAX_TOKEN_LEN];
            int      columns;
            NodeList images;    /* NODE_GALLERY_IMAGE children */
        } make_gallery;

        /* NODE_GALLERY_IMAGE */
        struct {
            char src[MAX_TOKEN_LEN];
            int  offset_top;    /* px offset e.g. 24 — 0 means none */
        } gallery_image;

        /* NODE_MAKE_FORM */
        struct {
            char     title[MAX_TOKEN_LEN];
            char     eyebrow[MAX_TOKEN_LEN];
            NodeList children;   /* NODE_FORM_FIELD + NODE_SUBMIT_BTN */
        } make_form;

        /* NODE_FORM_FIELD */
        struct {
            char field_type[MAX_TOKEN_LEN];     /* "text" "email" "textarea" */
            char field_label[MAX_TOKEN_LEN];
            char placeholder[MAX_TOKEN_LEN];
        } form_field;

        /* NODE_SUBMIT_BTN */
        struct {
            char label[MAX_TOKEN_LEN];
            char modal_id[MAX_TOKEN_LEN];
        } submit_btn;

        /* NODE_MAKE_MODAL */
        struct {
            char     modal_id[MAX_TOKEN_LEN];
            char     icon[MAX_TOKEN_LEN];       /* Material Symbol name */
            char     color[MAX_TOKEN_LEN];      /* "primary" "secondary" */
            char     heading[MAX_TOKEN_LEN];
            char     body_text[MAX_TOKEN_LEN];
            char     dismiss_label[MAX_TOKEN_LEN];
        } make_modal;

        /* NODE_MAKE_FOOTER */
        struct {
            char brand[MAX_TOKEN_LEN];
            char tagline[MAX_TOKEN_LEN];
            char contact_heading[MAX_TOKEN_LEN];
            char address[MAX_TOKEN_LEN];
            char copyright[MAX_TOKEN_LEN];
        } make_footer;

    } as;
};


/* ─────────────────────────────────────────────
   AST constructor and printer
   ───────────────────────────────────────────── */
ASTNode *ast_make_node(NodeType type, int line, int col);
void     ast_free(ASTNode *node);
void     ast_print(ASTNode *node, int indent);

#endif /* AST_H */
