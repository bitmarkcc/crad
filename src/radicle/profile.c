#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <libssh/libssh.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <json-c/json.h>

#include <profile.h>
#include <id.h>
#include <util.h>

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
    if (!rad_home) fprintf(stderr,"Can't find Radicle Home directory\n");
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

Pubkey profile_get_pubkey() {
    Pubkey pubkey;
    pubkey.bytes = 0;
    char* rad_home = get_rad_home();
    if (!rad_home) {
	fprintf(stderr,"Can't get Radicle home directory\n");
	return pubkey;
    }
    char* keydir = malloc(strlen(rad_home)+6);
    strcpy(keydir,rad_home);
    strcat(keydir,"/keys");
    if (access(keydir,F_OK)) {
	fprintf(stderr,"Can't find Radicle keys directory\n");
	return pubkey;
    }
    char* pubkeyfile = malloc(strlen(keydir)+13);
    strcpy(pubkeyfile,keydir);
    strcat(pubkeyfile,"/radicle.pub");
    ssh_key key;
    if (ssh_pki_import_pubkey_file(pubkeyfile,&key) != SSH_OK) {
	fprintf(stderr,"Failed to import pubkey file\n");
	return pubkey;
    }
    uint8_t* pubkey_raw = 0;
    if (ssh_pki_get_pubkey_raw(key,&pubkey_raw) != SSH_OK) {
	fprintf(stderr,"Failed to get raw public key\n");
	return pubkey;
    }
    pubkey.bytes = pubkey_raw;
    free(rad_home);
    free(keydir);
    free(pubkeyfile);
    return pubkey;
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

    size_t rad_home_len = strlen(rad_home);

    json_object* obj = json_object_new_object();
    json_object* node_obj = json_object_new_object();
    json_object_object_add(node_obj,"alias",json_object_new_string(alias));
    json_object_object_add(obj,"node",node_obj);

    char* config_file = malloc(rad_home_len+13);
    strcpy(config_file,rad_home);
    strcat(config_file,"/config.json");

    FILE* f = fopen(config_file,"w");
    if (!f) {
	fprintf(stderr,"Can't open config file for writing");
	free(rad_home);
	free(config_file);
	return false;
    }
    fprintf(f,"%s",json_object_to_json_string(obj));
    fclose(f);
    free(config_file);
    
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
    if (rc != SSH_OK) {
	fprintf(stderr,"Failed to get raw public key\n");
	free(rad_home);
	free(keydir);
	free(privkeyfile);
	free(pubkeyfile);
	if (pubkey_raw) free(pubkey_raw);
	return false;
    }

    char* did = pubkey_to_did(pubkey_raw);

    if (did) {
	printf("Your Radicle DID is %s\n",did);
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

char* profile_get_alias (const char* rad_home) {

    char* config_file = malloc(strlen(rad_home)+13);
    strcpy(config_file,rad_home);
    strcat(config_file,"/config.json");

    FILE* f = fopen(config_file,"r");
    if (!f) {
	fprintf(stderr,"Cannot open config file for reading\n");
	return 0;
    }

    char* buf = 0;
    size_t len = 0;
    ssize_t n_bytes_read = getdelim(&buf,&len,'\0',f);
    if (n_bytes_read<0) {
	fprintf(stderr,"Failed to read config file\n");
	return 0;
    }
    json_object* config_obj = json_tokener_parse(buf);
    json_object_object_foreach(config_obj,key,val) {
	if (!strcmp(key,"node")) {
	    json_object_object_foreach(val,key2,val2) {
		if (!strcmp(key2,"alias")) {
		    return rad_strip('"',json_object_to_json_string(val2));
		}
	    }
	}
    }
    return 0;
}

Storage profile_get_storage () {
    Storage s;
    s.path = 0;
    char* rad_home = get_rad_home();
    if (!rad_home) return s;
    s.path = malloc(strlen(rad_home)+9);
    strcpy(s.path,rad_home);
    strcat(s.path,"/storage");
    StorageInfo si;
    si.name = profile_get_alias(rad_home);
    si.email = malloc(strlen(si.name)*2+2);
    strcpy(si.email,si.name);
    strcat(si.email,"@");
    strcat(si.email,si.name);
    s.info = si;
    return s;
}
