#ifndef RAD_BASE58_H
#define RAD_BASE58_H

#include <stdint.h>

uint8_t* decode_base58(const char* str, int max_ret_len);

char* encode_base58(const uint8_t* inp, size_t len);

#endif
