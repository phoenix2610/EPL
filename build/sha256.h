#ifndef SHA256_H
#define SHA256_H

#include <stddef.h>

void sha256_hex(const unsigned char *data, size_t len, char out[65]);

int  random_bytes(unsigned char *out, size_t n);

#endif
