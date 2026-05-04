# factory_link.ps1
# Post-processes factory_*.html to inject shared nav + sidebar
# Run after: .\epl.exe factory.EPF

$pages = @(
    [pscustomobject]@{ file="factory_dashboard.html"; label="Dashboard"; icon="&#9672;" },
    [pscustomobject]@{ file="factory_employees.html"; label="Employees"; icon="&#9671;" },
    [pscustomobject]@{ file="factory_bom.html";       label="BOM";       icon="&#9670;" },
    [pscustomobject]@{ file="factory_products.html";  label="Products";  icon="&#9674;" },
    [pscustomobject]@{ file="factory_inventory.html"; label="Inventory"; icon="&#9673;" }
)

$sharedCss = @'
<style id="vt-shell-css">
  :root { --nav-h:56px; --side-w:230px; --bg:#0f172a; --surf:#1e293b; --acc:#6366f1; --acc2:#818cf8; --bdr:#334155; --txt:#f1f5f9; --muted:#94a3b8; --faint:#475569; }
  *, *::before, *::after { box-sizing: border-box; }
  html, body { margin:0; padding:0 !important; min-height:100vh; background:var(--bg); }
  body { display:flex; flex-direction:column; }

  /* ── Topnav ── */
  #vt-nav {
    position:fixed; top:0; left:0; right:0; height:var(--nav-h);
    background:var(--surf); border-bottom:1px solid var(--bdr);
    display:flex; align-items:center; padding:0 1.5rem; gap:1rem;
    z-index:200; backdrop-filter:blur(8px);
  }
  .vt-brand {
    font-family:-apple-system,BlinkMacSystemFont,'Segoe UI',sans-serif;
    font-weight:800; font-size:1.05rem; color:var(--acc2);
    letter-spacing:.04em; margin-right:auto; white-space:nowrap;
  }
  .vt-brand small { color:var(--faint); font-weight:400; font-size:.85rem; }
  .vt-live {
    font-size:.7rem; letter-spacing:.06em; color:var(--faint);
    display:flex; align-items:center; gap:.4rem;
  }
  .vt-dot {
    width:7px; height:7px; border-radius:50%; background:#22d3ee;
    box-shadow:0 0 6px #22d3ee; animation:pulse 2s infinite;
  }
  @keyframes pulse { 0%,100%{opacity:1} 50%{opacity:.4} }

  /* ── Sidebar ── */
  #vt-side {
    position:fixed; top:var(--nav-h); left:0; bottom:0; width:var(--side-w);
    background:var(--surf); border-right:1px solid var(--bdr);
    display:flex; flex-direction:column; z-index:100; overflow-y:auto;
  }
  .vt-sec {
    padding:.9rem 1.25rem .3rem;
    font-size:.6rem; text-transform:uppercase; letter-spacing:.12em; color:var(--faint);
  }
  .vt-lnk {
    display:flex; align-items:center; gap:.65rem;
    padding:.58rem 1rem; margin:.1rem .6rem;
    border-radius:8px; text-decoration:none;
    color:var(--muted); font-size:.875rem;
    transition:background .15s, color .15s;
  }
  .vt-lnk:hover { background:rgba(99,102,241,.1); color:var(--txt); }
  .vt-lnk.on   { background:rgba(99,102,241,.18); color:var(--acc2); font-weight:600; }
  .vt-lnk i    { font-style:normal; font-size:.95rem; width:1.2rem; text-align:center; }
  .vt-spacer   { flex:1; }

  /* Sidebar quick stats */
  .vt-qs    { padding:.5rem 1.25rem; }
  .vt-ql    { font-size:.65rem; color:var(--faint); margin-bottom:.15rem; text-transform:uppercase; letter-spacing:.08em; }
  .vt-qv    { font-size:1.15rem; font-weight:700; margin-bottom:.3rem; }
  .vt-bar   { height:3px; background:rgba(255,255,255,.06); border-radius:2px; }
  .vt-fill  { height:100%; border-radius:2px; }

  /* ── Main area ── */
  #vt-main {
    margin-top:var(--nav-h);
    margin-left:var(--side-w);
    flex:1; padding:2rem;
  }
  .epl-page { max-width:none !important; }
</style>
'@

foreach ($p in $pages) {
    if (-not (Test-Path $p.file)) { Write-Warning "Not found: $($p.file)"; continue }

    $html = Get-Content $p.file -Raw -Encoding UTF8

    # Build sidebar links — current page gets 'on' class
    $links = ""
    foreach ($q in $pages) {
        $cls = if ($q.file -eq $p.file) { " on" } else { "" }
        $links += "  <a href=`"$($q.file)`" class=`"vt-lnk$cls`"><i>$($q.icon)</i>$($q.label)</a>`n"
    }

    $navBlock = @"
$sharedCss
<nav id="vt-nav">
  <div class="vt-brand">VulcanTech <small>Industries</small></div>
  <div class="vt-live"><div class="vt-dot"></div>Zone 7 &nbsp;&middot;&nbsp; Shift Alpha &nbsp;&middot;&nbsp; LIVE</div>
</nav>

<aside id="vt-side">
  <div class="vt-sec">Modules</div>
$links
  <div class="vt-spacer"></div>

  <div class="vt-sec">Live Metrics</div>

  <div class="vt-qs">
    <div class="vt-ql">Shift Efficiency</div>
    <div class="vt-qv" style="color:#6366f1;">81%</div>
    <div class="vt-bar"><div class="vt-fill" style="width:81%;background:#6366f1;"></div></div>
  </div>

  <div class="vt-qs">
    <div class="vt-ql">Orders Filled</div>
    <div class="vt-qv" style="color:#22d3ee;">64 / 87</div>
    <div class="vt-bar"><div class="vt-fill" style="width:73%;background:#22d3ee;"></div></div>
  </div>

  <div class="vt-qs">
    <div class="vt-ql">Raw Stock</div>
    <div class="vt-qv" style="color:#10b981;">2,840 units</div>
    <div class="vt-bar"><div class="vt-fill" style="width:88%;background:#10b981;"></div></div>
  </div>

  <div class="vt-qs" style="margin-bottom:1.25rem;">
    <div class="vt-ql">Inventory Alert</div>
    <div style="font-size:.78rem;font-weight:700;color:#f87171;margin-top:.15rem;">&#9888; REORDER REQUIRED</div>
  </div>
</aside>

<div id="vt-main">
"@

    # Inject nav block after <body> tag
    $html = $html -replace '(<body>\s*)', "<body>`n$navBlock`n"

    # Close vt-main wrapper before </body>
    $html = $html -replace '</body>', "</div>`n</body>"

    Set-Content $p.file $html -Encoding UTF8
    Write-Host "[Linked] $($p.file)" -ForegroundColor Green
}

Write-Host ""
Write-Host "  All factory pages linked. Open factory_dashboard.html to start." -ForegroundColor Cyan
