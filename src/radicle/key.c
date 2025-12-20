#include <stdlib.h>
#include <libssh/libssh.h>
#include <stdio.h>
#include <string.h>
#include <openssl/evp.h>

#include <key.h>
#include <base58.h>
#include <base64.h>
#include <profile.h>
#include <print.h>
#include <util.h>

int key_sign_openssh (char** out_raw, char** out_full, const Pubkey signer, const uint8_t* inp, size_t len) {
    char* sig = 0;
    ssh_key privkey = 0;
    if (profile_get_privkey(&privkey)) {
	return 1;
    }
    uint8_t* privkey_raw = 0;
    if (ssh_pki_get_privkey_raw(privkey,&privkey_raw) != SSH_OK) {
	fprintf(stderr,"Failed to get raw private key\n");
	return 1;
    }
    uint8_t* pubkey_raw = 0;
    if (ssh_pki_get_pubkey_raw(privkey,&pubkey_raw) != SSH_OK) {
	fprintf(stderr,"Failed to get raw public key\n");
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

int key_sign_bytes (uint8_t out [64], const Pubkey signer, const uint8_t* inp, size_t len) {
    uint8_t* sig = malloc(64);
    size_t siglen = 64;
    ssh_key privkey = 0;
    if (profile_get_privkey(&privkey)) {
	return 1;
    }
    uint8_t* privkey_raw = 0;
    if (ssh_pki_get_privkey_raw(privkey,&privkey_raw) != SSH_OK) {
	eprintf("Failed to get raw private key");
	goto err;
    }
    EVP_PKEY* okey = EVP_PKEY_new_raw_private_key(EVP_PKEY_ED25519,0,privkey_raw,32); // openssl key
    
    EVP_MD_CTX* mdctx = 0;
    int ret = 0;

    if (!(mdctx = EVP_MD_CTX_create())) goto err;
    if (EVP_DigestSignInit(mdctx,0,0,0,okey)!=1) goto err;
    if (EVP_DigestSign(mdctx,sig,&siglen,inp,len)!=1) goto err;

    memcpy(out,sig,64);
 err:
    return 1;
}

int key_sign_base58 (char** out, const Pubkey signer, const uint8_t* inp, size_t len) {
    uint8_t* sig = malloc(64);
    size_t siglen = 64;
    ssh_key privkey = 0;
    if (profile_get_privkey(&privkey)) {
	return 1;
    }
    uint8_t* privkey_raw = 0;
    if (ssh_pki_get_privkey_raw(privkey,&privkey_raw) != SSH_OK) {
	eprintf("Failed to get raw private key");
	goto err;
    }
    EVP_PKEY* okey = EVP_PKEY_new_raw_private_key(EVP_PKEY_ED25519,0,privkey_raw,32); // openssl key
    
    EVP_MD_CTX* mdctx = 0;
    int ret = 0;

    if (!(mdctx = EVP_MD_CTX_create())) goto err;
    if (EVP_DigestSignInit(mdctx,0,0,0,okey)!=1) goto err;
    if (EVP_DigestSign(mdctx,sig,&siglen,inp,len)!=1) goto err;

    *out = encode_base58(sig,64);
    
 err:
    return 1;
}

char* rad_sig_to_ssh_format (const uint8_t* sig_bytes, Pubkey signer) {
    uint8_t ssh_sig_bytes [177] =
	{0x53,0x53,0x48,0x53,0x49,0x47,0x00,0x00,0x00,0x01,0x00,0x00,0x00,0x33,0x00,0x00,
	 0x00,0x0b,0x73,0x73,0x68,0x2d,0x65,0x64,0x32,0x35,0x35,0x31,0x39,0x00,0x00,0x00,
	 0x20,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, // pubkey
	 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	 0x00,0x00,0x00,0x00,0x07,0x72,0x61,0x64,0x69,0x63,0x6c,0x65,0x00,0x00,0x00,0x00,
	 0x00,0x00,0x00,0x06,0x73,0x68,0x61,0x32,0x35,0x36,0x00,0x00,0x00,0x53,0x00,0x00,
	 0x00,0x0b,0x73,0x73,0x68,0x2d,0x65,0x64,0x32,0x35,0x35,0x31,0x39,0x00,0x00,0x00,
	 0x40,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, // sig
	 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	 0x00};
    memcpy(ssh_sig_bytes+33,signer.bytes,32);
    memcpy(ssh_sig_bytes+113,sig_bytes,64);
    char* ssh_sig_base64 = encode_base64(ssh_sig_bytes,177);
    char* out = malloc(strlen(ssh_sig_base64)+62);
    sprintf(out,"-----BEGIN SSH SIGNATURE-----\n%s\n-----END SSH SIGNATURE-----",rad_str_with_line_size(ssh_sig_base64,70));
    return out;
}
