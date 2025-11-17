#ifndef RADICLE_UTIL_H
#define RADICLE_UTIL_H

#include <stdint.h>
#include <stdbool.h>

#define RAD_BUFSIZ 128
#define RAD_BUFSIZ2 1024

char* rad_strcpy (char* out, const char* inp, int from, int len);

void rad_strip (char* s);

char* rad_hex_to_uchar (const char* hex);

bool rad_is_space (char c);

bool rad_get_input (char* str, size_t bufsiz);

char* rad_to_lower (const char* str);

#endif
