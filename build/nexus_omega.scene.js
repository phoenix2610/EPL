/* EPL v1.6 — Generated scene: NEXUS OMEGA */
(function(){
'use strict';

  /* ── Renderer ── */
  var W = window.innerWidth;
  var H = window.innerHeight;
  var renderer = new THREE.WebGLRenderer({
    canvas:    document.getElementById('epl-canvas'),
    antialias: true,
    alpha:     true
  });
  renderer.setPixelRatio(Math.min(window.devicePixelRatio,2));
  renderer.setSize(W, H);
  renderer.setClearColor(0x000000, 0);

  /* ── Scene + Camera ── */
  var scene  = new THREE.Scene();
  var camera = new THREE.PerspectiveCamera(50,W/H,0.5,4000);

  window.addEventListener('resize', function(){
    W = window.innerWidth; H = window.innerHeight;
    renderer.setSize(W, H);
    camera.aspect = W/H;
    camera.updateProjectionMatrix();
  });

  /* ── Shared state ── */
  var hoverMeshes = [];
  var allRings    = [];
  var coreMesh    = null;
  var coreGrp     = null;
  var armRings    = [];
  var cLight      = null;

  scene.fog = new THREE.FogExp2(0x010110, 0.00038);
  scene.background = new THREE.Color(0x010110);

  /* ── Star field ── */
  (function(){
    var n=2200;
    var pos=new Float32Array(n*3);
    var col=new Float32Array(n*3);
    for(var i=0;i<n;i++){
      var r=700+Math.random()*1400;
      var th=Math.random()*Math.PI*2;
      var ph=Math.acos(1-Math.random()*2);
      pos[i*3]  =r*Math.sin(ph)*Math.cos(th);
      pos[i*3+1]=r*Math.cos(ph);
      pos[i*3+2]=r*Math.sin(ph)*Math.sin(th);
      var h=Math.random();
      col[i*3]=0.5+h*0.5;
      col[i*3+1]=0.5+(1-h)*0.3;
      col[i*3+2]=0.8+Math.random()*0.2;
    }
    var g=new THREE.BufferGeometry();
    g.setAttribute('position',
      new THREE.BufferAttribute(pos,3));
    g.setAttribute('color',
      new THREE.BufferAttribute(col,3));
    scene.add(new THREE.Points(g,
      new THREE.PointsMaterial({
        size:0.95,transparent:true,
        opacity:0.36,vertexColors:true
      })));
  })();

  /* ── Nebula clouds ── */
  (function(){
    function neb(c,col,mn,mx,op,fl){
      var pos=new Float32Array(c*3);
      for(var i=0;i<c;i++){
        var r=mn+Math.random()*(mx-mn);
        var th=Math.random()*Math.PI*2;
        var ph=Math.acos(1-Math.random()*2);
        pos[i*3]  =r*Math.sin(ph)*Math.cos(th);
        pos[i*3+1]=r*Math.cos(ph)*(fl||1);
        pos[i*3+2]=r*Math.sin(ph)*Math.sin(th);
      }
      var g=new THREE.BufferGeometry();
      g.setAttribute('position',
        new THREE.BufferAttribute(pos,3));
      return new THREE.Points(g,
        new THREE.PointsMaterial({
          color:col,size:2.8,transparent:true,
          opacity:op,depthWrite:false,
          blending:THREE.AdditiveBlending
        }));
    }
    scene.add(neb(600,0x2518a0,250,600,0.025,0.5));
    scene.add(neb(450,0x0a2a60,300,700,0.020,0.6));
    scene.add(neb(350,0x380828,150,400,0.032,0.4));
  })();

  /* ── Lighting ── */
  scene.add(new THREE.AmbientLight(0x0d0824, 6));
  cLight = new THREE.PointLight(0xC8A840, 12, 500);
  scene.add(cLight);
  var fA = new THREE.PointLight(0x1a3a8a, 4, 800);
  fA.position.set(320, 200, 100);
  scene.add(fA);
  var fB = new THREE.PointLight(0x3010a0, 3, 700);
  fB.position.set(-200, -180, 220);
  scene.add(fB);

  /* ── Camera setup ── */
  var sph    = {th:0.65, ph:1.1, r:500};
  var tgtSph = {th:0.65, ph:1.1, r:500};
  var isDragging = false;
  var didDrag    = false;
  var px = 0, py = 0;
  var lastInt  = Date.now();
  var autoRot  = true;
  var hovLabel = null;

  function updateCam() {
    camera.position.set(
      sph.r * Math.sin(sph.ph) * Math.sin(sph.th),
      sph.r * Math.cos(sph.ph),
      sph.r * Math.sin(sph.ph) * Math.cos(sph.th));
    camera.lookAt(0, 0, 0);
  }
  updateCam();

  /* ── Mouse drag ── */
  var cvs = document.getElementById('epl-canvas');
  cvs.addEventListener('mousedown', function(e){
    isDragging = true; didDrag = false;
    px = e.clientX; py = e.clientY;
    autoRot = false; lastInt = Date.now();
  });
  window.addEventListener('mouseup', function(){
    isDragging = false;
  });
  window.addEventListener('mousemove', function(e){
    if (isDragging) {
      var dx = e.clientX-px, dy = e.clientY-py;
      if (Math.abs(dx)>2||Math.abs(dy)>2) didDrag = true;
      tgtSph.th -= dx * 0.0042;
      tgtSph.ph  = Math.max(0.08,
        Math.min(2.96, tgtSph.ph - dy*0.0042));
      px = e.clientX; py = e.clientY;
      lastInt = Date.now();
      var tt = document.getElementById('epl-tt');
      if (tt) tt.style.opacity = '0';
    } else {
      doRaycast(e.clientX, e.clientY);
    }
  });

  /* ── Scroll zoom ── */
  cvs.addEventListener('wheel', function(e){
    tgtSph.r = Math.max(150,
      Math.min(1200, tgtSph.r + e.deltaY*0.3));
    autoRot = false; lastInt = Date.now();
    e.preventDefault();
  }, {passive:false});

  /* ── Touch support ── */
  var lt = null;
  cvs.addEventListener('touchstart', function(e){
    if (e.touches.length===1) {
      isDragging = true; didDrag = false;
      lt = {x:e.touches[0].clientX, y:e.touches[0].clientY};
    }
    autoRot = false; lastInt = Date.now();
  });
  cvs.addEventListener('touchend', function(){
    isDragging = false; lt = null;
  });
  cvs.addEventListener('touchmove', function(e){
    if (!lt || e.touches.length!==1) return;
    var dx=e.touches[0].clientX-lt.x;
    var dy=e.touches[0].clientY-lt.y;
    if (Math.abs(dx)>2||Math.abs(dy)>2) didDrag=true;
    tgtSph.th -= dx*0.0042;
    tgtSph.ph = Math.max(0.08,
      Math.min(2.96, tgtSph.ph-dy*0.0042));
    lt = {x:e.touches[0].clientX, y:e.touches[0].clientY};
    lastInt = Date.now();
    e.preventDefault();
  }, {passive:false});

  /* ── Raycaster ── */
  var raycaster = new THREE.Raycaster();
  var mouse2    = new THREE.Vector2();

  function doRaycast(cx, cy) {
    var rect = cvs.getBoundingClientRect();
    mouse2.x = ((cx-rect.left)/rect.width)*2-1;
    mouse2.y = -((cy-rect.top)/rect.height)*2+1;
    raycaster.setFromCamera(mouse2, camera);
    var hits = raycaster.intersectObjects(hoverMeshes);
    var tt   = document.getElementById('epl-tt');
    if (hits.length) {
      var d = hits[0].object.userData;
      hovLabel = d.label;
      if (tt) {
        tt.textContent = d.isCore
          ? d.label + ' · Core Star'
          : d.label + ' · ' + d.ring + ' · ' + d.status;
        tt.style.left =
          Math.min(cx+16, window.innerWidth-240)+'px';
        tt.style.top  =
          Math.max(cy-36, 62)+'px';
        tt.style.opacity = '1';
        tt.style.borderColor = d.isCore
          ? 'rgba(232,201,106,0.38)'
          : 'rgba(110,95,240,0.28)';
        tt.style.color = d.isCore ? '#E8C96A' : '#8B8BAA';
      }
      cvs.style.cursor = 'pointer';
    } else {
      hovLabel = null;
      if (tt) tt.style.opacity = '0';
      cvs.style.cursor = isDragging?'grabbing':'grab';
    }
  }

  /* ── Click → detail panel ── */
  cvs.addEventListener('click', function(e){
    if (didDrag) return;
    var rect = cvs.getBoundingClientRect();
    mouse2.x = ((e.clientX-rect.left)/rect.width)*2-1;
    mouse2.y = -((e.clientY-rect.top)/rect.height)*2+1;
    raycaster.setFromCamera(mouse2, camera);
    var hits = raycaster.intersectObjects(hoverMeshes);
    if (hits.length) {
      openDetailPanel(hits[0].object.userData);
      var tt = document.getElementById('epl-tt');
      if (tt) tt.style.opacity = '0';
    }
  });
  cvs.addEventListener('mouseleave', function(){
    hovLabel = null;
    var tt = document.getElementById('epl-tt');
    if (tt) tt.style.opacity = '0';
  });

  /* ── Detail panel logic ── */
  function openDetailPanel(d) {
    var panel = document.getElementById('epl-panel');
    if (!panel) return;
    var titleEl  = document.getElementById('ep-title');
    var ringEl   = document.getElementById('ep-ring');
    var statusEl = document.getElementById('ep-status');
    var descEl   = document.getElementById('ep-desc');
    var badgeEl  = document.getElementById('ep-badge');
    if (titleEl)  titleEl.textContent  = d.label  || '—';
    if (ringEl)   ringEl.textContent   = d.ring   || '—';
    if (statusEl) statusEl.textContent = d.status || '—';
    if (descEl)   descEl.textContent   = d.desc   || '—';
    if (badgeEl) {
      badgeEl.textContent = d.status || '';
      var colors = {
        'Completed':  {bg:'rgba(139,127,232,0.12)',
                       c:'#AFA9EC',
                       b:'rgba(139,127,232,0.35)'},
        'In Progress':{bg:'rgba(239,159,39,0.12)',
                       c:'#FAC775',
                       b:'rgba(239,159,39,0.35)'},
        'Overdue':    {bg:'rgba(216,90,48,0.12)',
                       c:'#F09595',
                       b:'rgba(216,90,48,0.4)'},
        'Pending':    {bg:'rgba(80,72,160,0.08)',
                       c:'#5048A0',
                       b:'rgba(80,72,160,0.25)'}
      };
      var sc = colors[d.status] || colors['Pending'];
      badgeEl.style.background   = sc.bg;
      badgeEl.style.color        = sc.c;
      badgeEl.style.borderColor  = sc.b;
    }
    panel.classList.add('open');
  }

  var closeBtn = document.getElementById('ep-close');
  if (closeBtn) {
    closeBtn.addEventListener('click', function(){
      var panel = document.getElementById('epl-panel');
      if (panel) panel.classList.remove('open');
    });
  }

  /* ── Core Star ── */
  coreGrp = new THREE.Group();
  scene.add(coreGrp);
  coreMesh = new THREE.Mesh(
    new THREE.SphereGeometry(16, 52, 52),
    new THREE.MeshStandardMaterial({
      color:'#E8C050',
      emissive:'#B08000',
      emissiveIntensity:2.8,
      roughness:0.06,
      metalness:0.98
    }));
  coreMesh.userData = {
    isCore:true,
    label:'CMD NEXUS OMEGA',
    ring:'Core',
    status:'Active',
    desc:'Central project node'
  };
  hoverMeshes.push(coreMesh);
  coreGrp.add(coreMesh);

  /* Core glow shells */
  [  [24,'#E8C050',0.22],
     [36,'#E8C050',0.11],
     [52,'#E8C050',0.055],
     [72,'#E8C050',0.024],
     [100,'#E8C050',0.009]
  ].forEach(function(d){
    coreGrp.add(new THREE.Mesh(
      new THREE.SphereGeometry(d[0],18,18),
      new THREE.MeshBasicMaterial({
        color:d[1],transparent:true,opacity:d[2],
        depthWrite:false,
        blending:THREE.AdditiveBlending
      })));
  });

  /* Armillary rings */
  armRings = [];
  [
    {r:28,rx:0,           rz:0,           rs:0.004, c:'#FFD060'},
    {r:28,rx:Math.PI/2,   rz:0,           rs:-0.003,c:'#FF9820'},
    {r:28,rx:Math.PI/4,   rz:Math.PI/4,   rs:0.0022,c:'#E8C840'}
  ].forEach(function(d){
    var m = new THREE.Mesh(
      new THREE.TorusGeometry(d.r, 0.7, 8, 84),
      new THREE.MeshBasicMaterial({
        color:d.c, transparent:true, opacity:0.55,
        blending:THREE.AdditiveBlending
      }));
    m.rotation.x = d.rx;
    m.rotation.z = d.rz;
    m.userData   = {rspd: d.rs};
    coreGrp.add(m);
    armRings.push(m);
  });

  /* ── Orbital Ring 0 (r=52) ── */
  (function(){
    var grp = new THREE.Group();
    grp.rotation.x = 0.0000;
    grp.rotation.z = 0.0000;

    /* Ring torus */
    grp.add(new THREE.Mesh(
      new THREE.TorusGeometry(52.0, 0.55, 8, 140),
      new THREE.MeshBasicMaterial({
        color:'#8B7FE8', transparent:true,
        opacity:0.15, depthWrite:false
      })));

    /* Glow torus */
    grp.add(new THREE.Mesh(
      new THREE.TorusGeometry(52.0, 4.2, 8, 140),
      new THREE.MeshBasicMaterial({
        color:'#8B7FE8', transparent:true,
        opacity:0.026, depthWrite:false,
        blending:THREE.AdditiveBlending
      })));

    /* Completion arc 80% */
    (function(){
      var ae = 0.800000 * Math.PI * 2;
      var segs = Math.max(16,
        Math.floor(ae/(Math.PI*2)*160));
      var pts = [];
      for (var i=0; i<=segs; i++) {
        var a = ae * (i/segs);
        pts.push(new THREE.Vector3(
          Math.cos(a)*52.0, 0,
          Math.sin(a)*52.0));
      }
      var curve = new THREE.CatmullRomCurve3(pts, false);
      [[1.8,0.82],[6.5,0.10],[13,0.032]]
      .forEach(function(d){
        grp.add(new THREE.Mesh(
          new THREE.TubeGeometry(curve,segs,d[0],7,false),
          new THREE.MeshBasicMaterial({
            color:'#8B7FE8', transparent:true,
            opacity:d[1], depthWrite:false,
            blending:THREE.AdditiveBlending
          })));
      });
    })();

    /* Tick marks */
    for (var k=0; k<9; k++) {
      var ta = (k/9)*Math.PI*2;
      var active = (Math.random()>0.38);
      var tm = new THREE.Mesh(
        new THREE.SphereGeometry(active?1.6:0.9, 6, 6),
        new THREE.MeshBasicMaterial({
          color: active?'#8B7FE8':'#151530',
          transparent:true,
          opacity: active?0.6:0.12,
          blending: active ?
            THREE.AdditiveBlending:
            THREE.NormalBlending
        }));
      tm.position.set(
        Math.cos(ta)*52.0, 0, Math.sin(ta)*52.0);
      grp.add(tm);
    }

    var ringNodes = [];

    /* Node: Zero-Day Exploit Alpha */
    (function(){
      var ng = new THREE.Group();
      ng.position.set(
        Math.cos(0.0000)*52.0, 0,
        Math.sin(0.0000)*52.0);

      var node = new THREE.Mesh(
        new THREE.SphereGeometry(5.8, 24, 24),
        new THREE.MeshStandardMaterial({
          color:'#8B7FE8',
          emissive:'#4030cc',
          emissiveIntensity:1.20,
          roughness:0.22,
          metalness:0.82
        }));
      node.userData = {
        label:'Zero-Day Exploit Alpha',
        status:'Completed',
        ring:'Ring 0',
        isCore:false,
        desc:''
      };
      hoverMeshes.push(node);
      ng.add(node);

      /* Glow shells */
      [[10,0.180],[17,0.080],[26,0.030]]
      .forEach(function(d){
        ng.add(new THREE.Mesh(
          new THREE.SphereGeometry(d[0],12,12),
          new THREE.MeshBasicMaterial({
            color:'#8B7FE8',transparent:true,
            opacity:d[1],depthWrite:false,
            blending:THREE.AdditiveBlending
          })));
      });

      /* Pulse ring */
      var pr = new THREE.Mesh(
        new THREE.TorusGeometry(8.5, 0.55, 8, 44),
        new THREE.MeshBasicMaterial({
          color:'#8B7FE8', transparent:true,
          opacity:0.55, depthWrite:false,
          blending:THREE.AdditiveBlending
        }));
      pr.rotation.x = Math.PI/2;
      ng.add(pr);
      node.userData.pulseRing = pr;

      /* Ghost trail */
      var ghosts = [];
      for (var k=1; k<=7; k++) {
        var gs = new THREE.Mesh(
          new THREE.SphereGeometry(5.8*(1-k*0.1),7,7),
          new THREE.MeshBasicMaterial({
            color:'#8B7FE8', transparent:true,
            opacity:0.22*Math.pow(1-k/8,1.8),
            depthWrite:false,
            blending:THREE.AdditiveBlending
          }));
        grp.add(gs);
        ghosts.push({mesh:gs, delta:k*0.036});
      }

      grp.add(ng);
      ringNodes.push({
        ng:     ng,
        node:   node,
        angle:  0.0000,
        radius: 52.0,
        speed:  0.012000,
        ghosts: ghosts
      });
    })();

    /* Node: Signal Ghost Protocol */
    (function(){
      var ng = new THREE.Group();
      ng.position.set(
        Math.cos(1.2600)*52.0, 0,
        Math.sin(1.2600)*52.0);

      var node = new THREE.Mesh(
        new THREE.SphereGeometry(5.8, 24, 24),
        new THREE.MeshStandardMaterial({
          color:'#8B7FE8',
          emissive:'#4030cc',
          emissiveIntensity:1.20,
          roughness:0.22,
          metalness:0.82
        }));
      node.userData = {
        label:'Signal Ghost Protocol',
        status:'Completed',
        ring:'Ring 0',
        isCore:false,
        desc:''
      };
      hoverMeshes.push(node);
      ng.add(node);

      /* Glow shells */
      [[10,0.180],[17,0.080],[26,0.030]]
      .forEach(function(d){
        ng.add(new THREE.Mesh(
          new THREE.SphereGeometry(d[0],12,12),
          new THREE.MeshBasicMaterial({
            color:'#8B7FE8',transparent:true,
            opacity:d[1],depthWrite:false,
            blending:THREE.AdditiveBlending
          })));
      });

      /* Pulse ring */
      var pr = new THREE.Mesh(
        new THREE.TorusGeometry(8.5, 0.55, 8, 44),
        new THREE.MeshBasicMaterial({
          color:'#8B7FE8', transparent:true,
          opacity:0.55, depthWrite:false,
          blending:THREE.AdditiveBlending
        }));
      pr.rotation.x = Math.PI/2;
      ng.add(pr);
      node.userData.pulseRing = pr;

      /* Ghost trail */
      var ghosts = [];
      for (var k=1; k<=7; k++) {
        var gs = new THREE.Mesh(
          new THREE.SphereGeometry(5.8*(1-k*0.1),7,7),
          new THREE.MeshBasicMaterial({
            color:'#8B7FE8', transparent:true,
            opacity:0.22*Math.pow(1-k/8,1.8),
            depthWrite:false,
            blending:THREE.AdditiveBlending
          }));
        grp.add(gs);
        ghosts.push({mesh:gs, delta:k*0.036});
      }

      grp.add(ng);
      ringNodes.push({
        ng:     ng,
        node:   node,
        angle:  1.2600,
        radius: 52.0,
        speed:  0.012000,
        ghosts: ghosts
      });
    })();

    /* Node: Neural Firewall Bypass */
    (function(){
      var ng = new THREE.Group();
      ng.position.set(
        Math.cos(2.5100)*52.0, 0,
        Math.sin(2.5100)*52.0);

      var node = new THREE.Mesh(
        new THREE.SphereGeometry(5.8, 24, 24),
        new THREE.MeshStandardMaterial({
          color:'#8B7FE8',
          emissive:'#4030cc',
          emissiveIntensity:1.20,
          roughness:0.22,
          metalness:0.82
        }));
      node.userData = {
        label:'Neural Firewall Bypass',
        status:'Completed',
        ring:'Ring 0',
        isCore:false,
        desc:''
      };
      hoverMeshes.push(node);
      ng.add(node);

      /* Glow shells */
      [[10,0.180],[17,0.080],[26,0.030]]
      .forEach(function(d){
        ng.add(new THREE.Mesh(
          new THREE.SphereGeometry(d[0],12,12),
          new THREE.MeshBasicMaterial({
            color:'#8B7FE8',transparent:true,
            opacity:d[1],depthWrite:false,
            blending:THREE.AdditiveBlending
          })));
      });

      /* Pulse ring */
      var pr = new THREE.Mesh(
        new THREE.TorusGeometry(8.5, 0.55, 8, 44),
        new THREE.MeshBasicMaterial({
          color:'#8B7FE8', transparent:true,
          opacity:0.55, depthWrite:false,
          blending:THREE.AdditiveBlending
        }));
      pr.rotation.x = Math.PI/2;
      ng.add(pr);
      node.userData.pulseRing = pr;

      /* Ghost trail */
      var ghosts = [];
      for (var k=1; k<=7; k++) {
        var gs = new THREE.Mesh(
          new THREE.SphereGeometry(5.8*(1-k*0.1),7,7),
          new THREE.MeshBasicMaterial({
            color:'#8B7FE8', transparent:true,
            opacity:0.22*Math.pow(1-k/8,1.8),
            depthWrite:false,
            blending:THREE.AdditiveBlending
          }));
        grp.add(gs);
        ghosts.push({mesh:gs, delta:k*0.036});
      }

      grp.add(ng);
      ringNodes.push({
        ng:     ng,
        node:   node,
        angle:  2.5100,
        radius: 52.0,
        speed:  0.012000,
        ghosts: ghosts
      });
    })();

    /* Node: Quantum Cipher Crack */
    (function(){
      var ng = new THREE.Group();
      ng.position.set(
        Math.cos(3.7700)*52.0, 0,
        Math.sin(3.7700)*52.0);

      var node = new THREE.Mesh(
        new THREE.SphereGeometry(5.8, 24, 24),
        new THREE.MeshStandardMaterial({
          color:'#8B7FE8',
          emissive:'#4030cc',
          emissiveIntensity:1.20,
          roughness:0.22,
          metalness:0.82
        }));
      node.userData = {
        label:'Quantum Cipher Crack',
        status:'Completed',
        ring:'Ring 0',
        isCore:false,
        desc:''
      };
      hoverMeshes.push(node);
      ng.add(node);

      /* Glow shells */
      [[10,0.180],[17,0.080],[26,0.030]]
      .forEach(function(d){
        ng.add(new THREE.Mesh(
          new THREE.SphereGeometry(d[0],12,12),
          new THREE.MeshBasicMaterial({
            color:'#8B7FE8',transparent:true,
            opacity:d[1],depthWrite:false,
            blending:THREE.AdditiveBlending
          })));
      });

      /* Pulse ring */
      var pr = new THREE.Mesh(
        new THREE.TorusGeometry(8.5, 0.55, 8, 44),
        new THREE.MeshBasicMaterial({
          color:'#8B7FE8', transparent:true,
          opacity:0.55, depthWrite:false,
          blending:THREE.AdditiveBlending
        }));
      pr.rotation.x = Math.PI/2;
      ng.add(pr);
      node.userData.pulseRing = pr;

      /* Ghost trail */
      var ghosts = [];
      for (var k=1; k<=7; k++) {
        var gs = new THREE.Mesh(
          new THREE.SphereGeometry(5.8*(1-k*0.1),7,7),
          new THREE.MeshBasicMaterial({
            color:'#8B7FE8', transparent:true,
            opacity:0.22*Math.pow(1-k/8,1.8),
            depthWrite:false,
            blending:THREE.AdditiveBlending
          }));
        grp.add(gs);
        ghosts.push({mesh:gs, delta:k*0.036});
      }

      grp.add(ng);
      ringNodes.push({
        ng:     ng,
        node:   node,
        angle:  3.7700,
        radius: 52.0,
        speed:  0.012000,
        ghosts: ghosts
      });
    })();

    /* Node: Deep Cover Insertion */
    (function(){
      var ng = new THREE.Group();
      ng.position.set(
        Math.cos(5.0300)*52.0, 0,
        Math.sin(5.0300)*52.0);

      var node = new THREE.Mesh(
        new THREE.SphereGeometry(5.8, 24, 24),
        new THREE.MeshStandardMaterial({
          color:'#EF9F27',
          emissive:'#996010',
          emissiveIntensity:2.00,
          roughness:0.22,
          metalness:0.82
        }));
      node.userData = {
        label:'Deep Cover Insertion',
        status:'In Progress',
        ring:'Ring 0',
        isCore:false,
        desc:''
      };
      hoverMeshes.push(node);
      ng.add(node);

      /* Glow shells */
      [[10,0.180],[17,0.080],[26,0.030]]
      .forEach(function(d){
        ng.add(new THREE.Mesh(
          new THREE.SphereGeometry(d[0],12,12),
          new THREE.MeshBasicMaterial({
            color:'#EF9F27',transparent:true,
            opacity:d[1],depthWrite:false,
            blending:THREE.AdditiveBlending
          })));
      });

      /* Ghost trail */
      var ghosts = [];
      for (var k=1; k<=7; k++) {
        var gs = new THREE.Mesh(
          new THREE.SphereGeometry(5.8*(1-k*0.1),7,7),
          new THREE.MeshBasicMaterial({
            color:'#EF9F27', transparent:true,
            opacity:0.22*Math.pow(1-k/8,1.8),
            depthWrite:false,
            blending:THREE.AdditiveBlending
          }));
        grp.add(gs);
        ghosts.push({mesh:gs, delta:k*0.036});
      }

      grp.add(ng);
      ringNodes.push({
        ng:     ng,
        node:   node,
        angle:  5.0300,
        radius: 52.0,
        speed:  0.012000,
        ghosts: ghosts
      });
    })();

    scene.add(grp);
    allRings.push({
      grp:    grp,
      nodes:  ringNodes,
      speed:  0.012000,
      radius: 52.0
    });
  })();

  /* ── Orbital Ring 1 (r=94) ── */
  (function(){
    var grp = new THREE.Group();
    grp.rotation.x = 0.0000;
    grp.rotation.z = 0.0000;

    /* Ring torus */
    grp.add(new THREE.Mesh(
      new THREE.TorusGeometry(94.0, 0.55, 8, 140),
      new THREE.MeshBasicMaterial({
        color:'#4A9AEF', transparent:true,
        opacity:0.15, depthWrite:false
      })));

    /* Glow torus */
    grp.add(new THREE.Mesh(
      new THREE.TorusGeometry(94.0, 4.2, 8, 140),
      new THREE.MeshBasicMaterial({
        color:'#4A9AEF', transparent:true,
        opacity:0.026, depthWrite:false,
        blending:THREE.AdditiveBlending
      })));

    /* Completion arc 60% */
    (function(){
      var ae = 0.600000 * Math.PI * 2;
      var segs = Math.max(16,
        Math.floor(ae/(Math.PI*2)*160));
      var pts = [];
      for (var i=0; i<=segs; i++) {
        var a = ae * (i/segs);
        pts.push(new THREE.Vector3(
          Math.cos(a)*94.0, 0,
          Math.sin(a)*94.0));
      }
      var curve = new THREE.CatmullRomCurve3(pts, false);
      [[1.8,0.82],[6.5,0.10],[13,0.032]]
      .forEach(function(d){
        grp.add(new THREE.Mesh(
          new THREE.TubeGeometry(curve,segs,d[0],7,false),
          new THREE.MeshBasicMaterial({
            color:'#4A9AEF', transparent:true,
            opacity:d[1], depthWrite:false,
            blending:THREE.AdditiveBlending
          })));
      });
    })();

    /* Tick marks */
    for (var k=0; k<9; k++) {
      var ta = (k/9)*Math.PI*2;
      var active = (Math.random()>0.38);
      var tm = new THREE.Mesh(
        new THREE.SphereGeometry(active?1.6:0.9, 6, 6),
        new THREE.MeshBasicMaterial({
          color: active?'#4A9AEF':'#151530',
          transparent:true,
          opacity: active?0.6:0.12,
          blending: active ?
            THREE.AdditiveBlending:
            THREE.NormalBlending
        }));
      tm.position.set(
        Math.cos(ta)*94.0, 0, Math.sin(ta)*94.0);
      grp.add(tm);
    }

    var ringNodes = [];

    /* Node: Void Sweep Network */
    (function(){
      var ng = new THREE.Group();
      ng.position.set(
        Math.cos(0.4000)*94.0, 0,
        Math.sin(0.4000)*94.0);

      var node = new THREE.Mesh(
        new THREE.SphereGeometry(5.8, 24, 24),
        new THREE.MeshStandardMaterial({
          color:'#8B7FE8',
          emissive:'#4030cc',
          emissiveIntensity:1.20,
          roughness:0.22,
          metalness:0.82
        }));
      node.userData = {
        label:'Void Sweep Network',
        status:'Completed',
        ring:'Ring 1',
        isCore:false,
        desc:''
      };
      hoverMeshes.push(node);
      ng.add(node);

      /* Glow shells */
      [[10,0.180],[17,0.080],[26,0.030]]
      .forEach(function(d){
        ng.add(new THREE.Mesh(
          new THREE.SphereGeometry(d[0],12,12),
          new THREE.MeshBasicMaterial({
            color:'#8B7FE8',transparent:true,
            opacity:d[1],depthWrite:false,
            blending:THREE.AdditiveBlending
          })));
      });

      /* Pulse ring */
      var pr = new THREE.Mesh(
        new THREE.TorusGeometry(8.5, 0.55, 8, 44),
        new THREE.MeshBasicMaterial({
          color:'#4A9AEF', transparent:true,
          opacity:0.55, depthWrite:false,
          blending:THREE.AdditiveBlending
        }));
      pr.rotation.x = Math.PI/2;
      ng.add(pr);
      node.userData.pulseRing = pr;

      /* Ghost trail */
      var ghosts = [];
      for (var k=1; k<=7; k++) {
        var gs = new THREE.Mesh(
          new THREE.SphereGeometry(5.8*(1-k*0.1),7,7),
          new THREE.MeshBasicMaterial({
            color:'#8B7FE8', transparent:true,
            opacity:0.22*Math.pow(1-k/8,1.8),
            depthWrite:false,
            blending:THREE.AdditiveBlending
          }));
        grp.add(gs);
        ghosts.push({mesh:gs, delta:k*0.036});
      }

      grp.add(ng);
      ringNodes.push({
        ng:     ng,
        node:   node,
        angle:  0.4000,
        radius: 94.0,
        speed:  0.007800,
        ghosts: ghosts
      });
    })();

    /* Node: Ion Storm Intercept */
    (function(){
      var ng = new THREE.Group();
      ng.position.set(
        Math.cos(1.6500)*94.0, 0,
        Math.sin(1.6500)*94.0);

      var node = new THREE.Mesh(
        new THREE.SphereGeometry(5.8, 24, 24),
        new THREE.MeshStandardMaterial({
          color:'#8B7FE8',
          emissive:'#4030cc',
          emissiveIntensity:1.20,
          roughness:0.22,
          metalness:0.82
        }));
      node.userData = {
        label:'Ion Storm Intercept',
        status:'Completed',
        ring:'Ring 1',
        isCore:false,
        desc:''
      };
      hoverMeshes.push(node);
      ng.add(node);

      /* Glow shells */
      [[10,0.180],[17,0.080],[26,0.030]]
      .forEach(function(d){
        ng.add(new THREE.Mesh(
          new THREE.SphereGeometry(d[0],12,12),
          new THREE.MeshBasicMaterial({
            color:'#8B7FE8',transparent:true,
            opacity:d[1],depthWrite:false,
            blending:THREE.AdditiveBlending
          })));
      });

      /* Pulse ring */
      var pr = new THREE.Mesh(
        new THREE.TorusGeometry(8.5, 0.55, 8, 44),
        new THREE.MeshBasicMaterial({
          color:'#4A9AEF', transparent:true,
          opacity:0.55, depthWrite:false,
          blending:THREE.AdditiveBlending
        }));
      pr.rotation.x = Math.PI/2;
      ng.add(pr);
      node.userData.pulseRing = pr;

      /* Ghost trail */
      var ghosts = [];
      for (var k=1; k<=7; k++) {
        var gs = new THREE.Mesh(
          new THREE.SphereGeometry(5.8*(1-k*0.1),7,7),
          new THREE.MeshBasicMaterial({
            color:'#8B7FE8', transparent:true,
            opacity:0.22*Math.pow(1-k/8,1.8),
            depthWrite:false,
            blending:THREE.AdditiveBlending
          }));
        grp.add(gs);
        ghosts.push({mesh:gs, delta:k*0.036});
      }

      grp.add(ng);
      ringNodes.push({
        ng:     ng,
        node:   node,
        angle:  1.6500,
        radius: 94.0,
        speed:  0.007800,
        ghosts: ghosts
      });
    })();

    /* Node: Dark Corridor Recon */
    (function(){
      var ng = new THREE.Group();
      ng.position.set(
        Math.cos(2.9000)*94.0, 0,
        Math.sin(2.9000)*94.0);

      var node = new THREE.Mesh(
        new THREE.SphereGeometry(5.8, 24, 24),
        new THREE.MeshStandardMaterial({
          color:'#8B7FE8',
          emissive:'#4030cc',
          emissiveIntensity:1.20,
          roughness:0.22,
          metalness:0.82
        }));
      node.userData = {
        label:'Dark Corridor Recon',
        status:'Completed',
        ring:'Ring 1',
        isCore:false,
        desc:''
      };
      hoverMeshes.push(node);
      ng.add(node);

      /* Glow shells */
      [[10,0.180],[17,0.080],[26,0.030]]
      .forEach(function(d){
        ng.add(new THREE.Mesh(
          new THREE.SphereGeometry(d[0],12,12),
          new THREE.MeshBasicMaterial({
            color:'#8B7FE8',transparent:true,
            opacity:d[1],depthWrite:false,
            blending:THREE.AdditiveBlending
          })));
      });

      /* Pulse ring */
      var pr = new THREE.Mesh(
        new THREE.TorusGeometry(8.5, 0.55, 8, 44),
        new THREE.MeshBasicMaterial({
          color:'#4A9AEF', transparent:true,
          opacity:0.55, depthWrite:false,
          blending:THREE.AdditiveBlending
        }));
      pr.rotation.x = Math.PI/2;
      ng.add(pr);
      node.userData.pulseRing = pr;

      /* Ghost trail */
      var ghosts = [];
      for (var k=1; k<=7; k++) {
        var gs = new THREE.Mesh(
          new THREE.SphereGeometry(5.8*(1-k*0.1),7,7),
          new THREE.MeshBasicMaterial({
            color:'#8B7FE8', transparent:true,
            opacity:0.22*Math.pow(1-k/8,1.8),
            depthWrite:false,
            blending:THREE.AdditiveBlending
          }));
        grp.add(gs);
        ghosts.push({mesh:gs, delta:k*0.036});
      }

      grp.add(ng);
      ringNodes.push({
        ng:     ng,
        node:   node,
        angle:  2.9000,
        radius: 94.0,
        speed:  0.007800,
        ghosts: ghosts
      });
    })();

    /* Node: Nebula Gate Breach */
    (function(){
      var ng = new THREE.Group();
      ng.position.set(
        Math.cos(4.1500)*94.0, 0,
        Math.sin(4.1500)*94.0);

      var node = new THREE.Mesh(
        new THREE.SphereGeometry(5.8, 24, 24),
        new THREE.MeshStandardMaterial({
          color:'#EF9F27',
          emissive:'#996010',
          emissiveIntensity:2.00,
          roughness:0.22,
          metalness:0.82
        }));
      node.userData = {
        label:'Nebula Gate Breach',
        status:'In Progress',
        ring:'Ring 1',
        isCore:false,
        desc:''
      };
      hoverMeshes.push(node);
      ng.add(node);

      /* Glow shells */
      [[10,0.180],[17,0.080],[26,0.030]]
      .forEach(function(d){
        ng.add(new THREE.Mesh(
          new THREE.SphereGeometry(d[0],12,12),
          new THREE.MeshBasicMaterial({
            color:'#EF9F27',transparent:true,
            opacity:d[1],depthWrite:false,
            blending:THREE.AdditiveBlending
          })));
      });

      /* Ghost trail */
      var ghosts = [];
      for (var k=1; k<=7; k++) {
        var gs = new THREE.Mesh(
          new THREE.SphereGeometry(5.8*(1-k*0.1),7,7),
          new THREE.MeshBasicMaterial({
            color:'#EF9F27', transparent:true,
            opacity:0.22*Math.pow(1-k/8,1.8),
            depthWrite:false,
            blending:THREE.AdditiveBlending
          }));
        grp.add(gs);
        ghosts.push({mesh:gs, delta:k*0.036});
      }

      grp.add(ng);
      ringNodes.push({
        ng:     ng,
        node:   node,
        angle:  4.1500,
        radius: 94.0,
        speed:  0.007800,
        ghosts: ghosts
      });
    })();

    /* Node: Phase Shift Ambush */
    (function(){
      var ng = new THREE.Group();
      ng.position.set(
        Math.cos(5.4000)*94.0, 0,
        Math.sin(5.4000)*94.0);

      var node = new THREE.Mesh(
        new THREE.SphereGeometry(5.8, 24, 24),
        new THREE.MeshStandardMaterial({
          color:'#D85A30',
          emissive:'#991510',
          emissiveIntensity:2.20,
          roughness:0.22,
          metalness:0.82
        }));
      node.userData = {
        label:'Phase Shift Ambush',
        status:'Overdue',
        ring:'Ring 1',
        isCore:false,
        desc:''
      };
      hoverMeshes.push(node);
      ng.add(node);

      /* Glow shells */
      [[10,0.180],[17,0.080],[26,0.030]]
      .forEach(function(d){
        ng.add(new THREE.Mesh(
          new THREE.SphereGeometry(d[0],12,12),
          new THREE.MeshBasicMaterial({
            color:'#D85A30',transparent:true,
            opacity:d[1],depthWrite:false,
            blending:THREE.AdditiveBlending
          })));
      });

      /* Warn ring */
      var wr = new THREE.Mesh(
        new THREE.TorusGeometry(9.5, 0.45, 8, 44),
        new THREE.MeshBasicMaterial({
          color:'#D85A30', transparent:true,
          opacity:0.4, depthWrite:false,
          blending:THREE.AdditiveBlending
        }));
      wr.rotation.x = Math.PI/3;
      ng.add(wr);
      node.userData.warnRing = wr;

      /* Ghost trail */
      var ghosts = [];
      for (var k=1; k<=7; k++) {
        var gs = new THREE.Mesh(
          new THREE.SphereGeometry(5.8*(1-k*0.1),7,7),
          new THREE.MeshBasicMaterial({
            color:'#D85A30', transparent:true,
            opacity:0.22*Math.pow(1-k/8,1.8),
            depthWrite:false,
            blending:THREE.AdditiveBlending
          }));
        grp.add(gs);
        ghosts.push({mesh:gs, delta:k*0.036});
      }

      grp.add(ng);
      ringNodes.push({
        ng:     ng,
        node:   node,
        angle:  5.4000,
        radius: 94.0,
        speed:  0.007800,
        ghosts: ghosts
      });
    })();

    scene.add(grp);
    allRings.push({
      grp:    grp,
      nodes:  ringNodes,
      speed:  0.007800,
      radius: 94.0
    });
  })();

  /* ── Orbital Ring 2 (r=138) ── */
  (function(){
    var grp = new THREE.Group();
    grp.rotation.x = 0.0000;
    grp.rotation.z = 0.0000;

    /* Ring torus */
    grp.add(new THREE.Mesh(
      new THREE.TorusGeometry(138.0, 0.55, 8, 140),
      new THREE.MeshBasicMaterial({
        color:'#22B890', transparent:true,
        opacity:0.15, depthWrite:false
      })));

    /* Glow torus */
    grp.add(new THREE.Mesh(
      new THREE.TorusGeometry(138.0, 4.2, 8, 140),
      new THREE.MeshBasicMaterial({
        color:'#22B890', transparent:true,
        opacity:0.026, depthWrite:false,
        blending:THREE.AdditiveBlending
      })));

    /* Completion arc 75% */
    (function(){
      var ae = 0.750000 * Math.PI * 2;
      var segs = Math.max(16,
        Math.floor(ae/(Math.PI*2)*160));
      var pts = [];
      for (var i=0; i<=segs; i++) {
        var a = ae * (i/segs);
        pts.push(new THREE.Vector3(
          Math.cos(a)*138.0, 0,
          Math.sin(a)*138.0));
      }
      var curve = new THREE.CatmullRomCurve3(pts, false);
      [[1.8,0.82],[6.5,0.10],[13,0.032]]
      .forEach(function(d){
        grp.add(new THREE.Mesh(
          new THREE.TubeGeometry(curve,segs,d[0],7,false),
          new THREE.MeshBasicMaterial({
            color:'#22B890', transparent:true,
            opacity:d[1], depthWrite:false,
            blending:THREE.AdditiveBlending
          })));
      });
    })();

    /* Tick marks */
    for (var k=0; k<8; k++) {
      var ta = (k/8)*Math.PI*2;
      var active = (Math.random()>0.38);
      var tm = new THREE.Mesh(
        new THREE.SphereGeometry(active?1.6:0.9, 6, 6),
        new THREE.MeshBasicMaterial({
          color: active?'#22B890':'#151530',
          transparent:true,
          opacity: active?0.6:0.12,
          blending: active ?
            THREE.AdditiveBlending:
            THREE.NormalBlending
        }));
      tm.position.set(
        Math.cos(ta)*138.0, 0, Math.sin(ta)*138.0);
      grp.add(tm);
    }

    var ringNodes = [];

    /* Node: Bioscan Array Omega */
    (function(){
      var ng = new THREE.Group();
      ng.position.set(
        Math.cos(0.6000)*138.0, 0,
        Math.sin(0.6000)*138.0);

      var node = new THREE.Mesh(
        new THREE.SphereGeometry(5.8, 24, 24),
        new THREE.MeshStandardMaterial({
          color:'#8B7FE8',
          emissive:'#4030cc',
          emissiveIntensity:1.20,
          roughness:0.22,
          metalness:0.82
        }));
      node.userData = {
        label:'Bioscan Array Omega',
        status:'Completed',
        ring:'Ring 2',
        isCore:false,
        desc:''
      };
      hoverMeshes.push(node);
      ng.add(node);

      /* Glow shells */
      [[10,0.180],[17,0.080],[26,0.030]]
      .forEach(function(d){
        ng.add(new THREE.Mesh(
          new THREE.SphereGeometry(d[0],12,12),
          new THREE.MeshBasicMaterial({
            color:'#8B7FE8',transparent:true,
            opacity:d[1],depthWrite:false,
            blending:THREE.AdditiveBlending
          })));
      });

      /* Pulse ring */
      var pr = new THREE.Mesh(
        new THREE.TorusGeometry(8.5, 0.55, 8, 44),
        new THREE.MeshBasicMaterial({
          color:'#22B890', transparent:true,
          opacity:0.55, depthWrite:false,
          blending:THREE.AdditiveBlending
        }));
      pr.rotation.x = Math.PI/2;
      ng.add(pr);
      node.userData.pulseRing = pr;

      /* Ghost trail */
      var ghosts = [];
      for (var k=1; k<=7; k++) {
        var gs = new THREE.Mesh(
          new THREE.SphereGeometry(5.8*(1-k*0.1),7,7),
          new THREE.MeshBasicMaterial({
            color:'#8B7FE8', transparent:true,
            opacity:0.22*Math.pow(1-k/8,1.8),
            depthWrite:false,
            blending:THREE.AdditiveBlending
          }));
        grp.add(gs);
        ghosts.push({mesh:gs, delta:k*0.036});
      }

      grp.add(ng);
      ringNodes.push({
        ng:     ng,
        node:   node,
        angle:  0.6000,
        radius: 138.0,
        speed:  0.005000,
        ghosts: ghosts
      });
    })();

    /* Node: Nanobot Swarm Control */
    (function(){
      var ng = new THREE.Group();
      ng.position.set(
        Math.cos(2.0000)*138.0, 0,
        Math.sin(2.0000)*138.0);

      var node = new THREE.Mesh(
        new THREE.SphereGeometry(5.8, 24, 24),
        new THREE.MeshStandardMaterial({
          color:'#8B7FE8',
          emissive:'#4030cc',
          emissiveIntensity:1.20,
          roughness:0.22,
          metalness:0.82
        }));
      node.userData = {
        label:'Nanobot Swarm Control',
        status:'Completed',
        ring:'Ring 2',
        isCore:false,
        desc:''
      };
      hoverMeshes.push(node);
      ng.add(node);

      /* Glow shells */
      [[10,0.180],[17,0.080],[26,0.030]]
      .forEach(function(d){
        ng.add(new THREE.Mesh(
          new THREE.SphereGeometry(d[0],12,12),
          new THREE.MeshBasicMaterial({
            color:'#8B7FE8',transparent:true,
            opacity:d[1],depthWrite:false,
            blending:THREE.AdditiveBlending
          })));
      });

      /* Pulse ring */
      var pr = new THREE.Mesh(
        new THREE.TorusGeometry(8.5, 0.55, 8, 44),
        new THREE.MeshBasicMaterial({
          color:'#22B890', transparent:true,
          opacity:0.55, depthWrite:false,
          blending:THREE.AdditiveBlending
        }));
      pr.rotation.x = Math.PI/2;
      ng.add(pr);
      node.userData.pulseRing = pr;

      /* Ghost trail */
      var ghosts = [];
      for (var k=1; k<=7; k++) {
        var gs = new THREE.Mesh(
          new THREE.SphereGeometry(5.8*(1-k*0.1),7,7),
          new THREE.MeshBasicMaterial({
            color:'#8B7FE8', transparent:true,
            opacity:0.22*Math.pow(1-k/8,1.8),
            depthWrite:false,
            blending:THREE.AdditiveBlending
          }));
        grp.add(gs);
        ghosts.push({mesh:gs, delta:k*0.036});
      }

      grp.add(ng);
      ringNodes.push({
        ng:     ng,
        node:   node,
        angle:  2.0000,
        radius: 138.0,
        speed:  0.005000,
        ghosts: ghosts
      });
    })();

    /* Node: Cryo Stasis Override */
    (function(){
      var ng = new THREE.Group();
      ng.position.set(
        Math.cos(3.3500)*138.0, 0,
        Math.sin(3.3500)*138.0);

      var node = new THREE.Mesh(
        new THREE.SphereGeometry(5.8, 24, 24),
        new THREE.MeshStandardMaterial({
          color:'#8B7FE8',
          emissive:'#4030cc',
          emissiveIntensity:1.20,
          roughness:0.22,
          metalness:0.82
        }));
      node.userData = {
        label:'Cryo Stasis Override',
        status:'Completed',
        ring:'Ring 2',
        isCore:false,
        desc:''
      };
      hoverMeshes.push(node);
      ng.add(node);

      /* Glow shells */
      [[10,0.180],[17,0.080],[26,0.030]]
      .forEach(function(d){
        ng.add(new THREE.Mesh(
          new THREE.SphereGeometry(d[0],12,12),
          new THREE.MeshBasicMaterial({
            color:'#8B7FE8',transparent:true,
            opacity:d[1],depthWrite:false,
            blending:THREE.AdditiveBlending
          })));
      });

      /* Pulse ring */
      var pr = new THREE.Mesh(
        new THREE.TorusGeometry(8.5, 0.55, 8, 44),
        new THREE.MeshBasicMaterial({
          color:'#22B890', transparent:true,
          opacity:0.55, depthWrite:false,
          blending:THREE.AdditiveBlending
        }));
      pr.rotation.x = Math.PI/2;
      ng.add(pr);
      node.userData.pulseRing = pr;

      /* Ghost trail */
      var ghosts = [];
      for (var k=1; k<=7; k++) {
        var gs = new THREE.Mesh(
          new THREE.SphereGeometry(5.8*(1-k*0.1),7,7),
          new THREE.MeshBasicMaterial({
            color:'#8B7FE8', transparent:true,
            opacity:0.22*Math.pow(1-k/8,1.8),
            depthWrite:false,
            blending:THREE.AdditiveBlending
          }));
        grp.add(gs);
        ghosts.push({mesh:gs, delta:k*0.036});
      }

      grp.add(ng);
      ringNodes.push({
        ng:     ng,
        node:   node,
        angle:  3.3500,
        radius: 138.0,
        speed:  0.005000,
        ghosts: ghosts
      });
    })();

    /* Node: Toxin Web Neutralise */
    (function(){
      var ng = new THREE.Group();
      ng.position.set(
        Math.cos(4.7200)*138.0, 0,
        Math.sin(4.7200)*138.0);

      var node = new THREE.Mesh(
        new THREE.SphereGeometry(5.8, 24, 24),
        new THREE.MeshStandardMaterial({
          color:'#EF9F27',
          emissive:'#996010',
          emissiveIntensity:2.00,
          roughness:0.22,
          metalness:0.82
        }));
      node.userData = {
        label:'Toxin Web Neutralise',
        status:'In Progress',
        ring:'Ring 2',
        isCore:false,
        desc:''
      };
      hoverMeshes.push(node);
      ng.add(node);

      /* Glow shells */
      [[10,0.180],[17,0.080],[26,0.030]]
      .forEach(function(d){
        ng.add(new THREE.Mesh(
          new THREE.SphereGeometry(d[0],12,12),
          new THREE.MeshBasicMaterial({
            color:'#EF9F27',transparent:true,
            opacity:d[1],depthWrite:false,
            blending:THREE.AdditiveBlending
          })));
      });

      /* Ghost trail */
      var ghosts = [];
      for (var k=1; k<=7; k++) {
        var gs = new THREE.Mesh(
          new THREE.SphereGeometry(5.8*(1-k*0.1),7,7),
          new THREE.MeshBasicMaterial({
            color:'#EF9F27', transparent:true,
            opacity:0.22*Math.pow(1-k/8,1.8),
            depthWrite:false,
            blending:THREE.AdditiveBlending
          }));
        grp.add(gs);
        ghosts.push({mesh:gs, delta:k*0.036});
      }

      grp.add(ng);
      ringNodes.push({
        ng:     ng,
        node:   node,
        angle:  4.7200,
        radius: 138.0,
        speed:  0.005000,
        ghosts: ghosts
      });
    })();

    scene.add(grp);
    allRings.push({
      grp:    grp,
      nodes:  ringNodes,
      speed:  0.005000,
      radius: 138.0
    });
  })();

  /* ── Orbital Ring 3 (r=182) ── */
  (function(){
    var grp = new THREE.Group();
    grp.rotation.x = 0.0000;
    grp.rotation.z = 0.0000;

    /* Ring torus */
    grp.add(new THREE.Mesh(
      new THREE.TorusGeometry(182.0, 0.55, 8, 140),
      new THREE.MeshBasicMaterial({
        color:'#F0A030', transparent:true,
        opacity:0.15, depthWrite:false
      })));

    /* Glow torus */
    grp.add(new THREE.Mesh(
      new THREE.TorusGeometry(182.0, 4.2, 8, 140),
      new THREE.MeshBasicMaterial({
        color:'#F0A030', transparent:true,
        opacity:0.026, depthWrite:false,
        blending:THREE.AdditiveBlending
      })));

    /* Completion arc 25% */
    (function(){
      var ae = 0.250000 * Math.PI * 2;
      var segs = Math.max(16,
        Math.floor(ae/(Math.PI*2)*160));
      var pts = [];
      for (var i=0; i<=segs; i++) {
        var a = ae * (i/segs);
        pts.push(new THREE.Vector3(
          Math.cos(a)*182.0, 0,
          Math.sin(a)*182.0));
      }
      var curve = new THREE.CatmullRomCurve3(pts, false);
      [[1.8,0.82],[6.5,0.10],[13,0.032]]
      .forEach(function(d){
        grp.add(new THREE.Mesh(
          new THREE.TubeGeometry(curve,segs,d[0],7,false),
          new THREE.MeshBasicMaterial({
            color:'#F0A030', transparent:true,
            opacity:d[1], depthWrite:false,
            blending:THREE.AdditiveBlending
          })));
      });
    })();

    /* Tick marks */
    for (var k=0; k<8; k++) {
      var ta = (k/8)*Math.PI*2;
      var active = (Math.random()>0.38);
      var tm = new THREE.Mesh(
        new THREE.SphereGeometry(active?1.6:0.9, 6, 6),
        new THREE.MeshBasicMaterial({
          color: active?'#F0A030':'#151530',
          transparent:true,
          opacity: active?0.6:0.12,
          blending: active ?
            THREE.AdditiveBlending:
            THREE.NormalBlending
        }));
      tm.position.set(
        Math.cos(ta)*182.0, 0, Math.sin(ta)*182.0);
      grp.add(tm);
    }

    var ringNodes = [];

    /* Node: Warp Drive Calibrate */
    (function(){
      var ng = new THREE.Group();
      ng.position.set(
        Math.cos(0.5000)*182.0, 0,
        Math.sin(0.5000)*182.0);

      var node = new THREE.Mesh(
        new THREE.SphereGeometry(5.8, 24, 24),
        new THREE.MeshStandardMaterial({
          color:'#8B7FE8',
          emissive:'#4030cc',
          emissiveIntensity:1.20,
          roughness:0.22,
          metalness:0.82
        }));
      node.userData = {
        label:'Warp Drive Calibrate',
        status:'Completed',
        ring:'Ring 3',
        isCore:false,
        desc:''
      };
      hoverMeshes.push(node);
      ng.add(node);

      /* Glow shells */
      [[10,0.180],[17,0.080],[26,0.030]]
      .forEach(function(d){
        ng.add(new THREE.Mesh(
          new THREE.SphereGeometry(d[0],12,12),
          new THREE.MeshBasicMaterial({
            color:'#8B7FE8',transparent:true,
            opacity:d[1],depthWrite:false,
            blending:THREE.AdditiveBlending
          })));
      });

      /* Pulse ring */
      var pr = new THREE.Mesh(
        new THREE.TorusGeometry(8.5, 0.55, 8, 44),
        new THREE.MeshBasicMaterial({
          color:'#F0A030', transparent:true,
          opacity:0.55, depthWrite:false,
          blending:THREE.AdditiveBlending
        }));
      pr.rotation.x = Math.PI/2;
      ng.add(pr);
      node.userData.pulseRing = pr;

      /* Ghost trail */
      var ghosts = [];
      for (var k=1; k<=7; k++) {
        var gs = new THREE.Mesh(
          new THREE.SphereGeometry(5.8*(1-k*0.1),7,7),
          new THREE.MeshBasicMaterial({
            color:'#8B7FE8', transparent:true,
            opacity:0.22*Math.pow(1-k/8,1.8),
            depthWrite:false,
            blending:THREE.AdditiveBlending
          }));
        grp.add(gs);
        ghosts.push({mesh:gs, delta:k*0.036});
      }

      grp.add(ng);
      ringNodes.push({
        ng:     ng,
        node:   node,
        angle:  0.5000,
        radius: 182.0,
        speed:  0.003000,
        ghosts: ghosts
      });
    })();

    /* Node: Singularity Monitor */
    (function(){
      var ng = new THREE.Group();
      ng.position.set(
        Math.cos(1.8800)*182.0, 0,
        Math.sin(1.8800)*182.0);

      var node = new THREE.Mesh(
        new THREE.SphereGeometry(5.8, 24, 24),
        new THREE.MeshStandardMaterial({
          color:'#EF9F27',
          emissive:'#996010',
          emissiveIntensity:2.00,
          roughness:0.22,
          metalness:0.82
        }));
      node.userData = {
        label:'Singularity Monitor',
        status:'In Progress',
        ring:'Ring 3',
        isCore:false,
        desc:''
      };
      hoverMeshes.push(node);
      ng.add(node);

      /* Glow shells */
      [[10,0.180],[17,0.080],[26,0.030]]
      .forEach(function(d){
        ng.add(new THREE.Mesh(
          new THREE.SphereGeometry(d[0],12,12),
          new THREE.MeshBasicMaterial({
            color:'#EF9F27',transparent:true,
            opacity:d[1],depthWrite:false,
            blending:THREE.AdditiveBlending
          })));
      });

      /* Ghost trail */
      var ghosts = [];
      for (var k=1; k<=7; k++) {
        var gs = new THREE.Mesh(
          new THREE.SphereGeometry(5.8*(1-k*0.1),7,7),
          new THREE.MeshBasicMaterial({
            color:'#EF9F27', transparent:true,
            opacity:0.22*Math.pow(1-k/8,1.8),
            depthWrite:false,
            blending:THREE.AdditiveBlending
          }));
        grp.add(gs);
        ghosts.push({mesh:gs, delta:k*0.036});
      }

      grp.add(ng);
      ringNodes.push({
        ng:     ng,
        node:   node,
        angle:  1.8800,
        radius: 182.0,
        speed:  0.003000,
        ghosts: ghosts
      });
    })();

    /* Node: Dark Energy Harvest */
    (function(){
      var ng = new THREE.Group();
      ng.position.set(
        Math.cos(3.2700)*182.0, 0,
        Math.sin(3.2700)*182.0);

      var node = new THREE.Mesh(
        new THREE.SphereGeometry(5.8, 24, 24),
        new THREE.MeshStandardMaterial({
          color:'#D85A30',
          emissive:'#991510',
          emissiveIntensity:2.20,
          roughness:0.22,
          metalness:0.82
        }));
      node.userData = {
        label:'Dark Energy Harvest',
        status:'Overdue',
        ring:'Ring 3',
        isCore:false,
        desc:''
      };
      hoverMeshes.push(node);
      ng.add(node);

      /* Glow shells */
      [[10,0.180],[17,0.080],[26,0.030]]
      .forEach(function(d){
        ng.add(new THREE.Mesh(
          new THREE.SphereGeometry(d[0],12,12),
          new THREE.MeshBasicMaterial({
            color:'#D85A30',transparent:true,
            opacity:d[1],depthWrite:false,
            blending:THREE.AdditiveBlending
          })));
      });

      /* Warn ring */
      var wr = new THREE.Mesh(
        new THREE.TorusGeometry(9.5, 0.45, 8, 44),
        new THREE.MeshBasicMaterial({
          color:'#D85A30', transparent:true,
          opacity:0.4, depthWrite:false,
          blending:THREE.AdditiveBlending
        }));
      wr.rotation.x = Math.PI/3;
      ng.add(wr);
      node.userData.warnRing = wr;

      /* Ghost trail */
      var ghosts = [];
      for (var k=1; k<=7; k++) {
        var gs = new THREE.Mesh(
          new THREE.SphereGeometry(5.8*(1-k*0.1),7,7),
          new THREE.MeshBasicMaterial({
            color:'#D85A30', transparent:true,
            opacity:0.22*Math.pow(1-k/8,1.8),
            depthWrite:false,
            blending:THREE.AdditiveBlending
          }));
        grp.add(gs);
        ghosts.push({mesh:gs, delta:k*0.036});
      }

      grp.add(ng);
      ringNodes.push({
        ng:     ng,
        node:   node,
        angle:  3.2700,
        radius: 182.0,
        speed:  0.003000,
        ghosts: ghosts
      });
    })();

    /* Node: Gravity Well Survey */
    (function(){
      var ng = new THREE.Group();
      ng.position.set(
        Math.cos(4.6500)*182.0, 0,
        Math.sin(4.6500)*182.0);

      var node = new THREE.Mesh(
        new THREE.SphereGeometry(5.8, 24, 24),
        new THREE.MeshStandardMaterial({
          color:'#1a1840',
          emissive:'#06041e',
          emissiveIntensity:0.08,
          roughness:0.22,
          metalness:0.82
        }));
      node.userData = {
        label:'Gravity Well Survey',
        status:'Pending',
        ring:'Ring 3',
        isCore:false,
        desc:''
      };
      hoverMeshes.push(node);
      ng.add(node);

      /* Glow shells */
      [[10,0.020],[17,0.010],[26,0.005]]
      .forEach(function(d){
        ng.add(new THREE.Mesh(
          new THREE.SphereGeometry(d[0],12,12),
          new THREE.MeshBasicMaterial({
            color:'#1a1840',transparent:true,
            opacity:d[1],depthWrite:false,
            blending:THREE.AdditiveBlending
          })));
      });

      /* Ghost trail */
      var ghosts = [];
      for (var k=1; k<=7; k++) {
        var gs = new THREE.Mesh(
          new THREE.SphereGeometry(5.8*(1-k*0.1),7,7),
          new THREE.MeshBasicMaterial({
            color:'#1a1840', transparent:true,
            opacity:0*Math.pow(1-k/8,1.8),
            depthWrite:false,
            blending:THREE.AdditiveBlending
          }));
        grp.add(gs);
        ghosts.push({mesh:gs, delta:k*0.036});
      }

      grp.add(ng);
      ringNodes.push({
        ng:     ng,
        node:   node,
        angle:  4.6500,
        radius: 182.0,
        speed:  0.003000,
        ghosts: ghosts
      });
    })();

    scene.add(grp);
    allRings.push({
      grp:    grp,
      nodes:  ringNodes,
      speed:  0.003000,
      radius: 182.0
    });
  })();

  /* ── Orbital Ring 4 (r=226) ── */
  (function(){
    var grp = new THREE.Group();
    grp.rotation.x = 0.0000;
    grp.rotation.z = 0.0000;

    /* Ring torus */
    grp.add(new THREE.Mesh(
      new THREE.TorusGeometry(226.0, 0.55, 8, 140),
      new THREE.MeshBasicMaterial({
        color:'#E05090', transparent:true,
        opacity:0.15, depthWrite:false
      })));

    /* Glow torus */
    grp.add(new THREE.Mesh(
      new THREE.TorusGeometry(226.0, 4.2, 8, 140),
      new THREE.MeshBasicMaterial({
        color:'#E05090', transparent:true,
        opacity:0.026, depthWrite:false,
        blending:THREE.AdditiveBlending
      })));

    /* Tick marks */
    for (var k=0; k<8; k++) {
      var ta = (k/8)*Math.PI*2;
      var active = (Math.random()>0.38);
      var tm = new THREE.Mesh(
        new THREE.SphereGeometry(active?1.6:0.9, 6, 6),
        new THREE.MeshBasicMaterial({
          color: active?'#E05090':'#151530',
          transparent:true,
          opacity: active?0.6:0.12,
          blending: active ?
            THREE.AdditiveBlending:
            THREE.NormalBlending
        }));
      tm.position.set(
        Math.cos(ta)*226.0, 0, Math.sin(ta)*226.0);
      grp.add(tm);
    }

    var ringNodes = [];

    /* Node: Memory Splice Archive */
    (function(){
      var ng = new THREE.Group();
      ng.position.set(
        Math.cos(0.8000)*226.0, 0,
        Math.sin(0.8000)*226.0);

      var node = new THREE.Mesh(
        new THREE.SphereGeometry(5.8, 24, 24),
        new THREE.MeshStandardMaterial({
          color:'#EF9F27',
          emissive:'#996010',
          emissiveIntensity:2.00,
          roughness:0.22,
          metalness:0.82
        }));
      node.userData = {
        label:'Memory Splice Archive',
        status:'In Progress',
        ring:'Ring 4',
        isCore:false,
        desc:''
      };
      hoverMeshes.push(node);
      ng.add(node);

      /* Glow shells */
      [[10,0.180],[17,0.080],[26,0.030]]
      .forEach(function(d){
        ng.add(new THREE.Mesh(
          new THREE.SphereGeometry(d[0],12,12),
          new THREE.MeshBasicMaterial({
            color:'#EF9F27',transparent:true,
            opacity:d[1],depthWrite:false,
            blending:THREE.AdditiveBlending
          })));
      });

      /* Ghost trail */
      var ghosts = [];
      for (var k=1; k<=7; k++) {
        var gs = new THREE.Mesh(
          new THREE.SphereGeometry(5.8*(1-k*0.1),7,7),
          new THREE.MeshBasicMaterial({
            color:'#EF9F27', transparent:true,
            opacity:0.22*Math.pow(1-k/8,1.8),
            depthWrite:false,
            blending:THREE.AdditiveBlending
          }));
        grp.add(gs);
        ghosts.push({mesh:gs, delta:k*0.036});
      }

      grp.add(ng);
      ringNodes.push({
        ng:     ng,
        node:   node,
        angle:  0.8000,
        radius: 226.0,
        speed:  0.001800,
        ghosts: ghosts
      });
    })();

    /* Node: Emotion Engine v9 */
    (function(){
      var ng = new THREE.Group();
      ng.position.set(
        Math.cos(2.3600)*226.0, 0,
        Math.sin(2.3600)*226.0);

      var node = new THREE.Mesh(
        new THREE.SphereGeometry(5.8, 24, 24),
        new THREE.MeshStandardMaterial({
          color:'#1a1840',
          emissive:'#06041e',
          emissiveIntensity:0.08,
          roughness:0.22,
          metalness:0.82
        }));
      node.userData = {
        label:'Emotion Engine v9',
        status:'Pending',
        ring:'Ring 4',
        isCore:false,
        desc:''
      };
      hoverMeshes.push(node);
      ng.add(node);

      /* Glow shells */
      [[10,0.020],[17,0.010],[26,0.005]]
      .forEach(function(d){
        ng.add(new THREE.Mesh(
          new THREE.SphereGeometry(d[0],12,12),
          new THREE.MeshBasicMaterial({
            color:'#1a1840',transparent:true,
            opacity:d[1],depthWrite:false,
            blending:THREE.AdditiveBlending
          })));
      });

      /* Ghost trail */
      var ghosts = [];
      for (var k=1; k<=7; k++) {
        var gs = new THREE.Mesh(
          new THREE.SphereGeometry(5.8*(1-k*0.1),7,7),
          new THREE.MeshBasicMaterial({
            color:'#1a1840', transparent:true,
            opacity:0*Math.pow(1-k/8,1.8),
            depthWrite:false,
            blending:THREE.AdditiveBlending
          }));
        grp.add(gs);
        ghosts.push({mesh:gs, delta:k*0.036});
      }

      grp.add(ng);
      ringNodes.push({
        ng:     ng,
        node:   node,
        angle:  2.3600,
        radius: 226.0,
        speed:  0.001800,
        ghosts: ghosts
      });
    })();

    /* Node: Dream Protocol Sync */
    (function(){
      var ng = new THREE.Group();
      ng.position.set(
        Math.cos(3.9200)*226.0, 0,
        Math.sin(3.9200)*226.0);

      var node = new THREE.Mesh(
        new THREE.SphereGeometry(5.8, 24, 24),
        new THREE.MeshStandardMaterial({
          color:'#D85A30',
          emissive:'#991510',
          emissiveIntensity:2.20,
          roughness:0.22,
          metalness:0.82
        }));
      node.userData = {
        label:'Dream Protocol Sync',
        status:'Overdue',
        ring:'Ring 4',
        isCore:false,
        desc:''
      };
      hoverMeshes.push(node);
      ng.add(node);

      /* Glow shells */
      [[10,0.180],[17,0.080],[26,0.030]]
      .forEach(function(d){
        ng.add(new THREE.Mesh(
          new THREE.SphereGeometry(d[0],12,12),
          new THREE.MeshBasicMaterial({
            color:'#D85A30',transparent:true,
            opacity:d[1],depthWrite:false,
            blending:THREE.AdditiveBlending
          })));
      });

      /* Warn ring */
      var wr = new THREE.Mesh(
        new THREE.TorusGeometry(9.5, 0.45, 8, 44),
        new THREE.MeshBasicMaterial({
          color:'#D85A30', transparent:true,
          opacity:0.4, depthWrite:false,
          blending:THREE.AdditiveBlending
        }));
      wr.rotation.x = Math.PI/3;
      ng.add(wr);
      node.userData.warnRing = wr;

      /* Ghost trail */
      var ghosts = [];
      for (var k=1; k<=7; k++) {
        var gs = new THREE.Mesh(
          new THREE.SphereGeometry(5.8*(1-k*0.1),7,7),
          new THREE.MeshBasicMaterial({
            color:'#D85A30', transparent:true,
            opacity:0.22*Math.pow(1-k/8,1.8),
            depthWrite:false,
            blending:THREE.AdditiveBlending
          }));
        grp.add(gs);
        ghosts.push({mesh:gs, delta:k*0.036});
      }

      grp.add(ng);
      ringNodes.push({
        ng:     ng,
        node:   node,
        angle:  3.9200,
        radius: 226.0,
        speed:  0.001800,
        ghosts: ghosts
      });
    })();

    /* Node: Consciousness Fork */
    (function(){
      var ng = new THREE.Group();
      ng.position.set(
        Math.cos(5.4800)*226.0, 0,
        Math.sin(5.4800)*226.0);

      var node = new THREE.Mesh(
        new THREE.SphereGeometry(5.8, 24, 24),
        new THREE.MeshStandardMaterial({
          color:'#1a1840',
          emissive:'#06041e',
          emissiveIntensity:0.08,
          roughness:0.22,
          metalness:0.82
        }));
      node.userData = {
        label:'Consciousness Fork',
        status:'Pending',
        ring:'Ring 4',
        isCore:false,
        desc:''
      };
      hoverMeshes.push(node);
      ng.add(node);

      /* Glow shells */
      [[10,0.020],[17,0.010],[26,0.005]]
      .forEach(function(d){
        ng.add(new THREE.Mesh(
          new THREE.SphereGeometry(d[0],12,12),
          new THREE.MeshBasicMaterial({
            color:'#1a1840',transparent:true,
            opacity:d[1],depthWrite:false,
            blending:THREE.AdditiveBlending
          })));
      });

      /* Ghost trail */
      var ghosts = [];
      for (var k=1; k<=7; k++) {
        var gs = new THREE.Mesh(
          new THREE.SphereGeometry(5.8*(1-k*0.1),7,7),
          new THREE.MeshBasicMaterial({
            color:'#1a1840', transparent:true,
            opacity:0*Math.pow(1-k/8,1.8),
            depthWrite:false,
            blending:THREE.AdditiveBlending
          }));
        grp.add(gs);
        ghosts.push({mesh:gs, delta:k*0.036});
      }

      grp.add(ng);
      ringNodes.push({
        ng:     ng,
        node:   node,
        angle:  5.4800,
        radius: 226.0,
        speed:  0.001800,
        ghosts: ghosts
      });
    })();

    scene.add(grp);
    allRings.push({
      grp:    grp,
      nodes:  ringNodes,
      speed:  0.001800,
      radius: 226.0
    });
  })();

  /* ── Orbital Ring 5 (r=272) ── */
  (function(){
    var grp = new THREE.Group();
    grp.rotation.x = 0.0000;
    grp.rotation.z = 0.0000;

    /* Ring torus */
    grp.add(new THREE.Mesh(
      new THREE.TorusGeometry(272.0, 0.55, 8, 140),
      new THREE.MeshBasicMaterial({
        color:'#D85A30', transparent:true,
        opacity:0.15, depthWrite:false
      })));

    /* Glow torus */
    grp.add(new THREE.Mesh(
      new THREE.TorusGeometry(272.0, 4.2, 8, 140),
      new THREE.MeshBasicMaterial({
        color:'#D85A30', transparent:true,
        opacity:0.026, depthWrite:false,
        blending:THREE.AdditiveBlending
      })));

    /* Completion arc 50% */
    (function(){
      var ae = 0.500000 * Math.PI * 2;
      var segs = Math.max(16,
        Math.floor(ae/(Math.PI*2)*160));
      var pts = [];
      for (var i=0; i<=segs; i++) {
        var a = ae * (i/segs);
        pts.push(new THREE.Vector3(
          Math.cos(a)*272.0, 0,
          Math.sin(a)*272.0));
      }
      var curve = new THREE.CatmullRomCurve3(pts, false);
      [[1.8,0.82],[6.5,0.10],[13,0.032]]
      .forEach(function(d){
        grp.add(new THREE.Mesh(
          new THREE.TubeGeometry(curve,segs,d[0],7,false),
          new THREE.MeshBasicMaterial({
            color:'#D85A30', transparent:true,
            opacity:d[1], depthWrite:false,
            blending:THREE.AdditiveBlending
          })));
      });
    })();

    /* Tick marks */
    for (var k=0; k<8; k++) {
      var ta = (k/8)*Math.PI*2;
      var active = (Math.random()>0.38);
      var tm = new THREE.Mesh(
        new THREE.SphereGeometry(active?1.6:0.9, 6, 6),
        new THREE.MeshBasicMaterial({
          color: active?'#D85A30':'#151530',
          transparent:true,
          opacity: active?0.6:0.12,
          blending: active ?
            THREE.AdditiveBlending:
            THREE.NormalBlending
        }));
      tm.position.set(
        Math.cos(ta)*272.0, 0, Math.sin(ta)*272.0);
      grp.add(tm);
    }

    var ringNodes = [];

    /* Node: Multiverse Anchor Alpha */
    (function(){
      var ng = new THREE.Group();
      ng.position.set(
        Math.cos(1.0000)*272.0, 0,
        Math.sin(1.0000)*272.0);

      var node = new THREE.Mesh(
        new THREE.SphereGeometry(5.8, 24, 24),
        new THREE.MeshStandardMaterial({
          color:'#8B7FE8',
          emissive:'#4030cc',
          emissiveIntensity:1.20,
          roughness:0.22,
          metalness:0.82
        }));
      node.userData = {
        label:'Multiverse Anchor Alpha',
        status:'Completed',
        ring:'Ring 5',
        isCore:false,
        desc:''
      };
      hoverMeshes.push(node);
      ng.add(node);

      /* Glow shells */
      [[10,0.180],[17,0.080],[26,0.030]]
      .forEach(function(d){
        ng.add(new THREE.Mesh(
          new THREE.SphereGeometry(d[0],12,12),
          new THREE.MeshBasicMaterial({
            color:'#8B7FE8',transparent:true,
            opacity:d[1],depthWrite:false,
            blending:THREE.AdditiveBlending
          })));
      });

      /* Pulse ring */
      var pr = new THREE.Mesh(
        new THREE.TorusGeometry(8.5, 0.55, 8, 44),
        new THREE.MeshBasicMaterial({
          color:'#D85A30', transparent:true,
          opacity:0.55, depthWrite:false,
          blending:THREE.AdditiveBlending
        }));
      pr.rotation.x = Math.PI/2;
      ng.add(pr);
      node.userData.pulseRing = pr;

      /* Ghost trail */
      var ghosts = [];
      for (var k=1; k<=7; k++) {
        var gs = new THREE.Mesh(
          new THREE.SphereGeometry(5.8*(1-k*0.1),7,7),
          new THREE.MeshBasicMaterial({
            color:'#8B7FE8', transparent:true,
            opacity:0.22*Math.pow(1-k/8,1.8),
            depthWrite:false,
            blending:THREE.AdditiveBlending
          }));
        grp.add(gs);
        ghosts.push({mesh:gs, delta:k*0.036});
      }

      grp.add(ng);
      ringNodes.push({
        ng:     ng,
        node:   node,
        angle:  1.0000,
        radius: 272.0,
        speed:  0.000900,
        ghosts: ghosts
      });
    })();

    /* Node: Reality Tear Seal */
    (function(){
      var ng = new THREE.Group();
      ng.position.set(
        Math.cos(2.5700)*272.0, 0,
        Math.sin(2.5700)*272.0);

      var node = new THREE.Mesh(
        new THREE.SphereGeometry(5.8, 24, 24),
        new THREE.MeshStandardMaterial({
          color:'#8B7FE8',
          emissive:'#4030cc',
          emissiveIntensity:1.20,
          roughness:0.22,
          metalness:0.82
        }));
      node.userData = {
        label:'Reality Tear Seal',
        status:'Completed',
        ring:'Ring 5',
        isCore:false,
        desc:''
      };
      hoverMeshes.push(node);
      ng.add(node);

      /* Glow shells */
      [[10,0.180],[17,0.080],[26,0.030]]
      .forEach(function(d){
        ng.add(new THREE.Mesh(
          new THREE.SphereGeometry(d[0],12,12),
          new THREE.MeshBasicMaterial({
            color:'#8B7FE8',transparent:true,
            opacity:d[1],depthWrite:false,
            blending:THREE.AdditiveBlending
          })));
      });

      /* Pulse ring */
      var pr = new THREE.Mesh(
        new THREE.TorusGeometry(8.5, 0.55, 8, 44),
        new THREE.MeshBasicMaterial({
          color:'#D85A30', transparent:true,
          opacity:0.55, depthWrite:false,
          blending:THREE.AdditiveBlending
        }));
      pr.rotation.x = Math.PI/2;
      ng.add(pr);
      node.userData.pulseRing = pr;

      /* Ghost trail */
      var ghosts = [];
      for (var k=1; k<=7; k++) {
        var gs = new THREE.Mesh(
          new THREE.SphereGeometry(5.8*(1-k*0.1),7,7),
          new THREE.MeshBasicMaterial({
            color:'#8B7FE8', transparent:true,
            opacity:0.22*Math.pow(1-k/8,1.8),
            depthWrite:false,
            blending:THREE.AdditiveBlending
          }));
        grp.add(gs);
        ghosts.push({mesh:gs, delta:k*0.036});
      }

      grp.add(ng);
      ringNodes.push({
        ng:     ng,
        node:   node,
        angle:  2.5700,
        radius: 272.0,
        speed:  0.000900,
        ghosts: ghosts
      });
    })();

    /* Node: Causality Loop Break */
    (function(){
      var ng = new THREE.Group();
      ng.position.set(
        Math.cos(4.1400)*272.0, 0,
        Math.sin(4.1400)*272.0);

      var node = new THREE.Mesh(
        new THREE.SphereGeometry(5.8, 24, 24),
        new THREE.MeshStandardMaterial({
          color:'#EF9F27',
          emissive:'#996010',
          emissiveIntensity:2.00,
          roughness:0.22,
          metalness:0.82
        }));
      node.userData = {
        label:'Causality Loop Break',
        status:'In Progress',
        ring:'Ring 5',
        isCore:false,
        desc:''
      };
      hoverMeshes.push(node);
      ng.add(node);

      /* Glow shells */
      [[10,0.180],[17,0.080],[26,0.030]]
      .forEach(function(d){
        ng.add(new THREE.Mesh(
          new THREE.SphereGeometry(d[0],12,12),
          new THREE.MeshBasicMaterial({
            color:'#EF9F27',transparent:true,
            opacity:d[1],depthWrite:false,
            blending:THREE.AdditiveBlending
          })));
      });

      /* Ghost trail */
      var ghosts = [];
      for (var k=1; k<=7; k++) {
        var gs = new THREE.Mesh(
          new THREE.SphereGeometry(5.8*(1-k*0.1),7,7),
          new THREE.MeshBasicMaterial({
            color:'#EF9F27', transparent:true,
            opacity:0.22*Math.pow(1-k/8,1.8),
            depthWrite:false,
            blending:THREE.AdditiveBlending
          }));
        grp.add(gs);
        ghosts.push({mesh:gs, delta:k*0.036});
      }

      grp.add(ng);
      ringNodes.push({
        ng:     ng,
        node:   node,
        angle:  4.1400,
        radius: 272.0,
        speed:  0.000900,
        ghosts: ghosts
      });
    })();

    /* Node: Entropy Suppressor Zeta */
    (function(){
      var ng = new THREE.Group();
      ng.position.set(
        Math.cos(5.7100)*272.0, 0,
        Math.sin(5.7100)*272.0);

      var node = new THREE.Mesh(
        new THREE.SphereGeometry(5.8, 24, 24),
        new THREE.MeshStandardMaterial({
          color:'#1a1840',
          emissive:'#06041e',
          emissiveIntensity:0.08,
          roughness:0.22,
          metalness:0.82
        }));
      node.userData = {
        label:'Entropy Suppressor Zeta',
        status:'Pending',
        ring:'Ring 5',
        isCore:false,
        desc:''
      };
      hoverMeshes.push(node);
      ng.add(node);

      /* Glow shells */
      [[10,0.020],[17,0.010],[26,0.005]]
      .forEach(function(d){
        ng.add(new THREE.Mesh(
          new THREE.SphereGeometry(d[0],12,12),
          new THREE.MeshBasicMaterial({
            color:'#1a1840',transparent:true,
            opacity:d[1],depthWrite:false,
            blending:THREE.AdditiveBlending
          })));
      });

      /* Ghost trail */
      var ghosts = [];
      for (var k=1; k<=7; k++) {
        var gs = new THREE.Mesh(
          new THREE.SphereGeometry(5.8*(1-k*0.1),7,7),
          new THREE.MeshBasicMaterial({
            color:'#1a1840', transparent:true,
            opacity:0*Math.pow(1-k/8,1.8),
            depthWrite:false,
            blending:THREE.AdditiveBlending
          }));
        grp.add(gs);
        ghosts.push({mesh:gs, delta:k*0.036});
      }

      grp.add(ng);
      ringNodes.push({
        ng:     ng,
        node:   node,
        angle:  5.7100,
        radius: 272.0,
        speed:  0.000900,
        ghosts: ghosts
      });
    })();

    scene.add(grp);
    allRings.push({
      grp:    grp,
      nodes:  ringNodes,
      speed:  0.000900,
      radius: 272.0
    });
  })();

  /* ── Legend ── */
  (function(){
    var legend = document.getElementById('epl-legend');
    if (!legend) return;
    var items = [
      {c:'#8B7FE8', l:'Completed'},
      {c:'#EF9F27', l:'In Progress'},
      {c:'#D85A30', l:'Overdue'},
      {c:'#1a1840', l:'Pending',
       b:'0.5px solid #4a45a0'}
    ];
    items.forEach(function(it){
      var d = document.createElement('div');
      d.className = 'leg-item';
      var dot = document.createElement('div');
      dot.className = 'leg-dot';
      dot.style.background = it.c;
      if (it.b) dot.style.border = it.b;
      var txt = document.createElement('span');
      txt.className = 'leg-txt';
      txt.textContent = it.l;
      d.appendChild(dot);
      d.appendChild(txt);
      legend.appendChild(d);
    });
  })();

  /* ── Animation loop ── */
  var frame = 0;

  function animate() {
    requestAnimationFrame(animate);
    frame++;
    var t = frame * 0.016;

    /* Auto-rotate after idle */
    if (!isDragging && Date.now()-lastInt > 4500)
      autoRot = true;
    if (autoRot) tgtSph.th += 0.00055;

    /* Smooth camera */
    sph.th += (tgtSph.th - sph.th) * 0.065;
    sph.ph += (tgtSph.ph - sph.ph) * 0.065;
    sph.r  += (tgtSph.r  - sph.r)  * 0.065;
    updateCam();

    /* Core star pulse */
    if (coreMesh && coreGrp) {
      var p = 1 + Math.sin(t*1.2)*0.07;
      coreMesh.scale.setScalar(p);
      if (cLight) cLight.intensity = 10+Math.sin(t*1.2)*3;
      coreGrp.rotation.y += 0.0012;
      armRings.forEach(function(ar){
        ar.rotation.y += ar.userData.rspd;
      });
      /* Hover scale on core */
      var coreHov = (hovLabel === coreMesh.userData.label);
      var cc  = coreMesh.scale.x / p;
      var cct = coreHov ? 1.22 : 1.0;
      coreMesh.scale.setScalar((cc+(cct-cc)*0.10)*p);
    }

    /* Orbital nodes */
    allRings.forEach(function(ring) {
      ring.nodes.forEach(function(obj) {
        obj.angle += ring.speed;
        obj.ng.position.set(
          Math.cos(obj.angle)*ring.radius, 0,
          Math.sin(obj.angle)*ring.radius);

        /* Ghost trail */
        obj.ghosts.forEach(function(g) {
          var ga = obj.angle - g.delta;
          g.mesh.position.set(
            Math.cos(ga)*ring.radius, 0,
            Math.sin(ga)*ring.radius);
        });

        /* Pulse ring */
        if (obj.node.userData.pulseRing) {
          var pr = obj.node.userData.pulseRing;
          var ps = 1+0.18*Math.sin(t*2.2+obj.angle*2);
          pr.scale.setScalar(ps);
          pr.material.opacity =
            0.3+0.35*Math.sin(t*2.2+obj.angle*2);
          pr.rotation.y += 0.025;
        }

        /* Warn ring */
        if (obj.node.userData.warnRing) {
          var wr = obj.node.userData.warnRing;
          wr.rotation.y += 0.04;
          wr.material.opacity =
            0.25+0.25*Math.abs(Math.sin(t*3+obj.angle));
        }

        /* Hover scale */
        var isHov=(hovLabel===obj.node.userData.label);
        var cur = obj.node.scale.x;
        var ts  = isHov ? 1.58 : 1.0;
        obj.node.scale.setScalar(cur+(ts-cur)*0.12);
      });
    });

    renderer.render(scene, camera);
  }

  animate();
})();
