#include <stdlib.h>
#include <libssh/libssh.h>
#include <stdio.h>
#include <string.h>
#include <base58.h>

#include <key.h>
#include <profile.h>

int key_sign (char** out_raw, char** out_full, const Pubkey signer, const uint8_t* inp, size_t len) {
    char* sig = 0;
    ssh_key privkey = 0;
    if (profile_get_privkey(&privkey)) {
	return 1;
    }
    ssh_string sig_raw = 0;
    if (sshsig_sign(inp,len,privkey,0,"radicle",SSHSIG_DIGEST_SHA2_256,&sig,&sig_raw) != SSH_OK) {
	fprintf(stderr,"Failed to sign the data with the private key\n");
	return 1;
    }
    size_t sig_raw_len = ssh_string_len(sig_raw);
    char* sig_raw_str = ssh_string_to_char(sig_raw);
    uint8_t* sig_raw_buf = malloc(sig_raw_len);
    memcpy(sig_raw_buf,sig_raw_str,sig_raw_len);
    *out_raw = encode_base58(sig_raw_buf+19,sig_raw_len-19);
    *out_full = sig;
    return 0;
}

int key_sign_raw_unencoded (uint8_t** out, size_t* out_len, const Pubkey signer, const uint8_t* inp, size_t len) {
    char* sig = 0;
    ssh_key privkey = 0;
    if (profile_get_privkey(&privkey)) {
	return 1;
    }
    ssh_string sig_raw = 0;
    if (sshsig_sign(inp,len,privkey,0,"radicle",SSHSIG_DIGEST_SHA2_256,&sig,&sig_raw) != SSH_OK) {
	fprintf(stderr,"Failed to sign the data with the private key\n");
	return 1;
    }
    size_t sig_raw_len = ssh_string_len(sig_raw);
    char* sig_raw_str = ssh_string_to_char(sig_raw);
    uint8_t* sig_raw_buf = malloc(sig_raw_len);
    memcpy(sig_raw_buf,sig_raw_str,sig_raw_len);
    *out = sig_raw_buf+19;
    *out_len = sig_raw_len-19;
    return 0;
}
