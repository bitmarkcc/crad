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
    strcpy(did,"did:key:");
    strcat(did,encode_base58(did_buf,34));
    return did;
}

char* oid_to_rid (const Oid oid) {
    char* rid = encode_base58(oid.id,20);
    return rid;
}

bool oid_is_null (const Oid oid) {
    bool res = true;
    for (int i=0; i<20; i++) {
	if (oid.id[i]) res = false;
    }
    return res;
}

void oids_dedup (Oid** oids, size_t* n) { // todo implement
    return;
}

void oids_sort (Oid** oids, size_t n) { // todo implement

}
