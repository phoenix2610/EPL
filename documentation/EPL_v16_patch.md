# EPL v1.6 Patch — 3D Frontend Engine

New files: `scene_gen.h`, `scene_gen.c`
Patches: `token.h`, `ast.h`, `parser.c`, `codegen.c`, `binding.c`, `main.c`

Recompile:
```powershell
gcc -Wall -std=c11 -o epl main.c lexer.c token.c ast.c parser.c interpreter.c repl.c codegen.c binding.c describe.c scene_gen.c -lm
```

---

## 1. token.h

**FIND** (end of v1.5 tokens):
```c
    TOKEN_IMPORT,          /* import */
```

**ADD AFTER:**
```c
    /* v1.6 — 3D canvas keywords */
    TOKEN_CANVAS3D,        /* canvas3d   */
    TOKEN_ORBITAL,         /* orbital    */
    TOKEN_RING_KW,         /* ring       */
    TOKEN_RADIUS_KW,       /* radius     */
    TOKEN_SPEED_KW,        /* speed      */
    TOKEN_NODE_3D,         /* node       */
    TOKEN_AT_KW,           /* at         */
    TOKEN_LABELED,         /* labeled    */
    TOKEN_STATUS_KW,       /* status     */
    TOKEN_CORE_KW,         /* core       */
    TOKEN_STAR_KW,         /* star       */
    TOKEN_BACKGROUND_KW,   /* background */
    TOKEN_PANEL_KW,        /* panel      */
    TOKEN_DRAWER_KW,       /* drawer     */
    TOKEN_OFFLINE,         /* offline    */
    TOKEN_FROM_GRAPH,      /* (from graph — multi-word, handled in parser) */
    TOKEN_VOID_BG,         /* void       */
    TOKEN_SPACE_BG,        /* space      */
```

---

## 2. ast.h

### 2a. Add canvas/orbital/node enums after EPFTheme

**FIND:**
```c
} EPFTheme;
```

**ADD AFTER:**
```c
/* ─────────────────────────────────────────────
   3D Background presets  (v1.6)
   ───────────────────────────────────────────── */
typedef enum {
    BG3D_DEEP_SPACE,    /* dark blue/purple star field  */
    BG3D_VOID,          /* pure black minimal           */
    BG3D_NEBULA,        /* colorful particle clouds     */
    BG3D_CUSTOM         /* custom hex color             */
} Background3D;

/* ─────────────────────────────────────────────
   Node status (mirrors interpreter status)
   ───────────────────────────────────────────── */
typedef enum {
    NODE3D_COMPLETED,
    NODE3D_INPROGRESS,
    NODE3D_OVERDUE,
    NODE3D_PENDING
} Node3DStatus;

/* ─────────────────────────────────────────────
   Panel / Drawer position
   ───────────────────────────────────────────── */
typedef enum {
    PANEL_TOP, PANEL_RIGHT, PANEL_BOTTOM, PANEL_LEFT
} PanelPosition;
```

### 2b. Add v1.6 node types to NodeType enum

**FIND:**
```c
    /* v1.3 — Connection / Graph */
    NODE_MAKE_GRAPH,
    NODE_LINK,
    NODE_ROUTE,
    NODE_CONNECTED,
```

**ADD AFTER THE v1.5 entries:**
```c
    /* v1.6 — 3D Canvas */
    NODE_CANVAS3D,         /* canvas3d "name" background "x" ... ! */
    NODE_ORBITAL_RING,     /* orbital ring radius N color "x" speed N ... ! */
    NODE_NODE3D,           /* node at N labeled "x" status "x" */
    NODE_CORE_STAR,        /* core star color "x" labeled "x" */
    NODE_FROM_GRAPH,       /* from graph <varname> */
    NODE_PANEL3D,          /* panel "position" ... ! */
    NODE_DRAWER3D,         /* drawer "position" titled "x" ... ! */
```

### 2c. Add structs to ASTNode union

**FIND:**
```c
        /* NODE_IMPORT  (v1.4) */
        struct {
            char source[MAX_TOKEN_LEN];
            char alias[MAX_TOKEN_LEN];
        } import_stmt;
```

**ADD AFTER:**
```c
        /* NODE_CANVAS3D  (v1.6) */
        struct {
            char         name[MAX_TOKEN_LEN];
            Background3D background;
            char         bg_color[MAX_TOKEN_LEN];
            int          offline;        /* 1 = embed JS inline */
            NodeList     children;       /* orbital rings, core, from_graph */
        } canvas3d;

        /* NODE_ORBITAL_RING  (v1.6) */
        struct {
            double       radius;
            char         color[MAX_TOKEN_LEN];
            double       speed;
            double       tilt_x;
            double       tilt_z;
            NodeList     nodes;          /* NODE_NODE3D children */
        } orbital_ring;

        /* NODE_NODE3D  (v1.6) */
        struct {
            double       angle;          /* position on ring 0..2pi */
            char         label[MAX_TOKEN_LEN];
            Node3DStatus status;
            char         desc[MAX_TOKEN_LEN];
        } node3d;

        /* NODE_CORE_STAR  (v1.6) */
        struct {
            char color[MAX_TOKEN_LEN];
            char label[MAX_TOKEN_LEN];
            char desc[MAX_TOKEN_LEN];
        } core_star;

        /* NODE_FROM_GRAPH  (v1.6) */
        struct {
            char graph_var[MAX_TOKEN_LEN];
        } from_graph;

        /* NODE_PANEL3D  (v1.6) */
        struct {
            PanelPosition position;
            NodeList      children;      /* heading, stat, etc. */
        } panel3d;

        /* NODE_DRAWER3D  (v1.6) */
        struct {
            PanelPosition position;
            char          title[MAX_TOKEN_LEN];
            NodeList      children;
        } drawer3d;
```

---

## 3. NEW FILE — scene_gen.h

```c
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
```

---

## 4. NEW FILE — scene_gen.c

```c
#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include "scene_gen.h"
#include "binding.h"
#include "codegen.h"   /* for ThemeCSS helpers */

/* ─────────────────────────────────────────────
   EPL Scene Generator — scene_gen.c  (v1.6)
   ───────────────────────────────────────────── */

/* ═══════════════════════════════════════
   STATUS COLORS
   ═══════════════════════════════════════ */
static const char *status_hex(Node3DStatus s) {
    switch(s) {
        case NODE3D_COMPLETED:  return "#8B7FE8";
        case NODE3D_INPROGRESS: return "#EF9F27";
        case NODE3D_OVERDUE:    return "#D85A30";
        case NODE3D_PENDING:    return "#1a1840";
        default:                return "#8B7FE8";
    }
}
static const char *status_emissive(Node3DStatus s) {
    switch(s) {
        case NODE3D_COMPLETED:  return "#4030cc";
        case NODE3D_INPROGRESS: return "#996010";
        case NODE3D_OVERDUE:    return "#991510";
        case NODE3D_PENDING:    return "#06041e";
        default:                return "#4030cc";
    }
}
static double status_intensity(Node3DStatus s) {
    switch(s) {
        case NODE3D_COMPLETED:  return 1.2;
        case NODE3D_INPROGRESS: return 2.0;
        case NODE3D_OVERDUE:    return 2.2;
        case NODE3D_PENDING:    return 0.08;
        default:                return 1.2;
    }
}
static const char *status_label(Node3DStatus s) {
    switch(s) {
        case NODE3D_COMPLETED:  return "Completed";
        case NODE3D_INPROGRESS: return "In Progress";
        case NODE3D_OVERDUE:    return "Overdue";
        case NODE3D_PENDING:    return "Pending";
        default:                return "Unknown";
    }
}


/* ═══════════════════════════════════════
   BACKGROUND PRESETS
   ═══════════════════════════════════════ */
static void write_background(FILE *js, Background3D bg,
                              const char *custom) {
    switch(bg) {
        case BG3D_DEEP_SPACE:
            fprintf(js,
                "  scene.fog = new THREE.FogExp2(0x010110, 0.00038);\n"
                "  scene.background = new THREE.Color(0x010110);\n");
            break;
        case BG3D_VOID:
            fprintf(js,
                "  scene.background = new THREE.Color(0x000000);\n");
            break;
        case BG3D_NEBULA:
            fprintf(js,
                "  scene.fog = new THREE.FogExp2(0x080020, 0.00030);\n"
                "  scene.background = new THREE.Color(0x080020);\n");
            break;
        case BG3D_CUSTOM:
            fprintf(js,
                "  scene.background = new THREE.Color('%s');\n", custom);
            break;
    }
}

static void write_stars(FILE *js, Background3D bg) {
    if (bg == BG3D_VOID) return;
    fprintf(js,
        "  (function(){\n"
        "    var n=2200,pos=new Float32Array(n*3),col=new Float32Array(n*3);\n"
        "    for(var i=0;i<n;i++){\n"
        "      var r=700+Math.random()*1400;\n"
        "      var th=Math.random()*Math.PI*2;\n"
        "      var ph=Math.acos(1-Math.random()*2);\n"
        "      pos[i*3]=r*Math.sin(ph)*Math.cos(th);\n"
        "      pos[i*3+1]=r*Math.cos(ph);\n"
        "      pos[i*3+2]=r*Math.sin(ph)*Math.sin(th);\n"
        "      var h=Math.random();\n"
        "      col[i*3]=0.5+h*0.5;col[i*3+1]=0.5+(1-h)*0.3;col[i*3+2]=0.9;\n"
        "    }\n"
        "    var g=new THREE.BufferGeometry();\n"
        "    g.setAttribute('position',new THREE.BufferAttribute(pos,3));\n"
        "    g.setAttribute('color',new THREE.BufferAttribute(col,3));\n"
        "    scene.add(new THREE.Points(g,new THREE.PointsMaterial("
        "{size:0.95,transparent:true,opacity:0.36,vertexColors:true})));\n"
        "  })();\n\n");

    /* Nebula clouds */
    if (bg == BG3D_NEBULA || bg == BG3D_DEEP_SPACE) {
        fprintf(js,
            "  (function(){\n"
            "    function neb(c,col,mn,mx,op,fl){\n"
            "      var pos=new Float32Array(c*3);\n"
            "      for(var i=0;i<c;i++){\n"
            "        var r=mn+Math.random()*(mx-mn);\n"
            "        var th=Math.random()*Math.PI*2;\n"
            "        var ph=Math.acos(1-Math.random()*2);\n"
            "        pos[i*3]=r*Math.sin(ph)*Math.cos(th);\n"
            "        pos[i*3+1]=r*Math.cos(ph)*(fl||1);\n"
            "        pos[i*3+2]=r*Math.sin(ph)*Math.sin(th);\n"
            "      }\n"
            "      var g=new THREE.BufferGeometry();\n"
            "      g.setAttribute('position',new THREE.BufferAttribute(pos,3));\n"
            "      return new THREE.Points(g,new THREE.PointsMaterial("
            "{color:col,size:2.8,transparent:true,opacity:op,"
            "depthWrite:false,blending:THREE.AdditiveBlending}));\n"
            "    }\n"
            "    scene.add(neb(600,0x2518a0,250,600,0.025,0.5));\n"
            "    scene.add(neb(450,0x0a2a60,300,700,0.020,0.6));\n"
            "    scene.add(neb(350,0x380828,150,400,0.032,0.4));\n"
            "  })();\n\n");
    }
}

static void write_lighting(FILE *js) {
    fprintf(js,
        "  scene.add(new THREE.AmbientLight(0x0d0824, 6));\n"
        "  var cLight = new THREE.PointLight(0xC8A840, 12, 500);\n"
        "  scene.add(cLight);\n"
        "  var fA = new THREE.PointLight(0x1a3a8a, 4, 800);\n"
        "  fA.position.set(320, 200, 100); scene.add(fA);\n"
        "  var fB = new THREE.PointLight(0x3010a0, 3, 700);\n"
        "  fB.position.set(-200, -180, 220); scene.add(fB);\n\n");
}


/* ═══════════════════════════════════════
   CORE STAR
   ═══════════════════════════════════════ */
static void write_core_star(FILE *js, ASTNode *node) {
    const char *color  = node->as.core_star.color;
    const char *label  = node->as.core_star.label;

    /* Convert named color to hex — simple lookup */
    const char *hex = "#E8C050";
    const char *emissive = "#B08000";
    if (strstr(color, "gold") || strstr(color, "#E8C")) {
        hex = "#E8C050"; emissive = "#B08000";
    } else if (strstr(color, "blue")) {
        hex = "#4A9AEF"; emissive = "#1a4a8a";
    } else if (strstr(color, "indigo") || strstr(color, "purple")) {
        hex = "#8B7FE8"; emissive = "#4030cc";
    } else if (strstr(color, "green") || strstr(color, "emerald")) {
        hex = "#22B890"; emissive = "#0a6040";
    } else if (color[0] == '#') {
        hex = color; emissive = color;
    }

    fprintf(js,
        "  /* ── Core Star ── */\n"
        "  var coreGrp = new THREE.Group(); scene.add(coreGrp);\n"
        "  var coreMesh = new THREE.Mesh(\n"
        "    new THREE.SphereGeometry(16,52,52),\n"
        "    new THREE.MeshStandardMaterial({"
        "color:'%s',emissive:'%s',emissiveIntensity:2.8,"
        "roughness:0.06,metalness:0.98}));\n"
        "  coreMesh.userData = {isCore:true,label:'%s'};\n"
        "  hoverMeshes.push(coreMesh);\n"
        "  coreGrp.add(coreMesh);\n",
        hex, emissive, label);

    /* Glow shells */
    fprintf(js,
        "  [[24,'%s',0.22],[36,'%s',0.11],[52,'%s',0.055],"
        "[72,'%s',0.024],[100,'%s',0.009]].forEach(function(d){\n"
        "    coreGrp.add(new THREE.Mesh(\n"
        "      new THREE.SphereGeometry(d[0],18,18),\n"
        "      new THREE.MeshBasicMaterial({color:d[1],transparent:true,"
        "opacity:d[2],depthWrite:false,blending:THREE.AdditiveBlending})));\n"
        "  });\n",
        hex, hex, hex, hex, hex);

    /* Armillary rings */
    fprintf(js,
        "  var armRings = [];\n"
        "  [{r:28,rx:0,rz:0,rs:0.004,c:'#FFD060'},"
        "{r:28,rx:Math.PI/2,rz:0,rs:-0.003,c:'#FF9820'},"
        "{r:28,rx:Math.PI/4,rz:Math.PI/4,rs:0.0022,c:'#E8C840'}]"
        ".forEach(function(d){\n"
        "    var m=new THREE.Mesh(new THREE.TorusGeometry(d.r,0.7,8,84),\n"
        "      new THREE.MeshBasicMaterial({color:d.c,transparent:true,"
        "opacity:0.55,blending:THREE.AdditiveBlending}));\n"
        "    m.rotation.x=d.rx; m.rotation.z=d.rz;\n"
        "    m.userData={rspd:d.rs}; coreGrp.add(m); armRings.push(m);\n"
        "  });\n\n");
}


/* ═══════════════════════════════════════
   ORBITAL RING + NODES
   ═══════════════════════════════════════ */
static void write_orbital_ring(FILE *js, ASTNode *ring,
                                int ring_idx) {
    double radius = ring->as.orbital_ring.radius;
    double speed  = ring->as.orbital_ring.speed;
    double tilt_x = ring->as.orbital_ring.tilt_x;
    double tilt_z = ring->as.orbital_ring.tilt_z;
    const char *color = ring->as.orbital_ring.color;

    /* Map color name to hex */
    const char *hex = "#8B7FE8";
    if      (strcmp(color,"indigo")==0||strcmp(color,"purple")==0) hex="#8B7FE8";
    else if (strcmp(color,"blue")==0)   hex="#4A9AEF";
    else if (strcmp(color,"green")==0||strcmp(color,"emerald")==0) hex="#22B890";
    else if (strcmp(color,"amber")==0||strcmp(color,"orange")==0)  hex="#F0A030";
    else if (strcmp(color,"pink")==0||strcmp(color,"rose")==0)     hex="#E05090";
    else if (strcmp(color,"red")==0)    hex="#D85A30";
    else if (color[0]=='#')             hex=color;

    /* Count completed nodes for arc */
    int total = ring->as.orbital_ring.nodes.count;
    int done  = 0;
    for (int i = 0; i < total; i++) {
        ASTNode *n = ring->as.orbital_ring.nodes.items[i];
        if (n && n->type == NODE_NODE3D &&
            n->as.node3d.status == NODE3D_COMPLETED) done++;
    }
    double pct = total > 0 ? (double)done / total : 0.0;

    fprintf(js,
        "  /* ── Ring %d (r=%.0f) ── */\n"
        "  (function(){\n"
        "    var grp = new THREE.Group();\n"
        "    grp.rotation.x = %.4f;\n"
        "    grp.rotation.z = %.4f;\n"
        "    grp.add(new THREE.Mesh(\n"
        "      new THREE.TorusGeometry(%.1f, 0.55, 8, 140),\n"
        "      new THREE.MeshBasicMaterial({color:'%s',"
        "transparent:true,opacity:0.15,depthWrite:false})));\n"
        "    grp.add(new THREE.Mesh(\n"
        "      new THREE.TorusGeometry(%.1f, 4.2, 8, 140),\n"
        "      new THREE.MeshBasicMaterial({color:'%s',"
        "transparent:true,opacity:0.026,depthWrite:false,"
        "blending:THREE.AdditiveBlending})));\n",
        ring_idx, radius, tilt_x, tilt_z,
        radius, hex, radius, hex);

    /* Completion arc */
    if (pct > 0.01) {
        fprintf(js,
            "    /* completion arc */\n"
            "    (function(){\n"
            "      var ae = %.4f * Math.PI * 2;\n"
            "      var segs = Math.max(16, Math.floor(ae/(Math.PI*2)*160));\n"
            "      var pts = [];\n"
            "      for(var i=0;i<=segs;i++){\n"
            "        var a = ae*(i/segs);\n"
            "        pts.push(new THREE.Vector3("
            "Math.cos(a)*%.1f, 0, Math.sin(a)*%.1f));\n"
            "      }\n"
            "      var curve = new THREE.CatmullRomCurve3(pts,false);\n"
            "      [[1.8,0.82],[6.5,0.10],[13,0.032]].forEach(function(d){\n"
            "        grp.add(new THREE.Mesh(\n"
            "          new THREE.TubeGeometry(curve,segs,d[0],7,false),\n"
            "          new THREE.MeshBasicMaterial({color:'%s',"
            "transparent:true,opacity:d[1],depthWrite:false,"
            "blending:THREE.AdditiveBlending})));\n"
            "      });\n"
            "    })();\n",
            pct, radius, radius, hex);
    }

    /* Nodes */
    fprintf(js,
        "    var nodeObjs_%d = [];\n", ring_idx);

    for (int i = 0; i < ring->as.orbital_ring.nodes.count; i++) {
        ASTNode *nd = ring->as.orbital_ring.nodes.items[i];
        if (!nd || nd->type != NODE_NODE3D) continue;

        double  angle   = nd->as.node3d.angle;
        const char *nhex = status_hex(nd->as.node3d.status);
        const char *emhex= status_emissive(nd->as.node3d.status);
        double  emint   = status_intensity(nd->as.node3d.status);
        const char *lbl = nd->as.node3d.label;
        const char *slbl= status_label(nd->as.node3d.status);

        fprintf(js,
            "    (function(){\n"
            "      var ng = new THREE.Group();\n"
            "      ng.position.set(Math.cos(%.4f)*%.1f,0,Math.sin(%.4f)*%.1f);\n"
            "      var node = new THREE.Mesh(\n"
            "        new THREE.SphereGeometry(5.8,24,24),\n"
            "        new THREE.MeshStandardMaterial({color:'%s',"
            "emissive:'%s',emissiveIntensity:%.2f,"
            "roughness:0.22,metalness:0.82}));\n"
            "      node.userData = {label:'%s',status:'%s',"
            "ring:'Ring %d',isCore:false};\n"
            "      hoverMeshes.push(node);\n"
            "      ng.add(node);\n",
            angle, radius, angle, radius,
            nhex, emhex, emint,
            lbl, slbl, ring_idx);

        /* Glow shells */
        fprintf(js,
            "      [[10,%.2f],[17,%.2f],[26,%.2f]].forEach(function(d){\n"
            "        ng.add(new THREE.Mesh(\n"
            "          new THREE.SphereGeometry(d[0],12,12),\n"
            "          new THREE.MeshBasicMaterial({color:'%s',"
            "transparent:true,opacity:d[1],depthWrite:false,"
            "blending:THREE.AdditiveBlending})));\n"
            "      });\n",
            nd->as.node3d.status == NODE3D_PENDING ? 0.02 : 0.18,
            nd->as.node3d.status == NODE3D_PENDING ? 0.01 : 0.08,
            nd->as.node3d.status == NODE3D_PENDING ? 0.005: 0.03,
            nhex);

        /* Pulse ring for completed */
        if (nd->as.node3d.status == NODE3D_COMPLETED) {
            fprintf(js,
                "      var pr = new THREE.Mesh(\n"
                "        new THREE.TorusGeometry(8.5,0.55,8,44),\n"
                "        new THREE.MeshBasicMaterial({color:'%s',"
                "transparent:true,opacity:0.55,depthWrite:false,"
                "blending:THREE.AdditiveBlending}));\n"
                "      pr.rotation.x = Math.PI/2;\n"
                "      ng.add(pr); node.userData.pulseRing = pr;\n",
                hex);
        }

        /* Warn ring for overdue */
        if (nd->as.node3d.status == NODE3D_OVERDUE) {
            fprintf(js,
                "      var wr = new THREE.Mesh(\n"
                "        new THREE.TorusGeometry(9.5,0.45,8,44),\n"
                "        new THREE.MeshBasicMaterial({color:'#D85A30',"
                "transparent:true,opacity:0.4,depthWrite:false,"
                "blending:THREE.AdditiveBlending}));\n"
                "      wr.rotation.x = Math.PI/3;\n"
                "      ng.add(wr); node.userData.warnRing = wr;\n");
        }

        /* Ghost trail */
        fprintf(js,
            "      var ghosts_%d_%d = [];\n"
            "      for(var k=1;k<=7;k++){\n"
            "        var gs=new THREE.Mesh(\n"
            "          new THREE.SphereGeometry(5.8*(1-k*0.1),7,7),\n"
            "          new THREE.MeshBasicMaterial({color:'%s',"
            "transparent:true,opacity:%s*Math.pow(1-k/8,1.8),"
            "depthWrite:false,blending:THREE.AdditiveBlending}));\n"
            "        grp.add(gs);\n"
            "        ghosts_%d_%d.push({mesh:gs,delta:k*0.036});\n"
            "      }\n",
            ring_idx, i,
            nhex,
            nd->as.node3d.status == NODE3D_PENDING ? "0" : "0.22",
            ring_idx, i);

        fprintf(js,
            "      grp.add(ng);\n"
            "      nodeObjs_%d.push({ng:ng,node:node,"
            "angle:%.4f,radius:%.1f,speed:%.6f,"
            "ghosts:ghosts_%d_%d});\n"
            "    })();\n",
            ring_idx, angle, radius, speed,
            ring_idx, i);
    }

    /* Push ring group to scene and animation list */
    fprintf(js,
        "    scene.add(grp);\n"
        "    allRings.push({grp:grp,nodes:nodeObjs_%d,"
        "speed:%.6f,radius:%.1f});\n"
        "  })();\n\n",
        ring_idx, speed, radius);
}


/* ═══════════════════════════════════════
   TOOLTIP + PANEL HTML (injected into page)
   ═══════════════════════════════════════ */
static void write_overlay_html(FILE *html, ASTNode *canvas,
                                ASTNode *page) {
    fprintf(html,
        "  <div id='epl-tt'></div>\n"
        "  <div id='epl-panel'>\n"
        "    <div class='ep-head'>\n"
        "      <div class='ep-title' id='ep-title'>—</div>\n"
        "      <div class='ep-close' id='ep-close'>×</div>\n"
        "    </div>\n"
        "    <div class='ep-body'>\n"
        "      <div id='ep-badge' class='ep-badge'></div>\n"
        "      <div class='ep-div'></div>\n"
        "      <div class='ep-row'>"
        "<span class='ep-key'>Ring</span>"
        "<span class='ep-val' id='ep-ring'>—</span></div>\n"
        "      <div class='ep-row'>"
        "<span class='ep-key'>Status</span>"
        "<span class='ep-val' id='ep-status'>—</span></div>\n"
        "      <div class='ep-div'></div>\n"
        "      <div class='ep-desc' id='ep-desc'>—</div>\n"
        "    </div>\n"
        "  </div>\n");

    /* Panels from page children */
    if (!page) return;
    NodeList *children = NULL;
    if (page->type == NODE_MAKE_PAGE)
        children = &page->as.make_page.children;
    if (!children) return;

    for (int i = 0; i < children->count; i++) {
        ASTNode *child = children->items[i];
        if (!child) continue;
        if (child->type == NODE_PANEL3D) {
            const char *pos_class = "ep-panel-top";
            if (child->as.panel3d.position == PANEL_RIGHT)  pos_class = "ep-panel-right";
            if (child->as.panel3d.position == PANEL_BOTTOM) pos_class = "ep-panel-bottom";
            if (child->as.panel3d.position == PANEL_LEFT)   pos_class = "ep-panel-left";
            fprintf(html,
                "  <div class='epl-overlay-panel %s' id='panel-%d'>\n",
                pos_class, i);
            /* panel children are heading/stat/etc — 
               write as plain HTML placeholders for JS to fill */
            fprintf(html, "  </div>\n");
        }
        if (child->type == NODE_DRAWER3D) {
            const char *side = child->as.drawer3d.position == PANEL_LEFT
                               ? "left" : "right";
            fprintf(html,
                "  <div class='epl-drawer epl-drawer-%s' id='drawer-%d'>\n"
                "    <div class='ep-drawer-head'>\n"
                "      <span>%s</span>\n"
                "      <div class='ep-drawer-close' "
                "onclick='this.parentElement.parentElement"
                ".classList.remove(\"open\")'>×</div>\n"
                "    </div>\n"
                "    <div class='ep-drawer-body'></div>\n"
                "  </div>\n",
                side, i,
                child->as.drawer3d.title);
        }
    }
}


/* ═══════════════════════════════════════
   CSS GENERATION
   ═══════════════════════════════════════ */
static void write_css(FILE *css, SceneCtx *ctx) {
    fprintf(css,
        "/* EPL v1.6 Generated Styles */\n"
        "*,*::before,*::after{margin:0;padding:0;box-sizing:border-box;}\n"
        "html,body{width:100%%;height:100%%;overflow:hidden;"
        "font-family:'Rajdhani',sans-serif;color:%s;}\n"
        "body{background:%s;}\n"
        "#epl-root{position:relative;width:100%%;height:100vh;}\n"
        "#epl-canvas{display:block;width:100%%;height:100%%;"
        "cursor:grab;}\n"
        "#epl-canvas:active{cursor:grabbing;}\n\n",
        ctx->theme_text, ctx->theme_bg);

    /* Tooltip */
    fprintf(css,
        "#epl-tt{\n"
        "  position:absolute;z-index:20;\n"
        "  background:rgba(1,1,14,0.94);\n"
        "  border:0.5px solid rgba(110,95,240,0.28);\n"
        "  border-radius:7px;padding:7px 13px;\n"
        "  pointer-events:none;opacity:0;\n"
        "  transition:opacity .12s;\n"
        "  font-family:'Share Tech Mono',monospace;\n"
        "  font-size:9px;color:#8B8BAA;\n"
        "  letter-spacing:0.06em;white-space:nowrap;\n"
        "  box-shadow:0 2px 20px rgba(80,60,200,0.12);\n"
        "}\n\n");

    /* Detail panel */
    fprintf(css,
        "#epl-panel{\n"
        "  position:absolute;top:70px;right:-300px;z-index:15;\n"
        "  width:268px;background:rgba(2,1,20,0.97);\n"
        "  border:0.5px solid rgba(110,95,240,0.25);\n"
        "  border-radius:14px;overflow:hidden;\n"
        "  transition:right .36s cubic-bezier(.22,.68,0,1.18);\n"
        "  box-shadow:-6px 0 50px rgba(80,60,200,0.10);\n"
        "}\n"
        "#epl-panel.open{right:16px;}\n"
        ".ep-head{padding:15px 17px 13px;\n"
        "  border-bottom:0.5px solid rgba(110,95,240,0.08);\n"
        "  display:flex;justify-content:space-between;\n"
        "  align-items:flex-start;gap:8px;}\n"
        ".ep-title{font-size:14px;font-weight:500;\n"
        "  color:#E8E4FF;letter-spacing:0.04em;flex:1;}\n"
        ".ep-close{width:22px;height:22px;border-radius:5px;\n"
        "  cursor:pointer;background:rgba(110,95,240,0.07);\n"
        "  border:0.5px solid rgba(110,95,240,0.18);\n"
        "  display:grid;place-items:center;font-size:13px;\n"
        "  color:#5048A0;transition:all .15s;}\n"
        ".ep-close:hover{background:rgba(110,95,240,0.18);color:#E8E4FF;}\n"
        ".ep-body{padding:13px 17px 17px;\n"
        "  display:flex;flex-direction:column;gap:10px;}\n"
        ".ep-badge{display:inline-flex;align-items:center;\n"
        "  font-family:'Share Tech Mono',monospace;font-size:7.5px;\n"
        "  letter-spacing:0.1em;padding:3px 9px;border-radius:20px;\n"
        "  width:fit-content;border:0.5px solid transparent;}\n"
        ".ep-row{display:flex;justify-content:space-between;\n"
        "  align-items:flex-start;gap:8px;}\n"
        ".ep-key{font-family:'Share Tech Mono',monospace;font-size:7.5px;\n"
        "  color:#2e2c50;letter-spacing:0.12em;text-transform:uppercase;\n"
        "  padding-top:1px;flex-shrink:0;}\n"
        ".ep-val{font-size:10.5px;color:#7874A0;text-align:right;}\n"
        ".ep-div{height:0.5px;background:rgba(110,95,240,0.07);}\n"
        ".ep-desc{font-size:10px;color:#4A4870;\n"
        "  line-height:1.65;font-style:italic;}\n\n");

    /* Overlay panels */
    fprintf(css,
        ".epl-overlay-panel{\n"
        "  position:absolute;z-index:10;\n"
        "  background:rgba(2,1,20,0.88);\n"
        "  border:0.5px solid rgba(110,95,240,0.2);\n"
        "  border-radius:12px;padding:14px 18px;\n"
        "  backdrop-filter:blur(8px);\n"
        "  pointer-events:none;\n"
        "}\n"
        ".ep-panel-top{top:16px;left:50%%;transform:translateX(-50%%);}\n"
        ".ep-panel-right{top:70px;right:16px;width:240px;}\n"
        ".ep-panel-left{top:70px;left:16px;width:240px;}\n"
        ".ep-panel-bottom{bottom:16px;left:50%%;\n"
        "  transform:translateX(-50%%);}\n\n");

    /* Drawer */
    fprintf(css,
        ".epl-drawer{\n"
        "  position:absolute;top:0;bottom:0;z-index:25;\n"
        "  width:320px;background:rgba(2,1,20,0.99);\n"
        "  display:flex;flex-direction:column;\n"
        "  transition:transform .38s cubic-bezier(.22,.68,0,1.18);\n"
        "  box-shadow:0 0 60px rgba(60,40,180,0.14);\n"
        "}\n"
        ".epl-drawer-left{left:0;transform:translateX(-100%%);}\n"
        ".epl-drawer-right{right:0;transform:translateX(100%%);}\n"
        ".epl-drawer.open{transform:translateX(0);}\n"
        ".ep-drawer-head{\n"
        "  padding:18px 20px 14px;\n"
        "  border-bottom:0.5px solid rgba(110,95,240,0.1);\n"
        "  display:flex;justify-content:space-between;\n"
        "  align-items:center;font-size:14px;font-weight:500;\n"
        "  color:#E8E4FF;letter-spacing:0.1em;\n"
        "}\n"
        ".ep-drawer-close{\n"
        "  width:26px;height:26px;border-radius:6px;\n"
        "  cursor:pointer;background:rgba(110,95,240,0.08);\n"
        "  border:0.5px solid rgba(110,95,240,0.2);\n"
        "  display:grid;place-items:center;font-size:14px;\n"
        "  color:#5048A0;transition:all .15s;\n"
        "}\n"
        ".ep-drawer-close:hover{"
        "background:rgba(110,95,240,0.2);color:#E8E4FF;}\n"
        ".ep-drawer-body{flex:1;overflow-y:auto;padding:8px 12px;}\n\n");

    /* Stat cards inside panels */
    fprintf(css,
        ".ep-stat{\n"
        "  background:rgba(110,95,240,0.06);\n"
        "  border:0.5px solid rgba(110,95,240,0.15);\n"
        "  border-radius:10px;padding:10px 14px;\n"
        "  margin-bottom:8px;\n"
        "}\n"
        ".ep-stat-lbl{\n"
        "  font-family:'Share Tech Mono',monospace;\n"
        "  font-size:7px;color:#38345a;\n"
        "  letter-spacing:0.1em;text-transform:uppercase;\n"
        "  margin-bottom:3px;\n"
        "}\n"
        ".ep-stat-val{\n"
        "  font-size:22px;font-weight:600;color:%s;\n"
        "}\n\n",
        ctx->theme_accent);

    /* Bottom legend */
    fprintf(css,
        "#epl-legend{\n"
        "  position:absolute;bottom:0;left:0;right:0;z-index:10;\n"
        "  padding:9px 22px;display:flex;gap:14px;\n"
        "  align-items:center;flex-wrap:wrap;\n"
        "  background:linear-gradient(0deg,rgba(1,1,16,0.88) 50%%,"
        "transparent 100%%);\n"
        "  pointer-events:none;\n"
        "}\n"
        ".leg-item{display:flex;align-items:center;gap:5px;}\n"
        ".leg-dot{width:6px;height:6px;border-radius:50%;}\n"
        ".leg-txt{font-family:'Share Tech Mono',monospace;"
        "font-size:7.5px;color:#38345a;letter-spacing:0.08em;}\n");
}


/* ═══════════════════════════════════════
   ANIMATION LOOP
   ═══════════════════════════════════════ */
static void write_animation(FILE *js) {
    fprintf(js,
        "\n  /* ── Animation ── */\n"
        "  var frame = 0;\n"
        "  var lastInt = Date.now();\n"
        "  var autoRot = true;\n\n"
        "  function animate() {\n"
        "    requestAnimationFrame(animate);\n"
        "    frame++;\n"
        "    var t = frame * 0.016;\n\n"
        "    /* auto-rotate */\n"
        "    if (!isDragging && Date.now()-lastInt > 4500) autoRot=true;\n"
        "    if (autoRot) tgtSph.th += 0.00055;\n\n"
        "    /* smooth camera */\n"
        "    sph.th += (tgtSph.th - sph.th) * 0.065;\n"
        "    sph.ph += (tgtSph.ph - sph.ph) * 0.065;\n"
        "    sph.r  += (tgtSph.r  - sph.r)  * 0.065;\n"
        "    updateCam();\n\n"
        "    /* core pulse */\n"
        "    if (coreMesh) {\n"
        "      var p = 1 + Math.sin(t*1.2)*0.07;\n"
        "      coreMesh.scale.setScalar(p);\n"
        "      cLight.intensity = 10 + Math.sin(t*1.2)*3;\n"
        "      coreGrp.rotation.y += 0.0012;\n"
        "      armRings.forEach(function(ar){\n"
        "        ar.rotation.y += ar.userData.rspd;\n"
        "      });\n"
        "    }\n\n"
        "    /* orbital nodes */\n"
        "    allRings.forEach(function(ring){\n"
        "      ring.nodes.forEach(function(obj){\n"
        "        obj.angle += ring.speed;\n"
        "        obj.ng.position.set(\n"
        "          Math.cos(obj.angle)*ring.radius, 0,\n"
        "          Math.sin(obj.angle)*ring.radius);\n"
        "        obj.ghosts.forEach(function(g){\n"
        "          var ga = obj.angle - g.delta;\n"
        "          g.mesh.position.set(\n"
        "            Math.cos(ga)*ring.radius, 0,\n"
        "            Math.sin(ga)*ring.radius);\n"
        "        });\n"
        "        if (obj.node.userData.pulseRing) {\n"
        "          var pr = obj.node.userData.pulseRing;\n"
        "          var ps = 1 + 0.18*Math.sin(t*2.2+obj.angle*2);\n"
        "          pr.scale.setScalar(ps);\n"
        "          pr.material.opacity=0.3+0.35*Math.sin(t*2.2+obj.angle*2);\n"
        "          pr.rotation.y += 0.025;\n"
        "        }\n"
        "        if (obj.node.userData.warnRing) {\n"
        "          var wr = obj.node.userData.warnRing;\n"
        "          wr.rotation.y += 0.04;\n"
        "          wr.material.opacity=0.25+0.25*Math.abs(Math.sin(t*3));\n"
        "        }\n"
        "        /* hover scale */\n"
        "        var isHov = (hoveredLabel===obj.node.userData.label);\n"
        "        var cur = obj.node.scale.x;\n"
        "        var ts  = isHov ? 1.58 : 1.0;\n"
        "        obj.node.scale.setScalar(cur+(ts-cur)*0.12);\n"
        "      });\n"
        "    });\n\n"
        "    renderer.render(scene, camera);\n"
        "  }\n"
        "  animate();\n");
}


/* ═══════════════════════════════════════
   CAMERA + INTERACTION
   ═══════════════════════════════════════ */
static void write_camera(FILE *js) {
    fprintf(js,
        "  /* ── Camera ── */\n"
        "  var sph    = {th:0.65, ph:1.1, r:500};\n"
        "  var tgtSph = {th:0.65, ph:1.1, r:500};\n"
        "  var isDragging = false, didDrag = false;\n"
        "  var px = 0, py = 0;\n"
        "  var hoveredLabel = null;\n\n"
        "  function updateCam() {\n"
        "    camera.position.set(\n"
        "      sph.r*Math.sin(sph.ph)*Math.sin(sph.th),\n"
        "      sph.r*Math.cos(sph.ph),\n"
        "      sph.r*Math.sin(sph.ph)*Math.cos(sph.th));\n"
        "    camera.lookAt(0,0,0);\n"
        "  }\n"
        "  updateCam();\n\n"

        "  var canvas = document.getElementById('epl-canvas');\n"
        "  canvas.addEventListener('mousedown', function(e){\n"
        "    isDragging=true; didDrag=false;\n"
        "    px=e.clientX; py=e.clientY;\n"
        "    autoRot=false; lastInt=Date.now();\n"
        "  });\n"
        "  window.addEventListener('mouseup', function(){\n"
        "    isDragging=false;\n"
        "  });\n"
        "  window.addEventListener('mousemove', function(e){\n"
        "    if (!isDragging) {\n"
        "      rayCast(e.clientX, e.clientY);\n"
        "      return;\n"
        "    }\n"
        "    var dx=e.clientX-px, dy=e.clientY-py;\n"
        "    if (Math.abs(dx)>2||Math.abs(dy)>2) didDrag=true;\n"
        "    tgtSph.th -= dx*0.0042;\n"
        "    tgtSph.ph  = Math.max(0.08,Math.min(2.96,tgtSph.ph-dy*0.0042));\n"
        "    px=e.clientX; py=e.clientY;\n"
        "    lastInt=Date.now();\n"
        "    document.getElementById('epl-tt').style.opacity='0';\n"
        "  });\n"
        "  canvas.addEventListener('wheel', function(e){\n"
        "    tgtSph.r=Math.max(150,Math.min(1200,tgtSph.r+e.deltaY*0.3));\n"
        "    autoRot=false; lastInt=Date.now();\n"
        "    e.preventDefault();\n"
        "  },{passive:false});\n\n"

        "  /* ── Raycaster ── */\n"
        "  var raycaster = new THREE.Raycaster();\n"
        "  var mouse2 = new THREE.Vector2();\n"
        "  function rayCast(cx, cy) {\n"
        "    var rect = canvas.getBoundingClientRect();\n"
        "    mouse2.x = ((cx-rect.left)/rect.width)*2-1;\n"
        "    mouse2.y = -((cy-rect.top)/rect.height)*2+1;\n"
        "    raycaster.setFromCamera(mouse2, camera);\n"
        "    var hits = raycaster.intersectObjects(hoverMeshes);\n"
        "    var tt = document.getElementById('epl-tt');\n"
        "    if (hits.length) {\n"
        "      var d = hits[0].object.userData;\n"
        "      hoveredLabel = d.label;\n"
        "      tt.textContent = d.isCore ? d.label+' · Core'\n"
        "        : d.label+' · '+d.ring+' · '+d.status;\n"
        "      tt.style.left = Math.min(cx+16,window.innerWidth-220)+'px';\n"
        "      tt.style.top  = Math.max(cy-36,62)+'px';\n"
        "      tt.style.opacity = '1';\n"
        "      canvas.style.cursor = 'pointer';\n"
        "    } else {\n"
        "      hoveredLabel = null;\n"
        "      tt.style.opacity = '0';\n"
        "      canvas.style.cursor = isDragging?'grabbing':'grab';\n"
        "    }\n"
        "  }\n"
        "  canvas.addEventListener('click', function(e){\n"
        "    if (didDrag) return;\n"
        "    var rect = canvas.getBoundingClientRect();\n"
        "    mouse2.x = ((e.clientX-rect.left)/rect.width)*2-1;\n"
        "    mouse2.y = -((e.clientY-rect.top)/rect.height)*2+1;\n"
        "    raycaster.setFromCamera(mouse2, camera);\n"
        "    var hits = raycaster.intersectObjects(hoverMeshes);\n"
        "    if (hits.length) openPanel(hits[0].object.userData);\n"
        "  });\n\n"

        "  /* ── Panel interaction ── */\n"
        "  function openPanel(d) {\n"
        "    document.getElementById('ep-title').textContent = d.label||'—';\n"
        "    document.getElementById('ep-ring').textContent  = d.ring||'—';\n"
        "    document.getElementById('ep-status').textContent= d.status||'—';\n"
        "    document.getElementById('ep-desc').textContent  = d.desc||'—';\n"
        "    var badge = document.getElementById('ep-badge');\n"
        "    badge.textContent = d.status||'';\n"
        "    document.getElementById('epl-panel').classList.add('open');\n"
        "  }\n"
        "  document.getElementById('ep-close').addEventListener('click',\n"
        "    function(){document.getElementById('epl-panel')"
        ".classList.remove('open');});\n\n");
}


/* ═══════════════════════════════════════
   MAIN SCENE JS ASSEMBLY
   ═══════════════════════════════════════ */
static void write_scene_js(SceneCtx *ctx, ASTNode *canvas) {
    FILE *js = ctx->js;

    fprintf(js,
        "/* EPL v1.6 Generated Scene — %s */\n"
        "(function(){\n"
        "'use strict';\n\n"
        "  var W = window.innerWidth;\n"
        "  var H = window.innerHeight;\n"
        "  var renderer = new THREE.WebGLRenderer({\n"
        "    canvas: document.getElementById('epl-canvas'),\n"
        "    antialias: true, alpha: true\n"
        "  });\n"
        "  renderer.setPixelRatio(Math.min(window.devicePixelRatio,2));\n"
        "  renderer.setSize(W, H);\n"
        "  renderer.setClearColor(0x000000, 0);\n\n"
        "  var scene  = new THREE.Scene();\n"
        "  var camera = new THREE.PerspectiveCamera(50, W/H, 0.5, 4000);\n"
        "  window.addEventListener('resize', function(){\n"
        "    W=window.innerWidth; H=window.innerHeight;\n"
        "    renderer.setSize(W,H);\n"
        "    camera.aspect=W/H; camera.updateProjectionMatrix();\n"
        "  });\n\n",
        canvas->as.canvas3d.name);

    write_background(js, canvas->as.canvas3d.background,
                     canvas->as.canvas3d.bg_color);
    write_stars(js, canvas->as.canvas3d.background);
    write_lighting(js);
    write_camera(js);

    fprintf(js,
        "  var hoverMeshes = [];\n"
        "  var allRings    = [];\n"
        "  var coreMesh    = null;\n"
        "  var coreGrp     = null;\n"
        "  var armRings    = [];\n"
        "  var cLight      = scene.getObjectByName ? "
        "scene.children[1] : null;\n\n");

    /* Rebuild cLight reference properly */
    fprintf(js,
        "  /* re-grab cLight after lighting setup */\n"
        "  scene.traverse(function(o){\n"
        "    if(o.isPointLight && !cLight) cLight=o;\n"
        "  });\n\n");

    int ring_idx = 0;
    for (int i = 0; i < canvas->as.canvas3d.children.count; i++) {
        ASTNode *child = canvas->as.canvas3d.children.items[i];
        if (!child) continue;
        if (child->type == NODE_ORBITAL_RING)
            write_orbital_ring(js, child, ring_idx++);
        if (child->type == NODE_CORE_STAR) {
            write_core_star(js, child);
            fprintf(js,
                "  coreMesh = coreGrp.children[0];\n");
        }
    }

    write_animation(js);

    /* Legend update */
    fprintf(js,
        "\n  /* ── Legend ── */\n"
        "  var legend = document.getElementById('epl-legend');\n"
        "  if (legend) {\n"
        "    var items = [\n"
        "      {c:'#8B7FE8',l:'Completed'},\n"
        "      {c:'#EF9F27',l:'In Progress'},\n"
        "      {c:'#D85A30',l:'Overdue'},\n"
        "      {c:'#1a1840',l:'Pending'}\n"
        "    ];\n"
        "    items.forEach(function(it){\n"
        "      var d=document.createElement('div');\n"
        "      d.className='leg-item';\n"
        "      d.innerHTML='<div class=\"leg-dot\" style=\"background:'+it.c"
        "+'\"></div><span class=\"leg-txt\">'+it.l+'</span>';\n"
        "      legend.appendChild(d);\n"
        "    });\n"
        "  }\n\n"
        "})();\n");
}


/* ═══════════════════════════════════════
   SHELL HTML
   ═══════════════════════════════════════ */
static void write_shell_html(SceneCtx *ctx, ASTNode *page,
                              ASTNode *canvas) {
    const char *title = "EPL Page";
    if (canvas)
        title = canvas->as.canvas3d.name;

    fprintf(ctx->html,
        "<!DOCTYPE html>\n"
        "<html lang=\"en\">\n"
        "<head>\n"
        "  <meta charset=\"UTF-8\">\n"
        "  <meta name=\"viewport\" "
        "content=\"width=device-width,initial-scale=1.0\">\n"
        "  <title>%s</title>\n"
        "  <link href=\"https://fonts.googleapis.com/css2?"
        "family=Rajdhani:wght@300;400;500;600"
        "&family=Share+Tech+Mono&display=swap\" rel=\"stylesheet\">\n",
        title);

    if (ctx->offline) {
        /* embed CSS inline */
        fprintf(ctx->html, "  <style>\n");
        /* read css file and embed */
        rewind(ctx->css);
        char buf[512];
        while (fgets(buf, sizeof(buf), ctx->css))
            fprintf(ctx->html, "%s", buf);
        fprintf(ctx->html, "  </style>\n");
    } else {
        fprintf(ctx->html,
            "  <link rel=\"stylesheet\" href=\"%s.style.css\">\n",
            ctx->base);
    }

    fprintf(ctx->html,
        "</head>\n"
        "<body>\n"
        "<div id=\"epl-root\">\n"
        "  <canvas id=\"epl-canvas\"></canvas>\n");

    if (canvas)
        write_overlay_html(ctx->html, canvas, page);

    fprintf(ctx->html,
        "  <div id=\"epl-legend\"></div>\n"
        "</div>\n\n");

    /* Three.js */
    fprintf(ctx->html,
        "<script src=\"https://cdnjs.cloudflare.com/ajax/libs/"
        "three.js/r128/three.min.js\"></script>\n");

    if (ctx->offline) {
        fprintf(ctx->html, "<script>\n");
        rewind(ctx->js);
        char buf[512];
        while (fgets(buf, sizeof(buf), ctx->js))
            fprintf(ctx->html, "%s", buf);
        fprintf(ctx->html, "</script>\n");
    } else {
        fprintf(ctx->html,
            "<script src=\"%s.data.js\"></script>\n"
            "<script src=\"%s.scene.js\"></script>\n",
            ctx->base, ctx->base);
    }

    fprintf(ctx->html, "</body>\n</html>\n");
}


/* ═══════════════════════════════════════
   PUBLIC ENTRY POINT
   ═══════════════════════════════════════ */
void scene_gen_page(Interpreter *interp, Env *env,
                    ASTNode *page_node) {
    /* Find canvas3d child */
    ASTNode *canvas = NULL;
    NodeList *children = &page_node->as.make_page.children;
    for (int i = 0; i < children->count; i++) {
        if (children->items[i] &&
            children->items[i]->type == NODE_CANVAS3D) {
            canvas = children->items[i];
            break;
        }
    }
    if (!canvas) return;   /* no 3D canvas — fall through to normal codegen */

    /* Build base filename */
    char base[512];
    strncpy(base, page_node->as.make_page.filename, sizeof(base)-1);
    char *dot = strrchr(base, '.');
    if (dot) *dot = '\0';

    /* Open output files */
    char path[600];
    snprintf(path, sizeof(path), "%s.scene.js", base);  FILE *js  = fopen(path,"w");
    snprintf(path, sizeof(path), "%s.data.js",  base);  FILE *data = fopen(path,"w");
    snprintf(path, sizeof(path), "%s.style.css",base);  FILE *css  = fopen(path,"w");
    char html_path[600];
    snprintf(html_path, sizeof(html_path), "%s",
             page_node->as.make_page.filename);
    FILE *html = fopen(html_path, "w");

    if (!js || !data || !css || !html) {
        fprintf(stderr, "[EPL Error] Cannot open output files for %s\n",
                page_node->as.make_page.filename);
        return;
    }

    /* Build theme */
    ThemeCSS theme = codegen_get_theme(page_node->as.make_page.theme,
                                       page_node->as.make_page.color);

    SceneCtx ctx;
    ctx.js      = js;
    ctx.data    = data;
    ctx.css     = css;
    ctx.html    = html;
    ctx.interp  = interp;
    ctx.env     = env;
    ctx.offline = canvas->as.canvas3d.offline;
    strncpy(ctx.base, base, sizeof(ctx.base)-1);
    strncpy(ctx.theme_bg,      theme.bg,      sizeof(ctx.theme_bg)-1);
    strncpy(ctx.theme_surface, theme.surface, sizeof(ctx.theme_surface)-1);
    strncpy(ctx.theme_accent,  theme.accent,  sizeof(ctx.theme_accent)-1);
    strncpy(ctx.theme_text,    theme.text,    sizeof(ctx.theme_text)-1);
    strncpy(ctx.theme_border,  theme.border,  sizeof(ctx.theme_border)-1);

    write_css(&ctx, &ctx);
    write_scene_js(&ctx, canvas);
    fprintf(data, "/* EPL v1.6 Data layer — %s */\n", base);
    write_shell_html(&ctx, page_node, canvas);

    fclose(js); fclose(data); fclose(css); fclose(html);

    /* Update .epbind */
    binding_update(page_node->as.make_page.filename, "html-3d");
    snprintf(path, sizeof(path), "%s.scene.js", base);
    binding_update(path, "js-scene");
    snprintf(path, sizeof(path), "%s.style.css", base);
    binding_update(path, "css");

    printf("[EPL] Generated: %s + %s.scene.js + %s.style.css\n",
           page_node->as.make_page.filename, base, base);
}
```

---

## 5. parser.c — Add v1.6 parse functions

### 5a. Add to statement dispatcher

**FIND:**
```c
        case TOKEN_MAKE_PAGE:   return parse_make_page(p);
```

**ADD AFTER IT:**
```c
        case TOKEN_CANVAS3D:    return parse_canvas3d(p);
        case TOKEN_ORBITAL:     return parse_orbital_ring(p);
        case TOKEN_NODE_3D:     return parse_node3d(p);
        case TOKEN_CORE_KW:     return parse_core_star(p);
        case TOKEN_PANEL_KW:    return parse_panel3d(p);
        case TOKEN_DRAWER_KW:   return parse_drawer3d(p);
```

### 5b. Add parse functions

**FIND:**
```c
/* ═════════════════════════════════════════════
   WAIT STATEMENT
```

**ADD BEFORE IT:**

```c
/* ═════════════════════════════════════════════
   NODE3D  (v1.6)
   node at 0.5 labeled "Task" status "completed"
   ═════════════════════════════════════════════ */
ASTNode *parse_node3d(Parser *p) {
    Token t = advance(p);   /* consume 'node' */
    ASTNode *node = ast_make_node(NODE_NODE3D, t.line, t.col);
    node->as.node3d.angle  = 0.0;
    node->as.node3d.status = NODE3D_PENDING;
    node->as.node3d.label[0]= '\0';
    node->as.node3d.desc[0] = '\0';

    /* at <number> */
    if (check(p, TOKEN_AT_KW)) {
        advance(p);
        if (check(p, TOKEN_NUMBER)) {
            node->as.node3d.angle = atof(current(p).value);
            advance(p);
        }
    }
    /* labeled "text" */
    if (check(p, TOKEN_LABELED)) {
        advance(p);
        if (check(p, TOKEN_STRING)) {
            strncpy(node->as.node3d.label,
                current(p).value, MAX_TOKEN_LEN-1);
            advance(p);
        }
    }
    /* status "completed|inprogress|overdue|pending" */
    if (check(p, TOKEN_STATUS_KW)) {
        advance(p);
        if (check(p, TOKEN_STRING)) {
            const char *s = current(p).value;
            if      (strcmp(s,"completed")==0)  node->as.node3d.status = NODE3D_COMPLETED;
            else if (strcmp(s,"inprogress")==0)  node->as.node3d.status = NODE3D_INPROGRESS;
            else if (strcmp(s,"overdue")==0)     node->as.node3d.status = NODE3D_OVERDUE;
            else                                 node->as.node3d.status = NODE3D_PENDING;
            advance(p);
        }
    }
    return node;
}


/* ═════════════════════════════════════════════
   CORE STAR  (v1.6)
   core star color "gold" labeled "Project"
   ═════════════════════════════════════════════ */
ASTNode *parse_core_star(Parser *p) {
    Token t = advance(p);   /* consume 'core' */
    match(p, TOKEN_STAR_KW);
    ASTNode *node = ast_make_node(NODE_CORE_STAR, t.line, t.col);
    node->as.core_star.color[0] = '\0';
    node->as.core_star.label[0] = '\0';
    node->as.core_star.desc[0]  = '\0';
    strncpy(node->as.core_star.color, "gold", MAX_TOKEN_LEN-1);

    if (check(p, TOKEN_WITH_COLOR)) {
        advance(p);
        if (check(p, TOKEN_STRING)) {
            strncpy(node->as.core_star.color,
                current(p).value, MAX_TOKEN_LEN-1);
            advance(p);
        }
    }
    if (check(p, TOKEN_LABELED)) {
        advance(p);
        if (check(p, TOKEN_STRING)) {
            strncpy(node->as.core_star.label,
                current(p).value, MAX_TOKEN_LEN-1);
            advance(p);
        }
    }
    return node;
}


/* ═════════════════════════════════════════════
   ORBITAL RING  (v1.6)
   orbital ring radius 62 color "indigo" speed 0.007
       node at 0.5 labeled "Task" status "completed"
   !
   ═════════════════════════════════════════════ */
ASTNode *parse_orbital_ring(Parser *p) {
    Token t = advance(p);   /* consume 'orbital' */
    match(p, TOKEN_RING_KW);
    ASTNode *node = ast_make_node(NODE_ORBITAL_RING, t.line, t.col);
    nodelist_init(&node->as.orbital_ring.nodes);
    node->as.orbital_ring.radius = 100.0;
    node->as.orbital_ring.speed  = 0.003;
    node->as.orbital_ring.tilt_x = 0.0;
    node->as.orbital_ring.tilt_z = 0.0;
    strncpy(node->as.orbital_ring.color, "indigo", MAX_TOKEN_LEN-1);

    /* radius N */
    if (check(p, TOKEN_RADIUS_KW)) {
        advance(p);
        if (check(p, TOKEN_NUMBER)) {
            node->as.orbital_ring.radius = atof(current(p).value);
            advance(p);
        }
    }
    /* color "name" */
    if (check(p, TOKEN_WITH_COLOR)||check(p, TOKEN_STYLED)) {
        advance(p);
        if (check(p, TOKEN_STRING)) {
            strncpy(node->as.orbital_ring.color,
                current(p).value, MAX_TOKEN_LEN-1);
            advance(p);
        }
    }
    /* speed N */
    if (check(p, TOKEN_SPEED_KW)) {
        advance(p);
        if (check(p, TOKEN_NUMBER)) {
            node->as.orbital_ring.speed = atof(current(p).value);
            advance(p);
        }
    }
    /* tilt x N z N (optional) */
    if (check(p, TOKEN_X_AXIS)) {
        advance(p);
        if (check(p, TOKEN_NUMBER)) {
            node->as.orbital_ring.tilt_x = atof(current(p).value);
            advance(p);
        }
    }
    if (check(p, TOKEN_Z_AXIS)||check(p, TOKEN_Y_AXIS)) {
        advance(p);
        if (check(p, TOKEN_NUMBER)) {
            node->as.orbital_ring.tilt_z = atof(current(p).value);
            advance(p);
        }
    }

    /* nodes until ! */
    while (!check(p, TOKEN_TERMINATOR) && !check(p, TOKEN_EOF)) {
        if (check(p, TOKEN_NEWLINE)) { advance(p); continue; }
        if (check(p, TOKEN_NODE_3D)) {
            ASTNode *nd = parse_node3d(p);
            if (nd) nodelist_push(&node->as.orbital_ring.nodes, nd);
        } else {
            advance(p);
        }
    }
    match(p, TOKEN_TERMINATOR);
    node->end_line = current(p).line;
    return node;
}


/* ═════════════════════════════════════════════
   CANVAS3D  (v1.6)
   canvas3d "name" background "deep space"
       orbital ring ... !
       core star ...
   !
   ═════════════════════════════════════════════ */
ASTNode *parse_canvas3d(Parser *p) {
    Token t = advance(p);   /* consume 'canvas3d' */
    ASTNode *node = ast_make_node(NODE_CANVAS3D, t.line, t.col);
    nodelist_init(&node->as.canvas3d.children);
    node->as.canvas3d.background = BG3D_DEEP_SPACE;
    node->as.canvas3d.offline    = 0;
    node->as.canvas3d.name[0]    = '\0';
    node->as.canvas3d.bg_color[0]= '\0';

    /* optional name string */
    if (check(p, TOKEN_STRING)) {
        strncpy(node->as.canvas3d.name,
            current(p).value, MAX_TOKEN_LEN-1);
        advance(p);
    }

    /* background "preset" */
    if (check(p, TOKEN_BACKGROUND_KW)) {
        advance(p);
        if (check(p, TOKEN_STRING)) {
            const char *bg = current(p).value;
            if      (strstr(bg,"space")) node->as.canvas3d.background = BG3D_DEEP_SPACE;
            else if (strstr(bg,"void"))  node->as.canvas3d.background = BG3D_VOID;
            else if (strstr(bg,"nebula"))node->as.canvas3d.background = BG3D_NEBULA;
            else {
                node->as.canvas3d.background = BG3D_CUSTOM;
                strncpy(node->as.canvas3d.bg_color, bg, MAX_TOKEN_LEN-1);
            }
            advance(p);
        }
    }

    /* offline keyword */
    if (check(p, TOKEN_OFFLINE)) {
        node->as.canvas3d.offline = 1;
        advance(p);
    }

    /* children until ! */
    while (!check(p, TOKEN_TERMINATOR) && !check(p, TOKEN_EOF)) {
        if (check(p, TOKEN_NEWLINE)) { advance(p); continue; }
        ASTNode *child = parse_statement(p);
        if (child) nodelist_push(&node->as.canvas3d.children, child);
    }
    match(p, TOKEN_TERMINATOR);
    node->end_line = current(p).line;
    return node;
}


/* ═════════════════════════════════════════════
   PANEL3D  (v1.6)
   panel "right"
       heading "Title"
       make stat "Active" from 12
   !
   ═════════════════════════════════════════════ */
ASTNode *parse_panel3d(Parser *p) {
    Token t = advance(p);   /* consume 'panel' */
    ASTNode *node = ast_make_node(NODE_PANEL3D, t.line, t.col);
    nodelist_init(&node->as.panel3d.children);
    node->as.panel3d.position = PANEL_RIGHT;

    if (check(p, TOKEN_STRING)) {
        const char *pos = current(p).value;
        if      (strcmp(pos,"top")==0)    node->as.panel3d.position = PANEL_TOP;
        else if (strcmp(pos,"left")==0)   node->as.panel3d.position = PANEL_LEFT;
        else if (strcmp(pos,"bottom")==0) node->as.panel3d.position = PANEL_BOTTOM;
        else                              node->as.panel3d.position = PANEL_RIGHT;
        advance(p);
    }

    while (!check(p, TOKEN_TERMINATOR) && !check(p, TOKEN_EOF)) {
        if (check(p, TOKEN_NEWLINE)) { advance(p); continue; }
        ASTNode *child = parse_statement(p);
        if (child) nodelist_push(&node->as.panel3d.children, child);
    }
    match(p, TOKEN_TERMINATOR);
    node->end_line = current(p).line;
    return node;
}


/* ═════════════════════════════════════════════
   DRAWER3D  (v1.6)
   drawer "right" titled "All Tasks"
       make table from tasks ...
   !
   ═════════════════════════════════════════════ */
ASTNode *parse_drawer3d(Parser *p) {
    Token t = advance(p);
    ASTNode *node = ast_make_node(NODE_DRAWER3D, t.line, t.col);
    nodelist_init(&node->as.drawer3d.children);
    node->as.drawer3d.position = PANEL_RIGHT;
    node->as.drawer3d.title[0] = '\0';

    if (check(p, TOKEN_STRING)) {
        const char *pos = current(p).value;
        if (strcmp(pos,"left")==0) node->as.drawer3d.position = PANEL_LEFT;
        advance(p);
    }
    if (check(p, TOKEN_TITLED)) {
        advance(p);
        if (check(p, TOKEN_STRING)) {
            strncpy(node->as.drawer3d.title,
                current(p).value, MAX_TOKEN_LEN-1);
            advance(p);
        }
    }

    while (!check(p, TOKEN_TERMINATOR) && !check(p, TOKEN_EOF)) {
        if (check(p, TOKEN_NEWLINE)) { advance(p); continue; }
        ASTNode *child = parse_statement(p);
        if (child) nodelist_push(&node->as.drawer3d.children, child);
    }
    match(p, TOKEN_TERMINATOR);
    node->end_line = current(p).line;
    return node;
}
```

---

## 6. codegen.c — wire scene_gen

**ADD at top of codegen.c:**
```c
#include "scene_gen.h"
```

**FIND** in `codegen_page`:
```c
    FILE *out = fopen(filename, "w");
```

**REPLACE WITH:**
```c
    /* If page contains canvas3d — route to scene generator */
    for (int i = 0; i < node->as.make_page.children.count; i++) {
        ASTNode *child = node->as.make_page.children.items[i];
        if (child && child->type == NODE_CANVAS3D) {
            scene_gen_page(interp, env, node);
            return;
        }
    }

    FILE *out = fopen(filename, "w");
```

---

## 7. lexer.c — add v1.6 keywords

**ADD to keyword_table[] (alphabetical):**
```c
    { "at",         TOKEN_AT_KW       },
    { "background", TOKEN_BACKGROUND_KW },
    { "canvas3d",   TOKEN_CANVAS3D    },
    { "core",       TOKEN_CORE_KW     },
    { "drawer",     TOKEN_DRAWER_KW   },
    { "labeled",    TOKEN_LABELED     },
    { "offline",    TOKEN_OFFLINE     },
    { "orbital",    TOKEN_ORBITAL     },
    { "panel",      TOKEN_PANEL_KW    },
    { "radius",     TOKEN_RADIUS_KW   },
    { "ring",       TOKEN_RING_KW     },
    { "speed",      TOKEN_SPEED_KW    },
    { "star",       TOKEN_STAR_KW     },
    { "status",     TOKEN_STATUS_KW   },
```

**NOTE:** `TOKEN_Z_AXIS` — add to token.h and lexer:
```c
    TOKEN_Z_AXIS,    /* z */
    { "z", TOKEN_Z_AXIS },
```

---

## 8. Recompile

```powershell
gcc -Wall -std=c11 -o epl main.c lexer.c token.c ast.c parser.c interpreter.c repl.c codegen.c binding.c describe.c scene_gen.c -lm
```

---

## 9. Test file — orbital.EPF

```epf
describe dashboard as "Orbital task timeline — EPL v1.6 3D demo"

make page saved as "orbital.html" styled as "dark"

    canvas3d "Monarch Rings" background "deep space"

        core star color "gold" labeled "Shadow Nexus"

        orbital ring radius 62 color "indigo" speed 0.007
            node at 0 labeled "Morning Supplement" status "completed"
            node at 1.57 labeled "Evening Vitamin" status "pending"
            node at 3.14 labeled "Water Intake" status "completed"
            node at 4.71 labeled "Stretching" status "inprogress"
        !

        orbital ring radius 104 color "blue" speed 0.004
            node at 0.5 labeled "Omega-3 Weekly" status "completed"
            node at 2.1 labeled "Progress Photo" status "inprogress"
            node at 3.7 labeled "Meal Prep" status "overdue"
            node at 5.3 labeled "Sleep Log" status "completed"
        !

        orbital ring radius 146 color "green" speed 0.0027
            node at 1.0 labeled "Blood Panel" status "completed"
            node at 2.8 labeled "Coach Check-in" status "pending"
            node at 4.5 labeled "Creatine Dose" status "completed"
        !

        orbital ring radius 188 color "amber" speed 0.0015
            node at 0.8 labeled "Body Comp Scan" status "overdue"
            node at 2.6 labeled "Supplement Log" status "completed"
            node at 4.3 labeled "Hydration Check" status "completed"
        !

        orbital ring radius 230 color "pink" speed 0.0007
            node at 1.2 labeled "Diet Full Review" status "inprogress"
            node at 3.3 labeled "Supplement Restock" status "pending"
            node at 5.5 labeled "Lab Work Order" status "completed"
        !

    !

    panel "right"
        heading "Monarch Rings"
        subheading "Orbital Timeline"
        make stat "Total Tasks" from 19
        make stat "Completed" from 10
        make stat "Overdue" from 2
    !

!
```

---

## 10. Expected output

```
[EPL] Generated: orbital.html + orbital.scene.js + orbital.style.css
```

Open `orbital.html` in browser:
- Full Three.js WebGL scene
- 5 orbital rings with tilts, completion arcs, glow
- 19 task nodes with status colors, pulse rings, ghost trails
- Gold core star with armillary rings
- Spring camera — drag to orbit, scroll to zoom, click to inspect
- Raycaster hover tooltips
- Slide-in detail panel on node click
- Bottom legend
- Right panel with stats overlay
```
