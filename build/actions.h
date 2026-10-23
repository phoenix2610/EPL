#ifndef ACTIONS_H
#define ACTIONS_H

#include "ast.h"

int actions_check_page(const char *epf_path, ASTNode *program);

int actions_check_server(const char *ep_path, ASTNode *program);

#endif
