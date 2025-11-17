#ifndef RAD_BASE64_H
#define RAD_BASE64_H

#include <stdint.h>

uint8_t* decode_base64(const char* str, int max_ret_len);

#endif
