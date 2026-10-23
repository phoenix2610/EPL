#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include "scene_gen.h"
#include "binding.h"
#include "codegen.h"

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

static const char *color_to_hex(const char *color) {
    if (!color || color[0] == '#') return color ? color : "#8B7FE8";
    if (strcmp(color,"indigo")==0||strcmp(color,"purple")==0) return "#8B7FE8";
    if (strcmp(color,"blue")==0)    return "#4A9AEF";
    if (strcmp(color,"green")==0||strcmp(color,"emerald")==0) return "#22B890";
    if (strcmp(color,"amber")==0||strcmp(color,"orange")==0)  return "#F0A030";
    if (strcmp(color,"pink")==0||strcmp(color,"rose")==0)     return "#E05090";
    if (strcmp(color,"red")==0)     return "#D85A30";
    if (strcmp(color,"gold")==0)    return "#E8C050";
    if (strcmp(color,"white")==0)   return "#FFFFFF";
    if (strcmp(color,"cyan")==0)    return "#22D3EE";
    return "#8B7FE8";
}

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
                "  scene.background = new THREE.Color('%s');\n",
                custom ? custom : "#010110");
            break;
    }
}

static void write_stars(FILE *js, Background3D bg) {
    if (bg == BG3D_VOID) return;

    fprintf(js,
        "\n  /* ── Star field ── */\n"
        "  (function(){\n"
        "    var n=2200;\n"
        "    var pos=new Float32Array(n*3);\n"
        "    var col=new Float32Array(n*3);\n"
        "    for(var i=0;i<n;i++){\n"
        "      var r=700+Math.random()*1400;\n"
        "      var th=Math.random()*Math.PI*2;\n"
        "      var ph=Math.acos(1-Math.random()*2);\n"
        "      pos[i*3]  =r*Math.sin(ph)*Math.cos(th);\n"
        "      pos[i*3+1]=r*Math.cos(ph);\n"
        "      pos[i*3+2]=r*Math.sin(ph)*Math.sin(th);\n"
        "      var h=Math.random();\n"
        "      col[i*3]=0.5+h*0.5;\n"
        "      col[i*3+1]=0.5+(1-h)*0.3;\n"
        "      col[i*3+2]=0.8+Math.random()*0.2;\n"
        "    }\n"
        "    var g=new THREE.BufferGeometry();\n"
        "    g.setAttribute('position',\n"
        "      new THREE.BufferAttribute(pos,3));\n"
        "    g.setAttribute('color',\n"
        "      new THREE.BufferAttribute(col,3));\n"
        "    scene.add(new THREE.Points(g,\n"
        "      new THREE.PointsMaterial({\n"
        "        size:0.95,transparent:true,\n"
        "        opacity:0.36,vertexColors:true\n"
        "      })));\n"
        "  })();\n\n");

    if (bg == BG3D_NEBULA || bg == BG3D_DEEP_SPACE) {
        fprintf(js,
            "  /* ── Nebula clouds ── */\n"
            "  (function(){\n"
            "    function neb(c,col,mn,mx,op,fl){\n"
            "      var pos=new Float32Array(c*3);\n"
            "      for(var i=0;i<c;i++){\n"
            "        var r=mn+Math.random()*(mx-mn);\n"
            "        var th=Math.random()*Math.PI*2;\n"
            "        var ph=Math.acos(1-Math.random()*2);\n"
            "        pos[i*3]  =r*Math.sin(ph)*Math.cos(th);\n"
            "        pos[i*3+1]=r*Math.cos(ph)*(fl||1);\n"
            "        pos[i*3+2]=r*Math.sin(ph)*Math.sin(th);\n"
            "      }\n"
            "      var g=new THREE.BufferGeometry();\n"
            "      g.setAttribute('position',\n"
            "        new THREE.BufferAttribute(pos,3));\n"
            "      return new THREE.Points(g,\n"
            "        new THREE.PointsMaterial({\n"
            "          color:col,size:2.8,transparent:true,\n"
            "          opacity:op,depthWrite:false,\n"
            "          blending:THREE.AdditiveBlending\n"
            "        }));\n"
            "    }\n"
            "    scene.add(neb(600,0x2518a0,250,600,0.025,0.5));\n"
            "    scene.add(neb(450,0x0a2a60,300,700,0.020,0.6));\n"
            "    scene.add(neb(350,0x380828,150,400,0.032,0.4));\n"
            "  })();\n\n");
    }
}

static void write_lighting(FILE *js) {
    fprintf(js,
        "  /* ── Lighting ── */\n"
        "  scene.add(new THREE.AmbientLight(0x0d0824, 6));\n"
        "  cLight = new THREE.PointLight(0xC8A840, 12, 500);\n"
        "  scene.add(cLight);\n"
        "  var fA = new THREE.PointLight(0x1a3a8a, 4, 800);\n"
        "  fA.position.set(320, 200, 100);\n"
        "  scene.add(fA);\n"
        "  var fB = new THREE.PointLight(0x3010a0, 3, 700);\n"
        "  fB.position.set(-200, -180, 220);\n"
        "  scene.add(fB);\n\n");
}

static void write_core_star(FILE *js, ASTNode *node) {
    const char *color   = node->as.core_star.color;
    const char *label   = node->as.core_star.label;
    const char *hex     = color_to_hex(color);
    const char *emissive = "#B08000";

    if (strstr(color,"gold"))          emissive = "#B08000";
    else if (strstr(color,"blue"))     emissive = "#1a4a8a";
    else if (strstr(color,"indigo")||
             strstr(color,"purple"))   emissive = "#4030cc";
    else if (strstr(color,"green")||
             strstr(color,"emerald"))  emissive = "#0a6040";
    else if (color[0]=='#')            emissive = hex;

    fprintf(js,
        "  /* ── Core Star ── */\n"
        "  coreGrp = new THREE.Group();\n"
        "  scene.add(coreGrp);\n"
        "  coreMesh = new THREE.Mesh(\n"
        "    new THREE.SphereGeometry(16, 52, 52),\n"
        "    new THREE.MeshStandardMaterial({\n"
        "      color:'%s',\n"
        "      emissive:'%s',\n"
        "      emissiveIntensity:2.8,\n"
        "      roughness:0.06,\n"
        "      metalness:0.98\n"
        "    }));\n"
        "  coreMesh.userData = {\n"
        "    isCore:true,\n"
        "    label:'%s',\n"
        "    ring:'Core',\n"
        "    status:'Active',\n"
        "    desc:'Central project node'\n"
        "  };\n"
        "  hoverMeshes.push(coreMesh);\n"
        "  coreGrp.add(coreMesh);\n\n",
        hex, emissive, label);

    fprintf(js,
        "  /* Core glow shells */\n"
        "  [  [24,'%s',0.22],\n"
        "     [36,'%s',0.11],\n"
        "     [52,'%s',0.055],\n"
        "     [72,'%s',0.024],\n"
        "     [100,'%s',0.009]\n"
        "  ].forEach(function(d){\n"
        "    coreGrp.add(new THREE.Mesh(\n"
        "      new THREE.SphereGeometry(d[0],18,18),\n"
        "      new THREE.MeshBasicMaterial({\n"
        "        color:d[1],transparent:true,opacity:d[2],\n"
        "        depthWrite:false,\n"
        "        blending:THREE.AdditiveBlending\n"
        "      })));\n"
        "  });\n\n",
        hex, hex, hex, hex, hex);

    fprintf(js,
        "  /* Armillary rings */\n"
        "  armRings = [];\n"
        "  [\n"
        "    {r:28,rx:0,           rz:0,           rs:0.004, c:'#FFD060'},\n"
        "    {r:28,rx:Math.PI/2,   rz:0,           rs:-0.003,c:'#FF9820'},\n"
        "    {r:28,rx:Math.PI/4,   rz:Math.PI/4,   rs:0.0022,c:'#E8C840'}\n"
        "  ].forEach(function(d){\n"
        "    var m = new THREE.Mesh(\n"
        "      new THREE.TorusGeometry(d.r, 0.7, 8, 84),\n"
        "      new THREE.MeshBasicMaterial({\n"
        "        color:d.c, transparent:true, opacity:0.55,\n"
        "        blending:THREE.AdditiveBlending\n"
        "      }));\n"
        "    m.rotation.x = d.rx;\n"
        "    m.rotation.z = d.rz;\n"
        "    m.userData   = {rspd: d.rs};\n"
        "    coreGrp.add(m);\n"
        "    armRings.push(m);\n"
        "  });\n\n");
}

static void write_orbital_ring(FILE *js, ASTNode *ring,
                                int ring_idx) {
    double      radius = ring->as.orbital_ring.radius;
    double      speed  = ring->as.orbital_ring.speed;
    double      tilt_x = ring->as.orbital_ring.tilt_x;
    double      tilt_z = ring->as.orbital_ring.tilt_z;
    const char *hex    = color_to_hex(ring->as.orbital_ring.color);

    int total = ring->as.orbital_ring.nodes.count;
    int done  = 0;
    for (int i = 0; i < total; i++) {
        ASTNode *n = ring->as.orbital_ring.nodes.items[i];
        if (n && n->type == NODE_NODE3D &&
            n->as.node3d.status == NODE3D_COMPLETED) done++;
    }
    double pct = (total > 0) ? (double)done / total : 0.0;

    fprintf(js,
        "  /* ── Orbital Ring %d (r=%.0f) ── */\n"
        "  (function(){\n"
        "    var grp = new THREE.Group();\n"
        "    grp.rotation.x = %.4f;\n"
        "    grp.rotation.z = %.4f;\n\n"

        "    /* Ring torus */\n"
        "    grp.add(new THREE.Mesh(\n"
        "      new THREE.TorusGeometry(%.1f, 0.55, 8, 140),\n"
        "      new THREE.MeshBasicMaterial({\n"
        "        color:'%s', transparent:true,\n"
        "        opacity:0.15, depthWrite:false\n"
        "      })));\n\n"

        "    /* Glow torus */\n"
        "    grp.add(new THREE.Mesh(\n"
        "      new THREE.TorusGeometry(%.1f, 4.2, 8, 140),\n"
        "      new THREE.MeshBasicMaterial({\n"
        "        color:'%s', transparent:true,\n"
        "        opacity:0.026, depthWrite:false,\n"
        "        blending:THREE.AdditiveBlending\n"
        "      })));\n\n",
        ring_idx, radius,
        tilt_x, tilt_z,
        radius, hex,
        radius, hex);

    if (pct > 0.01) {
        fprintf(js,
            "    /* Completion arc %.0f%% */\n"
            "    (function(){\n"
            "      var ae = %.6f * Math.PI * 2;\n"
            "      var segs = Math.max(16,\n"
            "        Math.floor(ae/(Math.PI*2)*160));\n"
            "      var pts = [];\n"
            "      for (var i=0; i<=segs; i++) {\n"
            "        var a = ae * (i/segs);\n"
            "        pts.push(new THREE.Vector3(\n"
            "          Math.cos(a)*%.1f, 0,\n"
            "          Math.sin(a)*%.1f));\n"
            "      }\n"
            "      var curve = new THREE.CatmullRomCurve3(pts, false);\n"
            "      [[1.8,0.82],[6.5,0.10],[13,0.032]]\n"
            "      .forEach(function(d){\n"
            "        grp.add(new THREE.Mesh(\n"
            "          new THREE.TubeGeometry(curve,segs,d[0],7,false),\n"
            "          new THREE.MeshBasicMaterial({\n"
            "            color:'%s', transparent:true,\n"
            "            opacity:d[1], depthWrite:false,\n"
            "            blending:THREE.AdditiveBlending\n"
            "          })));\n"
            "      });\n"
            "    })();\n\n",
            pct * 100.0, pct,
            radius, radius, hex);
    }

    fprintf(js,
        "    /* Tick marks */\n"
        "    for (var k=0; k<%d; k++) {\n"
        "      var ta = (k/%d)*Math.PI*2;\n"
        "      var active = (Math.random()>0.38);\n"
        "      var tm = new THREE.Mesh(\n"
        "        new THREE.SphereGeometry(active?1.6:0.9, 6, 6),\n"
        "        new THREE.MeshBasicMaterial({\n"
        "          color: active?'%s':'#151530',\n"
        "          transparent:true,\n"
        "          opacity: active?0.6:0.12,\n"
        "          blending: active ?\n"
        "            THREE.AdditiveBlending:\n"
        "            THREE.NormalBlending\n"
        "        }));\n"
        "      tm.position.set(\n"
        "        Math.cos(ta)*%.1f, 0, Math.sin(ta)*%.1f);\n"
        "      grp.add(tm);\n"
        "    }\n\n",
        total > 0 ? total + 4 : 8,
        total > 0 ? total + 4 : 8,
        hex, radius, radius);

    fprintf(js,
        "    var ringNodes = [];\n\n");

    for (int i = 0; i < ring->as.orbital_ring.nodes.count; i++) {
        ASTNode *nd = ring->as.orbital_ring.nodes.items[i];
        if (!nd || nd->type != NODE_NODE3D) continue;

        double      angle  = nd->as.node3d.angle;
        Node3DStatus st    = nd->as.node3d.status;
        const char *nhex   = status_hex(st);
        const char *emhex  = status_emissive(st);
        double      emint  = status_intensity(st);
        const char *lbl    = nd->as.node3d.label;
        const char *slbl   = status_label(st);
        int         pending = (st == NODE3D_PENDING);
        int         done_n  = (st == NODE3D_COMPLETED);
        int         over_n  = (st == NODE3D_OVERDUE);

        fprintf(js,
            "    /* Node: %s */\n"
            "    (function(){\n"
            "      var ng = new THREE.Group();\n"
            "      ng.position.set(\n"
            "        Math.cos(%.4f)*%.1f, 0,\n"
            "        Math.sin(%.4f)*%.1f);\n\n"

            "      var node = new THREE.Mesh(\n"
            "        new THREE.SphereGeometry(5.8, 24, 24),\n"
            "        new THREE.MeshStandardMaterial({\n"
            "          color:'%s',\n"
            "          emissive:'%s',\n"
            "          emissiveIntensity:%.2f,\n"
            "          roughness:0.22,\n"
            "          metalness:0.82\n"
            "        }));\n"
            "      node.userData = {\n"
            "        label:'%s',\n"
            "        status:'%s',\n"
            "        ring:'Ring %d',\n"
            "        isCore:false,\n"
            "        desc:''\n"
            "      };\n"
            "      hoverMeshes.push(node);\n"
            "      ng.add(node);\n\n",
            lbl,
            angle, radius, angle, radius,
            nhex, emhex, emint,
            lbl, slbl, ring_idx);

        fprintf(js,
            "      /* Glow shells */\n"
            "      [[10,%.3f],[17,%.3f],[26,%.3f]]\n"
            "      .forEach(function(d){\n"
            "        ng.add(new THREE.Mesh(\n"
            "          new THREE.SphereGeometry(d[0],12,12),\n"
            "          new THREE.MeshBasicMaterial({\n"
            "            color:'%s',transparent:true,\n"
            "            opacity:d[1],depthWrite:false,\n"
            "            blending:THREE.AdditiveBlending\n"
            "          })));\n"
            "      });\n\n",
            pending ? 0.02 : 0.18,
            pending ? 0.01 : 0.08,
            pending ? 0.005: 0.03,
            nhex);

        if (done_n) {
            fprintf(js,
                "      /* Pulse ring */\n"
                "      var pr = new THREE.Mesh(\n"
                "        new THREE.TorusGeometry(8.5, 0.55, 8, 44),\n"
                "        new THREE.MeshBasicMaterial({\n"
                "          color:'%s', transparent:true,\n"
                "          opacity:0.55, depthWrite:false,\n"
                "          blending:THREE.AdditiveBlending\n"
                "        }));\n"
                "      pr.rotation.x = Math.PI/2;\n"
                "      ng.add(pr);\n"
                "      node.userData.pulseRing = pr;\n\n",
                hex);
        }

        if (over_n) {
            fprintf(js,
                "      /* Warn ring */\n"
                "      var wr = new THREE.Mesh(\n"
                "        new THREE.TorusGeometry(9.5, 0.45, 8, 44),\n"
                "        new THREE.MeshBasicMaterial({\n"
                "          color:'#D85A30', transparent:true,\n"
                "          opacity:0.4, depthWrite:false,\n"
                "          blending:THREE.AdditiveBlending\n"
                "        }));\n"
                "      wr.rotation.x = Math.PI/3;\n"
                "      ng.add(wr);\n"
                "      node.userData.warnRing = wr;\n\n");
        }

        fprintf(js,
            "      /* Ghost trail */\n"
            "      var ghosts = [];\n"
            "      for (var k=1; k<=7; k++) {\n"
            "        var gs = new THREE.Mesh(\n"
            "          new THREE.SphereGeometry(5.8*(1-k*0.1),7,7),\n"
            "          new THREE.MeshBasicMaterial({\n"
            "            color:'%s', transparent:true,\n"
            "            opacity:%s*Math.pow(1-k/8,1.8),\n"
            "            depthWrite:false,\n"
            "            blending:THREE.AdditiveBlending\n"
            "          }));\n"
            "        grp.add(gs);\n"
            "        ghosts.push({mesh:gs, delta:k*0.036});\n"
            "      }\n\n",
            nhex,
            pending ? "0" : "0.22");

        fprintf(js,
            "      grp.add(ng);\n"
            "      ringNodes.push({\n"
            "        ng:     ng,\n"
            "        node:   node,\n"
            "        angle:  %.4f,\n"
            "        radius: %.1f,\n"
            "        speed:  %.6f,\n"
            "        ghosts: ghosts\n"
            "      });\n"
            "    })();\n\n",
            angle, radius, speed);
    }

    fprintf(js,
        "    scene.add(grp);\n"
        "    allRings.push({\n"
        "      grp:    grp,\n"
        "      nodes:  ringNodes,\n"
        "      speed:  %.6f,\n"
        "      radius: %.1f\n"
        "    });\n"
        "  })();\n\n",
        speed, radius);
}

static void write_camera_and_interaction(FILE *js) {
    fprintf(js,
        "  /* ── Camera setup ── */\n"
        "  var sph    = {th:0.65, ph:1.1, r:500};\n"
        "  var tgtSph = {th:0.65, ph:1.1, r:500};\n"
        "  var isDragging = false;\n"
        "  var didDrag    = false;\n"
        "  var px = 0, py = 0;\n"
        "  var lastInt  = Date.now();\n"
        "  var autoRot  = true;\n"
        "  var hovLabel = null;\n\n"

        "  function updateCam() {\n"
        "    camera.position.set(\n"
        "      sph.r * Math.sin(sph.ph) * Math.sin(sph.th),\n"
        "      sph.r * Math.cos(sph.ph),\n"
        "      sph.r * Math.sin(sph.ph) * Math.cos(sph.th));\n"
        "    camera.lookAt(0, 0, 0);\n"
        "  }\n"
        "  updateCam();\n\n"

        "  /* ── Mouse drag ── */\n"
        "  var cvs = document.getElementById('epl-canvas');\n"
        "  cvs.addEventListener('mousedown', function(e){\n"
        "    isDragging = true; didDrag = false;\n"
        "    px = e.clientX; py = e.clientY;\n"
        "    autoRot = false; lastInt = Date.now();\n"
        "  });\n"
        "  window.addEventListener('mouseup', function(){\n"
        "    isDragging = false;\n"
        "  });\n"
        "  window.addEventListener('mousemove', function(e){\n"
        "    if (isDragging) {\n"
        "      var dx = e.clientX-px, dy = e.clientY-py;\n"
        "      if (Math.abs(dx)>2||Math.abs(dy)>2) didDrag = true;\n"
        "      tgtSph.th -= dx * 0.0042;\n"
        "      tgtSph.ph  = Math.max(0.08,\n"
        "        Math.min(2.96, tgtSph.ph - dy*0.0042));\n"
        "      px = e.clientX; py = e.clientY;\n"
        "      lastInt = Date.now();\n"
        "      var tt = document.getElementById('epl-tt');\n"
        "      if (tt) tt.style.opacity = '0';\n"
        "    } else {\n"
        "      doRaycast(e.clientX, e.clientY);\n"
        "    }\n"
        "  });\n\n"

        "  /* ── Scroll zoom ── */\n"
        "  cvs.addEventListener('wheel', function(e){\n"
        "    tgtSph.r = Math.max(150,\n"
        "      Math.min(1200, tgtSph.r + e.deltaY*0.3));\n"
        "    autoRot = false; lastInt = Date.now();\n"
        "    e.preventDefault();\n"
        "  }, {passive:false});\n\n"

        "  /* ── Touch support ── */\n"
        "  var lt = null;\n"
        "  cvs.addEventListener('touchstart', function(e){\n"
        "    if (e.touches.length===1) {\n"
        "      isDragging = true; didDrag = false;\n"
        "      lt = {x:e.touches[0].clientX, y:e.touches[0].clientY};\n"
        "    }\n"
        "    autoRot = false; lastInt = Date.now();\n"
        "  });\n"
        "  cvs.addEventListener('touchend', function(){\n"
        "    isDragging = false; lt = null;\n"
        "  });\n"
        "  cvs.addEventListener('touchmove', function(e){\n"
        "    if (!lt || e.touches.length!==1) return;\n"
        "    var dx=e.touches[0].clientX-lt.x;\n"
        "    var dy=e.touches[0].clientY-lt.y;\n"
        "    if (Math.abs(dx)>2||Math.abs(dy)>2) didDrag=true;\n"
        "    tgtSph.th -= dx*0.0042;\n"
        "    tgtSph.ph = Math.max(0.08,\n"
        "      Math.min(2.96, tgtSph.ph-dy*0.0042));\n"
        "    lt = {x:e.touches[0].clientX, y:e.touches[0].clientY};\n"
        "    lastInt = Date.now();\n"
        "    e.preventDefault();\n"
        "  }, {passive:false});\n\n");

    fprintf(js,
        "  /* ── Raycaster ── */\n"
        "  var raycaster = new THREE.Raycaster();\n"
        "  var mouse2    = new THREE.Vector2();\n\n"

        "  function doRaycast(cx, cy) {\n"
        "    var rect = cvs.getBoundingClientRect();\n"
        "    mouse2.x = ((cx-rect.left)/rect.width)*2-1;\n"
        "    mouse2.y = -((cy-rect.top)/rect.height)*2+1;\n"
        "    raycaster.setFromCamera(mouse2, camera);\n"
        "    var hits = raycaster.intersectObjects(hoverMeshes);\n"
        "    var tt   = document.getElementById('epl-tt');\n"
        "    if (hits.length) {\n"
        "      var d = hits[0].object.userData;\n"
        "      hovLabel = d.label;\n"
        "      if (tt) {\n"
        "        tt.textContent = d.isCore\n"
        "          ? d.label + ' · Core Star'\n"
        "          : d.label + ' · ' + d.ring + ' · ' + d.status;\n"
        "        tt.style.left =\n"
        "          Math.min(cx+16, window.innerWidth-240)+'px';\n"
        "        tt.style.top  =\n"
        "          Math.max(cy-36, 62)+'px';\n"
        "        tt.style.opacity = '1';\n"
        "        tt.style.borderColor = d.isCore\n"
        "          ? 'rgba(232,201,106,0.38)'\n"
        "          : 'rgba(110,95,240,0.28)';\n"
        "        tt.style.color = d.isCore ? '#E8C96A' : '#8B8BAA';\n"
        "      }\n"
        "      cvs.style.cursor = 'pointer';\n"
        "    } else {\n"
        "      hovLabel = null;\n"
        "      if (tt) tt.style.opacity = '0';\n"
        "      cvs.style.cursor = isDragging?'grabbing':'grab';\n"
        "    }\n"
        "  }\n\n"

        "  /* ── Click → detail panel ── */\n"
        "  cvs.addEventListener('click', function(e){\n"
        "    if (didDrag) return;\n"
        "    var rect = cvs.getBoundingClientRect();\n"
        "    mouse2.x = ((e.clientX-rect.left)/rect.width)*2-1;\n"
        "    mouse2.y = -((e.clientY-rect.top)/rect.height)*2+1;\n"
        "    raycaster.setFromCamera(mouse2, camera);\n"
        "    var hits = raycaster.intersectObjects(hoverMeshes);\n"
        "    if (hits.length) {\n"
        "      openDetailPanel(hits[0].object.userData);\n"
        "      var tt = document.getElementById('epl-tt');\n"
        "      if (tt) tt.style.opacity = '0';\n"
        "    }\n"
        "  });\n"
        "  cvs.addEventListener('mouseleave', function(){\n"
        "    hovLabel = null;\n"
        "    var tt = document.getElementById('epl-tt');\n"
        "    if (tt) tt.style.opacity = '0';\n"
        "  });\n\n"

        "  /* ── Detail panel logic ── */\n"
        "  function openDetailPanel(d) {\n"
        "    var panel = document.getElementById('epl-panel');\n"
        "    if (!panel) return;\n"
        "    var titleEl  = document.getElementById('ep-title');\n"
        "    var ringEl   = document.getElementById('ep-ring');\n"
        "    var statusEl = document.getElementById('ep-status');\n"
        "    var descEl   = document.getElementById('ep-desc');\n"
        "    var badgeEl  = document.getElementById('ep-badge');\n"
        "    if (titleEl)  titleEl.textContent  = d.label  || '—';\n"
        "    if (ringEl)   ringEl.textContent   = d.ring   || '—';\n"
        "    if (statusEl) statusEl.textContent = d.status || '—';\n"
        "    if (descEl)   descEl.textContent   = d.desc   || '—';\n"
        "    if (badgeEl) {\n"
        "      badgeEl.textContent = d.status || '';\n"
        "      var colors = {\n"
        "        'Completed':  {bg:'rgba(139,127,232,0.12)',\n"
        "                       c:'#AFA9EC',\n"
        "                       b:'rgba(139,127,232,0.35)'},\n"
        "        'In Progress':{bg:'rgba(239,159,39,0.12)',\n"
        "                       c:'#FAC775',\n"
        "                       b:'rgba(239,159,39,0.35)'},\n"
        "        'Overdue':    {bg:'rgba(216,90,48,0.12)',\n"
        "                       c:'#F09595',\n"
        "                       b:'rgba(216,90,48,0.4)'},\n"
        "        'Pending':    {bg:'rgba(80,72,160,0.08)',\n"
        "                       c:'#5048A0',\n"
        "                       b:'rgba(80,72,160,0.25)'}\n"
        "      };\n"
        "      var sc = colors[d.status] || colors['Pending'];\n"
        "      badgeEl.style.background   = sc.bg;\n"
        "      badgeEl.style.color        = sc.c;\n"
        "      badgeEl.style.borderColor  = sc.b;\n"
        "    }\n"
        "    panel.classList.add('open');\n"
        "  }\n\n"

        "  var closeBtn = document.getElementById('ep-close');\n"
        "  if (closeBtn) {\n"
        "    closeBtn.addEventListener('click', function(){\n"
        "      var panel = document.getElementById('epl-panel');\n"
        "      if (panel) panel.classList.remove('open');\n"
        "    });\n"
        "  }\n\n");
}

static void write_animation_loop(FILE *js) {
    fprintf(js,
        "  /* ── Animation loop ── */\n"
        "  var frame = 0;\n\n"
        "  function animate() {\n"
        "    requestAnimationFrame(animate);\n"
        "    frame++;\n"
        "    var t = frame * 0.016;\n\n"

        "    /* Auto-rotate after idle */\n"
        "    if (!isDragging && Date.now()-lastInt > 4500)\n"
        "      autoRot = true;\n"
        "    if (autoRot) tgtSph.th += 0.00055;\n\n"

        "    /* Smooth camera */\n"
        "    sph.th += (tgtSph.th - sph.th) * 0.065;\n"
        "    sph.ph += (tgtSph.ph - sph.ph) * 0.065;\n"
        "    sph.r  += (tgtSph.r  - sph.r)  * 0.065;\n"
        "    updateCam();\n\n"

        "    /* Core star pulse */\n"
        "    if (coreMesh && coreGrp) {\n"
        "      var p = 1 + Math.sin(t*1.2)*0.07;\n"
        "      coreMesh.scale.setScalar(p);\n"
        "      if (cLight) cLight.intensity = 10+Math.sin(t*1.2)*3;\n"
        "      coreGrp.rotation.y += 0.0012;\n"
        "      armRings.forEach(function(ar){\n"
        "        ar.rotation.y += ar.userData.rspd;\n"
        "      });\n"
        "      /* Hover scale on core */\n"
        "      var coreHov = (hovLabel === coreMesh.userData.label);\n"
        "      var cc  = coreMesh.scale.x / p;\n"
        "      var cct = coreHov ? 1.22 : 1.0;\n"
        "      coreMesh.scale.setScalar((cc+(cct-cc)*0.10)*p);\n"
        "    }\n\n"

        "    /* Orbital nodes */\n"
        "    allRings.forEach(function(ring) {\n"
        "      ring.nodes.forEach(function(obj) {\n"
        "        obj.angle += ring.speed;\n"
        "        obj.ng.position.set(\n"
        "          Math.cos(obj.angle)*ring.radius, 0,\n"
        "          Math.sin(obj.angle)*ring.radius);\n\n"

        "        /* Ghost trail */\n"
        "        obj.ghosts.forEach(function(g) {\n"
        "          var ga = obj.angle - g.delta;\n"
        "          g.mesh.position.set(\n"
        "            Math.cos(ga)*ring.radius, 0,\n"
        "            Math.sin(ga)*ring.radius);\n"
        "        });\n\n"

        "        /* Pulse ring */\n"
        "        if (obj.node.userData.pulseRing) {\n"
        "          var pr = obj.node.userData.pulseRing;\n"
        "          var ps = 1+0.18*Math.sin(t*2.2+obj.angle*2);\n"
        "          pr.scale.setScalar(ps);\n"
        "          pr.material.opacity =\n"
        "            0.3+0.35*Math.sin(t*2.2+obj.angle*2);\n"
        "          pr.rotation.y += 0.025;\n"
        "        }\n\n"

        "        /* Warn ring */\n"
        "        if (obj.node.userData.warnRing) {\n"
        "          var wr = obj.node.userData.warnRing;\n"
        "          wr.rotation.y += 0.04;\n"
        "          wr.material.opacity =\n"
        "            0.25+0.25*Math.abs(Math.sin(t*3+obj.angle));\n"
        "        }\n\n"

        "        /* Hover scale */\n"
        "        var isHov=(hovLabel===obj.node.userData.label);\n"
        "        var cur = obj.node.scale.x;\n"
        "        var ts  = isHov ? 1.58 : 1.0;\n"
        "        obj.node.scale.setScalar(cur+(ts-cur)*0.12);\n"
        "      });\n"
        "    });\n\n"

        "    renderer.render(scene, camera);\n"
        "  }\n\n"
        "  animate();\n");
}

static void write_legend(FILE *js) {
    fprintf(js,
        "  /* ── Legend ── */\n"
        "  (function(){\n"
        "    var legend = document.getElementById('epl-legend');\n"
        "    if (!legend) return;\n"
        "    var items = [\n"
        "      {c:'#8B7FE8', l:'Completed'},\n"
        "      {c:'#EF9F27', l:'In Progress'},\n"
        "      {c:'#D85A30', l:'Overdue'},\n"
        "      {c:'#1a1840', l:'Pending',\n"
        "       b:'0.5px solid #4a45a0'}\n"
        "    ];\n"
        "    items.forEach(function(it){\n"
        "      var d = document.createElement('div');\n"
        "      d.className = 'leg-item';\n"
        "      var dot = document.createElement('div');\n"
        "      dot.className = 'leg-dot';\n"
        "      dot.style.background = it.c;\n"
        "      if (it.b) dot.style.border = it.b;\n"
        "      var txt = document.createElement('span');\n"
        "      txt.className = 'leg-txt';\n"
        "      txt.textContent = it.l;\n"
        "      d.appendChild(dot);\n"
        "      d.appendChild(txt);\n"
        "      legend.appendChild(d);\n"
        "    });\n"
        "  })();\n\n");
}

static void write_overlay_html(FILE *html,
                                ASTNode *page_node) {
    fprintf(html,
        "  <div id='epl-tt'></div>\n\n"

        "  <!-- Detail panel -->\n"
        "  <div id='epl-panel'>\n"
        "    <div class='ep-head'>\n"
        "      <div class='ep-title' id='ep-title'>—</div>\n"
        "      <div class='ep-close' id='ep-close'>×</div>\n"
        "    </div>\n"
        "    <div class='ep-body'>\n"
        "      <div id='ep-badge' class='ep-badge'></div>\n"
        "      <div class='ep-div'></div>\n"
        "      <div class='ep-row'>\n"
        "        <span class='ep-key'>Ring</span>\n"
        "        <span class='ep-val' id='ep-ring'>—</span>\n"
        "      </div>\n"
        "      <div class='ep-row'>\n"
        "        <span class='ep-key'>Status</span>\n"
        "        <span class='ep-val' id='ep-status'>—</span>\n"
        "      </div>\n"
        "      <div class='ep-div'></div>\n"
        "      <div class='ep-desc' id='ep-desc'>—</div>\n"
        "    </div>\n"
        "  </div>\n\n");

    if (!page_node) return;
    NodeList *children = &page_node->as.make_page.children;
    for (int i = 0; i < children->count; i++) {
        ASTNode *child = children->items[i];
        if (!child) continue;

        if (child->type == NODE_PANEL3D) {
            const char *pc = "ep-panel-right";
            if (child->as.panel3d.position == PANEL_TOP)    pc="ep-panel-top";
            if (child->as.panel3d.position == PANEL_LEFT)   pc="ep-panel-left";
            if (child->as.panel3d.position == PANEL_BOTTOM) pc="ep-panel-bottom";
            fprintf(html,
                "  <div class='epl-overlay-panel %s'>\n", pc);
            NodeList *pc_list = &child->as.panel3d.children;
            for (int j = 0; j < pc_list->count; j++) {
                ASTNode *pc_child = pc_list->items[j];
                if (!pc_child) continue;
                if (pc_child->type == NODE_HEADING_OUT &&
                    pc_child->as.text_out.value &&
                    pc_child->as.text_out.value->type == NODE_STRING) {
                    fprintf(html,
                        "    <h1 class='ep-panel-h1'>%s</h1>\n",
                        pc_child->as.text_out.value->as.string.value);
                }
                if (pc_child->type == NODE_SUBHEADING &&
                    pc_child->as.text_out.value &&
                    pc_child->as.text_out.value->type == NODE_STRING) {
                    fprintf(html,
                        "    <h2 class='ep-panel-h2'>%s</h2>\n",
                        pc_child->as.text_out.value->as.string.value);
                }
                if (pc_child->type == NODE_MAKE_STAT &&
                    pc_child->as.make_stat.value &&
                    pc_child->as.make_stat.value->type == NODE_NUMBER) {
                    fprintf(html,
                        "    <div class='ep-stat'>\n"
                        "      <div class='ep-stat-lbl'>%s</div>\n"
                        "      <div class='ep-stat-val'>%.4g</div>\n"
                        "    </div>\n",
                        pc_child->as.make_stat.label,
                        pc_child->as.make_stat.value->as.number.value);
                }
            }
            fprintf(html, "  </div>\n\n");
        }

        if (child->type == NODE_DRAWER3D) {
            const char *side =
                (child->as.drawer3d.position == PANEL_LEFT)
                ? "left" : "right";
            fprintf(html,
                "  <div class='epl-drawer epl-drawer-%s'>\n"
                "    <div class='ep-drawer-head'>\n"
                "      <span>%s</span>\n"
                "      <div class='ep-drawer-close'\n"
                "        onclick=\"this.closest('.epl-drawer')"
                ".classList.remove('open')\">×</div>\n"
                "    </div>\n"
                "    <div class='ep-drawer-body'></div>\n"
                "  </div>\n\n",
                side,
                child->as.drawer3d.title);
        }
    }
}

static void write_css(FILE *css, SceneCtx *ctx) {
    fprintf(css,
        "/* EPL v1.6 Generated Styles */\n"
        "*,*::before,*::after{\n"
        "  margin:0; padding:0; box-sizing:border-box;\n"
        "}\n"
        "html,body{\n"
        "  width:100%%; height:100%%; overflow:hidden;\n"
        "  font-family:'Rajdhani',sans-serif;\n"
        "  color:%s;\n"
        "}\n"
        "body { background:%s; }\n"
        "#epl-root{\n"
        "  position:relative; width:100%%; height:100vh;\n"
        "}\n"
        "#epl-canvas{\n"
        "  display:block; width:100%%; height:100%%;\n"
        "  cursor:grab;\n"
        "}\n"
        "#epl-canvas:active { cursor:grabbing; }\n\n",
        ctx->theme_text, ctx->theme_bg);

    fprintf(css,
        "#epl-tt {\n"
        "  position:absolute; z-index:20;\n"
        "  background:rgba(1,1,14,0.94);\n"
        "  border:0.5px solid rgba(110,95,240,0.28);\n"
        "  border-radius:7px; padding:7px 13px;\n"
        "  pointer-events:none; opacity:0;\n"
        "  transition:opacity .12s;\n"
        "  font-family:'Share Tech Mono',monospace;\n"
        "  font-size:9px; color:#8B8BAA;\n"
        "  letter-spacing:0.06em; white-space:nowrap;\n"
        "  box-shadow:0 2px 20px rgba(80,60,200,0.12);\n"
        "}\n\n");

    fprintf(css,
        "#epl-panel {\n"
        "  position:absolute; top:70px; right:-300px;\n"
        "  z-index:15; width:268px;\n"
        "  background:rgba(2,1,20,0.97);\n"
        "  border:0.5px solid rgba(110,95,240,0.25);\n"
        "  border-radius:14px; overflow:hidden;\n"
        "  transition:right .36s cubic-bezier(.22,.68,0,1.18);\n"
        "  box-shadow:-6px 0 50px rgba(80,60,200,0.10);\n"
        "}\n"
        "#epl-panel.open { right:16px; }\n"
        ".ep-head {\n"
        "  padding:15px 17px 13px;\n"
        "  border-bottom:0.5px solid rgba(110,95,240,0.08);\n"
        "  display:flex; justify-content:space-between;\n"
        "  align-items:flex-start; gap:8px;\n"
        "}\n"
        ".ep-title {\n"
        "  font-size:14px; font-weight:500;\n"
        "  color:#E8E4FF; letter-spacing:0.04em;\n"
        "  line-height:1.35; flex:1;\n"
        "}\n"
        ".ep-close {\n"
        "  width:22px; height:22px; border-radius:5px;\n"
        "  cursor:pointer;\n"
        "  background:rgba(110,95,240,0.07);\n"
        "  border:0.5px solid rgba(110,95,240,0.18);\n"
        "  display:grid; place-items:center;\n"
        "  font-size:13px; color:#5048A0;\n"
        "  transition:background .15s, color .15s;\n"
        "  font-family:'Share Tech Mono',monospace;\n"
        "}\n"
        ".ep-close:hover {\n"
        "  background:rgba(110,95,240,0.18); color:#E8E4FF;\n"
        "}\n"
        ".ep-body {\n"
        "  padding:13px 17px 17px;\n"
        "  display:flex; flex-direction:column; gap:10px;\n"
        "}\n"
        ".ep-badge {\n"
        "  display:inline-flex; align-items:center;\n"
        "  font-family:'Share Tech Mono',monospace;\n"
        "  font-size:7.5px; letter-spacing:0.1em;\n"
        "  padding:3px 9px; border-radius:20px;\n"
        "  width:fit-content; border:0.5px solid transparent;\n"
        "}\n"
        ".ep-row {\n"
        "  display:flex; justify-content:space-between;\n"
        "  align-items:flex-start; gap:8px;\n"
        "}\n"
        ".ep-key {\n"
        "  font-family:'Share Tech Mono',monospace;\n"
        "  font-size:7.5px; color:#2e2c50;\n"
        "  letter-spacing:0.12em; text-transform:uppercase;\n"
        "  padding-top:1px; flex-shrink:0;\n"
        "}\n"
        ".ep-val {\n"
        "  font-size:10.5px; color:#7874A0; text-align:right;\n"
        "  line-height:1.4;\n"
        "}\n"
        ".ep-div {\n"
        "  height:0.5px;\n"
        "  background:rgba(110,95,240,0.07);\n"
        "}\n"
        ".ep-desc {\n"
        "  font-size:10px; color:#4A4870;\n"
        "  line-height:1.65; font-style:italic;\n"
        "}\n\n");

    fprintf(css,
        ".epl-overlay-panel {\n"
        "  position:absolute; z-index:10;\n"
        "  background:rgba(2,1,20,0.88);\n"
        "  border:0.5px solid rgba(110,95,240,0.2);\n"
        "  border-radius:12px; padding:16px 18px;\n"
        "  backdrop-filter:blur(8px);\n"
        "  min-width:180px;\n"
        "}\n"
        ".ep-panel-top {\n"
        "  top:16px; left:50%%;\n"
        "  transform:translateX(-50%%);\n"
        "}\n"
        ".ep-panel-right {\n"
        "  top:16px; right:16px; width:220px;\n"
        "}\n"
        ".ep-panel-left {\n"
        "  top:16px; left:16px; width:220px;\n"
        "}\n"
        ".ep-panel-bottom {\n"
        "  bottom:48px; left:50%%;\n"
        "  transform:translateX(-50%%);\n"
        "}\n"
        ".ep-panel-h1 {\n"
        "  font-size:15px; font-weight:500;\n"
        "  letter-spacing:0.14em; color:#E8E4FF;\n"
        "  text-transform:uppercase; margin-bottom:2px;\n"
        "}\n"
        ".ep-panel-h2 {\n"
        "  font-family:'Share Tech Mono',monospace;\n"
        "  font-size:7.5px; color:#38345a;\n"
        "  letter-spacing:0.2em; text-transform:uppercase;\n"
        "  margin-bottom:12px;\n"
        "}\n"
        ".ep-stat {\n"
        "  background:rgba(110,95,240,0.06);\n"
        "  border:0.5px solid rgba(110,95,240,0.15);\n"
        "  border-radius:10px; padding:10px 14px;\n"
        "  margin-bottom:8px;\n"
        "}\n"
        ".ep-stat-lbl {\n"
        "  font-family:'Share Tech Mono',monospace;\n"
        "  font-size:7px; color:#38345a;\n"
        "  letter-spacing:0.1em; text-transform:uppercase;\n"
        "  margin-bottom:3px;\n"
        "}\n"
        ".ep-stat-val {\n"
        "  font-size:22px; font-weight:600; color:%s;\n"
        "}\n\n",
        ctx->theme_accent);

    fprintf(css,
        ".epl-drawer {\n"
        "  position:absolute; top:0; bottom:0; z-index:25;\n"
        "  width:320px; background:rgba(2,1,20,0.99);\n"
        "  display:flex; flex-direction:column;\n"
        "  transition:transform .38s cubic-bezier(.22,.68,0,1.18);\n"
        "  box-shadow:0 0 60px rgba(60,40,180,0.14);\n"
        "}\n"
        ".epl-drawer-left  { left:0;  transform:translateX(-100%%); }\n"
        ".epl-drawer-right { right:0; transform:translateX(100%%);  }\n"
        ".epl-drawer.open  { transform:translateX(0); }\n"
        ".ep-drawer-head {\n"
        "  padding:18px 20px 14px;\n"
        "  border-bottom:0.5px solid rgba(110,95,240,0.1);\n"
        "  display:flex; justify-content:space-between;\n"
        "  align-items:center; font-size:14px; font-weight:500;\n"
        "  color:#E8E4FF; letter-spacing:0.1em;\n"
        "  text-transform:uppercase; flex-shrink:0;\n"
        "}\n"
        ".ep-drawer-close {\n"
        "  width:26px; height:26px; border-radius:6px;\n"
        "  cursor:pointer;\n"
        "  background:rgba(110,95,240,0.08);\n"
        "  border:0.5px solid rgba(110,95,240,0.2);\n"
        "  display:grid; place-items:center;\n"
        "  font-size:14px; color:#5048A0;\n"
        "  transition:background .15s, color .15s;\n"
        "}\n"
        ".ep-drawer-close:hover {\n"
        "  background:rgba(110,95,240,0.2); color:#E8E4FF;\n"
        "}\n"
        ".ep-drawer-body {\n"
        "  flex:1; overflow-y:auto; padding:8px 12px 16px;\n"
        "}\n\n");

    fprintf(css,
        "#epl-legend {\n"
        "  position:absolute; bottom:0; left:0; right:0;\n"
        "  z-index:10; padding:9px 22px;\n"
        "  display:flex; gap:14px;\n"
        "  align-items:center; flex-wrap:wrap;\n"
        "  background:linear-gradient(\n"
        "    0deg, rgba(1,1,16,0.88) 50%%, transparent 100%%);\n"
        "  pointer-events:none;\n"
        "}\n"
        ".leg-item {\n"
        "  display:flex; align-items:center; gap:5px;\n"
        "}\n"
        ".leg-dot {\n"
        "  width:6px; height:6px; border-radius:50%%; flex-shrink:0;\n"
        "}\n"
        ".leg-txt {\n"
        "  font-family:'Share Tech Mono',monospace;\n"
        "  font-size:7.5px; color:#38345a; letter-spacing:0.08em;\n"
        "}\n");
}

static void write_shell_html(SceneCtx *ctx,
                              ASTNode *page_node,
                              const char *title) {
    FILE *html = ctx->html;

    fprintf(html,
        "<!DOCTYPE html>\n"
        "<html lang=\"en\">\n"
        "<head>\n"
        "  <meta charset=\"UTF-8\">\n"
        "  <meta name=\"viewport\"\n"
        "    content=\"width=device-width,initial-scale=1.0\">\n"
        "  <title>%s</title>\n"
        "  <link href=\"https://fonts.googleapis.com/css2?"
        "family=Rajdhani:wght@300;400;500;600"
        "&family=Share+Tech+Mono&display=swap\"\n"
        "    rel=\"stylesheet\">\n",
        title);

    if (ctx->offline) {
        fprintf(html, "  <style>\n");
        rewind(ctx->css);
        char buf[512];
        while (fgets(buf, sizeof(buf), ctx->css))
            fprintf(html, "%s", buf);
        fprintf(html, "  </style>\n");
    } else {
        fprintf(html,
            "  <link rel=\"stylesheet\" href=\"%s.style.css\">\n",
            ctx->base);
    }

    fprintf(html,
        "</head>\n"
        "<body>\n"
        "<div id=\"epl-root\">\n"
        "  <canvas id=\"epl-canvas\"></canvas>\n\n");

    write_overlay_html(html, page_node);

    fprintf(html,
        "  <div id=\"epl-legend\"></div>\n"
        "</div>\n\n"
        "<!-- Three.js r128 -->\n"
        "<script src=\"https://cdnjs.cloudflare.com/ajax/libs/"
        "three.js/r128/three.min.js\"></script>\n");

    if (ctx->offline) {
        fprintf(html, "<script>\n");
        rewind(ctx->js);
        char buf[512];
        while (fgets(buf, sizeof(buf), ctx->js))
            fprintf(html, "%s", buf);
        fprintf(html, "</script>\n");
    } else {
        fprintf(html,
            "<script src=\"%s.data.js\"></script>\n"
            "<script src=\"%s.scene.js\"></script>\n",
            ctx->base, ctx->base);
    }

    fprintf(html, "</body>\n</html>\n");
}

static void write_full_scene_js(SceneCtx *ctx,
                                 ASTNode *canvas) {
    FILE *js = ctx->js;

    fprintf(js,
        "/* EPL v1.6 — Generated scene: %s */\n"
        "(function(){\n"
        "'use strict';\n\n"
        "  /* ── Renderer ── */\n"
        "  var W = window.innerWidth;\n"
        "  var H = window.innerHeight;\n"
        "  var renderer = new THREE.WebGLRenderer({\n"
        "    canvas:    document.getElementById('epl-canvas'),\n"
        "    antialias: true,\n"
        "    alpha:     true\n"
        "  });\n"
        "  renderer.setPixelRatio(Math.min(window.devicePixelRatio,2));\n"
        "  renderer.setSize(W, H);\n"
        "  renderer.setClearColor(0x000000, 0);\n\n"
        "  /* ── Scene + Camera ── */\n"
        "  var scene  = new THREE.Scene();\n"
        "  var camera = new THREE.PerspectiveCamera(50,W/H,0.5,4000);\n\n"
        "  window.addEventListener('resize', function(){\n"
        "    W = window.innerWidth; H = window.innerHeight;\n"
        "    renderer.setSize(W, H);\n"
        "    camera.aspect = W/H;\n"
        "    camera.updateProjectionMatrix();\n"
        "  });\n\n"
        "  /* ── Shared state ── */\n"
        "  var hoverMeshes = [];\n"
        "  var allRings    = [];\n"
        "  var coreMesh    = null;\n"
        "  var coreGrp     = null;\n"
        "  var armRings    = [];\n"
        "  var cLight      = null;\n\n",
        canvas->as.canvas3d.name);

    write_background(js, canvas->as.canvas3d.background,
                     canvas->as.canvas3d.bg_color);
    write_stars(js, canvas->as.canvas3d.background);
    write_lighting(js);
    write_camera_and_interaction(js);

    int ring_idx = 0;
    for (int i = 0; i < canvas->as.canvas3d.children.count; i++) {
        ASTNode *child = canvas->as.canvas3d.children.items[i];
        if (!child) continue;
        if (child->type == NODE_ORBITAL_RING)
            write_orbital_ring(js, child, ring_idx++);
        if (child->type == NODE_CORE_STAR)
            write_core_star(js, child);
    }

    write_legend(js);
    write_animation_loop(js);

    fprintf(js, "})();\n");
}

void scene_gen_page(Interpreter *interp, Env *env,
                    ASTNode *page_node) {
    ASTNode  *canvas   = NULL;
    NodeList *children = &page_node->as.make_page.children;
    for (int i = 0; i < children->count; i++) {
        if (children->items[i] &&
            children->items[i]->type == NODE_CANVAS3D) {
            canvas = children->items[i];
            break;
        }
    }
    if (!canvas) return;

    char base[512];
    strncpy(base, page_node->as.make_page.filename,
            sizeof(base)-1);
    char *dot = strrchr(base, '.');
    if (dot) *dot = '\0';

    char path[600];

    snprintf(path, sizeof(path), "%s.scene.js",  base);
    FILE *js   = fopen(path, "w");

    snprintf(path, sizeof(path), "%s.data.js",   base);
    FILE *data = fopen(path, "w");

    snprintf(path, sizeof(path), "%s.style.css", base);
    FILE *css  = fopen(path, "w");

    FILE *html = fopen(page_node->as.make_page.filename, "w");

    if (!js || !data || !css || !html) {
        fprintf(stderr,
            "[EPL Error] Cannot open output files for %s\n",
            page_node->as.make_page.filename);
        if (js)   fclose(js);
        if (data) fclose(data);
        if (css)  fclose(css);
        if (html) fclose(html);
        return;
    }

    ThemeCSS theme = codegen_get_theme(
        page_node->as.make_page.theme,
        page_node->as.make_page.color);

    SceneCtx ctx;
    memset(&ctx, 0, sizeof(ctx));
    ctx.js      = js;
    ctx.data    = data;
    ctx.css     = css;
    ctx.html    = html;
    ctx.interp  = interp;
    ctx.env     = env;
    ctx.offline = canvas->as.canvas3d.offline;
    strncpy(ctx.base,          base,           sizeof(ctx.base)-1);
    strncpy(ctx.theme_bg,      theme.bg,       sizeof(ctx.theme_bg)-1);
    strncpy(ctx.theme_surface, theme.surface,  sizeof(ctx.theme_surface)-1);
    strncpy(ctx.theme_accent,  theme.accent,   sizeof(ctx.theme_accent)-1);
    strncpy(ctx.theme_text,    theme.text,     sizeof(ctx.theme_text)-1);
    strncpy(ctx.theme_border,  theme.border,   sizeof(ctx.theme_border)-1);

    const char *title = canvas->as.canvas3d.name;
    if (!title || strlen(title) == 0)
        title = "EPL Scene";

    write_css(css, &ctx);
    write_full_scene_js(&ctx, canvas);

    fprintf(data,
        "/* EPL v1.6 Data — %s */\n"
        "/* Extend this file with dynamic data from .EP */\n",
        base);

    write_shell_html(&ctx, page_node, title);

    fclose(js);
    fclose(data);
    fclose(css);
    fclose(html);

    binding_update(interp->source_file, page_node->as.make_page.filename, "html-3d");

    snprintf(path, sizeof(path), "%s.scene.js",  base);
    binding_update(interp->source_file, path, "js-scene");

    snprintf(path, sizeof(path), "%s.style.css", base);
    binding_update(interp->source_file, path, "css");

    snprintf(path, sizeof(path), "%s.data.js",   base);
    binding_update(interp->source_file, path, "js-data");

    printf("[EPL] Generated: %s\n",
           page_node->as.make_page.filename);
    printf("[EPL]            %s.scene.js\n",  base);
    printf("[EPL]            %s.style.css\n", base);
    printf("[EPL]            %s.data.js\n",   base);
}
