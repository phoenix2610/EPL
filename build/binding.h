#ifndef BINDING_H
#define BINDING_H

/* ─────────────────────────────────────────────
   EPL Live Binding — binding.h  (v1.4)
   Tracks .EP → output file connections
   ───────────────────────────────────────────── */

void binding_update(const char *source_file, const char *output_file, const char *type);
void binding_remove(const char *output_file);
int  binding_get_html_files(const char *source_file, char out[][512], int max_out);
void binding_list(void);

#endif /* BINDING_H */
