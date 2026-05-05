#ifndef INTERPRETER_H
#define INTERPRETER_H

#include "ast.h"

/* ─────────────────────────────────────────────
   EPL Interpreter — interpreter.h  (v1.1)
   ───────────────────────────────────────────── */

#define MAX_CALL_DEPTH 256

/* ─────────────────────────────────────────────
   Value types
   ───────────────────────────────────────────── */
typedef enum {
    VAL_NUMBER,
    VAL_STRING,
    VAL_BOOLEAN,
    VAL_NULL,
    VAL_LIST,
    VAL_ACTIVITY,
    VAL_GRAPH          /* v1.3 — connection graph */
} ValueType;

typedef struct Value Value;

typedef struct {
    Value **items;
    int     count;
    int     capacity;
} ValueList;

struct Value {
    ValueType type;
    union {
        double     number;
        char      *string;
        int        boolean;
        ValueList *list;
        ASTNode   *activity;
        EPLGraph  *graph;      /* v1.3 */
    } as;
};

/* Value constructors */
Value *val_number(double n);
Value *val_string(const char *s);
Value *val_boolean(int b);
Value *val_null(void);
Value *val_list(void);
Value *val_activity(ASTNode *node);
Value *val_graph(void);          /* v1.3 */
Value *val_copy(Value *src);
void   val_free(Value *v);

/* Value utilities */
int    val_is_truthy(Value *v);
int    val_equals(Value *a, Value *b);
char  *val_to_string(Value *v);
void   val_print(Value *v);

/* Value list */
void vallist_init(ValueList *list);
void vallist_push(ValueList *list, Value *v);
void vallist_free(ValueList *list);


/* ─────────────────────────────────────────────
   Environment (variable scope)
   ───────────────────────────────────────────── */
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


/* ─────────────────────────────────────────────
   Signal (return / break / continue / error)
   ───────────────────────────────────────────── */
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


/* ─────────────────────────────────────────────
   Interpreter
   ───────────────────────────────────────────── */
/* Forward declaration — PageManifest is defined in codegen.h */
typedef struct PageManifest PageManifest;

typedef struct {
    Env    *global;
    Signal  signal;
    int     had_error;
    int     call_depth;
    const char *source_file;       /* v1.7 — source .EPF file for bindings */
    PageManifest *page_manifest;   /* v1.7 — multi-page nav, NULL for single-page */
} Interpreter;

void interpreter_init(Interpreter *interp, const char *source_file);
void   interpreter_free(Interpreter *interp);
void   interpreter_run(Interpreter *interp, ASTNode *program);


Value *eval(Interpreter *interp, ASTNode *node, Env *env);
void   exec_node(Interpreter *interp, ASTNode *node, Env *env);
void   exec_block(Interpreter *interp, NodeList *list, Env *env);

#endif /* INTERPRETER_H */
