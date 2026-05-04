#ifndef SCENE_GEN_H
#define SCENE_GEN_H

/* ─────────────────────────────────────────────
   EPL Scene Generator — scene_gen.h  (v1.6)
   Generates Three.js scene from canvas3d AST
   ───────────────────────────────────────────── */

#include "ast.h"
#include "interpreter.h"

/* Scene context passed through generation */
typedef struct {
    FILE        *js;       /* scene .js output          */
    FILE        *data;     /* data .js output           */
    FILE        *css;      /* style .css output         */
    FILE        *html;     /* shell .html output        */
    Interpreter *interp;
    Env         *env;
    int          offline;  /* embed JS inline           */
    char         base[512];/* base filename (no ext)    */
    char         theme_bg[32];
    char         theme_surface[32];
    char         theme_accent[32];
    char         theme_text[32];
    char         theme_border[32];
} SceneCtx;

/* Main entry point */
void scene_gen_page(Interpreter *interp, Env *env,
                    ASTNode *page_node);

/* Internal generators */
void scene_gen_canvas3d(SceneCtx *ctx, ASTNode *node);
void scene_gen_panels(SceneCtx *ctx, ASTNode *page_node);
void scene_gen_data(SceneCtx *ctx, ASTNode *page_node);

#endif /* SCENE_GEN_H */
