#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include <time.h>
#include <math.h>
#include "describe.h"

#include "ast_serial.inc"

static void epd_push(EPDIndex *idx, EPDEntry entry) {
    if (idx->count >= idx->capacity) {
        idx->capacity = idx->capacity == 0 ? 16 : idx->capacity * 2;
        idx->entries  = realloc(idx->entries,
                                idx->capacity * sizeof(EPDEntry));
    }
    idx->entries[idx->count++] = entry;
}

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
        case NODE_COLLECTION_OP: return "number";
        case NODE_WHERE_FILTER:  return "list";
        case NODE_STRING_OP:     return "string";
        case NODE_FORMAT_OP:     return "string";
        default:               return "value";
    }
}

static void walk_nodelist(NodeList *list, EPDIndex *idx,
                           int is_top_level);

static void process_pair(ASTNode *desc, ASTNode *target,
                          EPDIndex *idx) {
    EPDEntry e;
    memset(&e, 0, sizeof(e));

    strncpy(e.description, desc->as.describe_stmt.text,
            sizeof(e.description) - 1);
    strncpy(e.name, desc->as.describe_stmt.name,
            sizeof(e.name) - 1);
    e.line_start = desc->line;
    e.line_end   = desc->line;

    if (!target) {
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

            NodeList *ps = &target->as.activity_def.params;
            for (int i = 0; i < ps->count; i++) {
                if (i) strncat(e.params, ", ",
                               sizeof(e.params) - strlen(e.params) - 1);
                strncat(e.params, ps->items[i]->as.identifier.name,
                        sizeof(e.params) - strlen(e.params) - 1);
            }
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
            ASTNode *target = NULL;
            for (int j = i + 1; j < list->count; j++) {
                if (list->items[j]) { target = list->items[j]; break; }
            }
            process_pair(node, target, idx);

        } else if (is_top_level) {
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

EPDIndex *epd_build(ASTNode *program, const char *source_file) {
    EPDIndex *idx = calloc(1, sizeof(EPDIndex));

    if (!program || program->type != NODE_PROGRAM) return idx;

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
        start = 1;
    }

    NodeList remainder;
    remainder.items    = body->items + start;
    remainder.count    = body->count - start;
    remainder.capacity = body->capacity;
    walk_nodelist(&remainder, idx, 1);

    return idx;
}

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

        if (strlen(e->name) > 0)
            fprintf(f, "  %s", e->name);

        fprintf(f, "\n");

        if (e->type == EPD_ACTIVITY && strlen(e->params) > 0)
            fprintf(f, "  params: %s\n", e->params);

        if (e->type == EPD_DATA && strlen(e->data_type) > 0)
            fprintf(f, "  type: %s\n", e->data_type);

        fprintf(f, "  \"%s\"\n", e->description);
        fprintf(f, "\n");
    }

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

void epd_free(EPDIndex *idx) {
    if (!idx) return;
    free(idx->entries);
    free(idx);
}

static const char *epd_program_desc(EPDIndex *idx) {
    for (int i = 0; i < idx->count; i++)
        if (idx->entries[i].type == EPD_PROGRAM)
            return idx->entries[i].description;
    return NULL;
}

static void cg_push(CallGraph *cg, const char *mod, const char *caller,
                    const char *callee, int line) {
    if (cg->count >= cg->capacity) {
        cg->capacity = cg->capacity ? cg->capacity * 2 : 64;
        cg->edges = realloc(cg->edges, (size_t)cg->capacity * sizeof(CallEdge));
    }
    CallEdge *e = &cg->edges[cg->count++];
    snprintf(e->module, sizeof e->module, "%s", mod);
    snprintf(e->caller, sizeof e->caller, "%s", caller);
    snprintf(e->callee, sizeof e->callee, "%s", callee);
    e->line = line;
    e->via  = 0;
}

static void cg_push_def(CallGraph *cg, const char *mod, const char *name) {
    if (cg->def_count >= cg->def_capacity) {
        cg->def_capacity = cg->def_capacity ? cg->def_capacity * 2 : 64;
        cg->defs = realloc(cg->defs, (size_t)cg->def_capacity * sizeof(ActivityDef));
    }
    ActivityDef *dfn = &cg->defs[cg->def_count++];
    snprintf(dfn->module, sizeof dfn->module, "%s", mod);
    snprintf(dfn->name,   sizeof dfn->name,   "%s", name);
}

static void cg_walk(ASTNode *n, const char *mod, const char *scope,
                    CallGraph *cg) {
    if (!n) return;

    if (n->type == NODE_ACTIVITY_CALL)
        cg_push(cg, mod, scope, n->as.activity_call.name, n->line);
    if (n->type == NODE_SERVE_PORT)
        for (int i = 0; i < n->as.serve_port.offered.count; i++) {
            ASTNode *id = n->as.serve_port.offered.items[i];
            cg_push(cg, mod, scope, id->as.identifier.name, id->line);
            cg->edges[cg->count - 1].via = 'o';
        }
    if (n->type == NODE_ACTION_FORM || n->type == NODE_ACTION_BUTTON) {
        cg_push(cg, mod, scope, n->as.action_form.activity, n->line);
        cg->edges[cg->count - 1].via = 'p';
    }
    if (n->type == NODE_ACTIVITY_DEF)
        cg_push_def(cg, mod, n->as.activity_def.name);

    if ((int)n->type < 0 || (int)n->type >= AST_DESC_COUNT) return;
    const NodeDesc *d = &AST_DESC[n->type];

    for (unsigned i = 0; i < d->n; i++) {
        const FDesc *fd = &d->f[i];
        char *base = (char *)n + fd->off;

        if (fd->kind == F_NODE) {
            ASTNode *c = *(ASTNode **)base;
            if (c && c->type == NODE_ACTIVITY_DEF)
                cg_walk(c, mod, c->as.activity_def.name, cg);
            else
                cg_walk(c, mod, scope, cg);
        } else if (fd->kind == F_LIST) {
            NodeList *l = (NodeList *)base;
            for (int k = 0; k < l->count; k++) {
                ASTNode *c = l->items[k];
                if (c && c->type == NODE_ACTIVITY_DEF)
                    cg_walk(c, mod, c->as.activity_def.name, cg);
                else
                    cg_walk(c, mod, scope, cg);
            }
        }
    }
}

void callgraph_collect(ASTNode *program, const char *module_path, CallGraph *cg) {
    if (!program) return;
    cg_walk(program, module_path, "(top)", cg);
}

void callgraph_add_import(CallGraph *cg, const char *from, const char *to) {
    if (cg->import_count >= cg->import_capacity) {
        cg->import_capacity = cg->import_capacity ? cg->import_capacity * 2 : 32;
        cg->imports = realloc(cg->imports,
            (size_t)cg->import_capacity * sizeof(ImportEdge));
    }
    ImportEdge *e = &cg->imports[cg->import_count++];
    snprintf(e->from, sizeof e->from, "%s", from);
    snprintf(e->to,   sizeof e->to,   "%s", to);
}

void callgraph_free(CallGraph *cg) {
    if (!cg) return;
    free(cg->edges);
    free(cg->defs);
    free(cg->imports);
    cg->edges = NULL; cg->defs = NULL; cg->imports = NULL;
    cg->count = cg->capacity = 0;
    cg->def_count = cg->def_capacity = 0;
    cg->import_count = cg->import_capacity = 0;
}

static int cg_imports_reach(CallGraph *cg, const char *from, const char *to) {
    if (strcmp(from, to) == 0) return 1;
    const char *queue[512];
    int qn = 0, qi = 0;
    const char *seen[512];
    int sn = 0;
    queue[qn++] = from;
    seen[sn++]  = from;
    while (qi < qn) {
        const char *cur = queue[qi++];
        for (int i = 0; i < cg->import_count; i++) {
            if (strcmp(cg->imports[i].from, cur) != 0) continue;
            const char *nxt = cg->imports[i].to;
            if (strcmp(nxt, to) == 0) return 1;
            int already = 0;
            for (int s = 0; s < sn; s++)
                if (strcmp(seen[s], nxt) == 0) { already = 1; break; }
            if (!already && sn < 512 && qn < 512) {
                seen[sn++]  = nxt;
                queue[qn++] = nxt;
            }
        }
    }
    return 0;
}

static const char *cg_defining_module(const char *callee, CallGraph *cg) {
    for (int i = 0; i < cg->def_count; i++)
        if (strcmp(cg->defs[i].name, callee) == 0)
            return cg->defs[i].module;
    return NULL;
}

typedef enum { CALL_INTRA, CALL_IMPORTED, CALL_MISSING_IMPORT, CALL_NATIVE,
               CALL_PAGE } CallClass;

static CallClass cg_classify(CallGraph *cg, const CallEdge *e, const char **def_mod) {
    const char *dm = cg_defining_module(e->callee, cg);
    *def_mod = dm;
    if (!dm) return CALL_NATIVE;
    if (e->via == 'p') return CALL_PAGE;
    if (strcmp(dm, e->module) == 0) return CALL_INTRA;
    if (cg_imports_reach(cg, e->module, dm)) return CALL_IMPORTED;
    return CALL_MISSING_IMPORT;
}

static void epd_write_callgraph(FILE *f, EPDModule *modules, int n, CallGraph *cg) {
    if (!cg || cg->count == 0) return;

    int imported = 0, missing = 0, native = 0, page = 0;
    for (int i = 0; i < cg->count; i++) {
        const char *dm;
        switch (cg_classify(cg, &cg->edges[i], &dm)) {
            case CALL_PAGE:           page++;     break;
            case CALL_IMPORTED:       imported++; break;
            case CALL_MISSING_IMPORT: missing++;  break;
            case CALL_NATIVE:         native++;   break;
            default: break;
        }
    }

    fprintf(f, "\n═════════════════════════════════════════════\n");
    fprintf(f, "CALL GRAPH   (%d call sites · %d imported-cross-module · "
               "%d page actions · %d MISSING-IMPORT · %d native/unresolved)\n",
            cg->count, imported, page, missing, native);
    fprintf(f, "═════════════════════════════════════════════\n");
    fprintf(f,
        "Cross-module calls are validated against declared imports. A call to an\n"
        "activity defined in a module the caller never imports is flagged as a\n"
        "missing import — it will fail at runtime. Use the reverse view before\n"
        "changing an activity to see every caller.\n"
        "═════════════════════════════════════════════\n\n");

    fprintf(f, "module imports (declared):\n");
    if (cg->import_count == 0) {
        fprintf(f, "  (none)\n");
    } else {
        for (int i = 0; i < cg->import_count; i++)
            fprintf(f, "  %-28s →  %s\n", cg->imports[i].from, cg->imports[i].to);
    }

    fprintf(f, "\ncross-module callers (what breaks if you change an activity):\n");
    int printed_rev = 0;
    for (int m = 0; m < n; m++) {
        EPDIndex *idx = modules[m].index;
        for (int e = 0; e < idx->count; e++) {
            if (idx->entries[e].type != EPD_ACTIVITY) continue;
            const char *act = idx->entries[e].name;

            int header_done = 0;
            for (int i = 0; i < cg->count; i++) {
                if (strcmp(cg->edges[i].callee, act) != 0) continue;
                if (strcmp(cg->edges[i].module, modules[m].path) == 0) continue;
                const char *dm;
                CallClass c = cg_classify(cg, &cg->edges[i], &dm);
                if (c != CALL_IMPORTED && c != CALL_MISSING_IMPORT && c != CALL_PAGE) continue;
                if (!header_done) {
                    fprintf(f, "  %s:%s\n", modules[m].path, act);
                    header_done = 1; printed_rev = 1;
                }
                fprintf(f, "      ← %s: %s  (line %d)%s\n",
                        cg->edges[i].module, cg->edges[i].caller, cg->edges[i].line,
                        c == CALL_MISSING_IMPORT ? "   [MISSING IMPORT]" :
                        c == CALL_PAGE ? "   [page action, over HTTP]" :
                        cg->edges[i].via == 'o' ? "   [offered on a port]" : "");
            }
        }
    }
    if (!printed_rev) fprintf(f, "  (none)\n");

    if (missing > 0) {
        fprintf(f, "\nmissing imports (call will fail — activity defined but its "
                   "module is not imported):\n");
        for (int i = 0; i < cg->count; i++) {
            const char *dm;
            if (cg_classify(cg, &cg->edges[i], &dm) != CALL_MISSING_IMPORT) continue;
            fprintf(f, "  %s:%s  →  %s   (defined in %s; add: import \"%s\")  (line %d)\n",
                    cg->edges[i].module, cg->edges[i].caller, cg->edges[i].callee,
                    dm, dm, cg->edges[i].line);
        }
    }

    if (native > 0) {
        fprintf(f, "\nnative/unresolved calls (builtin, or undefined — check for typos):\n");
        for (int i = 0; i < cg->count; i++) {
            const char *dm;
            if (cg_classify(cg, &cg->edges[i], &dm) != CALL_NATIVE) continue;
            fprintf(f, "  %s:%s  →  %s  (line %d)\n",
                    cg->edges[i].module, cg->edges[i].caller,
                    cg->edges[i].callee, cg->edges[i].line);
        }
    }
}

void epd_write_workspace(const char *out_file, const char *root,
                         EPDModule *modules, int module_count,
                         CallGraph *cg) {
    FILE *f = fopen(out_file, "w");
    if (!f) {
        fprintf(stderr, "[EPL Error] Cannot write workspace index: %s\n", out_file);
        return;
    }

    int tot_act = 0, tot_data = 0, tot_sect = 0, tot_lines = 0, tot_entries = 0;
    for (int m = 0; m < module_count; m++) {
        tot_lines += modules[m].lines;
        EPDIndex *idx = modules[m].index;
        for (int i = 0; i < idx->count; i++) {
            switch (idx->entries[i].type) {
                case EPD_ACTIVITY: tot_act++;  break;
                case EPD_DATA:     tot_data++; break;
                case EPD_SECTION:
                case EPD_PAGE:     tot_sect++; break;
                default: break;
            }
            if (idx->entries[i].type != EPD_PROGRAM) tot_entries++;
        }
    }

    time_t now = time(NULL);
    char ts[32];
    strftime(ts, sizeof ts, "%Y-%m-%d %H:%M:%S", localtime(&now));

    fprintf(f, "EPD-WORKSPACE 1.0 — %s\n", root);
    fprintf(f, "generated: %s\n", ts);
    fprintf(f, "modules: %d   activities: %d   data: %d   views: %d   lines: %d\n",
            module_count, tot_act, tot_data, tot_sect, tot_lines);
    fprintf(f, "═════════════════════════════════════════════\n");
    fprintf(f,
        "Read this map to locate a module, then open that module's own .EPD,\n"
        "then only that module's source. Line ranges below point into source.\n"
        "═════════════════════════════════════════════\n\n");

    for (int m = 0; m < module_count; m++) {
        EPDIndex *idx = modules[m].index;
        const char *pdesc = epd_program_desc(idx);

        fprintf(f, "▸ %s   (%d lines)\n", modules[m].path, modules[m].lines);
        if (pdesc && *pdesc)
            fprintf(f, "    \"%s\"\n", pdesc);

        for (int i = 0; i < idx->count; i++) {
            EPDEntry *e = &idx->entries[i];
            if (e->type == EPD_PROGRAM) continue;

            char loc[32];
            if (e->line_start == e->line_end)
                snprintf(loc, sizeof loc, "%d", e->line_start);
            else
                snprintf(loc, sizeof loc, "%d-%d", e->line_start, e->line_end);

            fprintf(f, "    [%s] %-22s %-11s", epd_type_label(e->type), e->name, loc);
            if (e->type == EPD_ACTIVITY && e->params[0])
                fprintf(f, " taking %s", e->params);
            else if (e->type == EPD_DATA && e->data_type[0])
                fprintf(f, " %s", e->data_type);
            if (e->description[0])
                fprintf(f, "  \"%s\"", e->description);
            fprintf(f, "\n");
        }
        fprintf(f, "\n");
    }

    fprintf(f, "─────────────────────────────────────────────\n");
    fprintf(f, "%d modules · %d activities · %d data · %d entries · %d lines indexed\n",
            module_count, tot_act, tot_data, tot_entries, tot_lines);

    epd_write_callgraph(f, modules, module_count, cg);

    fclose(f);
}
