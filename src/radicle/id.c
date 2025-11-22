#include <stdlib.h>
#include <string.h>

#include <id.h>
#include <base58.h>

char* pubkey_to_did (const uint8_t* key) {
    uint8_t did_buf [34];
    did_buf[0] = 0xED;
    did_buf[1] = 0x1;
    memcpy(did_buf+2,key,32);
    char* did = malloc(57);
    strcpy(did,"did:key:z");
    strcat(did,encode_base58(did_buf,34));
    return did;
}

char* oid_to_rid (const Oid oid) {
    char* rid = malloc(30);
    strcpy(rid,"z");
    strcat(rid,encode_base58(oid.id,20));
    return rid;
}
