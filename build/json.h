#ifndef JSON_H
#define JSON_H

#include <stddef.h>
#include "interpreter.h"

Value *json_parse(const char *src, char *err, size_t errlen);

char *json_serialize(Value *v);

Value *json_path_get(Value *root, const char *path);

#endif
