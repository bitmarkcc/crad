#ifndef RADICLE_MAP_H
#define RADICLE_MAP_H

#include <json.h>

typedef struct {
    char* keys;
    size_t keys_size;
    json_object* values;
    size_t values_size;
} StrJsonMap;

#endif
