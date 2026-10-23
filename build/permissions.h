#ifndef PERMISSIONS_H
#define PERMISSIONS_H

#include <stddef.h>

typedef struct {
    char **items;
    int    count;
    int    cap;
} PermList;

typedef struct {
    char     program_dir[4096];
    char     file[4096];
    int      from_file;
    int      everything;
    int      any_command;
    int      any_env;
    PermList read_dirs;
    PermList write_dirs;
    PermList socket_dirs;
    PermList programs;
    PermList hosts;
    PermList env_names;
    PermList ports;
} Permissions;

int  perm_load(Permissions *p, const char *program_path, char *err, size_t errlen);
void perm_free(Permissions *p);

void perm_expand_home(const char *in, char *out, size_t outlen);

int perm_check_read   (Permissions *p, const char *path, char *msg, size_t n);
int perm_check_write  (Permissions *p, const char *path, char *msg, size_t n);
int perm_check_command(Permissions *p, const char *cmd,  char *msg, size_t n);
int perm_check_network(Permissions *p, const char *host, int port, char *msg, size_t n);
int perm_check_socket (Permissions *p, const char *path, char *msg, size_t n);
int perm_check_env    (Permissions *p, const char *name, char *msg, size_t n);
int perm_check_serve  (Permissions *p, int port, char *msg, size_t n);

#endif
