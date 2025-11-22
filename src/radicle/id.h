#ifndef RADICLE_ID_H
#define RADICLE_ID_H

#include <stdint.h>
#include <git2.h>

typedef struct {
    char id [32];
} Rid;

typedef git_oid Oid;

char* pubkey_to_did (const uint8_t* key);

char* oid_to_rid (const Oid oid);

#endif
