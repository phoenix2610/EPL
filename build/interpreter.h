#ifndef INTERPRETER_H
#define INTERPRETER_H

#include "ast.h"
#include "permissions.h"

#define MAX_CALL_DEPTH 256

typedef enum {
    VAL_NUMBER,
    VAL_STRING,
    VAL_BOOLEAN,
    VAL_NULL,
    VAL_LIST,
    VAL_ACTIVITY,
    VAL_GRAPH,
    VAL_MAP
} ValueType;

typedef struct Value Value;

typedef struct {
    Value **items;
    int     count;
    int     capacity;
} ValueList;

typedef struct {
    char  **keys;
    Value **values;
    int     count;
    int     capacity;
} ValueMap;

struct Value {
    ValueType type;
    union {
        double     number;
        char      *string;
        int        boolean;
        ValueList *list;
        ASTNode   *activity;
        EPLGraph  *graph;
        ValueMap  *map;
    } as;
};

Value *val_number(double n);
Value *val_string(const char *s);
Value *val_boolean(int b);
Value *val_null(void);
Value *val_list(void);
Value *val_activity(ASTNode *node);
Value *val_graph(void);
Value *val_copy(Value *src);
void   val_free(Value *v);

int    val_is_truthy(Value *v);
int    val_equals(Value *a, Value *b);
char  *val_to_string(Value *v);
void   val_print(Value *v);

void vallist_init(ValueList *list);
void vallist_push(ValueList *list, Value *v);
void vallist_free(ValueList *list);

Value *val_map(void);
void   valmap_set(ValueMap *m, const char *key, Value *v);
Value *valmap_get(ValueMap *m, const char *key);
void   valmap_free(ValueMap *m);

typedef struct Env Env;
struct Env {
    char   **names;
    Value  **values;
    int      count;
    int      capacity;
    Env     *parent;
};

Env   *env_new(Env *parent);
void   env_free(Env *env);
void   env_set(Env *env, const char *name, Value *value);
Value *env_get(Env *env, const char *name);
int    env_update(Env *env, const char *name, Value *value);

typedef enum {
    SIG_NONE,
    SIG_RETURN,
    SIG_BREAK,
    SIG_CONTINUE,
    SIG_ERROR
} SignalType;

typedef struct {
    SignalType type;
    Value     *value;
} Signal;

typedef struct PageManifest PageManifest;

typedef struct DBIndexCache DBIndexCache;

typedef struct {
    Env    *global;
    Signal  signal;
    int     had_error;
    int     call_depth;
    char    last_error[512];
    int     attempt_depth;
    const char *source_file;
    PageManifest *page_manifest;

    char      **imported_paths;
    int         imported_count;
    int         imported_cap;
    ASTNode   **imported_asts;
    int         imported_ast_count;
    int         import_depth;

    DBIndexCache *db_index;

    int     record_scope;

    Permissions *perms;
} Interpreter;

void interpreter_init(Interpreter *interp, const char *source_file);
void   interpreter_free(Interpreter *interp);
void   interpreter_run(Interpreter *interp, ASTNode *program);

Value *eval(Interpreter *interp, ASTNode *node, Env *env);
void   exec_node(Interpreter *interp, ASTNode *node, Env *env);
void   exec_block(Interpreter *interp, NodeList *list, Env *env);

#endif
