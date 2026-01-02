#ifndef RADICLE_COB_COMMON_H
#define RADICLE_COB_COMMON_H

#include <id.h>

typedef struct {
    char* name;
    Oid content;
} OidEmbed;

typedef struct {
    char* type_name;
    uint32_t version;
    char* message;
    size_t n_embeds;
    OidEmbed* embeds;
    size_t n_contents;
    char** contents;
} Create;

typedef struct {
    char* type_name;
    uint32_t version;
} Manifest;

typedef enum {
    COB_IDENTITY,
    COB_ISSUE,
    COB_PATCH
} CobType;

#endif
