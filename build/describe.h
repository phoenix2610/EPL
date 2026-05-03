#ifndef DESCRIBE_H
#define DESCRIBE_H

/* ─────────────────────────────────────────────
   EPL Semantic Index — describe.h  (v1.5)
   Extracts describe blocks → .EPD file
   ───────────────────────────────────────────── */

#include "ast.h"

/* Entry types in the .EPD file */
typedef enum {
    EPD_PROGRAM,     /* top-level program description          */
    EPD_ACTIVITY,    /* to name ... ! with describe above it   */
    EPD_BLOCK,       /* describe on a non-activity block       */
    EPD_DATA,        /* set X as ... with describe above it    */
    EPD_PAGE,        /* make page in .EPF                      */
    EPD_SECTION,     /* section in .EPF                        */
    EPD_UNKNOWN
} EPDEntryType;

/* One semantic entry */
typedef struct {
    EPDEntryType type;
    int          line_start;
    int          line_end;
    char         name[256];        /* activity name / variable name / ""  */
    char         description[512]; /* the describe text                   */
    char         params[256];      /* "x, y" for activities               */
    char         data_type[64];    /* "list", "number", "graph", etc.     */
} EPDEntry;

/* Index — collection of entries */
typedef struct {
    EPDEntry *entries;
    int       count;
    int       capacity;
} EPDIndex;

/* Main API */
EPDIndex *epd_build(ASTNode *program, const char *source_file);
void      epd_write(EPDIndex *idx, const char *source_file,
                    const char *out_file);
void      epd_free(EPDIndex *idx);

#endif /* DESCRIBE_H */
