#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include "describe.h"

/* ─────────────────────────────────────────────
   EPL Semantic Index — describe.c  (v1.5)
   Walks AST, pairs describe nodes with their
   targets, writes structured .EPD output.
   ───────────────────────────────────────────── */


/* ═════════════════════════════════════════════
   INDEX MANAGEMENT
   ═════════════════════════════════════════════ */
static void epd_push(EPDIndex *idx, EPDEntry entry) {
    if (idx->count >= idx->capacity) {
        idx->capacity = idx->capacity == 0 ? 16 : idx->capacity * 2;
        idx->entries  = realloc(idx->entries,
                                idx->capacity * sizeof(EPDEntry));
    }
    idx->entries[idx->count++] = entry;
}


/* ═════════════════════════════════════════════
   TYPE INFERENCE
   Guess the data type of a set node's value
   ═════════════════════════════════════════════ */
static const char *infer_type(ASTNode *value_node) {
    if (!value_node) return "unknown";
    switch (value_node->type) {
        case NODE_NUMBER:      return "number";
        case NODE_STRING:      return "string";
        case NODE_BOOLEAN:     return "boolean";
        case NODE_NULL:        return "null";
        case NODE_LIST:        return "list";
        case NODE_MAKE_GRAPH:  return "graph";
        case NODE_IDENTIFIER:  return "reference";
        case NODE_BINARY_OP:   return "expression";
        case NODE_COLLECTION_OP: return "number";   /* aggregation result */
        case NODE_WHERE_FILTER:  return "list";
        case NODE_STRING_OP:     return "string";
        case NODE_FORMAT_OP:     return "string";
        default:               return "value";
    }
}


/* ═════════════════════════════════════════════
   PARAMETER EXTRACTION
   Walk activity body to collect used param names
   ═════════════════════════════════════════════ */
static void collect_params_from_node(ASTNode *node,
                                      char *buf, int buflen,
                                      int *first);

static void collect_params_from_list(NodeList *list,
                                      char *buf, int buflen,
                                      int *first) {
    for (int i = 0; i < list->count; i++)
        collect_params_from_node(list->items[i], buf, buflen, first);
}

static void collect_params_from_node(ASTNode *node,
                                      char *buf, int buflen,
                                      int *first) {
    if (!node) return;

    /* We look for identifiers that are single lowercase letters
       or classic param names — x, y, z, a, b, c, n, m, p, q */
    static const char *classic[] = {
        "x","y","z","a","b","c","n","m","p","q",
        "x1","x2","y1","y2","n1","n2",NULL
    };

    if (node->type == NODE_IDENTIFIER) {
        const char *name = node->as.identifier.name;
        for (int i = 0; classic[i]; i++) {
            if (strcmp(name, classic[i]) == 0) {
                /* check not already in buf */
                char search[32];
                snprintf(search, sizeof(search), "%s", name);
                if (!strstr(buf, search)) {
                    if (!*first) strncat(buf, ", ", buflen - strlen(buf) - 1);
                    strncat(buf, name, buflen - strlen(buf) - 1);
                    *first = 0;
                }
                break;
            }
        }
        return;
    }

    /* recurse */
    switch (node->type) {
        case NODE_BINARY_OP:
            collect_params_from_node(node->as.binary_op.left,  buf, buflen, first);
            collect_params_from_node(node->as.binary_op.right, buf, buflen, first);
            break;
        case NODE_UNARY_OP:
            collect_params_from_node(node->as.unary_op.operand, buf, buflen, first);
            break;
        case NODE_RETURN:
            collect_params_from_node(node->as.return_stmt.value, buf, buflen, first);
            break;
        case NODE_SHOW:
            collect_params_from_node(node->as.show.value, buf, buflen, first);
            break;
        case NODE_IF:
            collect_params_from_node(node->as.if_stmt.condition, buf, buflen, first);
            collect_params_from_list(&node->as.if_stmt.then_body, buf, buflen, first);
            collect_params_from_list(&node->as.if_stmt.else_body, buf, buflen, first);
            break;
        case NODE_ACTIVITY_DEF:
            collect_params_from_list(&node->as.activity_def.body, buf, buflen, first);
            break;
        default:
            break;
    }
}


/* ═════════════════════════════════════════════
   AST WALKER
   Pairs each describe node with the node
   that immediately follows it
   ═════════════════════════════════════════════ */
static void walk_nodelist(NodeList *list, EPDIndex *idx,
                           int is_top_level);

static void process_pair(ASTNode *desc, ASTNode *target,
                          EPDIndex *idx) {
    EPDEntry e;
    memset(&e, 0, sizeof(e));

    /* copy description text */
    strncpy(e.description, desc->as.describe_stmt.text,
            sizeof(e.description) - 1);
    strncpy(e.name, desc->as.describe_stmt.name,
            sizeof(e.name) - 1);
    e.line_start = desc->line;
    e.line_end   = desc->line;

    if (!target) {
        /* orphan describe — standalone block description */
        e.type     = EPD_BLOCK;
        e.line_end = desc->line;
        epd_push(idx, e);
        return;
    }

    switch (target->type) {

        case NODE_ACTIVITY_DEF: {
            e.type       = EPD_ACTIVITY;
            e.line_start = target->line;
            e.line_end   = target->end_line > target->line
                           ? target->end_line : target->line;
            strncpy(e.name, target->as.activity_def.name,
                    sizeof(e.name) - 1);

            /* extract params */
            int first = 1;
            collect_params_from_list(&target->as.activity_def.body,
                e.params, sizeof(e.params), &first);
            break;
        }

        case NODE_SET: {
            e.type       = EPD_DATA;
            e.line_start = target->line;
            e.line_end   = target->line;
            strncpy(e.name, target->as.assignment.name,
                    sizeof(e.name) - 1);
            strncpy(e.data_type,
                    infer_type(target->as.assignment.value),
                    sizeof(e.data_type) - 1);
            break;
        }

        case NODE_MAKE_PAGE: {
            e.type       = EPD_PAGE;
            e.line_start = target->line;
            e.line_end   = target->end_line > target->line
                           ? target->end_line : target->line;
            if (strlen(e.name) == 0)
                strncpy(e.name, target->as.make_page.filename,
                        sizeof(e.name) - 1);
            break;
        }

        case NODE_SECTION_OUT: {
            e.type       = EPD_SECTION;
            e.line_start = target->line;
            e.line_end   = target->end_line > target->line
                           ? target->end_line : target->line;
            if (strlen(e.name) == 0)
                strncpy(e.name, target->as.section_out.title,
                        sizeof(e.name) - 1);
            break;
        }

        default:
            e.type     = EPD_BLOCK;
            e.line_end = target->end_line > target->line
                         ? target->end_line : target->line;
            break;
    }

    epd_push(idx, e);

    /* recurse into activity bodies and page children */
    if (target->type == NODE_ACTIVITY_DEF)
        walk_nodelist(&target->as.activity_def.body, idx, 0);
    if (target->type == NODE_MAKE_PAGE)
        walk_nodelist(&target->as.make_page.children, idx, 0);
    if (target->type == NODE_SECTION_OUT)
        walk_nodelist(&target->as.section_out.children, idx, 0);
}

static void walk_nodelist(NodeList *list, EPDIndex *idx,
                           int is_top_level) {
    for (int i = 0; i < list->count; i++) {
        ASTNode *node = list->items[i];
        if (!node) continue;

        if (node->type == NODE_DESCRIBE) {
            /* pair with next non-null sibling */
            ASTNode *target = NULL;
            for (int j = i + 1; j < list->count; j++) {
                if (list->items[j]) { target = list->items[j]; break; }
            }
            process_pair(node, target, idx);

        } else if (is_top_level) {
            /* for top-level non-describe nodes with no preceding describe,
               recurse into their children to find nested describes */
            switch (node->type) {
                case NODE_WHILE:
                    walk_nodelist(&node->as.while_stmt.body, idx, 0);
                    break;
                case NODE_IF:
                    walk_nodelist(&node->as.if_stmt.then_body, idx, 0);
                    walk_nodelist(&node->as.if_stmt.else_body, idx, 0);
                    break;
                case NODE_FOR_EACH:
                    walk_nodelist(&node->as.for_each.body, idx, 0);
                    break;
                case NODE_FOR_RANGE:
                    walk_nodelist(&node->as.for_range.body, idx, 0);
                    break;
                case NODE_MAKE_PAGE:
                    walk_nodelist(&node->as.make_page.children, idx, 0);
                    break;
                default:
                    break;
            }
        }
    }
}


/* ═════════════════════════════════════════════
   BUILD INDEX
   ═════════════════════════════════════════════ */
EPDIndex *epd_build(ASTNode *program, const char *source_file) {
    EPDIndex *idx = calloc(1, sizeof(EPDIndex));

    if (!program || program->type != NODE_PROGRAM) return idx;

    /* Check if first node is a program-level describe */
    NodeList *body = &program->as.program.body;
    int start = 0;

    if (body->count > 0 && body->items[0] &&
        body->items[0]->type == NODE_DESCRIBE) {
        EPDEntry e;
        memset(&e, 0, sizeof(e));
        e.type       = EPD_PROGRAM;
        e.line_start = 1;
        e.line_end   = program->end_line > 0
                       ? program->end_line : body->count;
        strncpy(e.description,
                body->items[0]->as.describe_stmt.text,
                sizeof(e.description) - 1);
        strncpy(e.name, source_file, sizeof(e.name) - 1);
        epd_push(idx, e);
        start = 1;   /* skip the first describe in the main walk */
    }

    /* Walk the rest */
    NodeList remainder;
    remainder.items    = body->items + start;
    remainder.count    = body->count - start;
    remainder.capacity = body->capacity;
    walk_nodelist(&remainder, idx, 1);

    return idx;
}


/* ═════════════════════════════════════════════
   WRITE .EPD FILE
   ═════════════════════════════════════════════ */
static const char *epd_type_label(EPDEntryType t) {
    switch (t) {
        case EPD_PROGRAM:  return "program ";
        case EPD_ACTIVITY: return "activity";
        case EPD_BLOCK:    return "block   ";
        case EPD_DATA:     return "data    ";
        case EPD_PAGE:     return "page    ";
        case EPD_SECTION:  return "section ";
        default:           return "unknown ";
    }
}

void epd_write(EPDIndex *idx, const char *source_file,
               const char *out_file) {
    FILE *f = fopen(out_file, "w");
    if (!f) {
        fprintf(stderr,
            "[EPL Error] Cannot write .EPD file: %s\n", out_file);
        return;
    }

    /* Header */
    time_t now = time(NULL);
    struct tm *tm_info = localtime(&now);
    char ts[32];
    strftime(ts, sizeof(ts), "%Y-%m-%d %H:%M:%S", tm_info);

    fprintf(f, "EPD 1.0 — %s\n", source_file);
    fprintf(f, "generated: %s\n", ts);
    fprintf(f,
        "─────────────────────────────────────────────\n\n");

    if (idx->count == 0) {
        fprintf(f, "(no describe blocks found)\n");
        fprintf(f,
            "\nTip: Add describe blocks to your .EP or .EPF file:\n");
        fprintf(f,
            "  describe program as \"What this file does\"\n");
        fprintf(f,
            "  describe add as \"Returns sum of x and y\"\n");
        fclose(f);
        return;
    }

    for (int i = 0; i < idx->count; i++) {
        EPDEntry *e = &idx->entries[i];

        /* Entry header line */
        if (e->line_start == e->line_end) {
            fprintf(f, "[%s]  line %-4d    ",
                epd_type_label(e->type), e->line_start);
        } else {
            int end_digits = e->line_end > 0 ? (int)log10(e->line_end + 1) + 1 : 1;
            fprintf(f, "[%s]  lines %d-%d%*s",
                epd_type_label(e->type),
                e->line_start, e->line_end,
                (int)(8 - end_digits), "");
        }

        /* Name */
        if (strlen(e->name) > 0)
            fprintf(f, "  %s", e->name);

        fprintf(f, "\n");

        /* Params (activities only) */
        if (e->type == EPD_ACTIVITY && strlen(e->params) > 0)
            fprintf(f, "  params: %s\n", e->params);

        /* Data type (data entries only) */
        if (e->type == EPD_DATA && strlen(e->data_type) > 0)
            fprintf(f, "  type: %s\n", e->data_type);

        /* Description */
        fprintf(f, "  \"%s\"\n", e->description);
        fprintf(f, "\n");
    }

    /* Footer stats */
    int acts = 0, blks = 0, data = 0, pages = 0, sects = 0;
    for (int i = 0; i < idx->count; i++) {
        switch (idx->entries[i].type) {
            case EPD_ACTIVITY: acts++;  break;
            case EPD_BLOCK:    blks++;  break;
            case EPD_DATA:     data++;  break;
            case EPD_PAGE:     pages++; break;
            case EPD_SECTION:  sects++; break;
            default: break;
        }
    }

    fprintf(f,
        "─────────────────────────────────────────────\n");
    fprintf(f, "entries: %d total", idx->count);
    if (acts)  fprintf(f, "  |  %d activities", acts);
    if (blks)  fprintf(f, "  |  %d blocks",     blks);
    if (data)  fprintf(f, "  |  %d data",        data);
    if (pages) fprintf(f, "  |  %d pages",       pages);
    if (sects) fprintf(f, "  |  %d sections",    sects);
    fprintf(f, "\n");

    fclose(f);
    printf("[EPL] Semantic index: %s  (%d entries)\n",
           out_file, idx->count);
}


/* ═════════════════════════════════════════════
   FREE
   ═════════════════════════════════════════════ */
void epd_free(EPDIndex *idx) {
    if (!idx) return;
    free(idx->entries);
    free(idx);
}
