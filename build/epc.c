#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include <stdint.h>
#include <sys/stat.h>
#include <unistd.h>
#include "epc.h"

#include "ast_serial.inc"

#define EPC_MAGIC   "EPLC"
#define EPC_FORMAT  1u

_Static_assert(AST_DESC_COUNT == (int)(sizeof(AST_DESC)/sizeof(AST_DESC[0])),
               "ast_serial.inc is out of date — rerun tools/gen_ast_serial.py");

typedef struct {
    FILE  *f;
    const unsigned char *buf;
    size_t               len;
    size_t               pos;
    int                  failed;
} EpcIO;

static uint32_t cache_key(void) {
    static uint32_t key = 0;
    static int ready = 0;
    if (!ready) {
        struct stat st;
        uint64_t stamp = (stat("/proc/self/exe", &st) == 0)
                       ? (uint64_t)st.st_mtime : 0;
        key = AST_LAYOUT_HASH;
        for (int i = 0; i < 8; i++) {
            key ^= (uint32_t)((stamp >> (i * 8)) & 0xff);
            key *= 16777619u;
        }
        ready = 1;
    }
    return key;
}

void epc_path_for(const char *source_path, char *out, size_t outlen) {
    snprintf(out, outlen, "%s", source_path);
    char *dot = strrchr(out, '.');
    char *sep = strrchr(out, '/');
    if (dot && (!sep || dot > sep)) {
        size_t used = (size_t)(dot - out);
        snprintf(out + used, outlen - used, ".EPC");
    } else {
        size_t used = strlen(out);
        snprintf(out + used, outlen - used, ".EPC");
    }
}

static int stat_source(const char *path, uint64_t *size, int64_t *mtime) {
    struct stat st;
    if (stat(path, &st) != 0) return 0;
    *size  = (uint64_t)st.st_size;
    *mtime = (int64_t)st.st_mtime;
    return 1;
}

static void w_raw(EpcIO *io, const void *p, size_t n) {
    if (io->failed) return;
    if (fwrite(p, 1, n, io->f) != n) io->failed = 1;
}
static void w_u32(EpcIO *io, uint32_t v) { w_raw(io, &v, sizeof v); }
static void w_i32(EpcIO *io, int32_t  v) { w_raw(io, &v, sizeof v); }
static void w_u16(EpcIO *io, uint16_t v) { w_raw(io, &v, sizeof v); }
static void w_u8 (EpcIO *io, uint8_t  v) { w_raw(io, &v, sizeof v); }

static void r_raw(EpcIO *io, void *p, size_t n) {
    if (io->failed) return;
    if (io->pos + n > io->len) { io->failed = 1; return; }
    memcpy(p, io->buf + io->pos, n);
    io->pos += n;
}

static const unsigned char *r_bytes(EpcIO *io, size_t n) {
    if (io->failed) return NULL;
    if (io->pos + n > io->len) { io->failed = 1; return NULL; }
    const unsigned char *p = io->buf + io->pos;
    io->pos += n;
    return p;
}
static uint32_t r_u32(EpcIO *io){ uint32_t v=0; r_raw(io,&v,sizeof v); return v; }
static int32_t  r_i32(EpcIO *io){ int32_t  v=0; r_raw(io,&v,sizeof v); return v; }
static uint16_t r_u16(EpcIO *io){ uint16_t v=0; r_raw(io,&v,sizeof v); return v; }
static uint8_t  r_u8 (EpcIO *io){ uint8_t  v=0; r_raw(io,&v,sizeof v); return v; }

static void write_node(EpcIO *io, ASTNode *n) {
    if (io->failed) return;

    w_u16(io, (uint16_t)n->type);
    w_i32(io, n->line);
    w_i32(io, n->end_line);
    w_i32(io, n->col);

    if ((int)n->type < 0 || (int)n->type >= AST_DESC_COUNT) { io->failed = 1; return; }
    const NodeDesc *d = &AST_DESC[n->type];

    for (unsigned i = 0; i < d->n; i++) {
        const FDesc *fd = &d->f[i];
        char *base = (char *)n + fd->off;

        switch (fd->kind) {
            case F_STR: {
                uint32_t len = (uint32_t)strnlen(base, fd->size);
                w_u32(io, len);
                w_raw(io, base, len);
                break;
            }
            case F_RAW:
                w_raw(io, base, fd->size);
                break;
            case F_NODE: {
                ASTNode *child = *(ASTNode **)base;
                w_u8(io, child ? 1 : 0);
                if (child) write_node(io, child);
                break;
            }
            case F_LIST: {
                NodeList *l = (NodeList *)base;
                w_u32(io, (uint32_t)l->count);
                for (int k = 0; k < l->count; k++) write_node(io, l->items[k]);
                break;
            }
        }
        if (io->failed) return;
    }
}

static ASTNode *read_node(EpcIO *io) {
    if (io->failed) return NULL;

    uint16_t type = r_u16(io);
    int32_t  line = r_i32(io);
    int32_t  endl = r_i32(io);
    int32_t  col  = r_i32(io);
    if (io->failed) return NULL;

    if ((int)type >= AST_DESC_COUNT) { io->failed = 1; return NULL; }

    ASTNode *n = ast_make_node((NodeType)type, line, col);
    n->end_line = endl;

    const NodeDesc *d = &AST_DESC[type];
    for (unsigned i = 0; i < d->n; i++) {
        const FDesc *fd = &d->f[i];
        char *base = (char *)n + fd->off;

        switch (fd->kind) {
            case F_STR: {
                uint32_t len = r_u32(io);
                if (io->failed || len >= fd->size) { io->failed = 1; ast_free(n); return NULL; }
                const unsigned char *src = r_bytes(io, len);
                if (!src) { ast_free(n); return NULL; }
                memcpy(base, src, len);
                base[len] = '\0';
                break;
            }
            case F_RAW:
                r_raw(io, base, fd->size);
                break;
            case F_NODE: {
                uint8_t present = r_u8(io);
                if (io->failed) { ast_free(n); return NULL; }
                *(ASTNode **)base = present ? read_node(io) : NULL;
                break;
            }
            case F_LIST: {
                NodeList *l = (NodeList *)base;
                nodelist_init(l);
                uint32_t cnt = r_u32(io);
                if (io->failed) { ast_free(n); return NULL; }
                for (uint32_t k = 0; k < cnt; k++) {
                    ASTNode *c = read_node(io);
                    if (!c) { io->failed = 1; ast_free(n); return NULL; }
                    nodelist_push(l, c);
                }
                break;
            }
        }
        if (io->failed) { ast_free(n); return NULL; }
    }
    return n;
}

ASTNode *epc_load(const char *source_path) {
    uint64_t src_size; int64_t src_mtime;
    if (!stat_source(source_path, &src_size, &src_mtime)) return NULL;

    char path[1152];
    epc_path_for(source_path, path, sizeof path);

    FILE *f = fopen(path, "rb");
    if (!f) return NULL;

    if (fseek(f, 0, SEEK_END) != 0) { fclose(f); return NULL; }
    long fsz = ftell(f);
    if (fsz <= 0) { fclose(f); return NULL; }
    rewind(f);

    unsigned char *blob = malloc((size_t)fsz);
    if (!blob) { fclose(f); return NULL; }
    size_t got = fread(blob, 1, (size_t)fsz, f);
    fclose(f);
    if (got != (size_t)fsz) { free(blob); return NULL; }

    EpcIO io = { NULL, blob, got, 0, 0 };

    char magic[4];
    r_raw(&io, magic, 4);
    if (io.failed || memcmp(magic, EPC_MAGIC, 4) != 0) { free(blob); return NULL; }

    uint32_t fmt    = r_u32(&io);
    uint32_t layout = r_u32(&io);
    uint64_t c_size = 0; r_raw(&io, &c_size, sizeof c_size);
    int64_t  c_mt   = 0; r_raw(&io, &c_mt,   sizeof c_mt);

    if (io.failed || fmt != EPC_FORMAT || layout != cache_key() ||
        c_size != src_size || c_mt != src_mtime) {
        free(blob);
        return NULL;
    }

    ASTNode *prog = read_node(&io);
    if (io.failed || io.pos != io.len) {
        if (prog) ast_free(prog);
        prog = NULL;
    }
    free(blob);
    return prog;
}

int epc_store(const char *source_path, ASTNode *program) {
    uint64_t src_size; int64_t src_mtime;
    if (!program || !stat_source(source_path, &src_size, &src_mtime)) return -1;

    char path[1152];
    epc_path_for(source_path, path, sizeof path);

    char tmp[1216];
    snprintf(tmp, sizeof tmp, "%s.tmp%d", path, (int)getpid());

    FILE *f = fopen(tmp, "wb");
    if (!f) return -1;

    EpcIO io = { f, NULL, 0, 0, 0 };
    w_raw(&io, EPC_MAGIC, 4);
    w_u32(&io, EPC_FORMAT);
    w_u32(&io, cache_key());
    w_raw(&io, &src_size,  sizeof src_size);
    w_raw(&io, &src_mtime, sizeof src_mtime);
    write_node(&io, program);

    int bad = io.failed || fclose(f) != 0;
    if (bad) { remove(tmp); return -1; }
    if (rename(tmp, path) != 0) { remove(tmp); return -1; }
    return 0;
}
