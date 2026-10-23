#ifndef DESCRIBE_H
#define DESCRIBE_H

#include "ast.h"

typedef enum {
    EPD_PROGRAM,
    EPD_ACTIVITY,
    EPD_BLOCK,
    EPD_DATA,
    EPD_PAGE,
    EPD_SECTION,
    EPD_UNKNOWN
} EPDEntryType;

typedef struct {
    EPDEntryType type;
    int          line_start;
    int          line_end;
    char         name[256];
    char         description[512];
    char         params[256];
    char         data_type[64];
} EPDEntry;

typedef struct {
    EPDEntry *entries;
    int       count;
    int       capacity;
} EPDIndex;

EPDIndex *epd_build(ASTNode *program, const char *source_file);
void      epd_write(EPDIndex *idx, const char *source_file,
                    const char *out_file);
void      epd_free(EPDIndex *idx);

typedef struct {
    char      path[512];
    int       lines;
    EPDIndex *index;
} EPDModule;

typedef struct {
    char module[512];
    char caller[128];
    char callee[128];
    int  line;
    char via;
} CallEdge;

typedef struct {
    char module[512];
    char name[128];
} ActivityDef;

typedef struct {
    char from[512];
    char to[512];
} ImportEdge;

typedef struct {
    CallEdge    *edges;
    int          count;
    int          capacity;
    ActivityDef *defs;
    int          def_count;
    int          def_capacity;
    ImportEdge  *imports;
    int          import_count;
    int          import_capacity;
} CallGraph;

void callgraph_collect(ASTNode *program, const char *module_path, CallGraph *cg);
void callgraph_add_import(CallGraph *cg, const char *from, const char *to);
void callgraph_free(CallGraph *cg);

void epd_write_workspace(const char *out_file, const char *root,
                         EPDModule *modules, int module_count,
                         CallGraph *cg);

#endif
