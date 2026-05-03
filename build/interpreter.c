#define _POSIX_C_SOURCE 200809L
#define __USE_MINGW_ANSI_STDIO 1
#undef __STRICT_ANSI__
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <ctype.h>
#include "interpreter.h"
#include "codegen.h"

/* ─────────────────────────────────────────────
   EPL Interpreter v1.2
   Adds: String ops, Number formatting, Type checks
   ───────────────────────────────────────────── */


/* ═════════════════════════════════════════════
   VALUE CONSTRUCTORS
   ═════════════════════════════════════════════ */

Value *val_number(double n) {
    Value *v = malloc(sizeof(Value));
    v->type = VAL_NUMBER;
    v->as.number = n;
    return v;
}

Value *val_string(const char *s) {
    Value *v = malloc(sizeof(Value));
    v->type = VAL_STRING;
    v->as.string = strdup(s);
    return v;
}

Value *val_boolean(int b) {
    Value *v = malloc(sizeof(Value));
    v->type = VAL_BOOLEAN;
    v->as.boolean = b ? 1 : 0;
    return v;
}

Value *val_null(void) {
    Value *v = malloc(sizeof(Value));
    v->type = VAL_NULL;
    return v;
}

Value *val_list(void) {
    Value *v = malloc(sizeof(Value));
    v->type = VAL_LIST;
    v->as.list = malloc(sizeof(ValueList));
    vallist_init(v->as.list);
    return v;
}

Value *val_activity(ASTNode *node) {
    Value *v = malloc(sizeof(Value));
    v->type = VAL_ACTIVITY;
    v->as.activity = node;
    return v;
}

Value *val_graph(void) {
    Value *v = malloc(sizeof(Value));
    v->type  = VAL_GRAPH;
    v->as.graph = calloc(1, sizeof(EPLGraph));
    return v;
}

Value *val_copy(Value *src) {
    if (!src) return val_null();
    switch (src->type) {
        case VAL_NUMBER:   return val_number(src->as.number);
        case VAL_STRING:   return val_string(src->as.string);
        case VAL_BOOLEAN:  return val_boolean(src->as.boolean);
        case VAL_NULL:     return val_null();
        case VAL_ACTIVITY: return val_activity(src->as.activity);
        case VAL_GRAPH: {
            Value *copy = val_graph();
            memcpy(copy->as.graph, src->as.graph, sizeof(EPLGraph));
            return copy;
        }
        case VAL_LIST: {
            Value *copy = val_list();
            for (int i = 0; i < src->as.list->count; i++)
                vallist_push(copy->as.list, val_copy(src->as.list->items[i]));
            return copy;
        }
    }
    return val_null();
}

void val_free(Value *v) {
    if (!v) return;
    if (v->type == VAL_STRING) free(v->as.string);
    if (v->type == VAL_LIST) {
        vallist_free(v->as.list);
        free(v->as.list);
    }
    if (v->type == VAL_GRAPH) free(v->as.graph);
    free(v);
}


/* ═════════════════════════════════════════════
   VALUE UTILITIES
   ═════════════════════════════════════════════ */

int val_is_truthy(Value *v) {
    if (!v) return 0;
    switch (v->type) {
        case VAL_BOOLEAN: return v->as.boolean;
        case VAL_NUMBER:  return v->as.number != 0;
        case VAL_STRING:  return strlen(v->as.string) > 0;
        case VAL_NULL:    return 0;
        case VAL_LIST:    return v->as.list->count > 0;
        case VAL_ACTIVITY:return 1;
        case VAL_GRAPH:   return 1;
    }
    return 0;
}

int val_equals(Value *a, Value *b) {
    if (!a || !b) return 0;
    if (a->type != b->type) {
        if (a->type == VAL_NUMBER && b->type == VAL_STRING) {
            char buf[64]; snprintf(buf, 64, "%g", a->as.number);
            return strcmp(buf, b->as.string) == 0;
        }
        if (a->type == VAL_STRING && b->type == VAL_NUMBER) {
            char buf[64]; snprintf(buf, 64, "%g", b->as.number);
            return strcmp(a->as.string, buf) == 0;
        }
        return 0;
    }
    switch (a->type) {
        case VAL_NUMBER:  return a->as.number == b->as.number;
        case VAL_STRING:  return strcmp(a->as.string, b->as.string) == 0;
        case VAL_BOOLEAN: return a->as.boolean == b->as.boolean;
        case VAL_NULL:    return 1;
        default:          return 0;
    }
}

/* Convert any value to a heap string — caller must free */
char *val_to_string(Value *v) {
    if (!v) return strdup("null");
    char buf[256];
    switch (v->type) {
        case VAL_NUMBER:
            if (v->as.number == (long)v->as.number)
                snprintf(buf, 256, "%ld", (long)v->as.number);
            else
                snprintf(buf, 256, "%g", v->as.number);
            return strdup(buf);
        case VAL_STRING:  return strdup(v->as.string);
        case VAL_BOOLEAN: return strdup(v->as.boolean ? "true" : "false");
        case VAL_NULL:    return strdup("null");
        case VAL_ACTIVITY:return strdup("[activity]");
        case VAL_GRAPH: {
            char buf[64];
            snprintf(buf, sizeof(buf), "[graph: %d nodes, %d edges]",
                v->as.graph->node_count,
                v->as.graph->edge_count);
            return strdup(buf);
        }
        case VAL_LIST: {
            char *result = malloc(4096);
            result[0] = '\0';
            strcat(result, "[");
            for (int i = 0; i < v->as.list->count; i++) {
                char *s = val_to_string(v->as.list->items[i]);
                strcat(result, s);
                free(s);
                if (i < v->as.list->count - 1) strcat(result, ", ");
            }
            strcat(result, "]");
            return result;
        }
    }
    return strdup("null");
}

void val_print(Value *v) {
    char *s = val_to_string(v);
    printf("%s", s);
    free(s);
}


/* ═════════════════════════════════════════════
   VALUE LIST
   ═════════════════════════════════════════════ */
void vallist_init(ValueList *list) {
    list->items    = NULL;
    list->count    = 0;
    list->capacity = 0;
}

void vallist_push(ValueList *list, Value *v) {
    if (list->count >= list->capacity) {
        list->capacity = list->capacity == 0 ? 8 : list->capacity * 2;
        list->items = realloc(list->items, list->capacity * sizeof(Value *));
    }
    list->items[list->count++] = v;
}

void vallist_free(ValueList *list) {
    for (int i = 0; i < list->count; i++)
        val_free(list->items[i]);
    free(list->items);
    list->items    = NULL;
    list->count    = 0;
    list->capacity = 0;
}


/* ═════════════════════════════════════════════
   ENVIRONMENT
   ═════════════════════════════════════════════ */
Env *env_new(Env *parent) {
    Env *env = calloc(1, sizeof(Env));
    env->parent = parent;
    return env;
}

void env_free(Env *env) {
    if (!env) return;
    for (int i = 0; i < env->count; i++) {
        free(env->names[i]);
        val_free(env->values[i]);
    }
    free(env->names);
    free(env->values);
    free(env);
}

void env_set(Env *env, const char *name, Value *value) {
    for (int i = 0; i < env->count; i++) {
        if (strcmp(env->names[i], name) == 0) {
            val_free(env->values[i]);
            env->values[i] = value;
            return;
        }
    }
    if (env->count >= env->capacity) {
        env->capacity = env->capacity == 0 ? 8 : env->capacity * 2;
        env->names  = realloc(env->names,  env->capacity * sizeof(char *));
        env->values = realloc(env->values, env->capacity * sizeof(Value *));
    }
    env->names[env->count]    = strdup(name);
    env->values[env->count++] = value;
}

Value *env_get(Env *env, const char *name) {
    Env *scope = env;
    while (scope) {
        for (int i = 0; i < scope->count; i++) {
            if (strcmp(scope->names[i], name) == 0)
                return scope->values[i];
        }
        scope = scope->parent;
    }
    return NULL;
}

int env_update(Env *env, const char *name, Value *value) {
    Env *scope = env;
    while (scope) {
        for (int i = 0; i < scope->count; i++) {
            if (strcmp(scope->names[i], name) == 0) {
                val_free(scope->values[i]);
                scope->values[i] = value;
                return 1;
            }
        }
        scope = scope->parent;
    }
    return 0;
}


/* ═════════════════════════════════════════════
   INTERPRETER INIT / FREE
   ═════════════════════════════════════════════ */
void interpreter_init(Interpreter *interp) {
    interp->global       = env_new(NULL);
    interp->signal.type  = SIG_NONE;
    interp->signal.value = NULL;
    interp->had_error    = 0;
    interp->call_depth   = 0;
}

void interpreter_free(Interpreter *interp) {
    env_free(interp->global);
}


/* ═════════════════════════════════════════════
   ERROR HELPER
   ═════════════════════════════════════════════ */
static void runtime_error(Interpreter *interp, ASTNode *node,
                           const char *msg) {
    fprintf(stderr, "[EPL Runtime Error] Line %d: %s\n",
        node ? node->line : 0, msg);
    interp->had_error    = 1;
    interp->signal.type  = SIG_ERROR;
    interp->signal.value = NULL;
}


/* ═════════════════════════════════════════════
   FORWARD DECLARATIONS
   ═════════════════════════════════════════════ */
static Value *call_activity(Interpreter *interp, const char *name,
                             ValueList *args, Env *env);


/* ═════════════════════════════════════════════
   GRAPH HELPERS — v1.3
   ═════════════════════════════════════════════ */

/* Add node if not already present */
static void graph_add_node(EPLGraph *g, const char *name) {
    for (int i = 0; i < g->node_count; i++) {
        if (strcmp(g->nodes[i], name) == 0) return;
    }
    if (g->node_count < EPL_GRAPH_MAX_NODES) {
        strncpy(g->nodes[g->node_count++], name, MAX_TOKEN_LEN - 1);
    }
}

/* Add directed edge from → to */
static void graph_add_edge(EPLGraph *g, const char *from, const char *to) {
    graph_add_node(g, from);
    graph_add_node(g, to);
    if (g->edge_count < EPL_GRAPH_MAX_EDGES) {
        strncpy(g->edges[g->edge_count].from, from, MAX_TOKEN_LEN - 1);
        strncpy(g->edges[g->edge_count].to,   to,   MAX_TOKEN_LEN - 1);
        g->edge_count++;
    }
}

/* BFS shortest path — returns list of node name strings, or empty list */
static Value *graph_bfs(EPLGraph *g, const char *start, const char *end) {
    if (g->node_count == 0) return val_list();

    /* visited array */
    int visited[EPL_GRAPH_MAX_NODES] = {0};
    /* parent index — to reconstruct path */
    int parent[EPL_GRAPH_MAX_NODES];
    memset(parent, -1, sizeof(parent));

    /* find start node index */
    int start_idx = -1, end_idx = -1;
    for (int i = 0; i < g->node_count; i++) {
        if (strcmp(g->nodes[i], start) == 0) start_idx = i;
        if (strcmp(g->nodes[i], end)   == 0) end_idx   = i;
    }
    if (start_idx < 0 || end_idx < 0) return val_list();
    if (start_idx == end_idx) {
        Value *path = val_list();
        vallist_push(path->as.list, val_string(start));
        return path;
    }

    /* BFS queue — simple circular array */
    int queue[EPL_GRAPH_MAX_NODES];
    int head = 0, tail = 0;
    queue[tail++] = start_idx;
    visited[start_idx] = 1;

    int found = 0;
    while (head < tail) {
        int curr = queue[head++];
        if (curr == end_idx) { found = 1; break; }

        /* enqueue all unvisited neighbours */
        for (int e = 0; e < g->edge_count; e++) {
            if (strcmp(g->edges[e].from, g->nodes[curr]) == 0) {
                /* find neighbour index */
                for (int n = 0; n < g->node_count; n++) {
                    if (strcmp(g->nodes[n], g->edges[e].to) == 0 &&
                        !visited[n]) {
                        visited[n]    = 1;
                        parent[n]     = curr;
                        queue[tail++] = n;
                    }
                }
            }
        }
    }

    if (!found) return val_list();   /* no path exists */

    /* Reconstruct path by walking parents backward */
    int path_buf[EPL_GRAPH_MAX_NODES];
    int path_len = 0;
    int cur = end_idx;
    while (cur != -1) {
        path_buf[path_len++] = cur;
        cur = parent[cur];
    }

    /* Reverse into a Value list */
    Value *result = val_list();
    for (int i = path_len - 1; i >= 0; i--) {
        vallist_push(result->as.list,
            val_string(g->nodes[path_buf[i]]));
    }
    return result;
}

/* BFS all reachable nodes from start */
static Value *graph_connected(EPLGraph *g, const char *start) {
    Value *result = val_list();
    if (g->node_count == 0) return result;

    int visited[EPL_GRAPH_MAX_NODES] = {0};
    int queue[EPL_GRAPH_MAX_NODES];
    int head = 0, tail = 0;

    /* find start index */
    int start_idx = -1;
    for (int i = 0; i < g->node_count; i++) {
        if (strcmp(g->nodes[i], start) == 0) { start_idx = i; break; }
    }
    if (start_idx < 0) return result;

    queue[tail++] = start_idx;
    visited[start_idx] = 1;

    while (head < tail) {
        int curr = queue[head++];
        /* add to result (skip the start node itself) */
        if (curr != start_idx)
            vallist_push(result->as.list, val_string(g->nodes[curr]));

        for (int e = 0; e < g->edge_count; e++) {
            if (strcmp(g->edges[e].from, g->nodes[curr]) == 0) {
                for (int n = 0; n < g->node_count; n++) {
                    if (strcmp(g->nodes[n], g->edges[e].to) == 0 &&
                        !visited[n]) {
                        visited[n]    = 1;
                        queue[tail++] = n;
                    }
                }
            }
        }
    }
    return result;
}


/* ═════════════════════════════════════════════
   GRAPH STATEMENT EVALUATORS — v1.3
   ═════════════════════════════════════════════ */

/* link customer to store in network */
static void exec_link(Interpreter *interp, ASTNode *node, Env *env) {
    Value *graph_val = env_get(env, node->as.link_stmt.graph);
    if (!graph_val || graph_val->type != VAL_GRAPH) {
        fprintf(stderr,
            "[EPL Error] Line %d: '%s' is not a graph\n",
            node->line, node->as.link_stmt.graph);
        interp->had_error = 1;
        return;
    }
    graph_add_edge(graph_val->as.graph,
        node->as.link_stmt.from,
        node->as.link_stmt.to);
}

/* route from X to Y in network → Value list (path) */
static Value *eval_route(Interpreter *interp, ASTNode *node, Env *env) {
    Value *graph_val = env_get(env, node->as.route_stmt.graph);
    if (!graph_val || graph_val->type != VAL_GRAPH) {
        fprintf(stderr,
            "[EPL Error] Line %d: '%s' is not a graph\n",
            node->line, node->as.route_stmt.graph);
        interp->had_error = 1;
        return val_list();
    }
    return graph_bfs(graph_val->as.graph,
        node->as.route_stmt.from,
        node->as.route_stmt.to);
}

/* connected to X in network → Value list (reachable nodes) */
static Value *eval_connected(Interpreter *interp, ASTNode *node, Env *env) {
    Value *graph_val = env_get(env, node->as.connected_stmt.graph);
    if (!graph_val || graph_val->type != VAL_GRAPH) {
        fprintf(stderr,
            "[EPL Error] Line %d: '%s' is not a graph\n",
            node->line, node->as.connected_stmt.graph);
        interp->had_error = 1;
        return val_list();
    }
    return graph_connected(graph_val->as.graph,
        node->as.connected_stmt.target);
}


/* ═════════════════════════════════════════════
   SMART COLLECTIONS — v1.1
   Built-in list intelligence:
     average of list
     total of list
     maximum of list
     minimum of list
     length of list
     sorted list
     reversed list
   ═════════════════════════════════════════════ */

/* eval_collection_op: handles NODE_COLLECTION_OP */
static Value *eval_collection_op(Interpreter *interp,
                                  ASTNode *node, Env *env) {
    Value *collection = eval(interp, node->as.collection_op.collection, env);

    /* length works on strings too */
    if (node->as.collection_op.op == COLLOP_LENGTH) {
        if (collection->type == VAL_STRING) {
            int len = (int)strlen(collection->as.string);
            val_free(collection);
            return val_number(len);
        }
        if (collection->type == VAL_LIST) {
            int len = collection->as.list->count;
            val_free(collection);
            return val_number(len);
        }
        val_free(collection);
        return val_number(0);
    }

    /* All other ops require a list */
    if (!collection || collection->type != VAL_LIST) {
        val_free(collection);
        return val_null();
    }

    ValueList *lst = collection->as.list;
    int n = lst->count;

    switch (node->as.collection_op.op) {

        case COLLOP_TOTAL: {
            double sum = 0;
            for (int i = 0; i < n; i++) {
                if (lst->items[i]->type == VAL_NUMBER)
                    sum += lst->items[i]->as.number;
            }
            val_free(collection);
            return val_number(sum);
        }

        case COLLOP_AVERAGE: {
            if (n == 0) { val_free(collection); return val_number(0); }
            double sum = 0;
            for (int i = 0; i < n; i++) {
                if (lst->items[i]->type == VAL_NUMBER)
                    sum += lst->items[i]->as.number;
            }
            val_free(collection);
            return val_number(sum / n);
        }

        case COLLOP_MAXIMUM: {
            if (n == 0) { val_free(collection); return val_null(); }
            double max = lst->items[0]->type == VAL_NUMBER
                         ? lst->items[0]->as.number : -1e300;
            for (int i = 1; i < n; i++) {
                if (lst->items[i]->type == VAL_NUMBER &&
                    lst->items[i]->as.number > max)
                    max = lst->items[i]->as.number;
            }
            val_free(collection);
            return val_number(max);
        }

        case COLLOP_MINIMUM: {
            if (n == 0) { val_free(collection); return val_null(); }
            double min = lst->items[0]->type == VAL_NUMBER
                         ? lst->items[0]->as.number : 1e300;
            for (int i = 1; i < n; i++) {
                if (lst->items[i]->type == VAL_NUMBER &&
                    lst->items[i]->as.number < min)
                    min = lst->items[i]->as.number;
            }
            val_free(collection);
            return val_number(min);
        }

        case COLLOP_SORTED: {
            /* Build a copy, sort ascending */
            Value *result = val_copy(collection);
            val_free(collection);
            ValueList *rl = result->as.list;
            /* Bubble sort — simple, correct */
            for (int i = 0; i < rl->count - 1; i++) {
                for (int j = 0; j < rl->count - 1 - i; j++) {
                    Value *a = rl->items[j];
                    Value *b = rl->items[j + 1];
                    int swap = 0;
                    if (a->type == VAL_NUMBER && b->type == VAL_NUMBER) {
                        swap = a->as.number > b->as.number;
                    } else {
                        char *sa = val_to_string(a);
                        char *sb = val_to_string(b);
                        swap = strcmp(sa, sb) > 0;
                        free(sa); free(sb);
                    }
                    if (swap) { rl->items[j] = b; rl->items[j+1] = a; }
                }
            }
            return result;
        }

        case COLLOP_REVERSED: {
            Value *result = val_copy(collection);
            val_free(collection);
            ValueList *rl = result->as.list;
            for (int i = 0; i < rl->count / 2; i++) {
                Value *tmp = rl->items[i];
                rl->items[i] = rl->items[rl->count - 1 - i];
                rl->items[rl->count - 1 - i] = tmp;
            }
            return result;
        }

        default:
            val_free(collection);
            return val_null();
    }
}


/* ═════════════════════════════════════════════
   SMART FILTER — v1.1
   set adults as people where age is greater than 18
   Handles NODE_WHERE_FILTER
   ═════════════════════════════════════════════ */
static Value *eval_where_filter(Interpreter *interp,
                                 ASTNode *node, Env *env) {
    Value *collection = eval(interp, node->as.where_filter.collection, env);

    if (!collection || collection->type != VAL_LIST) {
        val_free(collection);
        return val_list();   /* empty list on bad input */
    }

    Value *result = val_list();
    const char *item_var = node->as.where_filter.item_var;

    for (int i = 0; i < collection->as.list->count; i++) {
        Value *item = collection->as.list->items[i];

        /* Create a temporary scope with the item bound to item_var */
        Env *filter_env = env_new(env);
        env_set(filter_env, item_var, val_copy(item));

        /* Also expose map fields: if item is a number, bind it directly */
        /* (For future record types — currently items are scalars) */

        Value *passes = eval(interp, node->as.where_filter.condition, filter_env);
        int keep = val_is_truthy(passes);
        val_free(passes);
        env_free(filter_env);

        if (keep)
            vallist_push(result->as.list, val_copy(item));
    }

    val_free(collection);
    return result;
}


/* ═════════════════════════════════════════════
   STRING OPERATIONS — v1.2
   name in uppercase
   sentence split by " "
   parts joined by ", "
   text with "old" replaced by "new"
   name starts with "Ph"   → boolean
   name ends with "ix"     → boolean
   name contains "oen"     → boolean
   name trimmed
   ═════════════════════════════════════════════ */
static Value *eval_string_op(Interpreter *interp,
                              ASTNode *node, Env *env) {
    Value *target = eval(interp, node->as.string_op.target, env);
    Value *arg1   = node->as.string_op.arg1
                    ? eval(interp, node->as.string_op.arg1, env) : NULL;
    Value *arg2   = node->as.string_op.arg2
                    ? eval(interp, node->as.string_op.arg2, env) : NULL;

    Value *result = NULL;

    switch (node->as.string_op.op) {

        case STROP_UPPERCASE: {
            char *s = val_to_string(target);
            for (int i = 0; s[i]; i++) s[i] = toupper((unsigned char)s[i]);
            result = val_string(s);
            free(s);
            break;
        }

        case STROP_LOWERCASE: {
            char *s = val_to_string(target);
            for (int i = 0; s[i]; i++) s[i] = tolower((unsigned char)s[i]);
            result = val_string(s);
            free(s);
            break;
        }

        case STROP_TRIMMED: {
            char *s = val_to_string(target);
            /* trim leading */
            char *start = s;
            while (*start && isspace((unsigned char)*start)) start++;
            /* trim trailing */
            char *end = start + strlen(start) - 1;
            while (end > start && isspace((unsigned char)*end)) end--;
            *(end + 1) = '\0';
            result = val_string(start);
            free(s);
            break;
        }

        case STROP_SPLIT: {
            char *s   = val_to_string(target);
            char *delim = arg1 ? val_to_string(arg1) : strdup(" ");
            result = val_list();
            int dlen = strlen(delim);
            char *pos = s;
            char *found;
            while ((found = strstr(pos, delim)) != NULL) {
                int seglen = found - pos;
                char *seg  = malloc(seglen + 1);
                strncpy(seg, pos, seglen);
                seg[seglen] = '\0';
                vallist_push(result->as.list, val_string(seg));
                free(seg);
                pos = found + dlen;
            }
            /* last segment */
            vallist_push(result->as.list, val_string(pos));
            free(s); free(delim);
            break;
        }

        case STROP_JOINED: {
            char *delim = arg1 ? val_to_string(arg1) : strdup("");
            if (!target || target->type != VAL_LIST) {
                result = val_string("");
                free(delim);
                break;
            }
            /* estimate size */
            int total = 0;
            int dlen  = strlen(delim);
            for (int i = 0; i < target->as.list->count; i++) {
                char *s = val_to_string(target->as.list->items[i]);
                total += strlen(s) + dlen;
                free(s);
            }
            char *buf = malloc(total + 1);
            buf[0] = '\0';
            for (int i = 0; i < target->as.list->count; i++) {
                char *s = val_to_string(target->as.list->items[i]);
                strcat(buf, s);
                free(s);
                if (i < target->as.list->count - 1) strcat(buf, delim);
            }
            result = val_string(buf);
            free(buf); free(delim);
            break;
        }

        case STROP_REPLACED: {
            char *s       = val_to_string(target);
            char *search  = arg1 ? val_to_string(arg1) : strdup("");
            char *replace = arg2 ? val_to_string(arg2) : strdup("");
            int slen = strlen(search);
            int rlen = strlen(replace);

            if (slen == 0) {
                result = val_string(s);
                free(s); free(search); free(replace);
                break;
            }

            /* count occurrences */
            int count = 0;
            char *pos = s;
            while ((pos = strstr(pos, search)) != NULL) { count++; pos += slen; }

            char *out = malloc(strlen(s) + count * (rlen - slen) + 1);
            char *w   = out;
            pos = s;
            char *found;
            while ((found = strstr(pos, search)) != NULL) {
                int before = found - pos;
                memcpy(w, pos, before); w += before;
                memcpy(w, replace, rlen); w += rlen;
                pos = found + slen;
            }
            strcpy(w, pos);
            result = val_string(out);
            free(s); free(search); free(replace); free(out);
            break;
        }

        case STROP_STARTS_WITH: {
            char *s      = val_to_string(target);
            char *prefix = arg1 ? val_to_string(arg1) : strdup("");
            result = val_boolean(strncmp(s, prefix, strlen(prefix)) == 0);
            free(s); free(prefix);
            break;
        }

        case STROP_ENDS_WITH: {
            char *s      = val_to_string(target);
            char *suffix = arg1 ? val_to_string(arg1) : strdup("");
            int slen = strlen(s);
            int elen = strlen(suffix);
            int matches = (slen >= elen) &&
                          (strcmp(s + slen - elen, suffix) == 0);
            result = val_boolean(matches);
            free(s); free(suffix);
            break;
        }

        case STROP_CONTAINS: {
            char *s      = val_to_string(target);
            char *needle = arg1 ? val_to_string(arg1) : strdup("");
            result = val_boolean(strstr(s, needle) != NULL);
            free(s); free(needle);
            break;
        }

        default:
            result = val_null();
            break;
    }

    val_free(target);
    if (arg1) val_free(arg1);
    if (arg2) val_free(arg2);
    return result ? result : val_null();
}


/* ═════════════════════════════════════════════
   NUMBER FORMATTING — v1.2
   price as currency    → $19.99
   ratio as percentage  → 87.5%
   pi rounded to 2      → 3.14
   big as formatted     → 1,000,000
   ═════════════════════════════════════════════ */
static Value *eval_format_op(Interpreter *interp,
                              ASTNode *node, Env *env) {
    Value *target = eval(interp, node->as.format_op.target, env);
    double num    = (target && target->type == VAL_NUMBER)
                    ? target->as.number : 0.0;
    char buf[128];
    Value *result = NULL;

    switch (node->as.format_op.op) {

        case FMTOP_CURRENCY: {
            /* Always show 2 decimal places, prefix $ */
            snprintf(buf, sizeof(buf), "$%.2f", num);
            result = val_string(buf);
            break;
        }

        case FMTOP_PERCENTAGE: {
            snprintf(buf, sizeof(buf), "%.4g%%", num);
            result = val_string(buf);
            break;
        }

        case FMTOP_ROUNDED: {
            int places = 0;
            if (node->as.format_op.precision) {
                Value *p = eval(interp, node->as.format_op.precision, env);
                places   = (p && p->type == VAL_NUMBER) ? (int)p->as.number : 0;
                val_free(p);
            }
            double factor = pow(10.0, places);
            double rounded = round(num * factor) / factor;
            if (places == 0)
                snprintf(buf, sizeof(buf), "%ld", (long)rounded);
            else
                snprintf(buf, sizeof(buf), "%.*f", places, rounded);
            
            result = val_string(buf);
            break;
        }

        case FMTOP_FORMATTED: {
            /* Add thousands separators */
            long whole = (long)num;
            char tmp[64];
            snprintf(tmp, sizeof(tmp), "%ld", whole < 0 ? -whole : whole);
            int len    = strlen(tmp);
            int commas = (len - 1) / 3;
            char *out  = malloc(len + commas + 3);
            int wi = 0, ti = 0;
            if (whole < 0) out[wi++] = '-';
            int first = len % 3;
            if (first == 0) first = 3;
            for (int i = 0; i < first; i++) out[wi++] = tmp[ti++];
            while (ti < len) {
                out[wi++] = ',';
                for (int i = 0; i < 3; i++) out[wi++] = tmp[ti++];
            }
            out[wi] = '\0';
            result = val_string(out);
            free(out);
            break;
        }

        default:
            result = val_null();
            break;
    }

    val_free(target);
    return result ? result : val_null();
}


/* ═════════════════════════════════════════════
   TYPE CHECKING — v1.2
   x is a number  → true/false
   name is a string
   items is a list
   ═════════════════════════════════════════════ */
static Value *eval_type_check(Interpreter *interp,
                               ASTNode *node, Env *env) {
    Value *target = eval(interp, node->as.type_check.target, env);
    int result    = 0;

    switch (node->as.type_check.kind) {
        case TYPECHECK_NUMBER:  result = target && target->type == VAL_NUMBER;  break;
        case TYPECHECK_STRING:  result = target && target->type == VAL_STRING;  break;
        case TYPECHECK_BOOLEAN: result = target && target->type == VAL_BOOLEAN; break;
        case TYPECHECK_LIST:    result = target && target->type == VAL_LIST;    break;
        case TYPECHECK_NULL:    result = !target || target->type == VAL_NULL;   break;
    }

    val_free(target);
    return val_boolean(result);
}
Value *eval(Interpreter *interp, ASTNode *node, Env *env) {
    if (!node) return val_null();
    if (interp->signal.type != SIG_NONE &&
        interp->signal.type != SIG_RETURN) return val_null();

    switch (node->type) {

        /* ── Literals ── */
        case NODE_NUMBER:
            return val_number(node->as.number.value);

        case NODE_STRING:
            return val_string(node->as.string.value);

        case NODE_BOOLEAN:
            return val_boolean(node->as.boolean.value);

        case NODE_NULL:
            return val_null();

        /* ── Identifier — variable or activity call ── */
        case NODE_IDENTIFIER: {
            Value *v = env_get(env, node->as.identifier.name);
            if (!v) return val_null();
            if (v->type == VAL_ACTIVITY) {
                ValueList args;
                vallist_init(&args);
                Value *result = call_activity(interp,
                    node->as.identifier.name, &args, env);
                vallist_free(&args);
                return result;
            }
            return val_copy(v);
        }

        /* ── Activity call — set x by adding n1 and n2 ── */
        case NODE_ACTIVITY_CALL: {
            ValueList args;
            vallist_init(&args);
            for (int i = 0; i < node->as.activity_call.args.count; i++) {
                Value *arg = eval(interp,
                    node->as.activity_call.args.items[i], env);
                vallist_push(&args, arg);
            }
            Value *result = call_activity(interp,
                node->as.activity_call.name, &args, env);
            vallist_free(&args);
            return result;
        }

        /* ── List literal ── */
        case NODE_LIST: {
            Value *list = val_list();
            for (int i = 0; i < node->as.list.elements.count; i++) {
                Value *elem = eval(interp,
                    node->as.list.elements.items[i], env);
                vallist_push(list->as.list, elem);
            }
            return list;
        }

        /* ── Inline if ── */
        case NODE_INLINE_IF: {
            Value *cond = eval(interp, node->as.inline_if.condition, env);
            int truthy  = val_is_truthy(cond);
            val_free(cond);
            if (truthy)
                return eval(interp, node->as.inline_if.then_val, env);
            else
                return eval(interp, node->as.inline_if.else_val, env);
        }

        /* ── Graph ops — v1.3 ── */
        case NODE_MAKE_GRAPH:
            return val_graph();

        case NODE_ROUTE:
            return eval_route(interp, node, env);

        case NODE_CONNECTED:
            return eval_connected(interp, node, env);

        /* ── Smart Collection op — v1.1 ── */
        case NODE_COLLECTION_OP:
            return eval_collection_op(interp, node, env);

        /* ── Where filter — v1.1 ── */
        case NODE_WHERE_FILTER:
            return eval_where_filter(interp, node, env);

        /* ── Index access — v1.5: item N of LIST ── */
        case NODE_INDEX_ACCESS: {
            Value *list  = eval(interp, node->as.index_access.list,  env);
            Value *idx_v = eval(interp, node->as.index_access.index, env);
            if (!list || list->type != VAL_LIST || !idx_v ||
                idx_v->type != VAL_NUMBER) {
                val_free(list); val_free(idx_v);
                return val_null();
            }
            int idx = (int)idx_v->as.number;
            val_free(idx_v);
            if (idx < 0 || idx >= list->as.list->count) {
                val_free(list);
                return val_null();
            }
            Value *result = val_copy(list->as.list->items[idx]);
            val_free(list);
            return result;
        }

        /* ── String op — v1.2 ── */
        case NODE_STRING_OP:
            return eval_string_op(interp, node, env);

        /* ── Format op — v1.2 ── */
        case NODE_FORMAT_OP:
            return eval_format_op(interp, node, env);

        /* ── Type check — v1.2 ── */
        case NODE_TYPE_CHECK:
            return eval_type_check(interp, node, env);

        /* ── Unary op ── */
        case NODE_UNARY_OP: {
            Value *operand = eval(interp, node->as.unary_op.operand, env);
            if (node->as.unary_op.operator == TOKEN_NOT) {
                int result = !val_is_truthy(operand);
                val_free(operand);
                return val_boolean(result);
            }
            return operand;
        }

        /* ── Binary op ── */
        case NODE_BINARY_OP: {
            TokenType op = node->as.binary_op.operator;

            if (op == TOKEN_AND) {
                Value *left = eval(interp, node->as.binary_op.left, env);
                if (!val_is_truthy(left)) return left;
                val_free(left);
                return eval(interp, node->as.binary_op.right, env);
            }
            if (op == TOKEN_OR) {
                Value *left = eval(interp, node->as.binary_op.left, env);
                if (val_is_truthy(left)) return left;
                val_free(left);
                return eval(interp, node->as.binary_op.right, env);
            }

            Value *left  = eval(interp, node->as.binary_op.left,  env);
            Value *right = eval(interp, node->as.binary_op.right, env);
            Value *result = NULL;

            switch (op) {

                case TOKEN_PLUS:
                    if (left->type == VAL_STRING || right->type == VAL_STRING) {
                        char *ls = val_to_string(left);
                        char *rs = val_to_string(right);
                        char *cat = malloc(strlen(ls) + strlen(rs) + 1);
                        strcpy(cat, ls); strcat(cat, rs);
                        result = val_string(cat);
                        free(ls); free(rs); free(cat);
                    } else {
                        result = val_number(left->as.number + right->as.number);
                    }
                    break;

                case TOKEN_MINUS:
                    result = val_number(left->as.number - right->as.number);
                    break;

                case TOKEN_TIMES:
                case TOKEN_TIMES_OP:
                    result = val_number(left->as.number * right->as.number);
                    break;

                case TOKEN_DIVIDED_BY:
                    if (right->as.number == 0) {
                        runtime_error(interp, node, "Division by zero");
                        result = val_string("Error: Division by zero");
                    } else {
                        result = val_number(left->as.number / right->as.number);
                    }
                    break;

                case TOKEN_MODULO:
                    result = val_number(fmod(left->as.number, right->as.number));
                    break;

                case TOKEN_POWER_OF:
                    result = val_number(pow(left->as.number, right->as.number));
                    break;

                case TOKEN_EQUALS:
                case TOKEN_IS:
                    result = val_boolean(val_equals(left, right));
                    break;

                case TOKEN_IS_NOT:
                case TOKEN_NOT_EQUALS:
                    result = val_boolean(!val_equals(left, right));
                    break;

                case TOKEN_GREATER_THAN:
                    if (left->type == VAL_NUMBER && right->type == VAL_NUMBER)
                        result = val_boolean(left->as.number > right->as.number);
                    else result = val_boolean(0);
                    break;

                case TOKEN_LESS_THAN:
                    if (left->type == VAL_NUMBER && right->type == VAL_NUMBER)
                        result = val_boolean(left->as.number < right->as.number);
                    else result = val_boolean(0);
                    break;

                case TOKEN_AT_LEAST:
                    if (left->type == VAL_NUMBER && right->type == VAL_NUMBER)
                        result = val_boolean(left->as.number >= right->as.number);
                    else result = val_boolean(0);
                    break;

                case TOKEN_AT_MOST:
                    if (left->type == VAL_NUMBER && right->type == VAL_NUMBER)
                        result = val_boolean(left->as.number <= right->as.number);
                    else result = val_boolean(0);
                    break;

                case TOKEN_IS_ANY_OF: {
                    int found = 0;
                    if (right->type == VAL_LIST) {
                        for (int i = 0; i < right->as.list->count; i++) {
                            if (val_equals(left, right->as.list->items[i])) {
                                found = 1; break;
                            }
                        }
                    }
                    result = val_boolean(found);
                    break;
                }

                case TOKEN_IS_NONE_OF: {
                    int found = 0;
                    if (right->type == VAL_LIST) {
                        for (int i = 0; i < right->as.list->count; i++) {
                            if (val_equals(left, right->as.list->items[i])) {
                                found = 1; break;
                            }
                        }
                    }
                    result = val_boolean(!found);
                    break;
                }

                case TOKEN_IN: {
                    int found = 0;
                    if (right->type == VAL_LIST) {
                        for (int i = 0; i < right->as.list->count; i++) {
                            if (val_equals(left, right->as.list->items[i])) {
                                found = 1; break;
                            }
                        }
                    }
                    result = val_boolean(found);
                    break;
                }

                default:
                    result = val_null();
                    break;
            }

            val_free(left);
            val_free(right);
            return result ? result : val_null();
        }

        default:
            return val_null();
    }
}


/* ═════════════════════════════════════════════
   ACTIVITY CALL
   ═════════════════════════════════════════════ */
static Value *call_activity(Interpreter *interp, const char *name,
                             ValueList *args, Env *env) {
    if (interp->call_depth >= MAX_CALL_DEPTH) {
        fprintf(stderr, "[EPL Error] Maximum call depth exceeded\n");
        interp->had_error = 1;
        return val_null();
    }

    Value *activity_val = env_get(env, name);
    if (!activity_val || activity_val->type != VAL_ACTIVITY) {
        return val_null();
    }

    ASTNode *def   = activity_val->as.activity;
    Env     *local = env_new(env);

    static const char *param_names[] = {"x", "y", "z", "a", "b", "c"};
    int max_params = 6;
    for (int i = 0; i < args->count && i < max_params; i++) {
        env_set(local, param_names[i], val_copy(args->items[i]));
    }

    interp->call_depth++;
    exec_block(interp, &def->as.activity_def.body, local);
    interp->call_depth--;

    Value *result = val_null();
    if (interp->signal.type == SIG_RETURN) {
        result = interp->signal.value
                    ? interp->signal.value : val_null();
        interp->signal.type  = SIG_NONE;
        interp->signal.value = NULL;
    }

    env_free(local);
    return result;
}


/* ═════════════════════════════════════════════
   EXEC NODE — STATEMENT EXECUTOR
   ═════════════════════════════════════════════ */
void exec_node(Interpreter *interp, ASTNode *node, Env *env) {
    if (!node) return;
    if (interp->signal.type != SIG_NONE) return;

    switch (node->type) {

        case NODE_DESCRIBE:
            break;

        case NODE_LINK:
            exec_link(interp, node, env);
            break;

        case NODE_MAKE_PAGE:
            codegen_page(interp, env, node);
            break;

        case NODE_SET: {
            Value *val = eval(interp, node->as.assignment.value, env);
            env_set(env, node->as.assignment.name, val);
            break;
        }

        case NODE_UPDATE: {
            Value *val = eval(interp, node->as.assignment.value, env);
            if (!env_update(env, node->as.assignment.name, val)) {
                env_set(env, node->as.assignment.name, val);
            }
            break;
        }

        case NODE_SHOW: {
            Value *val = eval(interp, node->as.show.value, env);
            val_print(val);
            printf("\n");
            val_free(val);
            break;
        }

        case NODE_IF: {
            Value *cond = eval(interp, node->as.if_stmt.condition, env);
            int truthy  = val_is_truthy(cond);
            val_free(cond);

            Env *branch_env = env_new(env);
            if (truthy) {
                exec_block(interp, &node->as.if_stmt.then_body, branch_env);
            } else if (node->as.if_stmt.else_body.count > 0) {
                exec_block(interp, &node->as.if_stmt.else_body, branch_env);
            }
            env_free(branch_env);
            break;
        }

        case NODE_WHILE: {
            while (1) {
                Value *cond = eval(interp, node->as.while_stmt.condition, env);
                int truthy = val_is_truthy(cond);
                val_free(cond);
                if (!truthy) break;

                Env *loop_env = env_new(env);
                exec_block(interp, &node->as.while_stmt.body, loop_env);
                env_free(loop_env);

                if (interp->signal.type == SIG_BREAK) {
                    interp->signal.type = SIG_NONE; break;
                }
                if (interp->signal.type == SIG_CONTINUE) {
                    interp->signal.type = SIG_NONE; continue;
                }
                if (interp->signal.type != SIG_NONE) break;
            }
            break;
        }

        case NODE_REPEAT_INDEFINITELY: {
            while (1) {
                Env *loop_env = env_new(env);
                exec_block(interp, &node->as.repeat_indefinitely.body, loop_env);
                env_free(loop_env);

                if (interp->signal.type == SIG_BREAK) {
                    interp->signal.type = SIG_NONE; break;
                }
                if (interp->signal.type == SIG_CONTINUE) {
                    interp->signal.type = SIG_NONE; continue;
                }
                if (interp->signal.type != SIG_NONE) break;
            }
            break;
        }

        case NODE_REPEAT_TIMES: {
            Value *count_val = eval(interp, node->as.repeat_times.count, env);
            int count = (int)count_val->as.number;
            val_free(count_val);

            for (int i = 0; i < count; i++) {
                Env *loop_env = env_new(env);
                if (node->as.repeat_times.counter[0] != '\0') {
                    env_set(loop_env,
                        node->as.repeat_times.counter, val_number(i));
                }
                exec_block(interp, &node->as.repeat_times.body, loop_env);
                env_free(loop_env);

                if (interp->signal.type == SIG_BREAK) {
                    interp->signal.type = SIG_NONE; break;
                }
                if (interp->signal.type == SIG_CONTINUE) {
                    interp->signal.type = SIG_NONE; continue;
                }
                if (interp->signal.type != SIG_NONE) break;
            }
            break;
        }

        case NODE_REPEAT_UNTIL: {
            while (1) {
                Value *cond = eval(interp, node->as.repeat_until.condition, env);
                int done = val_is_truthy(cond);
                val_free(cond);
                if (done) break;

                Env *loop_env = env_new(env);
                exec_block(interp, &node->as.repeat_until.body, loop_env);
                env_free(loop_env);

                if (interp->signal.type == SIG_BREAK) {
                    interp->signal.type = SIG_NONE; break;
                }
                if (interp->signal.type == SIG_CONTINUE) {
                    interp->signal.type = SIG_NONE; continue;
                }
                if (interp->signal.type != SIG_NONE) break;
            }
            break;
        }

        case NODE_FOR_EACH: {
            Value *collection = eval(interp, node->as.for_each.collection, env);

            if (collection->type == VAL_LIST) {
                for (int i = 0; i < collection->as.list->count; i++) {
                    Env *loop_env = env_new(env);
                    env_set(loop_env, node->as.for_each.var,
                        val_copy(collection->as.list->items[i]));
                    exec_block(interp, &node->as.for_each.body, loop_env);
                    env_free(loop_env);

                    if (interp->signal.type == SIG_BREAK) {
                        interp->signal.type = SIG_NONE; break;
                    }
                    if (interp->signal.type == SIG_CONTINUE) {
                        interp->signal.type = SIG_NONE; continue;
                    }
                    if (interp->signal.type != SIG_NONE) break;
                }
            }
            val_free(collection);
            break;
        }

        case NODE_FOR_RANGE: {
            Value *from_val = eval(interp, node->as.for_range.from, env);
            Value *to_val   = eval(interp, node->as.for_range.to, env);
            int from = (int)from_val->as.number;
            int to   = (int)to_val->as.number;
            val_free(from_val); val_free(to_val);

            int step = node->as.for_range.downward ? -1 : 1;
            for (int i = from;
                 node->as.for_range.downward ? i >= to : i <= to;
                 i += step) {
                Env *loop_env = env_new(env);
                env_set(loop_env, node->as.for_range.var, val_number(i));
                exec_block(interp, &node->as.for_range.body, loop_env);
                env_free(loop_env);

                if (interp->signal.type == SIG_BREAK) {
                    interp->signal.type = SIG_NONE; break;
                }
                if (interp->signal.type == SIG_CONTINUE) {
                    interp->signal.type = SIG_NONE; continue;
                }
                if (interp->signal.type != SIG_NONE) break;
            }
            break;
        }

        case NODE_ACTIVITY_DEF: {
            Value *activity = val_activity(node);
            env_set(env, node->as.activity_def.name, activity);
            break;
        }

        case NODE_RETURN: {
            Value *val = eval(interp, node->as.return_stmt.value, env);
            interp->signal.type  = SIG_RETURN;
            interp->signal.value = val;
            break;
        }

        case NODE_BREAK:
            interp->signal.type = SIG_BREAK;
            break;

        case NODE_CONTINUE:
            interp->signal.type = SIG_CONTINUE;
            break;

        case NODE_CHECK: {
            Value *cond = eval(interp, node->as.check_stmt.condition, env);
            if (!val_is_truthy(cond)) {
                fprintf(stderr,
                    "[EPL Check Failed] Line %d: assertion failed\n",
                    node->line);
                interp->had_error = 1;
            }
            val_free(cond);
            break;
        }

        case NODE_PROVIDE: {
            Value *prompt = eval(interp, node->as.provide_stmt.prompt, env);
            char  *prompt_str = val_to_string(prompt);
            val_free(prompt);

            printf("%s", prompt_str);
            free(prompt_str);
            fflush(stdout);

            char buf[1024];
            if (fgets(buf, sizeof(buf), stdin)) {
                int len = strlen(buf);
                if (len > 0 && buf[len-1] == '\n') buf[len-1] = '\0';

                char *end;
                double num = strtod(buf, &end);
                Value *val;
                if (*end == '\0' && end != buf) {
                    val = val_number(num);
                } else {
                    val = val_string(buf);
                }
                env_set(env, node->as.provide_stmt.var, val);
            }
            break;
        }

        case NODE_ATTEMPT: {
            int prev_error = interp->had_error;
            interp->had_error = 0;

            exec_block(interp, &node->as.attempt_stmt.try_body, env);

            if (interp->had_error &&
                node->as.attempt_stmt.fail_body.count > 0) {
                interp->had_error    = 0;
                interp->signal.type  = SIG_NONE;
                exec_block(interp, &node->as.attempt_stmt.fail_body, env);
            }
            interp->had_error = prev_error;
            break;
        }

        case NODE_ADD_TO: {
            Value *item = eval(interp, node->as.add_to.item, env);
            Value *list = env_get(env, node->as.add_to.list);
            if (list && list->type == VAL_LIST) {
                vallist_push(list->as.list, item);
            } else {
                val_free(item);
            }
            break;
        }

        case NODE_WAIT: {
            printf("%s", node->as.wait_stmt.message);
            fflush(stdout);
            char buf[256];
            fgets(buf, sizeof(buf), stdin);
            break;
        }

        case NODE_EXECUTE: {
            const char *cmd = node->as.execute_stmt.command;
            if (node->as.execute_stmt.conditional) {
                #ifdef _WIN32
                    system(cmd);
                #else
                    system(node->as.execute_stmt.alt_command);
                #endif
            } else {
                system(cmd);
            }
            break;
        }

        default:
            break;
    }
}


/* ═════════════════════════════════════════════
   EXEC BLOCK
   ═════════════════════════════════════════════ */
void exec_block(Interpreter *interp, NodeList *list, Env *env) {
    for (int i = 0; i < list->count; i++) {
        if (interp->signal.type != SIG_NONE) break;
        exec_node(interp, list->items[i], env);
    }
}


/* ═════════════════════════════════════════════
   INTERPRETER RUN — MAIN ENTRY
   ═════════════════════════════════════════════ */
void interpreter_run(Interpreter *interp, ASTNode *program) {
    if (!program || program->type != NODE_PROGRAM) return;

    for (int i = 0; i < program->as.program.body.count; i++) {
        ASTNode *node = program->as.program.body.items[i];
        if (node && node->type == NODE_ACTIVITY_DEF) {
            exec_node(interp, node, interp->global);
        }
    }

    exec_block(interp, &program->as.program.body, interp->global);
}
