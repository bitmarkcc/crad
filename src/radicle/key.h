#ifndef RADICLE_KEY_H
#define RADICLE_KEY_H

#include <stdint.h>
#include <stddef.h>

typedef struct {
    uint8_t* bytes;
} Pubkey;

int key_sign_openssh (char** out_raw, char** out_full, const Pubkey signer, const uint8_t* inp, size_t len);

int key_sign_bytes (uint8_t out [64], const Pubkey signer, const uint8_t* inp, size_t len);

int key_sign_base58 (char** out, const Pubkey signer, const uint8_t* inp, size_t len);

char* rad_sig_to_ssh_format (const uint8_t* sig_bytes, Pubkey signer);

int rad_sshsig_verify (const uint8_t* data, size_t n_data, const char* sshsig, Pubkey signer);

#endif
