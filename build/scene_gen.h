#ifndef SCENE_GEN_H
#define SCENE_GEN_H

#include "ast.h"
#include "interpreter.h"

typedef struct {
    FILE        *js;
    FILE        *data;
    FILE        *css;
    FILE        *html;
    Interpreter *interp;
    Env         *env;
    int          offline;
    char         base[512];
    char         theme_bg[32];
    char         theme_surface[32];
    char         theme_accent[32];
    char         theme_text[32];
    char         theme_border[32];
} SceneCtx;

void scene_gen_page(Interpreter *interp, Env *env,
                    ASTNode *page_node);

void scene_gen_canvas3d(SceneCtx *ctx, ASTNode *node);
void scene_gen_panels(SceneCtx *ctx, ASTNode *page_node);
void scene_gen_data(SceneCtx *ctx, ASTNode *page_node);

#endif
