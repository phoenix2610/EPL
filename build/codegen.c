#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "codegen.h"
#include "binding.h"

/* ─────────────────────────────────────────────
   EPL Code Generator — codegen.c  (v1.4)
   Generates self-contained HTML from EPF AST
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
   PAGE ENTRY POINT
   ═════════════════════════════════════════════ */
void codegen_page(Interpreter *interp, Env *env, ASTNode *node) {
    const char *filename = node->as.make_page.filename;
    if (!filename || strlen(filename) == 0) filename = "output.html";

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

    write_head(&ctx, title);

    for (int i = 0; i < node->as.make_page.children.count; i++)
        codegen_node(&ctx, node->as.make_page.children.items[i]);

    write_foot(&ctx);
    fclose(out);

    /* Update .epbind */
    binding_update(filename, "html");

    printf("[EPL] Generated: %s\n", filename);
}
