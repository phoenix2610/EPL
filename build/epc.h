#ifndef EPC_H
#define EPC_H

#include "ast.h"

ASTNode *epc_load(const char *source_path);

int epc_store(const char *source_path, ASTNode *program);

void epc_path_for(const char *source_path, char *out, size_t outlen);

#endif
