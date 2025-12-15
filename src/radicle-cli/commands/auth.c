#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <commands/auth.h>
#include <profile.h>
#include <util.h>

int auth_run (Command c) {
    if (profile_load()) {
	if (password_loaded()) {
	    printf("You are already authenticated\n");
	    return 0;
	}
	ssh_key key = 0;
	if (profile_get_privkey(&key)) {
	    return 1;
	}
	return 0;
    }
    else {
	return auth_init();
    }
    return 0;
}

int auth_init() {
    printf("Initializing your radicle identity\n");
    char alias [RAD_BUFSIZ];
    char* env_user = getenv("USER");
    printf("? Enter your alias (%s) ",env_user);
    rad_get_input(alias,RAD_BUFSIZ);
    if (!strlen(alias)) rad_strcpy(alias,env_user,0,RAD_BUFSIZ-1);
    char* passphrase = 0;
    while (1) {
	printf("? Enter a passphrase: ");
	passphrase = get_password();
	printf("? Repeat passphrase: ");
	char* passphrase_repeat = get_password();
	if (!strcmp(passphrase,passphrase_repeat))
	    break;
	else
	    fprintf(stderr,"Passphrases do not match\n");
    }
    if (passphrase && !strlen(passphrase)) {
	free(passphrase);
	passphrase = 0;
    }
    printf("Creating your Ed25519 keypair...\n");
    uint8_t* seed = 0;
    char* env_seed = getenv("CRAD_KEYGEN_SEED");
    if (env_seed && strlen(env_seed)==64) {
	seed = rad_hex_to_uchar(env_seed);
    }
    profile_init(alias,passphrase,seed);
    
    if (seed) free(seed);
    if (passphrase) {
	memset(passphrase,0,strlen(passphrase));
	free(passphrase);
    }
    return 0;
}
