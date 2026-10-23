#ifndef AST_H
#define AST_H

#include "token.h"
#include <string.h>   

#define MAX_TEXT_LEN  512    
#define MAX_IDENT_LEN 128    
#define MAX_NAME_LEN  256

void ast_str_set(char *dst, size_t cap, const char *src, const char *field);
#define AST_SET_STR(dst, src) ast_str_set((dst), sizeof(dst), (src), #dst)

typedef enum {
    COLLOP_AVERAGE,
    COLLOP_TOTAL,
    COLLOP_MAXIMUM,
    COLLOP_MINIMUM,
    COLLOP_LENGTH,
    COLLOP_SORTED,
    COLLOP_REVERSED,
    COLLOP_KEYS,        
    COLLOP_CODE,        
    COLLOP_CHARACTER,   
    COLLOP_SHA256,      
    COLLOP_RANDOM_TOKEN 
} CollectionOp;

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

typedef enum {
    FMTOP_CURRENCY,
    FMTOP_PERCENTAGE,
    FMTOP_ROUNDED,
    FMTOP_FORMATTED
} FormatOp;

typedef enum {
    TYPECHECK_NUMBER,
    TYPECHECK_STRING,
    TYPECHECK_BOOLEAN,
    TYPECHECK_LIST,
    TYPECHECK_NULL,
    TYPECHECK_EMPTY     
} TypeCheckKind;

typedef enum {
    CHART_LINE,
    CHART_BAR,
    CHART_PIE
} ChartType;

typedef enum {
    THEME_LIGHT,
    THEME_DARK,
    THEME_INDIGO,
    THEME_EMERALD,
    THEME_ROSE,
    THEME_CUSTOM   
} EPFTheme;

typedef enum {
    BG3D_DEEP_SPACE,    
    BG3D_VOID,          
    BG3D_NEBULA,        
    BG3D_CUSTOM         
} Background3D;

typedef enum {
    NODE3D_COMPLETED,
    NODE3D_INPROGRESS,
    NODE3D_OVERDUE,
    NODE3D_PENDING
} Node3DStatus;

typedef enum {
    PANEL_TOP, PANEL_RIGHT, PANEL_BOTTOM, PANEL_LEFT
} PanelPosition;

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

typedef enum {
    
    NODE_PROGRAM,

    
    NODE_NUMBER,
    NODE_STRING,
    NODE_BOOLEAN,
    NODE_NULL,
    NODE_LIST,

    
    NODE_IDENTIFIER,

    
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
    NODE_FAIL_WITH,        
    NODE_ERROR_MSG,        
    NODE_PARSE_JSON,       
    NODE_TO_JSON,          
    NODE_ADD_TO,
    NODE_WAIT,
    NODE_EXECUTE,

    
    NODE_BINARY_OP,
    NODE_UNARY_OP,
    NODE_INLINE_IF,

    
    NODE_COLLECTION_OP,

    
    NODE_WHERE_FILTER,

    
    NODE_STRING_OP,

    
    NODE_FORMAT_OP,

    
    NODE_TYPE_CHECK,

    
    NODE_MAKE_GRAPH,       
    NODE_LINK,             
    NODE_ROUTE,            
    NODE_CONNECTED,        

    
    NODE_MAKE_PAGE,        
    NODE_SECTION_OUT,      
    NODE_ROW,              
    NODE_HEADING_OUT,      
    NODE_SUBHEADING,       
    NODE_PARAGRAPH,        
    NODE_LABEL_OUT,        
    NODE_BOLD_OUT,         
    NODE_DIVIDER,          
    NODE_SPACER,           
    NODE_MAKE_CHART,       
    NODE_MAKE_TABLE_OUT,   
    NODE_MAKE_REPORT,      
    NODE_MAKE_STAT,        
    NODE_IMPORT,           

    
    NODE_DB_MAKE_TABLE,    
    NODE_DB_INSERT,        
    NODE_DB_QUERY,         
    NODE_DB_DELETE,        
    NODE_DB_COUNT,         

    
    NODE_INDEX_ACCESS,     

    
    NODE_CANVAS3D,         
    NODE_ORBITAL_RING,     
    NODE_NODE3D,           
    NODE_CORE_STAR,        
    NODE_FROM_GRAPH,       
    NODE_PANEL3D,          
    NODE_DRAWER3D,         

    
    NODE_HTML_ELEMENT,     

    
    NODE_USE_DESIGN,       
    NODE_MAKE_NAVBAR,      
    NODE_NAV_LINK,         
    NODE_ACTION_BTN,       
    NODE_MAKE_HERO,        
    NODE_OVERLAY_CARD,     
    NODE_GUEST_PICKER,     
    NODE_DATE_PICKER,      
    NODE_CONTENT_SECTION,  
    NODE_COLUMN,           
    NODE_EYEBROW,          
    NODE_ITEM_LIST,        
    NODE_LIST_ENTRY,       
    NODE_FEATURE_IMAGE,    
    NODE_PRICE_BADGE,      
    NODE_MAKE_GALLERY,     
    NODE_GALLERY_IMAGE,    
    NODE_MAKE_FORM,        
    NODE_FORM_FIELD,       
    NODE_SUBMIT_BTN,       
    NODE_MAKE_MODAL,       
    NODE_DISMISS_BTN,      
    NODE_MAKE_FOOTER,      

    
    NODE_READ_FILE,        
    NODE_WRITE_FILE,       
    NODE_APPEND_FILE,      
    NODE_FILE_EXISTS,      
    NODE_DELETE_FILE,      
    NODE_RUN_CMD,          
    NODE_GET_ENV,          
    NODE_NOW,              
    NODE_SLEEP,            
    NODE_SEND_SOCKET,      
    NODE_POST_HTTP,        
    NODE_JSON_FIELD,       
    NODE_SERVE_SOCKET,     
    NODE_REPLY_WITH,       

    
    NODE_UPDATE_PART,      

    
    NODE_SERVE_PORT,       
    NODE_ACTION_FORM,      
    NODE_ACTION_BUTTON,    
    NODE_ACTION_INPUT,     

} NodeType;

enum { ACTION_AFTER_SHOW = 0, ACTION_AFTER_REFRESH = 1, ACTION_AFTER_GO = 2 };

typedef struct ASTNode ASTNode;

typedef struct {
    ASTNode **items;
    int       count;
    int       capacity;
} NodeList;

void nodelist_init(NodeList *list);
void nodelist_push(NodeList *list, ASTNode *node);
void nodelist_free(NodeList *list);

struct ASTNode {
    NodeType type;
    int      line;
    int      end_line;   
    int      col;

    union {
        /* NODE_PROGRAM */
        struct { NodeList body; } program;

        /* NODE_NUMBER */
        struct { double value; } number;

        /* NODE_STRING */
        struct { char value[MAX_TEXT_LEN]; } string;

        /* NODE_BOOLEAN */
        struct { int value; } boolean;

        /* NODE_IDENTIFIER */
        struct { char name[MAX_IDENT_LEN]; } identifier;

        /* NODE_LIST */
        struct { NodeList elements; } list;

        /* NODE_SET / NODE_UPDATE */
        struct {
            char name[MAX_IDENT_LEN];
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
            char counter[MAX_IDENT_LEN];
            NodeList body;
        } repeat_times;

        /* NODE_REPEAT_UNTIL */
        struct {
            ASTNode *condition;
            NodeList body;
        } repeat_until;

        /* NODE_FOR_EACH */
        struct {
            char var[MAX_IDENT_LEN];
            ASTNode *collection;
            NodeList body;
        } for_each;

        /* NODE_FOR_RANGE */
        struct {
            char var[MAX_IDENT_LEN];
            ASTNode *from;
            ASTNode *to;
            int      downward;
            NodeList body;
        } for_range;

        /* NODE_ACTIVITY_DEF */
        struct {
            char name[MAX_IDENT_LEN];
            NodeList params;
            NodeList body;
        } activity_def;

        /* NODE_ACTIVITY_CALL */
        struct {
            char name[MAX_IDENT_LEN];
            NodeList args;
        } activity_call;

        /* NODE_RETURN */
        struct { ASTNode *value; } return_stmt;

        /* NODE_CHECK */
        struct { ASTNode *condition; } check_stmt;

        /* NODE_DESCRIBE */
        struct {
            char name[MAX_IDENT_LEN];
            char text[MAX_TEXT_LEN];
        } describe_stmt;

        /* NODE_PROVIDE */
        struct {
            char var[MAX_IDENT_LEN];
            ASTNode *prompt;
        } provide_stmt;

        /* NODE_ATTEMPT */
        struct {
            NodeList try_body;
            NodeList fail_body;
        } attempt_stmt;

        /* NODE_FAIL_WITH */
        struct {
            ASTNode *message_expr;
        } fail_with;

        /* NODE_PARSE_JSON / NODE_TO_JSON */
        struct {
            ASTNode *value_expr;
        } json_convert;

        /* NODE_ADD_TO */
        struct {
            ASTNode *item;
            char list[MAX_IDENT_LEN];
        } add_to;

        /* NODE_WAIT */
        struct { char message[MAX_TEXT_LEN]; } wait_stmt;

        /* NODE_EXECUTE */
        struct {
            char command[MAX_TEXT_LEN];
            char alt_command[MAX_TEXT_LEN];
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

        /* NODE_COLLECTION_OP */
        struct {
            CollectionOp  op;
            ASTNode      *collection;
        } collection_op;

        /* NODE_WHERE_FILTER */
        struct {
            ASTNode *collection;
            char item_var[MAX_IDENT_LEN];
            ASTNode *condition;
        } where_filter;

        /* NODE_STRING_OP */
        struct {
            StringOp  op;
            ASTNode  *target;      
            ASTNode  *arg1;        
            ASTNode  *arg2;        
        } string_op;

        /* NODE_FORMAT_OP */
        struct {
            FormatOp  op;
            ASTNode  *target;
            ASTNode  *precision;   
        } format_op;

        /* NODE_TYPE_CHECK */
        struct {
            TypeCheckKind kind;
            ASTNode      *target;
        } type_check;

        /* NODE_MAKE_GRAPH */
        struct {
            char name[MAX_IDENT_LEN];       
        } make_graph;

        /* NODE_LINK */
        struct {
            char from[MAX_IDENT_LEN];
            char to[MAX_IDENT_LEN];
            char graph[MAX_IDENT_LEN];
        } link_stmt;

        /* NODE_ROUTE */
        struct {
            char from[MAX_IDENT_LEN];
            char to[MAX_IDENT_LEN];
            char graph[MAX_IDENT_LEN];
        } route_stmt;

        /* NODE_CONNECTED */
        struct {
            char target[MAX_IDENT_LEN];
            char graph[MAX_IDENT_LEN];
        } connected_stmt;

        /* NODE_MAKE_PAGE */
        struct {
            char filename[MAX_IDENT_LEN];
            EPFTheme theme;
            char color[MAX_IDENT_LEN];   
            NodeList children;
        } make_page;

        /* NODE_SECTION_OUT */
        struct {
            char title[MAX_TEXT_LEN];
            NodeList children;
        } section_out;

        /* NODE_ROW */
        struct {
            NodeList children;
        } row_out;

        /* NODE_HEADING_OUT / NODE_SUBHEADING /
           NODE_PARAGRAPH / NODE_LABEL_OUT / NODE_BOLD_OUT */
        struct {
            ASTNode *value;   
        } text_out;

        /* NODE_MAKE_CHART */
        struct {
            ChartType type;
            char title[MAX_TEXT_LEN];
            char filename[MAX_IDENT_LEN];  
            ASTNode  *x_data;    
            ASTNode  *y_data;    
            char x_label[MAX_TEXT_LEN];
            char y_label[MAX_TEXT_LEN];
        } make_chart;

        /* NODE_MAKE_TABLE_OUT */
        struct {
            char title[MAX_TEXT_LEN];
            char filename[MAX_IDENT_LEN];
            ASTNode *data;       
            ASTNode *headers;    
        } make_table_out;

        /* NODE_MAKE_REPORT */
        struct {
            char title[MAX_TEXT_LEN];
            char filename[MAX_IDENT_LEN];
            ASTNode *data;
        } make_report;

        /* NODE_MAKE_STAT */
        struct {
            char label[MAX_TEXT_LEN];
            ASTNode *value;
        } make_stat;

        /* NODE_IMPORT */
        struct {
            char source[MAX_TEXT_LEN];   
            char alias[MAX_IDENT_LEN];   
        } import_stmt;

        /* NODE_DB_MAKE_TABLE / NODE_DB_QUERY */
        struct {
            ASTNode *table;              
        } db_table;

        /* NODE_DB_INSERT */
        struct {
            ASTNode *table;
            NodeList field_names;        /* NODE_STRING */
            NodeList field_values;       
        } db_insert;

        /* NODE_DB_DELETE / NODE_DB_COUNT */
        struct {
            ASTNode *table;
            char     item_var[MAX_IDENT_LEN];  
            ASTNode *condition;                
        } db_filter;

        /* NODE_INDEX_ACCESS */
        struct {
            ASTNode *list;    
            ASTNode *index;   
        } index_access;

        /* NODE_CANVAS3D */
        struct {
            char name[MAX_IDENT_LEN];
            Background3D background;
            char bg_color[MAX_IDENT_LEN];
            int          offline;        
            NodeList     children;       
        } canvas3d;

        /* NODE_ORBITAL_RING */
        struct {
            double       radius;
            char color[MAX_IDENT_LEN];
            double       speed;
            double       tilt_x;
            double       tilt_z;
            NodeList     nodes;          /* NODE_NODE3D */
        } orbital_ring;

        /* NODE_NODE3D */
        struct {
            double       angle;          
            char label[MAX_TEXT_LEN];
            Node3DStatus status;
            char desc[MAX_TEXT_LEN];
        } node3d;

        /* NODE_CORE_STAR */
        struct {
            char color[MAX_IDENT_LEN];
            char label[MAX_TEXT_LEN];
            char desc[MAX_TEXT_LEN];
        } core_star;

        /* NODE_FROM_GRAPH */
        struct {
            char graph_var[MAX_IDENT_LEN];
        } from_graph;

        /* NODE_PANEL3D */
        struct {
            PanelPosition position;
            NodeList      children;      
        } panel3d;

        /* NODE_DRAWER3D */
        struct {
            PanelPosition position;
            char title[MAX_TEXT_LEN];
            NodeList      children;
        } drawer3d;

        /* NODE_HTML_ELEMENT */
        struct {
            char tag[MAX_IDENT_LEN];
            char id_attr[MAX_IDENT_LEN];
            char class_attr[MAX_IDENT_LEN];
            char src_attr[MAX_TEXT_LEN];
            char type_attr[MAX_IDENT_LEN];
            char onclick_attr[MAX_TEXT_LEN];
            char placeholder_attr[MAX_TEXT_LEN];
            char value_attr[MAX_TEXT_LEN];
            int      is_self_closing;
            ASTNode *text_content;  
            NodeList children;
        } html_element;

        

        /* NODE_USE_DESIGN */
        struct {
            char name[MAX_IDENT_LEN];       
            char primary[MAX_IDENT_LEN];    
            char secondary[MAX_IDENT_LEN];
            char background[MAX_IDENT_LEN];
            char headline_font[MAX_IDENT_LEN];
            char body_font[MAX_TEXT_LEN];
            char icon_set[MAX_IDENT_LEN];   
        } use_design;

        /* NODE_MAKE_NAVBAR */
        struct {
            char brand[MAX_IDENT_LEN];  
            char style[MAX_IDENT_LEN];  
            NodeList children;              /* NODE_NAV_LINK, NODE_ACTION_BTN */
        } make_navbar;

        /* NODE_NAV_LINK */
        struct {
            char label[MAX_TEXT_LEN];
            char href[MAX_TEXT_LEN];
        } nav_link;

        /* NODE_ACTION_BTN */
        struct {
            char label[MAX_TEXT_LEN];
            char modal_id[MAX_IDENT_LEN];   
        } action_btn;

        /* NODE_MAKE_HERO */
        struct {
            char title[MAX_TEXT_LEN];
            char accented[MAX_IDENT_LEN];   
            char subtitle[MAX_TEXT_LEN];
            char bg_image[MAX_IDENT_LEN];
            int      bg_dim;                    
            ASTNode *overlay_card;              
        } make_hero;

        /* NODE_OVERLAY_CARD */
        struct {
            char title[MAX_TEXT_LEN];
            NodeList children;              
        } overlay_card;

        /* NODE_GUEST_PICKER */
        struct {
            int default_count;              
        } guest_picker;

        /* NODE_DATE_PICKER */
        struct {
            char label[MAX_TEXT_LEN];      
        } date_picker;

        /* NODE_CONTENT_SECTION */
        struct {
            char id[MAX_IDENT_LEN];
            ASTNode *left_col;
            ASTNode *right_col;
        } content_section;

        /* NODE_COLUMN */
        struct {
            int      is_right;   
            NodeList children;
        } column;

        /* NODE_EYEBROW */
        struct {
            char text[MAX_TEXT_LEN];
        } eyebrow;

        /* NODE_ITEM_LIST */
        struct {
            NodeList entries;   /* NODE_LIST_ENTRY */
        } item_list;

        /* NODE_LIST_ENTRY */
        struct {
            char label[MAX_TEXT_LEN];
            int  number;            
            char detail[MAX_IDENT_LEN];
            int  offset;            
        } list_entry;

        /* NODE_FEATURE_IMAGE */
        struct {
            char src[MAX_TEXT_LEN];
            char aspect[MAX_IDENT_LEN];     
            char hover_mode[MAX_IDENT_LEN]; 
        } feature_image;

        /* NODE_PRICE_BADGE */
        struct {
            char price[MAX_IDENT_LEN];  
            char label[MAX_TEXT_LEN];  
        } price_badge;

        /* NODE_MAKE_GALLERY */
        struct {
            char title[MAX_TEXT_LEN];
            char subtitle[MAX_TEXT_LEN];
            int      columns;
            NodeList images;    /* NODE_GALLERY_IMAGE */
        } make_gallery;

        /* NODE_GALLERY_IMAGE */
        struct {
            char src[MAX_TEXT_LEN];
            int  offset_top;    
        } gallery_image;

        /* NODE_MAKE_FORM */
        struct {
            char title[MAX_TEXT_LEN];
            char eyebrow[MAX_TEXT_LEN];
            NodeList children;   /* NODE_FORM_FIELD */
        } make_form;

        /* NODE_FORM_FIELD */
        struct {
            char field_type[MAX_IDENT_LEN];     
            char field_label[MAX_TEXT_LEN];
            char placeholder[MAX_TEXT_LEN];
        } form_field;

        /* NODE_SUBMIT_BTN */
        struct {
            char label[MAX_TEXT_LEN];
            char modal_id[MAX_IDENT_LEN];
        } submit_btn;

        /* NODE_MAKE_MODAL */
        struct {
            char modal_id[MAX_IDENT_LEN];
            char icon[MAX_IDENT_LEN];       
            char color[MAX_IDENT_LEN];      
            char heading[MAX_TEXT_LEN];
            char body_text[MAX_TEXT_LEN];
            char dismiss_label[MAX_TEXT_LEN];
        } make_modal;

        /* NODE_MAKE_FOOTER */
        struct {
            char brand[MAX_IDENT_LEN];
            char tagline[MAX_TEXT_LEN];
            char contact_heading[MAX_TEXT_LEN];
            char address[MAX_IDENT_LEN];
            char copyright[MAX_IDENT_LEN];
        } make_footer;

        

        /* NODE_READ_FILE, NODE_FILE_EXISTS, NODE_DELETE_FILE */
        struct {
            ASTNode *path_expr;
        } file_op;

        /* NODE_WRITE_FILE, NODE_APPEND_FILE */
        struct {
            ASTNode *value_expr;
            ASTNode *path_expr;
        } file_write;

        /* NODE_RUN_CMD */
        struct {
            ASTNode *cmd_expr;
        } run_cmd;

        /* NODE_GET_ENV */
        struct {
            ASTNode *var_expr;
        } get_env;

        /* NODE_SLEEP */
        struct {
            ASTNode *duration_expr;
        } sleep_stmt;

        /* NODE_SEND_SOCKET */
        struct {
            ASTNode *json_expr;
            ASTNode *path_expr;
        } send_socket;

        /* NODE_POST_HTTP */
        struct {
            ASTNode *json_expr;
            ASTNode *url_expr;
        } post_http;

        /* NODE_JSON_FIELD */
        struct {
            ASTNode *key_expr;
            ASTNode *json_expr;
        } json_field;

        /* NODE_SERVE_SOCKET */
        struct {
            ASTNode *path_expr;
            char activity[MAX_IDENT_LEN];
        } serve_socket;

        /* NODE_REPLY_WITH */
        struct {
            ASTNode *value_expr;
        } reply_with;

        /* NODE_UPDATE_PART */
        struct {
            ASTNode *selector;               
            ASTNode *value;
            char     target[MAX_IDENT_LEN];  
            int      is_field;
        } update_part;

        /* NODE_SERVE_PORT */
        struct {
            ASTNode *port_expr;
            NodeList offered;                /* NODE_IDENTIFIER */
        } serve_port;

        /* NODE_ACTION_FORM / NODE_ACTION_BUTTON */
        struct {
            char     activity[MAX_IDENT_LEN];
            char     label[MAX_TEXT_LEN];    
            NodeList inputs;                 /* NODE_ACTION_INPUT */
            int      after;                  
            char     go_to[MAX_TEXT_LEN];    
        } action_form;

        /* NODE_ACTION_INPUT */
        struct {
            char name[MAX_IDENT_LEN];        
            char label[MAX_TEXT_LEN];
        } action_input;

    } as;
};

ASTNode *ast_make_node(NodeType type, int line, int col);
void     ast_free(ASTNode *node);
void     ast_print(ASTNode *node, int indent);

#endif 
