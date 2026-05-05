#ifndef CODEGEN_H
#define CODEGEN_H

/* ─────────────────────────────────────────────
   EPL Code Generator — codegen.h  (v1.7)
   Multi-page framework with built-in navigation
   ───────────────────────────────────────────── */

#include "ast.h"
#include "interpreter.h"

/* ── Theme CSS definitions ── */
typedef struct {
    const char *bg;
    const char *surface;
    const char *text;
    const char *subtext;
    const char *accent;
    const char *border;
    const char *chart_colors[6];
} ThemeCSS;

/* ─────────────────────────────────────────────
   Page Manifest — collected before any HTML is
   generated so every page knows its siblings.
   ───────────────────────────────────────────── */
#define EPL_MAX_PAGES 64

typedef struct PageEntry {
    char filename[256];  /* e.g. "factory_dashboard.html" */
    char label[256];     /* first heading text or filename stem */
    char icon[8];        /* unicode icon character (UTF-8)      */
} PageEntry;

typedef struct PageManifest {
    PageEntry pages[EPL_MAX_PAGES];
    int       count;
    int       active;   /* index of the page currently being written */
} PageManifest;

/* ── Code generation context ── */
typedef struct {
    FILE         *out;          /* output file handle          */
    Interpreter  *interp;       /* for eval-ing data nodes     */
    Env          *env;          /* current variable scope      */
    ThemeCSS      theme;        /* active theme                */
    int           chart_count;  /* unique chart IDs            */
    int           indent;       /* current HTML indent level   */
    PageManifest *manifest;     /* NULL = single-page mode     */
} CodegenCtx;

/* ── Main entry points ── */

/* Scan program AST and populate manifest with page filenames/labels.
   Call this BEFORE executing the program. */
void codegen_collect_pages(ASTNode *program, PageManifest *manifest,
                           Interpreter *interp, Env *env);

/* Generate a single HTML page. If manifest != NULL and manifest->count > 1,
   injects shared nav+sidebar automatically. */
void codegen_page(Interpreter *interp, Env *env, ASTNode *node,
                  PageManifest *manifest);

void codegen_node(CodegenCtx *ctx, ASTNode *node);

/* Theme loader */
ThemeCSS codegen_get_theme(EPFTheme theme, const char *custom_color);

#endif /* CODEGEN_H */
