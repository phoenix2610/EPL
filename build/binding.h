#ifndef BINDING_H
#define BINDING_H

/* ─────────────────────────────────────────────
   EPL Live Binding — binding.h  (v1.4)
   Tracks .EP → output file connections
   ───────────────────────────────────────────── */

void binding_update(const char *output_file, const char *type);
void binding_list(void);

#endif /* BINDING_H */
