#ifndef RADICLE_KEY_H
#define RADICLE_KEY_H

#include <stdint.h>
#include <stddef.h>

typedef struct {
    uint8_t* bytes;
} Pubkey;

int key_sign (char** out_raw, char** out_full, const Pubkey signer, const uint8_t* inp, size_t len);

#endif
