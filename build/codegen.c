#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "codegen.h"
#include "binding.h"
#include "scene_gen.h"

/* ─────────────────────────────────────────────
   EPL Code Generator — codegen.c  (v1.7)
   Multi-page framework with built-in navigation
   ───────────────────────────────────────────── */


/* ═════════════════════════════════════════════
   THEMES
   ═════════════════════════════════════════════ */
/* ── Friendly multi-word color name → CSS value lookup ── */
static const char *resolve_color_name(const char *name) {
    static const struct { const char *alias; const char *css; } map[] = {
        /* reds / pinks */
        {"hot pink",          "hotpink"},
        {"deep pink",         "deeppink"},
        {"pale pink",         "lightpink"},
        {"coral",             "coral"},
        {"salmon",            "salmon"},
        /* purples / blues */
        {"persian blue",      "#1c39bb"},
        {"royal blue",        "royalblue"},
        {"navy blue",         "navy"},
        {"sky blue",          "skyblue"},
        {"steel blue",        "steelblue"},
        {"midnight blue",     "midnightblue"},
        {"powder blue",       "powderblue"},
        {"slate blue",        "slateblue"},
        {"lavender",          "lavender"},
        {"violet",            "violet"},
        {"plum",              "plum"},
        {"orchid",            "orchid"},
        {"medium purple",     "mediumpurple"},
        /* greens */
        {"forest green",      "forestgreen"},
        {"lime green",        "limegreen"},
        {"olive green",       "olive"},
        {"teal",              "teal"},
        {"sea green",         "seagreen"},
        {"mint green",        "#98ff98"},
        /* oranges / yellows */
        {"gold",              "gold"},
        {"orange red",        "orangered"},
        {"dark orange",       "darkorange"},
        {"amber",             "#ffbf00"},
        /* neutrals */
        {"slate gray",        "slategray"},
        {"charcoal",          "#36454f"},
        {"ivory",             "ivory"},
        {NULL, NULL}
    };
    for (int i = 0; map[i].alias; i++) {
        if (strcmp(name, map[i].alias) == 0) return map[i].css;
    }
    return name;   /* pass through as-is (hex, single-word CSS name, etc.) */
}

ThemeCSS codegen_get_theme(EPFTheme theme, const char *custom_color) {
    ThemeCSS t;
    switch (theme) {
        case THEME_DARK:
            t.bg      = "#0f172a";
            t.surface = "#1e293b";
            t.text    = "#f1f5f9";
            t.subtext = "#94a3b8";
            t.accent  = "#6366f1";
            t.border  = "#334155";
            break;
        case THEME_INDIGO:
            t.bg      = "#eef2ff";
            t.surface = "#ffffff";
            t.text    = "#1e1b4b";
            t.subtext = "#6366f1";
            t.accent  = "#4f46e5";
            t.border  = "#c7d2fe";
            break;
        case THEME_EMERALD:
            t.bg      = "#ecfdf5";
            t.surface = "#ffffff";
            t.text    = "#064e3b";
            t.subtext = "#10b981";
            t.accent  = "#059669";
            t.border  = "#a7f3d0";
            break;
        case THEME_ROSE:
            t.bg      = "#fff1f2";
            t.surface = "#ffffff";
            t.text    = "#881337";
            t.subtext = "#f43f5e";
            t.accent  = "#e11d48";
            t.border  = "#fecdd3";
            break;
        case THEME_CUSTOM: {
            /* Derive a clean, consistent light palette from the custom color.
               The color becomes accent + subtext; bg/surface/border stay neutral. */
            static char acc[64];
            strncpy(acc, resolve_color_name(custom_color), 63);
            t.bg      = "#f8fafc";
            t.surface = "#ffffff";
            t.text    = "#0f172a";
            t.subtext = acc;
            t.accent  = acc;
            t.border  = "#e2e8f0";
            break;
        }
        default: /* THEME_LIGHT */
            t.bg      = "#f8fafc";
            t.surface = "#ffffff";
            t.text    = "#0f172a";
            t.subtext = "#64748b";
            t.accent  = "#6366f1";
            t.border  = "#e2e8f0";
            break;
    }
    /* override accent with explicit 'with color "..."' if provided */
    if (custom_color && strlen(custom_color) > 0 && theme != THEME_CUSTOM)
        t.accent = custom_color;

    t.chart_colors[0] = "#6366f1";
    t.chart_colors[1] = "#f43f5e";
    t.chart_colors[2] = "#10b981";
    t.chart_colors[3] = "#f59e0b";
    t.chart_colors[4] = "#3b82f6";
    t.chart_colors[5] = "#8b5cf6";
    return t;
}


/* ═════════════════════════════════════════════
   HELPERS
   ═════════════════════════════════════════════ */
static void indent(CodegenCtx *ctx) {
    for (int i = 0; i < ctx->indent; i++) fprintf(ctx->out, "  ");
}

/* Write a value as a JS array literal string */
static void write_js_array(CodegenCtx *ctx, Value *list) {
    fprintf(ctx->out, "[");
    if (list && list->type == VAL_LIST) {
        for (int i = 0; i < list->as.list->count; i++) {
            Value *item = list->as.list->items[i];
            if (item->type == VAL_STRING)
                fprintf(ctx->out, "\"%s\"", item->as.string);
            else if (item->type == VAL_NUMBER) {
                if (item->as.number == (long)item->as.number)
                    fprintf(ctx->out, "%ld", (long)item->as.number);
                else
                    fprintf(ctx->out, "%g", item->as.number);
            } else {
                char *s = val_to_string(item);
                fprintf(ctx->out, "\"%s\"", s);
                free(s);
            }
            if (i < list->as.list->count - 1) fprintf(ctx->out, ", ");
        }
    }
    fprintf(ctx->out, "]");
}


/* ═════════════════════════════════════════════
   HTML HEAD — embedded CSS + Chart.js CDN
   ═════════════════════════════════════════════ */
static void write_head(CodegenCtx *ctx, const char *title) {
    ThemeCSS *t = &ctx->theme;
    fprintf(ctx->out,
        "<!DOCTYPE html>\n"
        "<html lang=\"en\">\n"
        "<head>\n"
        "  <meta charset=\"UTF-8\">\n"
        "  <meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">\n"
        "  <title>%s</title>\n"
        "  <script src=\"https://cdn.jsdelivr.net/npm/chart.js@4.4.0/dist/chart.umd.min.js\"></script>\n"
        "  <style>\n"
        "    *, *::before, *::after { box-sizing: border-box; margin: 0; padding: 0; }\n"
        "    body {\n"
        "      font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', sans-serif;\n"
        "      background: %s;\n"
        "      color: %s;\n"
        "      line-height: 1.6;\n"
        "      padding: 2rem;\n"
        "    }\n"
        "    .epl-page { max-width: 1100px; margin: 0 auto; }\n"
        "    h1 { font-size: 2rem; font-weight: 700; color: %s; margin-bottom: 0.25rem; }\n"
        "    h2 { font-size: 1.25rem; font-weight: 400; color: %s; margin-bottom: 1.5rem; }\n"
        "    h3 { font-size: 1.1rem; font-weight: 600; color: %s; margin-bottom: 1rem; }\n"
        "    h4 { font-size: 0.9rem; font-weight: 600; color: %s; margin-bottom: 0.5rem; }\n"
        "    p  { color: %s; margin-bottom: 1rem; }\n"
        "    strong { font-weight: 700; color: %s; }\n"
        "    hr { border: none; border-top: 1px solid %s; margin: 1.5rem 0; }\n"
        "    .epl-spacer { height: 1.5rem; }\n"
        "    .epl-section {\n"
        "      background: %s;\n"
        "      border: 1px solid %s;\n"
        "      border-radius: 12px;\n"
        "      padding: 1.5rem;\n"
        "      margin-bottom: 1.5rem;\n"
        "    }\n"
        "    .epl-row {\n"
        "      display: grid;\n"
        "      grid-template-columns: repeat(auto-fit, minmax(200px, 1fr));\n"
        "      gap: 1rem;\n"
        "      margin-bottom: 1rem;\n"
        "    }\n"
        "    .epl-stat {\n"
        "      background: %s;\n"
        "      border: 1px solid %s;\n"
        "      border-radius: 10px;\n"
        "      padding: 1.25rem 1.5rem;\n"
        "      text-align: center;\n"
        "    }\n"
        "    .epl-stat-label { font-size: 0.8rem; color: %s; text-transform: uppercase; letter-spacing: 0.05em; margin-bottom: 0.4rem; }\n"
        "    .epl-stat-value { font-size: 2rem; font-weight: 700; color: %s; }\n"
        "    .epl-chart-wrap { margin-bottom: 1.5rem; }\n"
        "    .epl-chart-title { font-size: 0.85rem; font-weight: 600; color: %s; margin-bottom: 0.5rem; }\n"
        "    canvas { max-height: 320px; width: 100%% !important; }\n"
        "    table { width: 100%%; border-collapse: collapse; margin-bottom: 1rem; font-size: 0.9rem; }\n"
        "    th { background: %s; color: %s; padding: 0.65rem 1rem; text-align: left; font-weight: 600; }\n"
        "    td { padding: 0.6rem 1rem; border-bottom: 1px solid %s; color: %s; }\n"
        "    tr:last-child td { border-bottom: none; }\n"
        "    tr:hover td { background: %s; }\n"
        "    .epl-report { font-size: 0.9rem; color: %s; }\n"
        "    .epl-report-row { display: flex; justify-content: space-between; padding: 0.4rem 0; border-bottom: 1px solid %s; }\n"
        "    .epl-report-row:last-child { border-bottom: none; }\n"
        "  </style>\n"
        "</head>\n"
        "<body>\n"
        "<div class=\"epl-page\">\n",
        title,
        t->bg, t->text,
        t->text, t->subtext, t->text, t->subtext,
        t->subtext, t->text,
        t->border,
        t->surface, t->border,
        t->surface, t->border,
        t->subtext, t->accent,
        t->subtext,
        t->accent, t->text, t->border, t->text,
        t->bg,
        t->subtext, t->border
    );
}

static void write_foot(CodegenCtx *ctx) {
    fprintf(ctx->out, "</div>\n</body>\n</html>\n");
}


/* ═════════════════════════════════════════════
   MULTIPAGE SHELL — nav bar + sidebar
   Written immediately after <body> when the
   manifest contains more than one page.
   ═════════════════════════════════════════════ */

/* Icon set cycled per page index */
static const char *page_icons[] = {
    "&#9673;", "&#9671;", "&#9672;", "&#9670;", "&#9674;",
    "&#9650;", "&#9660;", "&#9668;", "&#9658;", "&#9632;"
};

static void write_shell_open(CodegenCtx *ctx, PageManifest *mf) {
    ThemeCSS *t = &ctx->theme;
    int       ai = mf->active;   /* index of current page */

    /* ── Shared shell CSS ── */
    fprintf(ctx->out,
        "<style id=\"epl-shell-css\">\n"
        "  :root {\n"
        "    --nav-h: 56px;\n"
        "    --side-w: 220px;\n"
        "    --shell-bg: %s;\n"
        "    --shell-surf: %s;\n"
        "    --shell-acc: %s;\n"
        "    --shell-bdr: %s;\n"
        "    --shell-txt: %s;\n"
        "    --shell-muted: %s;\n"
        "  }\n"
        "  *, *::before, *::after { box-sizing: border-box; }\n"
        "  html, body { margin: 0; padding: 0 !important; min-height: 100vh; background: var(--shell-bg); }\n"
        "  body { display: flex; flex-direction: column; }\n"
        /* ── Top nav ── */
        "  #epl-nav {\n"
        "    position: fixed; top: 0; left: 0; right: 0; height: var(--nav-h);\n"
        "    background: var(--shell-surf); border-bottom: 1px solid var(--shell-bdr);\n"
        "    display: flex; align-items: center; padding: 0 1.5rem; gap: 1rem;\n"
        "    z-index: 200; backdrop-filter: blur(8px);\n"
        "  }\n"
        "  .epl-brand {\n"
        "    font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', sans-serif;\n"
        "    font-weight: 800; font-size: 1rem; color: var(--shell-acc);\n"
        "    letter-spacing: .04em; margin-right: auto; white-space: nowrap;\n"
        "  }\n"
        "  .epl-brand small { color: var(--shell-muted); font-weight: 400; font-size: .8rem; }\n"
        "  .epl-live-badge {\n"
        "    font-size: .7rem; letter-spacing: .06em; color: var(--shell-muted);\n"
        "    display: flex; align-items: center; gap: .4rem;\n"
        "  }\n"
        "  .epl-live-dot {\n"
        "    width: 7px; height: 7px; border-radius: 50%; background: #22d3ee;\n"
        "    box-shadow: 0 0 6px #22d3ee; animation: epl-pulse 2s infinite;\n"
        "  }\n"
        "  @keyframes epl-pulse { 0%%,100%%{opacity:1} 50%%{opacity:.4} }\n"
        /* ── Sidebar ── */
        "  #epl-side {\n"
        "    position: fixed; top: var(--nav-h); left: 0; bottom: 0; width: var(--side-w);\n"
        "    background: var(--shell-surf); border-right: 1px solid var(--shell-bdr);\n"
        "    display: flex; flex-direction: column; z-index: 100; overflow-y: auto;\n"
        "  }\n"
        "  .epl-side-section {\n"
        "    padding: .9rem 1.25rem .3rem;\n"
        "    font-size: .6rem; text-transform: uppercase; letter-spacing: .12em;\n"
        "    color: var(--shell-muted);\n"
        "  }\n"
        "  .epl-nav-link {\n"
        "    display: flex; align-items: center; gap: .65rem;\n"
        "    padding: .58rem 1rem; margin: .1rem .6rem;\n"
        "    border-radius: 8px; text-decoration: none;\n"
        "    color: var(--shell-muted); font-size: .875rem;\n"
        "    transition: background .15s, color .15s;\n"
        "  }\n"
        "  .epl-nav-link:hover { background: rgba(99,102,241,.1); color: var(--shell-txt); }\n"
        "  .epl-nav-link.active { background: rgba(99,102,241,.18); color: var(--shell-acc); font-weight: 600; }\n"
        "  .epl-nav-link i { font-style: normal; font-size: .95rem; width: 1.2rem; text-align: center; }\n"
        /* ── Main content area ── */
        "  #epl-main {\n"
        "    margin-top: var(--nav-h);\n"
        "    margin-left: var(--side-w);\n"
        "    flex: 1; padding: 2rem;\n"
        "  }\n"
        "  .epl-page { max-width: none !important; }\n"
        "</style>\n",
        t->bg, t->surface, t->accent, t->border, t->text, t->subtext
    );

    /* ── Top nav bar ── */
    fprintf(ctx->out,
        "<nav id=\"epl-nav\">\n"
        "  <div class=\"epl-brand\">%s <small>%d pages</small></div>\n"
        "  <div class=\"epl-live-badge\"><div class=\"epl-live-dot\"></div>EPL &nbsp;&middot;&nbsp; v1.7</div>\n"
        "</nav>\n",
        mf->pages[ai].label,
        mf->count
    );

    /* ── Sidebar with page links ── */
    fprintf(ctx->out, "<aside id=\"epl-side\">\n");
    fprintf(ctx->out, "  <div class=\"epl-side-section\">Pages</div>\n");
    for (int i = 0; i < mf->count; i++) {
        const char *cls = (i == ai) ? " active" : "";
        fprintf(ctx->out,
            "  <a href=\"%s\" class=\"epl-nav-link%s\"><i>%s</i>%s</a>\n",
            mf->pages[i].filename,
            cls,
            mf->pages[i].icon,
            mf->pages[i].label
        );
    }
    fprintf(ctx->out, "</aside>\n");

    /* ── Main content wrapper ── */
    fprintf(ctx->out, "<div id=\"epl-main\">\n");
}

static void write_shell_close(CodegenCtx *ctx) {
    fprintf(ctx->out, "</div>\n");   /* close #epl-main */
}


/* ═════════════════════════════════════════════
   NODE GENERATORS
   ═════════════════════════════════════════════ */
static void gen_heading(CodegenCtx *ctx, ASTNode *node) {
    Value *v = eval(ctx->interp, node->as.text_out.value, ctx->env);
    char  *s = val_to_string(v);
    indent(ctx); fprintf(ctx->out, "<h1>%s</h1>\n", s);
    free(s); val_free(v);
}

static void gen_subheading(CodegenCtx *ctx, ASTNode *node) {
    Value *v = eval(ctx->interp, node->as.text_out.value, ctx->env);
    char  *s = val_to_string(v);
    indent(ctx); fprintf(ctx->out, "<h2>%s</h2>\n", s);
    free(s); val_free(v);
}

static void gen_paragraph(CodegenCtx *ctx, ASTNode *node) {
    Value *v = eval(ctx->interp, node->as.text_out.value, ctx->env);
    char  *s = val_to_string(v);
    indent(ctx); fprintf(ctx->out, "<p>%s</p>\n", s);
    free(s); val_free(v);
}

static void gen_label(CodegenCtx *ctx, ASTNode *node) {
    Value *v = eval(ctx->interp, node->as.text_out.value, ctx->env);
    char  *s = val_to_string(v);
    indent(ctx); fprintf(ctx->out, "<h4>%s</h4>\n", s);
    free(s); val_free(v);
}

static void gen_bold(CodegenCtx *ctx, ASTNode *node) {
    Value *v = eval(ctx->interp, node->as.text_out.value, ctx->env);
    char  *s = val_to_string(v);
    indent(ctx); fprintf(ctx->out, "<strong>%s</strong>\n", s);
    free(s); val_free(v);
}

static void gen_divider(CodegenCtx *ctx) {
    indent(ctx); fprintf(ctx->out, "<hr>\n");
}

static void gen_spacer(CodegenCtx *ctx) {
    indent(ctx); fprintf(ctx->out, "<div class=\"epl-spacer\"></div>\n");
}

static void gen_stat(CodegenCtx *ctx, ASTNode *node) {
    Value *v   = eval(ctx->interp, node->as.make_stat.value, ctx->env);
    char  *val = val_to_string(v);
    indent(ctx);
    fprintf(ctx->out,
        "<div class=\"epl-stat\">"
        "<div class=\"epl-stat-label\">%s</div>"
        "<div class=\"epl-stat-value\">%s</div>"
        "</div>\n",
        node->as.make_stat.label, val);
    free(val); val_free(v);
}

static void gen_chart(CodegenCtx *ctx, ASTNode *node) {
    int   id     = ++ctx->chart_count;
    char  ctype[8];
    const char *color = ctx->theme.chart_colors[id % 6];

    switch (node->as.make_chart.type) {
        case CHART_LINE: strcpy(ctype, "line"); break;
        case CHART_BAR:  strcpy(ctype, "bar");  break;
        case CHART_PIE:  strcpy(ctype, "pie");  break;
    }

    Value *xdata = eval(ctx->interp, node->as.make_chart.x_data, ctx->env);
    Value *ydata = eval(ctx->interp, node->as.make_chart.y_data, ctx->env);

    indent(ctx);
    fprintf(ctx->out, "<div class=\"epl-chart-wrap\">\n");
    ctx->indent++;

    if (strlen(node->as.make_chart.title) > 0) {
        indent(ctx);
        fprintf(ctx->out,
            "<div class=\"epl-chart-title\">%s</div>\n",
            node->as.make_chart.title);
    }

    indent(ctx);
    fprintf(ctx->out, "<canvas id=\"chart%d\"></canvas>\n", id);
    indent(ctx);
    fprintf(ctx->out, "<script>\n");
    ctx->indent++;
    indent(ctx);
    fprintf(ctx->out, "new Chart(document.getElementById('chart%d'), {\n", id);
    ctx->indent++;
    indent(ctx); fprintf(ctx->out, "type: '%s',\n", ctype);
    indent(ctx); fprintf(ctx->out, "data: {\n");
    ctx->indent++;
    indent(ctx); fprintf(ctx->out, "labels: ");
    write_js_array(ctx, xdata);
    fprintf(ctx->out, ",\n");
    indent(ctx); fprintf(ctx->out, "datasets: [{\n");
    ctx->indent++;
    indent(ctx); fprintf(ctx->out, "label: '%s',\n",
        strlen(node->as.make_chart.y_label) > 0
        ? node->as.make_chart.y_label : "Value");
    indent(ctx); fprintf(ctx->out, "data: ");
    write_js_array(ctx, ydata);
    fprintf(ctx->out, ",\n");

    if (node->as.make_chart.type == CHART_PIE) {
        indent(ctx);
        fprintf(ctx->out, "backgroundColor: [");
        for (int i = 0; i < 6; i++) {
            fprintf(ctx->out, "'%s'", ctx->theme.chart_colors[i]);
            if (i < 5) fprintf(ctx->out, ", ");
        }
        fprintf(ctx->out, "],\n");
    } else {
        indent(ctx);
        fprintf(ctx->out,
            "borderColor: '%s', backgroundColor: '%s33',\n"
            "fill: %s, tension: 0.4, borderWidth: 2\n",
            color, color,
            node->as.make_chart.type == CHART_LINE ? "true" : "false");
    }

    ctx->indent--;
    indent(ctx); fprintf(ctx->out, "}]\n");
    ctx->indent--;
    indent(ctx); fprintf(ctx->out, "},\n");
    indent(ctx); fprintf(ctx->out, "options: {\n");
    ctx->indent++;
    indent(ctx); fprintf(ctx->out, "responsive: true,\n");
    indent(ctx); fprintf(ctx->out, "plugins: { legend: { display: true } }\n");
    ctx->indent--;
    indent(ctx); fprintf(ctx->out, "}\n");
    ctx->indent--;
    indent(ctx); fprintf(ctx->out, "});\n");
    ctx->indent--;
    indent(ctx); fprintf(ctx->out, "</script>\n");
    ctx->indent--;
    indent(ctx); fprintf(ctx->out, "</div>\n");

    val_free(xdata); val_free(ydata);
}

static void gen_table(CodegenCtx *ctx, ASTNode *node) {
    Value *data    = eval(ctx->interp, node->as.make_table_out.data,    ctx->env);
    Value *headers = node->as.make_table_out.headers
                     ? eval(ctx->interp, node->as.make_table_out.headers, ctx->env)
                     : NULL;

    indent(ctx); fprintf(ctx->out, "<div>\n");
    ctx->indent++;

    if (strlen(node->as.make_table_out.title) > 0) {
        indent(ctx);
        fprintf(ctx->out, "<h3>%s</h3>\n", node->as.make_table_out.title);
    }

    indent(ctx); fprintf(ctx->out, "<table>\n");
    ctx->indent++;

    /* Header row */
    if (headers && headers->type == VAL_LIST && headers->as.list->count > 0) {
        indent(ctx); fprintf(ctx->out, "<thead><tr>\n");
        ctx->indent++;
        for (int i = 0; i < headers->as.list->count; i++) {
            char *h = val_to_string(headers->as.list->items[i]);
            indent(ctx); fprintf(ctx->out, "<th>%s</th>\n", h);
            free(h);
        }
        ctx->indent--;
        indent(ctx); fprintf(ctx->out, "</tr></thead>\n");
    }

    /* Body rows */
    indent(ctx); fprintf(ctx->out, "<tbody>\n");
    ctx->indent++;
    if (data && data->type == VAL_LIST) {
        for (int i = 0; i < data->as.list->count; i++) {
            Value *row = data->as.list->items[i];
            indent(ctx); fprintf(ctx->out, "<tr>\n");
            ctx->indent++;
            if (row->type == VAL_LIST) {
                /* Row is a list — multiple columns */
                for (int j = 0; j < row->as.list->count; j++) {
                    char *cell = val_to_string(row->as.list->items[j]);
                    indent(ctx); fprintf(ctx->out, "<td>%s</td>\n", cell);
                    free(cell);
                }
            } else {
                /* Row is a scalar — single column */
                char *cell = val_to_string(row);
                indent(ctx); fprintf(ctx->out, "<td>%s</td>\n", cell);
                free(cell);
            }
            ctx->indent--;
            indent(ctx); fprintf(ctx->out, "</tr>\n");
        }
    }
    ctx->indent--;
    indent(ctx); fprintf(ctx->out, "</tbody>\n");
    ctx->indent--;
    indent(ctx); fprintf(ctx->out, "</table>\n");
    ctx->indent--;
    indent(ctx); fprintf(ctx->out, "</div>\n");

    val_free(data);
    if (headers) val_free(headers);
}

static void gen_report(CodegenCtx *ctx, ASTNode *node) {
    Value *data = eval(ctx->interp, node->as.make_report.data, ctx->env);

    indent(ctx); fprintf(ctx->out, "<div class=\"epl-report\">\n");
    ctx->indent++;

    if (strlen(node->as.make_report.title) > 0) {
        indent(ctx);
        fprintf(ctx->out, "<h3>%s</h3>\n", node->as.make_report.title);
    }

    /* Show stats for numeric lists */
    if (data && data->type == VAL_LIST && data->as.list->count > 0) {
        double sum = 0, mx = -1e300, mn = 1e300;
        int    numeric = 0;
        for (int i = 0; i < data->as.list->count; i++) {
            Value *item = data->as.list->items[i];
            if (item->type == VAL_NUMBER) {
                sum += item->as.number;
                if (item->as.number > mx) mx = item->as.number;
                if (item->as.number < mn) mn = item->as.number;
                numeric++;
            }
        }

        indent(ctx); fprintf(ctx->out,
            "<div class=\"epl-report-row\">"
            "<span>Count</span><span>%d</span></div>\n",
            data->as.list->count);

        if (numeric > 0) {
            double avg = sum / numeric;
            indent(ctx); fprintf(ctx->out,
                "<div class=\"epl-report-row\">"
                "<span>Total</span><span>%.4g</span></div>\n", sum);
            indent(ctx); fprintf(ctx->out,
                "<div class=\"epl-report-row\">"
                "<span>Average</span><span>%.4g</span></div>\n", avg);
            indent(ctx); fprintf(ctx->out,
                "<div class=\"epl-report-row\">"
                "<span>Maximum</span><span>%.4g</span></div>\n", mx);
            indent(ctx); fprintf(ctx->out,
                "<div class=\"epl-report-row\">"
                "<span>Minimum</span><span>%.4g</span></div>\n", mn);
        }
    }

    ctx->indent--;
    indent(ctx); fprintf(ctx->out, "</div>\n");
    val_free(data);
}

static void gen_section(CodegenCtx *ctx, ASTNode *node) {
    indent(ctx); fprintf(ctx->out, "<div class=\"epl-section\">\n");
    ctx->indent++;
    if (strlen(node->as.section_out.title) > 0) {
        indent(ctx);
        fprintf(ctx->out, "<h3>%s</h3>\n", node->as.section_out.title);
    }
    for (int i = 0; i < node->as.section_out.children.count; i++)
        codegen_node(ctx, node->as.section_out.children.items[i]);
    ctx->indent--;
    indent(ctx); fprintf(ctx->out, "</div>\n");
}

static void gen_row(CodegenCtx *ctx, ASTNode *node) {
    indent(ctx); fprintf(ctx->out, "<div class=\"epl-row\">\n");
    ctx->indent++;
    for (int i = 0; i < node->as.row_out.children.count; i++)
        codegen_node(ctx, node->as.row_out.children.items[i]);
    ctx->indent--;
    indent(ctx); fprintf(ctx->out, "</div>\n");
}


/* ═════════════════════════════════════════════
   MAIN DISPATCH
   ═════════════════════════════════════════════ */
void codegen_node(CodegenCtx *ctx, ASTNode *node) {
    if (!node) return;

    /* For non-output nodes inside EPF — run them through the interpreter
       so variables get set before charts/tables reference them */
    switch (node->type) {
        case NODE_SET:
        case NODE_UPDATE:
        case NODE_DESCRIBE:
        case NODE_ACTIVITY_DEF:
            exec_node(ctx->interp, node, ctx->env);
            return;
        default: break;
    }

    switch (node->type) {
        case NODE_HEADING_OUT:   gen_heading(ctx, node);    break;
        case NODE_SUBHEADING:    gen_subheading(ctx, node); break;
        case NODE_PARAGRAPH:     gen_paragraph(ctx, node);  break;
        case NODE_LABEL_OUT:     gen_label(ctx, node);      break;
        case NODE_BOLD_OUT:      gen_bold(ctx, node);       break;
        case NODE_DIVIDER:       gen_divider(ctx);          break;
        case NODE_SPACER:        gen_spacer(ctx);           break;
        case NODE_MAKE_STAT:     gen_stat(ctx, node);       break;
        case NODE_MAKE_CHART:    gen_chart(ctx, node);      break;
        case NODE_MAKE_TABLE_OUT:gen_table(ctx, node);      break;
        case NODE_MAKE_REPORT:   gen_report(ctx, node);     break;
        case NODE_SECTION_OUT:   gen_section(ctx, node);    break;
        case NODE_ROW:           gen_row(ctx, node);        break;
        default: break;
    }
}

/* ═════════════════════════════════════════════
   COLLECT PAGES — pre-scan (v1.7)
   Walk program AST, find every NODE_MAKE_PAGE
   and record filename + first heading text.
   Called BEFORE execution begins.
   ═════════════════════════════════════════════ */
void codegen_collect_pages(ASTNode *program, PageManifest *manifest,
                           Interpreter *interp, Env *env) {
    if (!program || !manifest) return;
    manifest->count  = 0;
    manifest->active = 0;

    NodeList *body = &program->as.program.body;
    for (int i = 0; i < body->count && manifest->count < EPL_MAX_PAGES; i++) {
        ASTNode *node = body->items[i];
        if (!node || node->type != NODE_MAKE_PAGE) continue;

        /* Skip canvas3d pages — they don't use the nav shell */
        int has3d = 0;
        for (int j = 0; j < node->as.make_page.children.count; j++) {
            ASTNode *c = node->as.make_page.children.items[j];
            if (c && c->type == NODE_CANVAS3D) { has3d = 1; break; }
        }
        if (has3d) continue;

        PageEntry *pe = &manifest->pages[manifest->count];

        /* Filename */
        const char *fn = node->as.make_page.filename;
        if (!fn || fn[0] == '\0') fn = "output.html";
        strncpy(pe->filename, fn, 255);
        pe->filename[255] = '\0';

        /* Label — use first heading, fall back to filename stem */
        pe->label[0] = '\0';
        for (int j = 0; j < node->as.make_page.children.count; j++) {
            ASTNode *c = node->as.make_page.children.items[j];
            if (c && c->type == NODE_HEADING_OUT && c->as.text_out.value) {
                /* Evaluate the heading expression now, using the env as-is.
                   Variables may not all be set yet, but string literals work fine. */
                Value *v = eval(interp, c->as.text_out.value, env);
                if (v && v->type == VAL_STRING) {
                    strncpy(pe->label, v->as.string, 255);
                    pe->label[255] = '\0';
                }
                val_free(v);
                break;
            }
        }
        if (pe->label[0] == '\0') {
            /* Fall back to filename without path/extension */
            const char *base = strrchr(fn, '/') ? strrchr(fn, '/') + 1 :
                               (strrchr(fn, '\\') ? strrchr(fn, '\\') + 1 : fn);
            strncpy(pe->label, base, 255);
            char *dot = strrchr(pe->label, '.');
            if (dot) *dot = '\0';
        }

        /* Icon — cycle through the preset set */
        strncpy(pe->icon,
            page_icons[manifest->count % 10],
            sizeof(pe->icon) - 1);

        manifest->count++;
    }
}


/* ═════════════════════════════════════════════
   PAGE ENTRY POINT  (v1.7)
   ═════════════════════════════════════════════ */
void codegen_page(Interpreter *interp, Env *env, ASTNode *node,
                  PageManifest *manifest) {
    const char *filename = node->as.make_page.filename;
    if (!filename || strlen(filename) == 0) filename = "output.html";

    /* If page contains canvas3d — route to scene generator */
    for (int i = 0; i < node->as.make_page.children.count; i++) {
        ASTNode *child = node->as.make_page.children.items[i];
        if (child && child->type == NODE_CANVAS3D) {
            scene_gen_page(interp, env, node);
            return;
        }
    }

    /* Find which manifest slot this page occupies */
    int multi = (manifest && manifest->count > 1);
    if (multi) {
        for (int i = 0; i < manifest->count; i++) {
            if (strcmp(manifest->pages[i].filename, filename) == 0) {
                manifest->active = i;
                break;
            }
        }
    }

    FILE *out = fopen(filename, "w");
    if (!out) {
        fprintf(stderr, "[EPL Error] Cannot write to '%s'\n", filename);
        return;
    }

    CodegenCtx ctx;
    ctx.out         = out;
    ctx.interp      = interp;
    ctx.env         = env;
    ctx.theme       = codegen_get_theme(node->as.make_page.theme,
                                        node->as.make_page.color);
    ctx.chart_count = 0;
    ctx.indent      = 1;
    ctx.manifest    = manifest;

    /* Use first heading as page title, or filename */
    char title[256] = "EPL Page";
    for (int i = 0; i < node->as.make_page.children.count; i++) {
        ASTNode *child = node->as.make_page.children.items[i];
        if (child && child->type == NODE_HEADING_OUT && child->as.text_out.value) {
            Value *v = eval(interp, child->as.text_out.value, env);
            char  *s = val_to_string(v);
            strncpy(title, s, 255);
            free(s); val_free(v);
            break;
        }
    }

    /* Update manifest label with the live-evaluated title */
    if (multi) {
        strncpy(manifest->pages[manifest->active].label, title, 255);
    }

    write_head(&ctx, title);

    /* ── Multi-page shell: nav + sidebar ── */
    if (multi) {
        write_shell_open(&ctx, manifest);
    }

    for (int i = 0; i < node->as.make_page.children.count; i++)
        codegen_node(&ctx, node->as.make_page.children.items[i]);

    if (multi) {
        write_shell_close(&ctx);
    }

    write_foot(&ctx);
    fclose(out);

    /* Update .epbind */
    binding_update(interp->source_file, filename, "html");

    printf("[EPL] Generated: %s\n", filename);
}
