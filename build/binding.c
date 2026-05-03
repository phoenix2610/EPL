#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "binding.h"

/* ─────────────────────────────────────────────
   EPL Live Binding — binding.c  (v1.4)
   Writes/updates .epbind JSON next to source
   ───────────────────────────────────────────── */

#define BIND_FILE ".epbind"
#define MAX_OUTPUTS 64
#define MAX_PATH    512

typedef struct {
    char file[MAX_PATH];
    char type[32];
    char updated[32];
} BindEntry;

static BindEntry entries[MAX_OUTPUTS];
static int       entry_count = 0;

/* Load existing .epbind */
static void binding_load(void) {
    entry_count = 0;
    FILE *f = fopen(BIND_FILE, "r");
    if (!f) return;

    char line[MAX_PATH + 64];
    while (fgets(line, sizeof(line), f)) {
        if (strstr(line, "\"file\"")) {
            BindEntry e = {0};
            /* parse "file": "x", "type": "y", "updated": "z" */
            char *fp = strstr(line, "\"file\": \"");
            char *tp = strstr(line, "\"type\": \"");
            char *up = strstr(line, "\"updated\": \"");
            if (fp) sscanf(fp + 9, "%511[^\"]", e.file);
            if (tp) sscanf(tp + 9, "%31[^\"]",  e.type);
            if (up) sscanf(up + 12, "%31[^\"]", e.updated);
            if (strlen(e.file) > 0 && entry_count < MAX_OUTPUTS)
                entries[entry_count++] = e;
        }
    }
    fclose(f);
}

/* Save .epbind */
static void binding_save(void) {
    FILE *f = fopen(BIND_FILE, "w");
    if (!f) return;

    fprintf(f, "{\n  \"outputs\": [\n");
    for (int i = 0; i < entry_count; i++) {
        fprintf(f,
            "    { \"file\": \"%s\", \"type\": \"%s\", \"updated\": \"%s\" }%s\n",
            entries[i].file,
            entries[i].type,
            entries[i].updated,
            i < entry_count - 1 ? "," : "");
    }
    fprintf(f, "  ]\n}\n");
    fclose(f);
}

/* Get current timestamp string */
static void now_string(char *buf, int len) {
    time_t t = time(NULL);
    struct tm *tm = localtime(&t);
    strftime(buf, len, "%Y-%m-%d %H:%M:%S", tm);
}

/* Add or update an output entry */
void binding_update(const char *output_file, const char *type) {
    binding_load();

    char ts[32];
    now_string(ts, sizeof(ts));

    /* check if already exists */
    for (int i = 0; i < entry_count; i++) {
        if (strcmp(entries[i].file, output_file) == 0) {
            strncpy(entries[i].updated, ts, 31);
            binding_save();
            return;
        }
    }

    /* new entry */
    if (entry_count < MAX_OUTPUTS) {
        strncpy(entries[entry_count].file,    output_file, MAX_PATH - 1);
        strncpy(entries[entry_count].type,    type,        31);
        strncpy(entries[entry_count].updated, ts,          31);
        entry_count++;
    }

    binding_save();
}

/* List all bound outputs */
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
