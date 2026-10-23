#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "json.h"

typedef struct {
    const char *p;
    const char *start;
    char       *err;
    size_t      errlen;
    int         failed;
    int         depth;
} JParser;

#define JSON_MAX_DEPTH 128

static Value *parse_value(JParser *j);

static void jfail(JParser *j, const char *msg) {
    if (j->failed) return;
    j->failed = 1;
    if (j->err && j->errlen)
        snprintf(j->err, j->errlen, "%s at offset %d",
                 msg, (int)(j->p - j->start));
}

static void skip_ws(JParser *j) {
    while (*j->p == ' ' || *j->p == '\t' || *j->p == '\n' || *j->p == '\r')
        j->p++;
}

typedef struct {
    char  *data;
    size_t len;
    size_t cap;
} SBuf;

static void sb_init(SBuf *b) {
    b->cap  = 64;
    b->data = malloc(b->cap);
    b->len  = 0;
    b->data[0] = '\0';
}

static void sb_reserve(SBuf *b, size_t extra) {
    if (b->len + extra + 1 <= b->cap) return;
    while (b->len + extra + 1 > b->cap) b->cap *= 2;
    b->data = realloc(b->data, b->cap);
}

static void sb_putc(SBuf *b, char c) {
    sb_reserve(b, 1);
    b->data[b->len++] = c;
    b->data[b->len]   = '\0';
}

static void sb_puts(SBuf *b, const char *s) {
    size_t n = strlen(s);
    sb_reserve(b, n);
    memcpy(b->data + b->len, s, n);
    b->len += n;
    b->data[b->len] = '\0';
}

static void sb_put_utf8(SBuf *b, unsigned cp) {
    if (cp < 0x80) {
        sb_putc(b, (char)cp);
    } else if (cp < 0x800) {
        sb_putc(b, (char)(0xC0 | (cp >> 6)));
        sb_putc(b, (char)(0x80 | (cp & 0x3F)));
    } else if (cp < 0x10000) {
        sb_putc(b, (char)(0xE0 | (cp >> 12)));
        sb_putc(b, (char)(0x80 | ((cp >> 6) & 0x3F)));
        sb_putc(b, (char)(0x80 | (cp & 0x3F)));
    } else {
        sb_putc(b, (char)(0xF0 | (cp >> 18)));
        sb_putc(b, (char)(0x80 | ((cp >> 12) & 0x3F)));
        sb_putc(b, (char)(0x80 | ((cp >> 6) & 0x3F)));
        sb_putc(b, (char)(0x80 | (cp & 0x3F)));
    }
}

static int hex4(const char *s, unsigned *out) {
    unsigned v = 0;
    for (int i = 0; i < 4; i++) {
        char c = s[i];
        v <<= 4;
        if      (c >= '0' && c <= '9') v |= (unsigned)(c - '0');
        else if (c >= 'a' && c <= 'f') v |= (unsigned)(c - 'a' + 10);
        else if (c >= 'A' && c <= 'F') v |= (unsigned)(c - 'A' + 10);
        else return 0;
    }
    *out = v;
    return 1;
}

static char *parse_string_raw(JParser *j) {
    if (*j->p != '"') { jfail(j, "expected string"); return NULL; }
    j->p++;

    SBuf b; sb_init(&b);

    while (*j->p && *j->p != '"') {
        if (*j->p == '\\') {
            j->p++;
            switch (*j->p) {
                case '"':  sb_putc(&b, '"');  j->p++; break;
                case '\\': sb_putc(&b, '\\'); j->p++; break;
                case '/':  sb_putc(&b, '/');  j->p++; break;
                case 'b':  sb_putc(&b, '\b'); j->p++; break;
                case 'f':  sb_putc(&b, '\f'); j->p++; break;
                case 'n':  sb_putc(&b, '\n'); j->p++; break;
                case 'r':  sb_putc(&b, '\r'); j->p++; break;
                case 't':  sb_putc(&b, '\t'); j->p++; break;
                case 'u': {
                    unsigned cp;
                    if (!hex4(j->p + 1, &cp)) {
                        jfail(j, "bad \\u escape");
                        free(b.data); return NULL;
                    }
                    j->p += 5;
                    if (cp >= 0xD800 && cp <= 0xDBFF &&
                        j->p[0] == '\\' && j->p[1] == 'u') {
                        unsigned lo;
                        if (hex4(j->p + 2, &lo) && lo >= 0xDC00 && lo <= 0xDFFF) {
                            cp = 0x10000 + ((cp - 0xD800) << 10) + (lo - 0xDC00);
                            j->p += 6;
                        }
                    }
                    sb_put_utf8(&b, cp);
                    break;
                }
                default:
                    jfail(j, "invalid escape sequence");
                    free(b.data); return NULL;
            }
        } else {
            sb_putc(&b, *j->p++);
        }
    }

    if (*j->p != '"') {
        jfail(j, "unterminated string");
        free(b.data);
        return NULL;
    }
    j->p++;
    return b.data;
}

static Value *parse_object(JParser *j) {
    j->p++;
    Value *m = val_map();
    skip_ws(j);

    if (*j->p == '}') { j->p++; return m; }

    for (;;) {
        skip_ws(j);
        char *key = parse_string_raw(j);
        if (!key) { val_free(m); return NULL; }

        skip_ws(j);
        if (*j->p != ':') {
            jfail(j, "expected ':' after object key");
            free(key); val_free(m); return NULL;
        }
        j->p++;

        Value *v = parse_value(j);
        if (!v) { free(key); val_free(m); return NULL; }

        valmap_set(m->as.map, key, v);
        free(key);

        skip_ws(j);
        if (*j->p == ',') { j->p++; continue; }
        if (*j->p == '}') { j->p++; return m; }

        jfail(j, "expected ',' or '}' in object");
        val_free(m);
        return NULL;
    }
}

static Value *parse_array(JParser *j) {
    j->p++;
    Value *arr = val_list();
    skip_ws(j);

    if (*j->p == ']') { j->p++; return arr; }

    for (;;) {
        Value *v = parse_value(j);
        if (!v) { val_free(arr); return NULL; }
        vallist_push(arr->as.list, v);

        skip_ws(j);
        if (*j->p == ',') { j->p++; continue; }
        if (*j->p == ']') { j->p++; return arr; }

        jfail(j, "expected ',' or ']' in array");
        val_free(arr);
        return NULL;
    }
}

static Value *parse_value(JParser *j) {
    if (j->failed) return NULL;

    if (++j->depth > JSON_MAX_DEPTH) {
        jfail(j, "JSON nested too deeply");
        j->depth--;
        return NULL;
    }

    skip_ws(j);
    Value *result = NULL;

    switch (*j->p) {
        case '{': result = parse_object(j); break;
        case '[': result = parse_array(j);  break;
        case '"': {
            char *s = parse_string_raw(j);
            if (s) { result = val_string(s); free(s); }
            break;
        }
        case 't':
            if (strncmp(j->p, "true", 4) == 0) { j->p += 4; result = val_boolean(1); }
            else jfail(j, "invalid literal");
            break;
        case 'f':
            if (strncmp(j->p, "false", 5) == 0) { j->p += 5; result = val_boolean(0); }
            else jfail(j, "invalid literal");
            break;
        case 'n':
            if (strncmp(j->p, "null", 4) == 0) { j->p += 4; result = val_null(); }
            else jfail(j, "invalid literal");
            break;
        case '\0':
            jfail(j, "unexpected end of input");
            break;
        default: {
            if (*j->p == '-' || isdigit((unsigned char)*j->p)) {
                char *end;
                double n = strtod(j->p, &end);
                if (end == j->p) { jfail(j, "invalid number"); break; }
                j->p = end;
                result = val_number(n);
            } else {
                jfail(j, "unexpected character");
            }
            break;
        }
    }

    j->depth--;
    return result;
}

Value *json_parse(const char *src, char *err, size_t errlen) {
    if (!src) return NULL;

    JParser j = { src, src, err, errlen, 0, 0 };
    Value *v = parse_value(&j);
    if (!v) return NULL;

    skip_ws(&j);
    if (*j.p != '\0') {
        jfail(&j, "trailing content after JSON value");
        val_free(v);
        return NULL;
    }
    return v;
}

static void escape_into(SBuf *b, const char *s) {
    sb_putc(b, '"');
    for (const unsigned char *c = (const unsigned char *)s; *c; c++) {
        switch (*c) {
            case '"':  sb_puts(b, "\\\""); break;
            case '\\': sb_puts(b, "\\\\"); break;
            case '\b': sb_puts(b, "\\b");  break;
            case '\f': sb_puts(b, "\\f");  break;
            case '\n': sb_puts(b, "\\n");  break;
            case '\r': sb_puts(b, "\\r");  break;
            case '\t': sb_puts(b, "\\t");  break;
            default:
                if (*c < 0x20) {
                    char u[8];
                    snprintf(u, sizeof(u), "\\u%04x", *c);
                    sb_puts(b, u);
                } else {
                    sb_putc(b, (char)*c);
                }
        }
    }
    sb_putc(b, '"');
}

static void serialize_into(SBuf *b, Value *v) {
    if (!v) { sb_puts(b, "null"); return; }

    switch (v->type) {
        case VAL_NULL:    sb_puts(b, "null"); break;
        case VAL_BOOLEAN: sb_puts(b, v->as.boolean ? "true" : "false"); break;
        case VAL_STRING:  escape_into(b, v->as.string); break;
        case VAL_NUMBER: {
            char n[64];
            if (v->as.number == (long long)v->as.number)
                snprintf(n, sizeof(n), "%lld", (long long)v->as.number);
            else
                snprintf(n, sizeof(n), "%.17g", v->as.number);
            sb_puts(b, n);
            break;
        }
        case VAL_LIST:
            sb_putc(b, '[');
            for (int i = 0; i < v->as.list->count; i++) {
                if (i) sb_putc(b, ',');
                serialize_into(b, v->as.list->items[i]);
            }
            sb_putc(b, ']');
            break;
        case VAL_MAP:
            sb_putc(b, '{');
            for (int i = 0; i < v->as.map->count; i++) {
                if (i) sb_putc(b, ',');
                escape_into(b, v->as.map->keys[i]);
                sb_putc(b, ':');
                serialize_into(b, v->as.map->values[i]);
            }
            sb_putc(b, '}');
            break;
        default:
            sb_puts(b, "null");
            break;
    }
}

char *json_serialize(Value *v) {
    SBuf b; sb_init(&b);
    serialize_into(&b, v);
    return b.data;
}

Value *json_path_get(Value *root, const char *path) {
    if (!root || !path) return NULL;

    if (!strchr(path, '.')) {
        if (root->type == VAL_MAP) return valmap_get(root->as.map, path);
        if (root->type == VAL_LIST) {
            char *end;
            long idx = strtol(path, &end, 10);
            if (*end == '\0' && idx >= 0 && idx < root->as.list->count)
                return root->as.list->items[idx];
        }
        return NULL;
    }

    if (root->type == VAL_MAP) {
        Value *whole = valmap_get(root->as.map, path);
        if (whole) return whole;
    }

    char segment[256];
    const char *dot = strchr(path, '.');
    size_t seglen = (size_t)(dot - path);
    if (seglen >= sizeof(segment)) return NULL;
    memcpy(segment, path, seglen);
    segment[seglen] = '\0';

    Value *next = NULL;
    if (root->type == VAL_MAP) {
        next = valmap_get(root->as.map, segment);
    } else if (root->type == VAL_LIST) {
        char *end;
        long idx = strtol(segment, &end, 10);
        if (*end == '\0' && idx >= 0 && idx < root->as.list->count)
            next = root->as.list->items[idx];
    }
    if (!next) return NULL;

    return json_path_get(next, dot + 1);
}
