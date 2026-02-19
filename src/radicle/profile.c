#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <json-c/json.h>
#include <sqlite3.h>

#include <profile.h>
#include <id.h>
#include <util.h>
#include <print.h>

char* get_rad_home () {
    char* rad_home = 0;
    char* env_rad_home = getenv("CRAD_HOME");
    if (env_rad_home) {
	rad_home = strdup(env_rad_home);
    }
    else {
	char* env_home = getenv("HOME");
	if (env_home) {
	    rad_home = malloc(strlen(env_home)+11);
	    strcpy(rad_home,env_home);
	    strcat(rad_home,"/.cradicle");
	}
    }
    if (!rad_home) fprintf(stderr,"Can't find Radicle Home directory\n");
    return rad_home;
}

char* get_rad_node_home () {
    char* rad_home = 0;
    char* env_rad_home = getenv("RAD_HOME");
    if (env_rad_home) {
	rad_home = strdup(env_rad_home);
    }
    else {
	char* env_home = getenv("HOME");
	if (env_home) {
	    rad_home = malloc(strlen(env_home)+11);
	    strcpy(rad_home,env_home);
	    strcat(rad_home,"/.radicle");
	}
    }
    if (!rad_home) fprintf(stderr,"Can't find Radicle Home directory\n");
    return rad_home;
}

char* get_cob_dir () {
    const char* rad_home = get_rad_home();
    char* cob_dir = malloc(strlen(rad_home)+6);
    sprintf(cob_dir,"%s/cobs",rad_home);
    return cob_dir;
}

char* get_cob_cache_file () {
    const char* rad_home = get_rad_home();
    char* cob_cache_file = malloc(strlen(rad_home)+15);
    sprintf(cob_cache_file,"%s/cobs/cache.db",rad_home);
    return cob_cache_file;
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
    ssh_key key = 0;
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

char* profile_get_password (const char* rad_home) {
    char* passphrase = malloc(RAD_BUFSIZ);
    char* pw_file = malloc(strlen(rad_home)+13);
    sprintf(pw_file,"%s/.pw/radicle",rad_home);
    FILE* f = 0;
    if (password_loaded()) {
	f = fopen(pw_file,"r");
	if (!f) {
	    fprintf(stderr,"Can't open password file for reading\n");
	    return 0;
	}
	if (!fgets(passphrase,RAD_BUFSIZ,f)) {
	    fprintf(stderr,"Can't read passphrase from password file\n");
	    return 0;
	}
	fclose(f);
	rad_rstrip_nl(passphrase);
    }
    else {
	printf("? passphrase: ");
	passphrase = get_password();
	f = fopen(pw_file,"w");
	if (!f) {
	    fprintf(stderr,"Can't open password file for writing\n");
	    return 0;
	}
	if (fputs(passphrase,f)==EOF) {
	    fprintf(stderr,"Failed to put password in password file\n");
	    return 0;
	}
	fclose(f);
    }
    return passphrase;
}

Pubkey profile_get_pubkey_from_privkey() {
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
    char* privkeyfile = malloc(strlen(keydir)+9);
    strcpy(privkeyfile,keydir);
    strcat(privkeyfile,"/radicle");
    ssh_key key = 0;
    char* passphrase = profile_get_password(rad_home);
    if (ssh_pki_import_privkey_file(privkeyfile,passphrase,0,0,&key) != SSH_OK) {
	fprintf(stderr,"Failed to import privkey file\n");
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
    free(privkeyfile);
    return pubkey;
}

int profile_get_privkey (ssh_key* key, char* passphrase) {
    char* rad_home = get_rad_home();
    if (!rad_home) {
	fprintf(stderr,"Can't get Radicle home directory\n");
	return 1;
    }
    char* keydir = malloc(strlen(rad_home)+6);
    strcpy(keydir,rad_home);
    strcat(keydir,"/keys");
    if (access(keydir,F_OK)) {
	fprintf(stderr,"Can't find Radicle keys directory\n");
	return 1;
    }
    char* privkeyfile = malloc(strlen(keydir)+9);
    strcpy(privkeyfile,keydir);
    strcat(privkeyfile,"/radicle");
    if (passphrase) {
	char* pw_fname = malloc(strlen(rad_home)+13);
	sprintf(pw_fname,"%s/.pw/radicle",rad_home);
	if (!password_loaded()) {
	    FILE* f = fopen(pw_fname,"w");
	    if (!f) {
		fprintf(stderr,"Can't open password file for writing\n");
		return 0;
	    }
	    if (fputs(passphrase,f)==EOF) {
		fprintf(stderr,"Failed to put password in password file\n");
		return 0;
	    }
	    fclose(f);
	}
    }
    else {
	passphrase = profile_get_password(rad_home);
    }
    if (ssh_pki_import_privkey_file(privkeyfile,passphrase,0,0,key) != SSH_OK) {
	fprintf(stderr,"Failed to import privkey file (check passphrase)\n");
	return 1;
    }
    if (passphrase) memset(passphrase,0,strlen(passphrase));
    return 0;
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

    char* storagedir = malloc(strlen(rad_home)+9);
    sprintf(storagedir,"%s/storage",rad_home);

    char* ssh_dir = malloc(strlen(rad_home)+6);
    sprintf(ssh_dir,"%s/.ssh",rad_home);
    
    if (access(keydir,F_OK)) {
	if (mkdir(keydir,0700)) {
	    fprintf(stderr,"Can't create keys directory\n");
	    free(rad_home);
	    free(keydir);
	    return false;
	}
    }

    if (access(storagedir,F_OK)) {
	if (mkdir(storagedir,0755)) {
	    fprintf(stderr,"Can't create storage directory\n");
	    return false;
	}
    }

    if (access(ssh_dir,F_OK)) {
	if (mkdir(ssh_dir,0755)) {
	    eprintf("Can't create ssh directory");
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
    if (chmod(privkeyfile,strtol("0600",0,8))) {
	eprintf("chmod failed");
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

    // add password to password file
    
    char* pw_dir = malloc(strlen(rad_home)+5);
    sprintf(pw_dir,"%s/.pw",rad_home);
    if (access(pw_dir,F_OK)) {
	char tmp_template[] = "/tmp/crad-pw-XXXXXX";
	char* tmp_dir = mkdtemp(tmp_template);
	if (!tmp_dir) {
	    fprintf(stderr,"Can't create temp password directory\n");
	    return false;
	}
	chmod(tmp_dir,0700);
	if (symlink(tmp_dir,pw_dir)) {
	    fprintf(stderr,"Can't create symlink to password directory\n");
	    return false;
	}
    }
    char* pw_file = malloc(strlen(pw_dir)+9);
    sprintf(pw_file,"%s/radicle",pw_dir);
    f = fopen(pw_file,"w");
    if (!f) {
	fprintf(stderr,"Can't open password file for writing\n");
	return 1;
    }
    if (fputs(passphrase,f)==EOF) {
	fprintf(stderr,"Failed to put password in password file\n");
	return 1;
    }
    fclose(f);
    
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

    // init cob cache
    const char* cob_dir = get_cob_dir();
    const char* db_file = get_cob_cache_file();
    if (access(cob_dir,F_OK)) {
	if (mkdir(get_cob_dir(),0700)) {
	    eprintf("failed to create cob dir");
	    return 1;
	}
	sqlite3* db = 0;
	sqlite3_stmt* stmt = 0;
	sqlite3_open(db_file,&db);
	if (!db) {
	    eprintf("failed to open cob db");
	    return 1;
	}
	const char* sql = "CREATE TABLE Allowed (RID TEXT, DID TEXT);";
	char* err_msg = 0;
	if (sqlite3_exec(db,sql,0,0,&err_msg)) {
	    eprintf("failed to execute sql command: %s",err_msg);
	    return 1;
	}
	sql = "CREATE TABLE Issues (ID BLOB PRIMARY KEY, RID BLOB, EntryID BLOB, EditID BLOB, Time INTEGER, Author TEXT, Alias TEXT, Status TEXT, Reason TEXT);";
	err_msg = 0;
	if (sqlite3_exec(db,sql,0,0,&err_msg)) {
	    eprintf("failed to execute sql command: %s",err_msg);
	    return 1;
	}
	sql = "CREATE TABLE Comments (ID BLOB PRIMARY KEY, EditID BLOB, Time INTEGER, Issue BLOB, ReplyTo BLOB, Author TEXT, Alias TEXT);";
	err_msg = 0;
	if (sqlite3_exec(db,sql,0,0,&err_msg)) {
	    eprintf("failed to execute sql command: %s",err_msg);
	    return 1;
	}
	sql = "CREATE TABLE Assignees (DID TEXT, Issue BLOB);";
	err_msg = 0;
	if (sqlite3_exec(db,sql,0,0,&err_msg)) {
	    eprintf("failed to execute sql command: %s",err_msg);
	    return 1;
	}
	sql = "CREATE TABLE Labels (Label TEXT, Issue BLOB);";
	err_msg = 0;
	if (sqlite3_exec(db,sql,0,0,&err_msg)) {
	    eprintf("failed to execute sql command: %s",err_msg);
	    return 1;
	}
	sql = "CREATE TABLE Reactions (ID BLOB PRIMARY KEY, Time INTEGER, Issue BLOB, ReplyTo BLOB);";
	err_msg = 0;
	if (sqlite3_exec(db,sql,0,0,&err_msg)) {
	    eprintf("failed to execute sql command: %s",err_msg);
	    return 1;
	}
    }

    // Create radicle (not Cradicle) keypair in $RAD_HOME, if $RAD_HOME/keys does not exists

    const char* rad_node_home = get_rad_node_home();

    char* argv[5];
    char* bin_path = malloc(strlen(rad_home)+32);
    sprintf(bin_path,"%s/bin/rad-auth-wrapped",rad_home);
    argv[0] = bin_path;
    argv[1] = "--alias";
    argv[2] = strdup(alias);
    argv[3] = "--stdin";
    argv[4] = 0;
    
    if (exec_command_inp(bin_path,argv,"\n")) {
	eprintf("rad auth (wrapped) command failed");
	return 1;
    }
    
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
    si.email = malloc(strlen(si.name)+2+128);
    strcpy(si.email,si.name);
    strcat(si.email,"@");
    strcat(si.email,pubkey_to_did(profile_get_pubkey().bytes)+8);
    s.info = si;
    return s;
}

bool password_loaded() {
    const char* rad_home = get_rad_home();
    if (rad_home) {
	char* pwfile = malloc(strlen(rad_home)+13);
	sprintf(pwfile,"%s/.pw/radicle",rad_home);
	if (!access(pwfile,F_OK)) {
	    return true;
	}
    }
    return false;
}

int load_cleartext_privkey_file (const char* rad_home) {
    ssh_key key = 0;
    if (profile_get_privkey(&key,0)) {
	eprintf("failed to get privkey");
	return 1;
    }
    char* privkey_fname = malloc(strlen(rad_home)+17);
    sprintf(privkey_fname,"%s/.pw/radicle.key",rad_home);
    int rc = ssh_pki_export_privkey_file_format(key,0,0,0,privkey_fname,SSH_FILE_FORMAT_OPENSSH); // null pword
    if (rc != SSH_OK) {
	eprintf("failed to write to a private key file\n");
	return 1;
    }
    if (chmod(privkey_fname,strtol("0600",0,8))) {
	eprintf("chmod failed");
	return false;
    }
}

int unload_cleartext_privkey_file (const char* rad_home) {
    char* privkey_fname = malloc(strlen(rad_home)+17);
    sprintf(privkey_fname,"%s/.pw/radicle.key",rad_home);
    if (remove(privkey_fname)) {
	eprintf("failed to delete cleartext private key");
	return 1;
    }
}

int unload_password () {
    char* rad_home = get_rad_home();
    if (rad_home) {
	char* pw_fname = malloc(strlen(rad_home)+13);
	sprintf(pw_fname,"%s/.pw/radicle",rad_home);
	if (access(pw_fname,F_OK)) {
	    eprintf("password already unloaded");
	    return 1;
	}
	if (remove(pw_fname)) {
	    eprintf("failed to delete password file");
	    return 1;
	}
    }
    return 0;
}
