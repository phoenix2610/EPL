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
            if (strcmp(custom_color, "solo leveling") == 0) {
                t.bg      = "#0b0914";
                t.surface = "#130f23";
                t.text    = "#fdf4ff";
                t.subtext = "#c026d3";
                t.accent  = "#f472b6";
                t.border  = "#9333ea";
            } else {
                t.bg      = "#f8fafc";
                t.surface = "#ffffff";
                t.text    = "#0f172a";
                t.subtext = acc;
                t.accent  = acc;
                t.border  = "#e2e8f0";
            }
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

    if (custom_color && strcmp(custom_color, "solo leveling") == 0) {
        t.chart_colors[0] = "#f472b6";
        t.chart_colors[1] = "#c026d3";
        t.chart_colors[2] = "#e879f9";
        t.chart_colors[3] = "#d946ef";
        t.chart_colors[4] = "#9333ea";
        t.chart_colors[5] = "#a855f7";
    }
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

/* Helper to inject external files like css/base.css into the HTML */
static void inject_file_content(CodegenCtx *ctx, const char *path) {
    FILE *f = fopen(path, "r");
    if (!f) {
        /* Silently ignore if file missing, or could print a warning */
        return;
    }
    char buf[1024];
    size_t n;
    while ((n = fread(buf, 1, sizeof(buf), f)) > 0) {
        fwrite(buf, 1, n, ctx->out);
    }
    fclose(f);
}



/* ═════════════════════════════════════════════
   HTML HEAD — embedded CSS + Chart.js CDN
   ═════════════════════════════════════════════ */
static void write_head(CodegenCtx *ctx, const char *title, const char *custom_color) {
    fprintf(ctx->out,
        "<!DOCTYPE html>\n"
        "<html lang=\"en\">\n"
        "<head>\n"
        "  <meta charset=\"UTF-8\">\n"
        "  <meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">\n"
        "  <title>%s</title>\n"
        "  <script src=\"https://cdn.jsdelivr.net/npm/chart.js@4.4.0/dist/chart.umd.min.js\"></script>\n"
        "  <style>\n",
        title
    );
    inject_file_content(ctx, "css/epl_base.css");
    
    if (custom_color && strcmp(custom_color, "solo leveling") == 0) {
        fprintf(ctx->out, "  body, .epl-page { background: #0b0914 !important; color: #e2d8f0; }\n");
        fprintf(ctx->out, "  .nav { background: rgba(11, 9, 20, 0.85) !important; border-bottom: 1px solid rgba(192, 38, 211, 0.3) !important; }\n");
        fprintf(ctx->out, "  .epl-section { background: #130f23 !important; border: 1px solid #9333ea !important; border-radius: 16px !important; box-shadow: 0 0 30px rgba(147, 51, 234, 0.15), inset 0 0 20px rgba(147, 51, 234, 0.05) !important; }\n");
        fprintf(ctx->out, "  .epl-stat { background: #1a1432 !important; border: 1px solid #c026d3 !important; border-radius: 12px; margin: 0.5rem; box-shadow: 0 0 20px rgba(192, 38, 211, 0.2) !important; }\n");
        fprintf(ctx->out, "  .epl-stat-value { color: #f472b6 !important; text-shadow: 0 0 20px rgba(244, 114, 182, 0.8) !important; }\n");
        fprintf(ctx->out, "  .epl-chart-wrap { background: #130f23 !important; border: 1px solid #c026d3 !important; border-radius: 16px !important; box-shadow: 0 0 40px rgba(192, 38, 211, 0.25) !important; }\n");
        fprintf(ctx->out, "  canvas { filter: drop-shadow(0px 0px 8px rgba(244, 114, 182, 0.8)); }\n");
        fprintf(ctx->out, "  h1, h2, h3, h4 { color: #fdf4ff !important; text-shadow: 0 0 15px rgba(192, 38, 211, 0.6) !important; font-weight: 800 !important; letter-spacing: 0.05em; }\n");
        fprintf(ctx->out, "  hr { background: linear-gradient(90deg, transparent, rgba(192, 38, 211, 0.8), transparent) !important; opacity: 1 !important; }\n");
        fprintf(ctx->out, "  table { border-radius: 12px; overflow: hidden; box-shadow: 0 0 20px rgba(192, 38, 211, 0.15); }\n");
        fprintf(ctx->out, "  table th { background: #271441 !important; border-bottom: 2px solid #c026d3 !important; color: #f472b6 !important; text-shadow: 0 0 10px rgba(244, 114, 182, 0.5); }\n");
        fprintf(ctx->out, "  table td { background: #1a1432 !important; border-bottom: 1px solid rgba(192, 38, 211, 0.2) !important; }\n");
        fprintf(ctx->out, "  .epl-report-row:hover { color: #f472b6 !important; text-shadow: 0 0 10px rgba(244, 114, 182, 0.5); }\n");
    }

    fprintf(ctx->out,
        "  </style>\n"
        "</head>\n"
        "<body>\n"
        "<!-- Custom Cursor -->\n"
        "<div class=\"cursor\" id=\"cur\"></div>\n"
        "<div class=\"cursor-ring\" id=\"ring\"></div>\n"
        "<div class=\"epl-page\">\n"
    );
}

static void write_foot(CodegenCtx *ctx) {
    fprintf(ctx->out, "</div>\n<script>\n");
    inject_file_content(ctx, "js/app.js");
    fprintf(ctx->out, "\n</script>\n</body>\n</html>\n");
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
    /* ── Top nav bar with links ── */
    fprintf(ctx->out,
        "<style>\n"
        "  .nav { position: fixed; top: 0; width: 100%%; z-index: 1000; padding: 1.5rem 3rem; display: flex; justify-content: space-between; align-items: center; border-bottom: 1px solid rgba(255,255,255,0.06); background: rgba(5,5,8,0.85); backdrop-filter: blur(20px); transition: padding 0.3s ease; }\n"
        "  .nav.scrolled { padding: 1rem 3rem; }\n"
        "  .nav-logo { display: flex; align-items: center; gap: 10px; font-size: 1.1rem; font-weight: 800; letter-spacing: 0.15em; color: #fff; text-decoration: none; }\n"
        "  .logo-img { height: 32px; width: auto; }\n"
        "  .nav-links { display: flex; gap: 2.5rem; align-items: center; }\n"
        "  .nav-links a { color: rgba(255,255,255,0.5); text-decoration: none; font-size: 0.85rem; letter-spacing: 0.08em; transition: color 0.2s; }\n"
        "  .nav-links a:hover, .nav-links a.active { color: #fff; }\n"
        "  .nav-cta { background: #fff; color: #000; padding: 0.6rem 1.5rem; border-radius: 2px; font-size: 0.85rem; font-weight: 700; letter-spacing: 0.06em; text-decoration: none; transition: all 0.2s; }\n"
        "  .nav-cta:hover { background: #c8ff00; color: #000; }\n"
        "  .epl-main { padding-top: 80px; }\n"
        "</style>\n"
        "<nav class=\"nav\" id=\"main-nav\">\n"
        "  <a class=\"nav-logo\" href=\"index.html\">\n"
        "    <img src=\"../documentation/EPL_logo.png\" alt=\"EPL\" class=\"logo-img\">\n"
        "    EPL\n"
        "  </a>\n"
        "  <div class=\"nav-links\">\n"
    );

    int ai = mf->active;
    for (int i = 0; i < mf->count; i++) {
        const char *cls = (i == ai) ? " active" : "";
        /* Optionally skip signup.html from normal links if it's the CTA */
        if (strcmp(mf->pages[i].filename, "signup.html") == 0) continue;
        
        fprintf(ctx->out,
            "    <a href=\"%s\" class=\"%s\">%s</a>\n",
            mf->pages[i].filename, cls, mf->pages[i].label
        );
    }

    fprintf(ctx->out,
        "  </div>\n"
        "  <a class=\"nav-cta\" href=\"signup.html\">Early Access</a>\n"
        "</nav>\n"
        "<div class=\"epl-main\">\n"
    );
}

static void write_shell_close(CodegenCtx *ctx) {
    fprintf(ctx->out, "</div>\n");   /* close epl-main */
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


static void gen_html_element(CodegenCtx *ctx, ASTNode *node) {
    indent(ctx);
    fprintf(ctx->out, "<%s", node->as.html_element.tag);
    
    if (strlen(node->as.html_element.id_attr) > 0)
        fprintf(ctx->out, " id=\"%s\"", node->as.html_element.id_attr);
    if (strlen(node->as.html_element.class_attr) > 0)
        fprintf(ctx->out, " class=\"%s\"", node->as.html_element.class_attr);
    if (strlen(node->as.html_element.src_attr) > 0)
        fprintf(ctx->out, " src=\"%s\"", node->as.html_element.src_attr);
    if (strlen(node->as.html_element.type_attr) > 0)
        fprintf(ctx->out, " type=\"%s\"", node->as.html_element.type_attr);
    if (strlen(node->as.html_element.onclick_attr) > 0)
        fprintf(ctx->out, " onclick=\"%s\"", node->as.html_element.onclick_attr);
    if (strlen(node->as.html_element.placeholder_attr) > 0)
        fprintf(ctx->out, " placeholder=\"%s\"", node->as.html_element.placeholder_attr);
    if (strlen(node->as.html_element.value_attr) > 0)
        fprintf(ctx->out, " value=\"%s\"", node->as.html_element.value_attr);
        
    if (node->as.html_element.is_self_closing) {
        fprintf(ctx->out, " />\n");
        return;
    }
    
    fprintf(ctx->out, ">");
    
    if (node->as.html_element.text_content) {
        Value *val = eval(ctx->interp, node->as.html_element.text_content, ctx->env);
        char *str = val_to_string(val);
        fprintf(ctx->out, "%s", str);
        free(str);
        val_free(val);
    }
    
    if (node->as.html_element.children.count > 0) {
        fprintf(ctx->out, "\n");
        ctx->indent++;
        for (int i = 0; i < node->as.html_element.children.count; i++) {
            codegen_node(ctx, node->as.html_element.children.items[i]);
        }
        ctx->indent--;
        indent(ctx);
    }
    
    fprintf(ctx->out, "</%s>\n", node->as.html_element.tag);
}

/* ═══════════════════════════════════════════
   EPF v2.0 COMPONENT CODE GENERATORS
   ═══════════════════════════════════════════ */

/* ── Design system context (populated by NODE_USE_DESIGN) ── */
typedef struct {
    char primary[64];
    char secondary[64];
    char background[64];
    char headline_font[64];
    char body_font[64];
    char icon_set[64];
    int  has_material_icons;
} DesignCtx;

static DesignCtx g_design = {
    "#a3f69b", "#e9c349", "#131313",
    "Noto Serif", "Inter", "Material Symbols", 1
};

/* Inject Tailwind + Google Fonts + Material Icons head block */
static void gen_use_design(CodegenCtx *ctx, ASTNode *node) {
    if (strlen(node->as.use_design.primary)       > 0)
        strncpy(g_design.primary,       node->as.use_design.primary,       63);
    if (strlen(node->as.use_design.secondary)     > 0)
        strncpy(g_design.secondary,     node->as.use_design.secondary,     63);
    if (strlen(node->as.use_design.background)    > 0)
        strncpy(g_design.background,    node->as.use_design.background,    63);
    if (strlen(node->as.use_design.headline_font) > 0)
        strncpy(g_design.headline_font, node->as.use_design.headline_font, 63);
    if (strlen(node->as.use_design.body_font)     > 0)
        strncpy(g_design.body_font,     node->as.use_design.body_font,     63);

    /* Output CDN links + Tailwind config */
    fprintf(ctx->out,
        "<script src='https://cdn.tailwindcss.com?plugins=forms,container-queries'></script>\n"
        "<link href='https://fonts.googleapis.com/css2?family=%s:ital,wght@0,400;0,700;1,400"
        "&family=%s:wght@300;400;500;600&display=swap' rel='stylesheet'/>\n"
        "<link href='https://fonts.googleapis.com/css2?family=Material+Symbols+Outlined"
        ":wght,FILL@100..700,0..1&display=swap' rel='stylesheet'/>\n"
        "<style>.material-symbols-outlined{font-variation-settings:'FILL' 0,'wght' 300,'GRAD' 0,'opsz' 24;}"
        ".glass-card{background:rgba(53,53,52,0.4);backdrop-filter:blur(24px);-webkit-backdrop-filter:blur(24px);}"
        ".text-glow{text-shadow:0 0 20px rgba(163,246,155,0.3);}"
        "#booking-modal{display:none;position:fixed;inset:0;z-index:100;background:rgba(0,0,0,0.8);"
        "backdrop-filter:blur(10px);align-items:center;justify-content:center;}"
        "#booking-modal.active{display:flex;animation:fadeIn 0.5s ease forwards;}"
        ".modal-content{background:%s;border:1px solid %s;padding:3rem;border-radius:1rem;"
        "text-align:center;color:#e5e2e1;transform:translateY(20px);animation:slideUp 0.5s ease forwards;}"
        "@keyframes slideUp{to{transform:translateY(0);}}"
        "@keyframes spin{from{transform:rotate(0deg);}to{transform:rotate(360deg);}}"
        ".animate-spin{animation:spin 1s linear infinite;display:inline-block;vertical-align:middle;}"
        "</style>\n"
        "<script>tailwind.config={darkMode:'class',theme:{extend:{colors:{"
        "'primary-fixed-dim':'%s','on-primary-fixed':'#002203','on-primary':'#003909',"
        "'primary-fixed':'%s','surface':'%s','background':'%s',"
        "'secondary':'%s','on-surface':'#e5e2e1',"
        "'on-surface-variant':'#bfcaba','surface-container-highest':'#353534',"
        "'surface-container-low':'#1c1b1b','surface-container':'#201f1f',"
        "'outline-variant':'#40493d','secondary-container':'#ae8d10',"
        "'on-secondary-container':'#342800'},"
        "fontFamily:{'headline':['%s'],'body':['%s'],'label':['%s']}}}}</script>\n",
        g_design.headline_font, g_design.body_font,
        g_design.background, g_design.primary,   /* modal bg/border */
        g_design.primary, g_design.primary,       /* primary-fixed-dim, primary-fixed */
        g_design.background, g_design.background, /* surface, background */
        g_design.secondary,                        /* secondary */
        g_design.headline_font, g_design.body_font, g_design.body_font
    );
}

/* ── NAVBAR ── */
static void gen_make_navbar(CodegenCtx *ctx, ASTNode *node) {
    fprintf(ctx->out,
        "<div id='tailwind-wrapper' class='bg-surface text-on-surface font-body"
        " selection:bg-primary-fixed selection:text-on-primary-fixed'>\n"
        "<nav class='fixed top-0 w-full z-50 bg-surface/40 backdrop-blur-xl"
        " flex justify-between items-center px-12 py-8 max-w-[1920px] mx-auto'>\n"
        "<div class='font-headline text-2xl italic text-[#e5e2e1] ml-8'>%s</div>\n"
        "<div class='hidden md:flex items-center gap-12'>\n",
        node->as.make_navbar.brand
    );
    for (int i = 0; i < node->as.make_navbar.children.count; i++) {
        ASTNode *c = node->as.make_navbar.children.items[i];
        if (!c) continue;
        if (c->type == NODE_NAV_LINK) {
            fprintf(ctx->out,
                "<a class='font-body text-[0.6875rem] uppercase tracking-widest"
                " text-[#e5e2e1]/70 transition-all hover:bg-[#88d982]/10 p-2'"
                " href='%s'>%s</a>\n",
                c->as.nav_link.href, c->as.nav_link.label);
        } else if (c->type == NODE_ACTION_BTN) {
            fprintf(ctx->out,
                "<button class='interactive-btn bg-primary-fixed text-on-primary-fixed"
                " px-8 py-3 rounded-xl font-body text-[0.6875rem] uppercase tracking-widest"
                " hover:brightness-110 transition-all'"
                " data-modal='%s'>%s</button>\n",
                c->as.action_btn.modal_id, c->as.action_btn.label);
        }
    }
    fprintf(ctx->out, "</div>\n</nav>\n");
}

/* ── HERO ── */
static void gen_make_hero(CodegenCtx *ctx, ASTNode *node) {
    fprintf(ctx->out,
        "<header class='relative h-screen w-full flex items-center overflow-hidden'>\n"
        "<div class='absolute inset-0 z-0'>\n"
        "<img class='w-full h-full object-cover brightness-[0.%d]' src='%s'/>\n"
        "</div>\n"
        "<div class='relative z-10 w-full px-12 md:px-24 flex flex-col md:flex-row"
        " items-end justify-between'>\n"
        "<div class='mb-12 md:mb-0 max-w-3xl'>\n"
        "<h1 class='font-headline text-[3.5rem] md:text-[6rem] leading-none tracking-tight"
        " text-on-surface mb-6 opacity-90'>\n"
        "%s <br/><span class='italic text-primary-fixed-dim'>%s</span>\n"
        "</h1>\n"
        "<p class='font-label text-sm uppercase tracking-[0.3em] text-secondary opacity-80'>%s</p>\n"
        "</div>\n",
        node->as.make_hero.bg_dim,
        node->as.make_hero.bg_image,
        node->as.make_hero.title,
        node->as.make_hero.accented,
        node->as.make_hero.subtitle
    );
    if (node->as.make_hero.overlay_card) {
        ASTNode *oc = node->as.make_hero.overlay_card;
        fprintf(ctx->out,
            "<div class='glass-card p-12 rounded-xl max-w-md w-full"
            " border border-outline-variant/10 shadow-2xl md:translate-y-24'>\n"
            "<h3 class='font-headline text-2xl mb-8'>%s</h3>\n"
            "<div class='space-y-6'>\n",
            oc->as.overlay_card.title);
        for (int i = 0; i < oc->as.overlay_card.children.count; i++) {
            ASTNode *c = oc->as.overlay_card.children.items[i];
            if (!c) continue;
            if (c->type == NODE_GUEST_PICKER) {
                fprintf(ctx->out,
                    "<div class='group'><label class='font-label text-[0.6875rem]"
                    " uppercase tracking-widest text-secondary mb-2 block'>Guests</label>\n"
                    "<div class='bg-surface-container-highest/40 p-4 rounded-lg flex"
                    " items-center justify-between group-hover:bg-surface-container-highest/60"
                    " transition-colors'>\n"
                    "<span class='text-sm'>%d People</span>\n"
                    "<span class='material-symbols-outlined text-sm'>expand_more</span>\n"
                    "</div></div>\n", c->as.guest_picker.default_count);
            } else if (c->type == NODE_DATE_PICKER) {
                fprintf(ctx->out,
                    "<div class='group'><label class='font-label text-[0.6875rem]"
                    " uppercase tracking-widest text-secondary mb-2 block'>Date</label>\n"
                    "<div class='bg-surface-container-highest/40 p-4 rounded-lg flex"
                    " items-center justify-between group-hover:bg-surface-container-highest/60"
                    " transition-colors'>\n"
                    "<span class='text-sm'>%s</span>\n"
                    "<span class='material-symbols-outlined text-sm'>calendar_today</span>\n"
                    "</div></div>\n", c->as.date_picker.label);
            } else if (c->type == NODE_ACTION_BTN) {
                fprintf(ctx->out,
                    "<button class='interactive-btn w-full bg-primary-fixed"
                    " text-on-primary-fixed py-5 rounded-xl font-label text-[0.6875rem]"
                    " uppercase tracking-[0.2em] font-bold hover:brightness-110"
                    " transition-all shadow-lg' data-modal='%s'>%s</button>\n",
                    c->as.action_btn.modal_id, c->as.action_btn.label);
            }
        }
        fprintf(ctx->out, "</div></div>\n");
    }
    fprintf(ctx->out, "</div>\n</header>\n");
}

/* ── CONTENT SECTION ── */
static void gen_content_section(CodegenCtx *ctx, ASTNode *node) {
    fprintf(ctx->out,
        "<section id='%s' class='py-48 px-12 md:px-24 max-w-[1920px] mx-auto overflow-hidden'>\n"
        "<div class='grid grid-cols-1 md:grid-cols-12 gap-24 items-start'>\n",
        node->as.content_section.id);

    /* Left column */
    if (node->as.content_section.left_col) {
        fprintf(ctx->out, "<div class='md:col-span-5 space-y-24'>\n");
        ASTNode *col = node->as.content_section.left_col;
        for (int i = 0; i < col->as.column.children.count; i++)
            codegen_node(ctx, col->as.column.children.items[i]);
        fprintf(ctx->out, "</div>\n");
    }

    /* Right column */
    if (node->as.content_section.right_col) {
        fprintf(ctx->out, "<div class='md:col-span-7 relative'>\n");
        ASTNode *col = node->as.content_section.right_col;
        for (int i = 0; i < col->as.column.children.count; i++)
            codegen_node(ctx, col->as.column.children.items[i]);
        fprintf(ctx->out, "</div>\n");
    }
    fprintf(ctx->out, "</div>\n</section>\n");
}

/* ── EYEBROW ── */
static void gen_eyebrow(CodegenCtx *ctx, ASTNode *node) {
    fprintf(ctx->out,
        "<span class='font-label text-[0.6875rem] uppercase tracking-[0.4em]"
        " text-primary-fixed-dim mb-4 block'>%s</span>\n",
        node->as.eyebrow.text);
}

/* ── ITEM LIST ── */
static void gen_item_list(CodegenCtx *ctx, ASTNode *node) {
    fprintf(ctx->out, "<div class='space-y-16 pl-8 border-l border-outline-variant/20'>\n");
    for (int i = 0; i < node->as.item_list.entries.count; i++) {
        ASTNode *e = node->as.item_list.entries.items[i];
        if (!e || e->type != NODE_LIST_ENTRY) continue;
        const char *offset = e->as.list_entry.offset ? " md:translate-x-12" : "";
        fprintf(ctx->out,
            "<div class='group cursor-default%s'>\n"
            "<div class='flex justify-between items-baseline mb-2'>\n"
            "<h4 class='font-headline text-xl group-hover:text-primary-fixed transition-colors'>%s</h4>\n"
            "<span class='font-headline text-4xl text-secondary'>%02d</span></div>\n"
            "<p class='text-sm text-on-surface-variant italic'>%s</p>\n"
            "</div>\n",
            offset, e->as.list_entry.label,
            e->as.list_entry.number, e->as.list_entry.detail);
    }
    fprintf(ctx->out, "</div>\n");
}

/* ── FEATURE IMAGE ── */
static void gen_feature_image(CodegenCtx *ctx, ASTNode *node) {
    const char *aspect = strlen(node->as.feature_image.aspect) > 0
                         ? node->as.feature_image.aspect : "4/5";
    int reveal = strstr(node->as.feature_image.hover_mode, "color") != NULL;
    fprintf(ctx->out,
        "<div class='aspect-[%s] bg-surface-container-low overflow-hidden rounded-xl'>\n"
        "<img class='w-full h-full object-cover%s transition-all duration-1000' src='%s'/>\n"
        "</div>\n",
        aspect,
        reveal ? " grayscale hover:grayscale-0" : "",
        node->as.feature_image.src);
}

/* ── PRICE BADGE ── */
static void gen_price_badge(CodegenCtx *ctx, ASTNode *node) {
    fprintf(ctx->out,
        "<div class='absolute -bottom-12 -left-12 glass-card p-12 rounded-xl hidden md:block'>\n"
        "<span class='font-label text-[0.6875rem] uppercase tracking-widest"
        " text-secondary block mb-2'>The Experience</span>\n"
        "<span class='font-headline text-5xl'>%s</span>\n"
        "<span class='font-label text-xs uppercase ml-2 text-on-surface-variant'>%s</span>\n"
        "</div>\n",
        node->as.price_badge.price, node->as.price_badge.label);
}

/* ── GALLERY ── */
static void gen_make_gallery(CodegenCtx *ctx, ASTNode *node) {
    fprintf(ctx->out,
        "<section class='py-48 bg-surface-container-low/30'>\n"
        "<div class='px-12 md:px-24 mb-24 text-center'>\n"
        "<h2 class='font-headline text-[4rem] mb-6'>%s</h2>\n"
        "<p class='font-label text-xs uppercase tracking-[0.5em] text-on-surface-variant'>%s</p>\n"
        "</div>\n"
        "<div class='grid grid-cols-1 md:grid-cols-%d gap-4 px-4 h-[800px]'>\n",
        node->as.make_gallery.title,
        node->as.make_gallery.subtitle,
        node->as.make_gallery.columns);

    for (int i = 0; i < node->as.make_gallery.images.count; i++) {
        ASTNode *img = node->as.make_gallery.images.items[i];
        if (!img || img->type != NODE_GALLERY_IMAGE) continue;
        const char *mt = img->as.gallery_image.offset_top > 0 ? " md:mt-24" : "";
        fprintf(ctx->out,
            "<div class='relative overflow-hidden group h-full%s'>\n"
            "<img class='w-full h-full object-cover grayscale brightness-75"
            " group-hover:scale-110 transition-transform duration-1000' src='%s'/>\n"
            "<div class='absolute inset-0 bg-gradient-to-t from-surface"
            " to-transparent opacity-60'></div>\n"
            "</div>\n",
            mt, img->as.gallery_image.src);
    }
    fprintf(ctx->out, "</div>\n</section>\n");
}

/* ── FORM ── */
static void gen_make_form(CodegenCtx *ctx, ASTNode *node) {
    fprintf(ctx->out,
        "<section class='py-48 px-12 md:px-24 flex flex-col items-center"
        " justify-center text-center'>\n"
        "<div class='max-w-2xl w-full'>\n"
        "<span class='font-label text-[0.6875rem] uppercase tracking-[0.5em]"
        " text-secondary mb-6 block'>%s</span>\n"
        "<h2 class='font-headline text-[3.5rem] mb-12'>%s</h2>\n"
        "<div class='space-y-8 text-left'>\n"
        "<div class='grid grid-cols-1 md:grid-cols-2 gap-8'>\n",
        node->as.make_form.eyebrow, node->as.make_form.title);

    int in_grid = 1;
    for (int i = 0; i < node->as.make_form.children.count; i++) {
        ASTNode *c = node->as.make_form.children.items[i];
        if (!c) continue;
        if (c->type == NODE_FORM_FIELD) {
            int is_textarea = strcmp(c->as.form_field.field_type, "textarea") == 0;
            if (is_textarea && in_grid) {
                fprintf(ctx->out, "</div>\n"); in_grid = 0;
            }
            if (is_textarea) {
                fprintf(ctx->out,
                    "<div class='relative'>\n"
                    "<label class='font-label text-xs text-on-surface-variant"
                    " uppercase tracking-widest mb-3 block'>%s</label>\n"
                    "<textarea class='w-full bg-surface-container-highest border-none p-5"
                    " rounded-lg focus:ring-1 focus:ring-primary-fixed/30 transition-all"
                    " placeholder:text-on-surface-variant/30 text-sm h-32'"
                    " placeholder='%s'></textarea>\n"
                    "</div>\n",
                    c->as.form_field.field_label, c->as.form_field.placeholder);
            } else {
                fprintf(ctx->out,
                    "<div class='relative'>\n"
                    "<label class='font-label text-xs text-on-surface-variant"
                    " uppercase tracking-widest mb-3 block'>%s</label>\n"
                    "<input class='w-full bg-surface-container-highest border-none p-5"
                    " rounded-lg focus:ring-1 focus:ring-primary-fixed/30 transition-all"
                    " placeholder:text-on-surface-variant/30 text-sm'"
                    " placeholder='%s' type='%s'/>\n"
                    "</div>\n",
                    c->as.form_field.field_label,
                    c->as.form_field.placeholder,
                    c->as.form_field.field_type);
            }
        } else if (c->type == NODE_SUBMIT_BTN) {
            fprintf(ctx->out,
                "<button class='interactive-btn w-full bg-secondary-container"
                " text-on-secondary-container py-6 rounded-xl font-label text-[0.75rem]"
                " uppercase tracking-[0.3em] font-bold hover:brightness-110"
                " transition-all' data-modal='%s'>%s</button>\n",
                c->as.submit_btn.modal_id, c->as.submit_btn.label);
        }
    }
    fprintf(ctx->out, "</div></div></section>\n");
}

/* ── MODAL ── */
static void gen_make_modal(CodegenCtx *ctx, ASTNode *node) {
    fprintf(ctx->out,
        "<div id='%s'>\n"
        "<div class='modal-content'>\n"
        "<span class='material-symbols-outlined'"
        " style='font-size:4rem;color:%s;margin-bottom:1rem;'>%s</span>\n"
        "<h2 class='font-headline text-3xl mb-4'>%s</h2>\n"
        "<p class='font-body text-on-surface-variant mb-8'>%s</p>\n"
        "<button id='close-%s' class='bg-surface-container-highest px-8 py-3"
        " rounded-xl font-label text-sm hover:bg-surface-variant"
        " transition-colors'>%s</button>\n"
        "</div></div>\n"
        "<script>\n"
        "document.querySelectorAll('[data-modal=\"%s\"]').forEach(btn=>{\n"
        "  btn.addEventListener('click',function(e){\n"
        "    e.preventDefault();\n"
        "    const orig=this.innerHTML;\n"
        "    this.innerHTML='<span class=\"material-symbols-outlined animate-spin\""
        " style=\"font-size:1.2rem;margin-right:0.5rem;\">sync</span> Processing...';\n"
        "    this.style.opacity='0.8';this.style.pointerEvents='none';\n"
        "    setTimeout(()=>{\n"
        "      this.innerHTML=orig;this.style.opacity='1';this.style.pointerEvents='auto';\n"
        "      document.getElementById('%s').classList.add('active');\n"
        "    },1500);\n"
        "  });\n"
        "});\n"
        "document.getElementById('close-%s').addEventListener('click',()=>{\n"
        "  document.getElementById('%s').classList.remove('active');\n"
        "});\n"
        "</script>\n",
        node->as.make_modal.modal_id,
        strcmp(node->as.make_modal.color, "primary") == 0 ? g_design.primary : g_design.secondary,
        node->as.make_modal.icon,
        node->as.make_modal.heading,
        node->as.make_modal.body_text,
        node->as.make_modal.modal_id,
        node->as.make_modal.dismiss_label,
        node->as.make_modal.modal_id,
        node->as.make_modal.modal_id,
        node->as.make_modal.modal_id,
        node->as.make_modal.modal_id);
}

/* ── FOOTER ── */
static void gen_make_footer(CodegenCtx *ctx, ASTNode *node) {
    fprintf(ctx->out,
        "<footer class='bg-[%s] w-full pt-24 pb-12 flex flex-col md:flex-row"
        " justify-between items-end px-20 gap-24'>\n"
        "<div class='flex-1'>\n"
        "<div class='font-headline text-4xl text-[#e5e2e1] mb-12'>%s</div>\n"
        "<p class='font-body text-[0.875rem] leading-relaxed text-[#e5e2e1]/50 max-w-sm'>%s</p>\n"
        "</div>\n"
        "<div class='text-right'>\n"
        "<p class='font-headline text-[2rem] text-[%s] mb-4'>%s</p>\n"
        "<p class='font-body text-[0.875rem] leading-relaxed text-[#e5e2e1]/50'>%s</p>\n"
        "<div class='mt-12 text-[#e5e2e1]/30 font-body text-[0.6875rem]"
        " uppercase tracking-widest'>© %s</div>\n"
        "</div>\n"
        "</footer>\n"
        "</div>\n", /* close tailwind-wrapper */
        g_design.background,
        node->as.make_footer.brand,
        node->as.make_footer.tagline,
        g_design.secondary,
        node->as.make_footer.contact_heading,
        node->as.make_footer.address,
        node->as.make_footer.copyright);
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
        case NODE_HTML_ELEMENT:  gen_html_element(ctx, node); break;
        /* v2.0 EPF */
        case NODE_USE_DESIGN:      gen_use_design(ctx, node); break;
        case NODE_MAKE_NAVBAR:     gen_make_navbar(ctx, node); break;
        case NODE_MAKE_HERO:       gen_make_hero(ctx, node); break;
        case NODE_CONTENT_SECTION: gen_content_section(ctx, node); break;
        case NODE_EYEBROW:         gen_eyebrow(ctx, node); break;
        case NODE_ITEM_LIST:       gen_item_list(ctx, node); break;
        case NODE_FEATURE_IMAGE:   gen_feature_image(ctx, node); break;
        case NODE_PRICE_BADGE:     gen_price_badge(ctx, node); break;
        case NODE_MAKE_GALLERY:    gen_make_gallery(ctx, node); break;
        case NODE_MAKE_FORM:       gen_make_form(ctx, node); break;
        case NODE_MAKE_MODAL:      gen_make_modal(ctx, node); break;
        case NODE_MAKE_FOOTER:     gen_make_footer(ctx, node); break;
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

    write_head(&ctx, title, node->as.make_page.color);

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
