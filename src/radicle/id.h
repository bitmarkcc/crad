#ifndef RADICLE_ID_H
#define RADICLE_ID_H

#include <stdint.h>
#include <stdbool.h>
#include <git2.h>

typedef git_oid Oid;

char* pubkey_to_did (const uint8_t* key);

char* oid_to_rid (const Oid oid);

Oid rid_to_oid (const char* rid_str);

bool oid_is_null (const Oid oid);

void oids_dedup (Oid** oids, size_t* n);

void oids_sort (Oid** oids, size_t n);

#endif
