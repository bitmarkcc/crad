#ifndef RADICLE_UTIL_H
#define RADICLE_UTIL_H

#include <stdint.h>
#include <stdbool.h>
#include <json-c/json.h>

#define RAD_BUFSIZ 256

typedef struct {
    size_t nkeys;
    char** keys;
    size_t n_values;
    json_object** values;
} StrJsonMap;

char* rad_strcpy (char* out, const char* inp, int from, int len);

void rad_rstrip_nl (char* str);

char* rad_strip (char c, const char* str);

char* rad_hex_to_uchar (const char* hex);

bool rad_is_space (char c);

bool rad_get_input (char* str, size_t bufsiz);

char* rad_to_lower (const char* str);

char* rad_remove_space_json (const char* str);

void rad_assert_equal (const uint8_t* a, const uint8_t* b, size_t n);

#endif
