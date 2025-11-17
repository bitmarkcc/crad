#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <libssh/libssh.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <profile.h>
#include <base58.h>

char* get_rad_home() {
    char* rad_home = 0;
    char* env_rad_home = getenv("RAD_HOME");
    if (env_rad_home) {
	rad_home = strdup(env_rad_home);
    }
    else {
	char* env_home = getenv("HOME");
	if (env_home) {
	    rad_home = malloc(strlen(env_home)+10);
	    strcpy(rad_home,env_home);
	    strcat(rad_home,"/.radicle");
	}
    }
    return rad_home;
}

bool profile_load() {

    bool ret = false;
    char* rad_home = get_rad_home();

    if (rad_home) {
	char* keydir = malloc(strlen(rad_home)+6);
	strcpy(keydir,rad_home);
	strcat(keydir,"/keys");
	if (!access(keydir,F_OK)) {
	    ret = true;
	}
	free(rad_home);
    }
    
    return ret;
}

bool profile_init (const char* alias, const char* passphrase, const uint8_t* seed) {

    char* rad_home = get_rad_home();

    if (!rad_home) {
	fprintf(stderr,"Can't find radicle home directory\n");
	return false;
    }

    if (access(rad_home,F_OK)) {
	if (mkdir(rad_home,0755)) {
	    fprintf(stderr,"Can't create radicle home directory\n");
	    free(rad_home);
	    return false;
	}
    }
    
    char* keydir = malloc(strlen(rad_home)+6);
    strcpy(keydir,rad_home);
    strcat(keydir,"/keys");

    if (access(keydir,F_OK)) {
	if (mkdir(keydir,0755)) {
	    fprintf(stderr,"Can't create keys directory\n");
	    free(rad_home);
	    free(keydir);
	    return false;
	}
    }
    
    ssh_key key = 0;
    int rc = ssh_pki_generate(SSH_KEYTYPE_ED25519,0,seed,&key);
    if (rc != SSH_OK) {
	fprintf(stderr,"Failed to generate private key\n");
	free(rad_home);
	free(keydir);
	return false;
    }

    char* privkeyfile = malloc(strlen(keydir)+9);
    strcpy(privkeyfile,keydir);
    strcat(privkeyfile,"/radicle");
    
    rc = ssh_pki_export_privkey_file_format(key,passphrase,0,0,privkeyfile,SSH_FILE_FORMAT_OPENSSH);
    if (rc != SSH_OK) {
	fprintf(stderr,"Failed to write to a private key file\n");
	free(rad_home);
	free(keydir);
	free(privkeyfile);
	return false;
    }

    char* pubkeyfile = malloc(strlen(keydir)+13);
    strcpy(pubkeyfile,keydir);
    strcat(pubkeyfile,"/radicle.pub");

    // todo remove user@host from output
    rc = ssh_pki_export_pubkey_file(key,pubkeyfile);
    if (rc != SSH_OK) {
	fprintf(stderr,"Failed to write to public key file\n");
	free(rad_home);
	free(keydir);
	free(privkeyfile);
	free(pubkeyfile);
	return false;
    }

    uint8_t* pubkey_raw = 0;
    rc = ssh_pki_get_pubkey_raw(key,&pubkey_raw);
    if (rc) {
	fprintf(stderr,"Failed to get raw public key\n");
	free(rad_home);
	free(keydir);
	free(privkeyfile);
	free(pubkeyfile);
	if (pubkey_raw) free(pubkey_raw);
	return false;
    }

    uint8_t did_buf [34];
    did_buf[0] = 0xED;
    did_buf[1] = 0x1;
    memcpy(did_buf+2,pubkey_raw,32);

    char* did = encode_base58(did_buf,34);

    if (did) {
	printf("Your Radicle DID is did:key:z%s\n",did);
    }
    else {
	fprintf(stderr,"Failed to obtain your Radicle DID\n");
	free(rad_home);
	free(keydir);
	free(privkeyfile);
	free(pubkeyfile);
	if (pubkey_raw) free(pubkey_raw);
	return false;
    }

    free(rad_home);
    free(keydir);
    free(privkeyfile);
    free(pubkeyfile);
    if (pubkey_raw) free(pubkey_raw);
    if (did) free(did);
    return true;
}
