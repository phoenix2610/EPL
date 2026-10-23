#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "binding.h"

#define BIND_FILE ".epbind"
#define MAX_OUTPUTS 64
#define MAX_PATH    512

typedef struct {
    char source[MAX_PATH];
    char file[MAX_PATH];
    char type[32];
    char updated[32];
} BindEntry;

static BindEntry entries[MAX_OUTPUTS];
static int       entry_count = 0;

static void binding_load(void) {
    entry_count = 0;
    FILE *f = fopen(BIND_FILE, "r");
    if (!f) return;

    char line[MAX_PATH + 128];
    while (fgets(line, sizeof(line), f)) {
        if (strstr(line, "\"file\"")) {
            BindEntry e = {0};
            char *sp = strstr(line, "\"source\": \"");
            char *fp = strstr(line, "\"file\": \"");
            char *tp = strstr(line, "\"type\": \"");
            char *up = strstr(line, "\"updated\": \"");
            if (sp) sscanf(sp + 11, "%511[^\"]", e.source);
            if (fp) sscanf(fp + 9, "%511[^\"]", e.file);
            if (tp) sscanf(tp + 9, "%31[^\"]",  e.type);
            if (up) sscanf(up + 12, "%31[^\"]", e.updated);
            if (strlen(e.file) > 0 && entry_count < MAX_OUTPUTS)
                entries[entry_count++] = e;
        }
    }
    fclose(f);
}

static void binding_save(void) {
    FILE *f = fopen(BIND_FILE, "w");
    if (!f) return;

    fprintf(f, "{\n  \"outputs\": [\n");
    for (int i = 0; i < entry_count; i++) {
        fprintf(f,
            "    { \"source\": \"%s\", \"file\": \"%s\", \"type\": \"%s\", \"updated\": \"%s\" }%s\n",
            entries[i].source,
            entries[i].file,
            entries[i].type,
            entries[i].updated,
            i < entry_count - 1 ? "," : "");
    }
    fprintf(f, "  ]\n}\n");
    fclose(f);
}

static void now_string(char *buf, int len) {
    time_t t = time(NULL);
    struct tm *tm = localtime(&t);
    strftime(buf, len, "%Y-%m-%d %H:%M:%S", tm);
}

void binding_update(const char *source_file, const char *output_file, const char *type) {
    binding_load();

    char ts[32];
    now_string(ts, sizeof(ts));

    for (int i = 0; i < entry_count; i++) {
        if (strcmp(entries[i].file, output_file) == 0) {
            strncpy(entries[i].source,  source_file, MAX_PATH - 1);
            strncpy(entries[i].updated, ts,          31);
            binding_save();
            return;
        }
    }

    if (entry_count < MAX_OUTPUTS) {
        strncpy(entries[entry_count].source,  source_file, MAX_PATH - 1);
        strncpy(entries[entry_count].file,    output_file, MAX_PATH - 1);
        strncpy(entries[entry_count].type,    type,        31);
        strncpy(entries[entry_count].updated, ts,          31);
        entry_count++;
    }

    binding_save();
}

void binding_remove(const char *output_file) {
    binding_load();
    int found = 0;
    for (int i = 0; i < entry_count; i++) {
        if (strcmp(entries[i].file, output_file) == 0) {
            for (int j = i; j < entry_count - 1; j++)
                entries[j] = entries[j + 1];
            entry_count--;
            found = 1;
            break;
        }
    }
    if (found) binding_save();
}

int binding_get_html_files(const char *source_file, char out[][512], int max_out) {
    binding_load();
    int n = 0;
    for (int i = 0; i < entry_count && n < max_out; i++) {
        if (strcmp(entries[i].type, "html") == 0 &&
            (!source_file || strcmp(entries[i].source, source_file) == 0)) {
            strncpy(out[n], entries[i].file, 511);
            out[n][511] = '\0';
            n++;
        }
    }
    return n;
}

void binding_list(void) {
    binding_load();
    if (entry_count == 0) {
        printf("No bound outputs.\n");
        return;
    }
    printf("EPL Bound Outputs:\n");
    for (int i = 0; i < entry_count; i++) {
        printf("  [%s] %s  (updated: %s)\n",
            entries[i].type,
            entries[i].file,
            entries[i].updated);
    }
}

int binding_source_of(const char *output_file, char *out, int outlen) {
    binding_load();
    for (int i = 0; i < entry_count; i++) {
        if (strcmp(entries[i].file, output_file) == 0 && entries[i].source[0]) {
            snprintf(out, outlen, "%s", entries[i].source);
            return 1;
        }
    }
    return 0;
}
