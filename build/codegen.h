#ifndef CODEGEN_H
#define CODEGEN_H

/* ─────────────────────────────────────────────
   EPL Code Generator — codegen.h  (v1.4)
   Handles HTML output for .EPF files
   ───────────────────────────────────────────── */

#include "ast.h"
#include "interpreter.h"

/* Theme CSS definitions */
typedef struct {
    const char *bg;
    const char *surface;
    const char *text;
    const char *subtext;
    const char *accent;
    const char *border;
    const char *chart_colors[6];
} ThemeCSS;

/* Code generation context */
typedef struct {
    FILE        *out;          /* output file handle          */
    Interpreter *interp;       /* for eval-ing data nodes     */
    Env         *env;          /* current variable scope      */
    ThemeCSS     theme;        /* active theme                */
    int          chart_count;  /* unique chart IDs            */
    int          indent;       /* current HTML indent level   */
} CodegenCtx;

/* Main entry points */
void codegen_page(Interpreter *interp, Env *env, ASTNode *node);
void codegen_node(CodegenCtx *ctx, ASTNode *node);

/* Theme loader */
ThemeCSS codegen_get_theme(EPFTheme theme, const char *custom_color);

#endif /* CODEGEN_H */
