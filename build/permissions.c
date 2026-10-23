#define _DEFAULT_SOURCE 1
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <ctype.h>
#include <limits.h>
#include <unistd.h>
#include <sys/stat.h>
#include "permissions.h"

#define PERM_FILE_NAME "permissions.txt"

static void list_add(PermList *l, const char *s) {
    if (l->count == l->cap) {
        l->cap = l->cap ? l->cap * 2 : 4;
        l->items = realloc(l->items, l->cap * sizeof(char *));
    }
    l->items[l->count++] = strdup(s);
}

static void list_free(PermList *l) {
    for (int i = 0; i < l->count; i++) free(l->items[i]);
    free(l->items);
    l->items = NULL;
    l->count = l->cap = 0;
}

void perm_expand_home(const char *in, char *out, size_t outlen) {
    const char *home = getenv("HOME");
    if (in[0] == '~' && (in[1] == '/' || in[1] == '\0') && home)
        snprintf(out, outlen, "%s%s", home, in + 1);
    else
        snprintf(out, outlen, "%s", in);
}

static void canonicalize(const char *path, const char *base, char *out, size_t outlen) {
    char expanded[PATH_MAX], abs[PATH_MAX * 2];
    perm_expand_home(path, expanded, sizeof expanded);
    if (expanded[0] == '/') {
        snprintf(abs, sizeof abs, "%s", expanded);
    } else {
        char cwd[PATH_MAX];
        if (!base) base = getcwd(cwd, sizeof cwd) ? cwd : ".";
        snprintf(abs, sizeof abs, "%s/%s", base, expanded);
    }

    char head[PATH_MAX * 2];
    snprintf(head, sizeof head, "%s", abs);
    char tail[PATH_MAX * 2] = "";
    char real[PATH_MAX];
    while (!realpath(head, real)) {
        char *slash = strrchr(head, '/');
        if (!slash || slash == head) { snprintf(real, sizeof real, "/"); head[0] = '\0'; break; }
        char rest[PATH_MAX * 2];
        snprintf(rest, sizeof rest, "%s%s%s", slash + 1, tail[0] ? "/" : "", tail);
        snprintf(tail, sizeof tail, "%s", rest);
        *slash = '\0';
    }

    char result[PATH_MAX * 2];
    snprintf(result, sizeof result, "%s", real);
    char *save = NULL;
    for (char *seg = strtok_r(tail, "/", &save); seg; seg = strtok_r(NULL, "/", &save)) {
        if (strcmp(seg, ".") == 0 || seg[0] == '\0') continue;
        if (strcmp(seg, "..") == 0) {
            char *slash = strrchr(result, '/');
            if (slash && slash != result) *slash = '\0';
            else snprintf(result, sizeof result, "/");
            continue;
        }
        size_t len = strlen(result);
        snprintf(result + len, sizeof result - len, "%s%s",
                 (len && result[len - 1] == '/') ? "" : "/", seg);
    }
    snprintf(out, outlen, "%s", result);
}

static int inside(const char *path, const char *dir) {
    size_t n = strlen(dir);
    if (n == 1 && dir[0] == '/') return 1;
    return strncmp(path, dir, n) == 0 && (path[n] == '\0' || path[n] == '/');
}

static int in_any(const char *path, PermList *l) {
    for (int i = 0; i < l->count; i++)
        if (inside(path, l->items[i])) return 1;
    return 0;
}

static void clean_value(const char *in, char *out, size_t outlen) {
    while (isspace((unsigned char)*in)) in++;
    snprintf(out, outlen, "%s", in);
    size_t n = strlen(out);
    while (n && isspace((unsigned char)out[n - 1])) out[--n] = '\0';
    if (n >= 2 && ((out[0] == '"' && out[n - 1] == '"') ||
                   (out[0] == '\'' && out[n - 1] == '\''))) {
        memmove(out, out + 1, n - 2);
        out[n - 2] = '\0';
    }
}

static int starts_with_ci(const char *s, const char *prefix, const char **rest) {
    size_t n = strlen(prefix);
    if (strncasecmp(s, prefix, n) != 0) return 0;
    if (s[n] && !isspace((unsigned char)s[n])) return 0;
    *rest = s + n;
    return 1;
}

static void add_each(PermList *l, const char *values, int canonical, const char *base) {
    char buf[4096];
    snprintf(buf, sizeof buf, "%s", values);
    char *save = NULL;
    for (char *part = strtok_r(buf, ",", &save); part; part = strtok_r(NULL, ",", &save)) {
        char v[4096];
        clean_value(part, v, sizeof v);
        if (!v[0]) continue;
        if (canonical) {
            char c[PATH_MAX * 2];
            canonicalize(v, base, c, sizeof c);
            list_add(l, c);
        } else {
            list_add(l, v);
        }
    }
}

static void clean_host(const char *in, char *out, size_t outlen) {
    char v[1024];
    clean_value(in, v, sizeof v);
    const char *s = v;
    if (strncasecmp(s, "http://", 7) == 0) s += 7;
    else if (strncasecmp(s, "https://", 8) == 0) s += 8;
    snprintf(out, outlen, "%s", s);
    char *slash = strchr(out, '/');
    if (slash) *slash = '\0';
    for (char *c = out; *c; c++) *c = (char)tolower((unsigned char)*c);
}

int perm_load(Permissions *p, const char *program_path, char *err, size_t errlen) {
    memset(p, 0, sizeof *p);

    char prog[PATH_MAX * 2];
    canonicalize(program_path ? program_path : "./program", NULL, prog, sizeof prog);
    snprintf(p->program_dir, sizeof p->program_dir, "%s", prog);
    char *slash = strrchr(p->program_dir, '/');
    if (slash && slash != p->program_dir) *slash = '\0';
    else snprintf(p->program_dir, sizeof p->program_dir, "/");

    snprintf(p->file, sizeof p->file, "%s/%s",
             strcmp(p->program_dir, "/") == 0 ? "" : p->program_dir, PERM_FILE_NAME);

    list_add(&p->read_dirs, p->program_dir);
    list_add(&p->write_dirs, p->program_dir);

    FILE *f = fopen(p->file, "r");
    if (!f) return 0;
    p->from_file = 1;

    char line[4096];
    int lineno = 0;
    while (fgets(line, sizeof line, f)) {
        lineno++;
        char *hash = strchr(line, '#');
        if (hash) *hash = '\0';
        char *s = line;
        while (isspace((unsigned char)*s)) s++;
        size_t n = strlen(s);
        while (n && isspace((unsigned char)s[n - 1])) s[--n] = '\0';
        if (!*s) continue;

        const char *rest;
        if (!starts_with_ci(s, "allow", &rest)) goto bad;
        while (isspace((unsigned char)*rest)) rest++;
        const char *v;

        if (strcasecmp(rest, "everything") == 0) {
            p->everything = 1;
        } else if (starts_with_ci(rest, "reading files in", &v)) {
            add_each(&p->read_dirs, v, 1, p->program_dir);
        } else if (starts_with_ci(rest, "writing files in", &v)) {
            add_each(&p->write_dirs, v, 1, p->program_dir);
            add_each(&p->read_dirs, v, 1, p->program_dir);
        } else if (strcasecmp(rest, "running commands") == 0 ||
                   strcasecmp(rest, "running any command") == 0) {
            p->any_command = 1;
        } else if (starts_with_ci(rest, "running", &v)) {
            add_each(&p->programs, v, 0, NULL);
        } else if (starts_with_ci(rest, "network to", &v)) {
            char buf[4096];
            snprintf(buf, sizeof buf, "%s", v);
            char *save = NULL;
            for (char *part = strtok_r(buf, ",", &save); part; part = strtok_r(NULL, ",", &save)) {
                char host[1024];
                clean_host(part, host, sizeof host);
                if (host[0]) list_add(&p->hosts, host);
            }
        } else if (starts_with_ci(rest, "sockets in", &v)) {
            add_each(&p->socket_dirs, v, 1, p->program_dir);
        } else if (starts_with_ci(rest, "serving on port", &v)) {
            add_each(&p->ports, v, 0, NULL);
        } else if (strcasecmp(rest, "environment variables") == 0) {
            p->any_env = 1;
        } else if (starts_with_ci(rest, "environment variable", &v)) {
            add_each(&p->env_names, v, 0, NULL);
        } else {
            goto bad;
        }
        continue;
    bad:
        snprintf(err, errlen,
                 "%s line %d is not a rule I understand: \"%.200s\"\n"
                 "  Each line starts with \"allow\", for example:\n"
                 "    allow reading files in ./data\n"
                 "    allow writing files in ./reports\n"
                 "    allow running \"pdftotext\"\n"
                 "    allow network to localhost:11434\n"
                 "    allow sockets in /tmp\n"
                 "    allow serving on port 8080\n"
                 "    allow environment variable API_KEY",
                 p->file, lineno, s);
        fclose(f);
        return -1;
    }
    fclose(f);
    return 0;
}

void perm_free(Permissions *p) {
    list_free(&p->read_dirs);   list_free(&p->write_dirs);
    list_free(&p->socket_dirs); list_free(&p->programs);
    list_free(&p->hosts);       list_free(&p->env_names);
    list_free(&p->ports);
}

static void refuse(Permissions *p, char *msg, size_t n, const char *what, const char *line) {
    snprintf(msg, n,
             "permission denied: %s. If the program should do this, the operator "
             "(not the program) can add this line to %s:  %s",
             what, p->file, line);
}

static void parent_dir(const char *path, char *out, size_t n) {
    snprintf(out, n, "%s", path);
    char *slash = strrchr(out, '/');
    if (slash && slash != out) *slash = '\0';
    else snprintf(out, n, "/");
}

int perm_check_read(Permissions *p, const char *path, char *msg, size_t n) {
    if (p->everything) return 1;
    char c[PATH_MAX * 2];
    canonicalize(path, NULL, c, sizeof c);
    if (in_any(c, &p->read_dirs)) return 1;
    char dir[PATH_MAX * 2], what[PATH_MAX * 2 + 64], line[PATH_MAX * 2 + 64];
    parent_dir(c, dir, sizeof dir);
    snprintf(what, sizeof what, "reading '%s' (outside the program's folder)", c);
    snprintf(line, sizeof line, "allow reading files in %s", dir);
    refuse(p, msg, n, what, line);
    return 0;
}

int perm_check_write(Permissions *p, const char *path, char *msg, size_t n) {
    char c[PATH_MAX * 2];
    canonicalize(path, NULL, c, sizeof c);
    char permfile[PATH_MAX * 2];
    canonicalize(p->file, NULL, permfile, sizeof permfile);
    if (strcmp(c, permfile) == 0) {
        snprintf(msg, n, "permission denied: a program can never change %s — "
                         "only the operator edits it", p->file);
        return 0;
    }
    if (p->everything || in_any(c, &p->write_dirs)) return 1;
    char dir[PATH_MAX * 2], what[PATH_MAX * 2 + 64], line[PATH_MAX * 2 + 64];
    parent_dir(c, dir, sizeof dir);
    snprintf(what, sizeof what, "changing '%s' (outside the program's folder)", c);
    snprintf(line, sizeof line, "allow writing files in %s", dir);
    refuse(p, msg, n, what, line);
    return 0;
}

static const char *SHELL_SPECIALS = ";&|`$<>(){}[]\\\"'*?~!#\n\r";

int perm_check_command(Permissions *p, const char *cmd, char *msg, size_t n) {
    if (p->everything || p->any_command) return 1;

    char name[256] = {0};
    const char *s = cmd;
    while (isspace((unsigned char)*s)) s++;
    size_t i = 0;
    while (s[i] && !isspace((unsigned char)s[i]) && i < sizeof name - 1) { name[i] = s[i]; i++; }

    int listed = 0;
    for (int k = 0; k < p->programs.count; k++)
        if (strcmp(p->programs.items[k], name) == 0) listed = 1;

    char what[600], line[400];
    if (listed && strpbrk(cmd, SHELL_SPECIALS) == NULL) return 1;
    if (listed) {
        snprintf(what, sizeof what,
                 "the command '%.200s' uses shell symbols (; | & $ quotes …), and "
                 "\"%s\" is only allowed as a plain command", cmd, name);
        refuse(p, msg, n, what, "allow running commands   (lets the program run ANY command — use with care)");
        return 0;
    }
    snprintf(what, sizeof what, "running the command '%.200s'", cmd);
    snprintf(line, sizeof line, "allow running \"%s\"", name[0] ? name : "PROGRAM");
    refuse(p, msg, n, what, line);
    return 0;
}

int perm_check_network(Permissions *p, const char *host, int port, char *msg, size_t n) {
    if (p->everything) return 1;
    char h[512], hp[600];
    snprintf(h, sizeof h, "%s", host);
    for (char *c = h; *c; c++) *c = (char)tolower((unsigned char)*c);
    snprintf(hp, sizeof hp, "%s:%d", h, port);
    for (int i = 0; i < p->hosts.count; i++) {
        const char *e = p->hosts.items[i];
        if (strcmp(e, hp) == 0) return 1;
        if (!strchr(e, ':') && strcmp(e, h) == 0) return 1;
    }
    char what[700], line[700];
    snprintf(what, sizeof what, "connecting to %s", hp);
    snprintf(line, sizeof line, "allow network to %s", hp);
    refuse(p, msg, n, what, line);
    return 0;
}

int perm_check_socket(Permissions *p, const char *path, char *msg, size_t n) {
    if (p->everything) return 1;
    char c[PATH_MAX * 2];
    canonicalize(path, NULL, c, sizeof c);
    if (in_any(c, &p->socket_dirs)) return 1;
    char dir[PATH_MAX * 2], what[PATH_MAX * 2 + 64], line[PATH_MAX * 2 + 64];
    parent_dir(c, dir, sizeof dir);
    snprintf(what, sizeof what, "using the socket '%s'", c);
    snprintf(line, sizeof line, "allow sockets in %s", dir);
    refuse(p, msg, n, what, line);
    return 0;
}

int perm_check_serve(Permissions *p, int port, char *msg, size_t n) {
    if (p->everything) return 1;
    char want[32];
    snprintf(want, sizeof want, "%d", port);
    for (int i = 0; i < p->ports.count; i++)
        if (strcmp(p->ports.items[i], want) == 0) return 1;
    char what[64], line[64];
    snprintf(what, sizeof what, "serving on port %d", port);
    snprintf(line, sizeof line, "allow serving on port %d", port);
    refuse(p, msg, n, what, line);
    return 0;
}

int perm_check_env(Permissions *p, const char *name, char *msg, size_t n) {
    if (p->everything || p->any_env) return 1;
    for (int i = 0; i < p->env_names.count; i++)
        if (strcmp(p->env_names.items[i], name) == 0) return 1;
    char what[400], line[400];
    snprintf(what, sizeof what, "reading the environment variable %s", name);
    snprintf(line, sizeof line, "allow environment variable %s", name);
    refuse(p, msg, n, what, line);
    return 0;
}
