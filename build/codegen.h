#ifndef CODEGEN_H
#define CODEGEN_H

#include "ast.h"
#include "interpreter.h"

typedef struct {
    const char *bg;
    const char *surface;
    const char *text;
    const char *subtext;
    const char *accent;
    const char *border;
    const char *chart_colors[6];
} ThemeCSS;

#define EPL_MAX_PAGES 64

typedef struct PageEntry {
    char filename[256];
    char label[256];
    char icon[8];
} PageEntry;

typedef struct PageManifest {
    PageEntry pages[EPL_MAX_PAGES];
    int       count;
    int       active;
} PageManifest;

typedef struct {
    FILE         *out;
    Interpreter  *interp;
    Env          *env;
    ThemeCSS      theme;
    int           chart_count;
    int           indent;
    PageManifest *manifest;
    int           actions;
} CodegenCtx;

void codegen_collect_pages(ASTNode *program, PageManifest *manifest,
                           Interpreter *interp, Env *env);

void codegen_page(Interpreter *interp, Env *env, ASTNode *node,
                  PageManifest *manifest);

void codegen_node(CodegenCtx *ctx, ASTNode *node);

ThemeCSS codegen_get_theme(EPFTheme theme, const char *custom_color);

#endif
