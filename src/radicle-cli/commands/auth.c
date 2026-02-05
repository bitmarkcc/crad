#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <commands/auth.h>
#include <profile.h>
#include <util.h>

AuthCommand command_auth_default() {
    AuthCommand cmd;
    cmd.err = 0;
    cmd.alias = 0;
    cmd.passphrase = 0;
    cmd.deauth = false;
    return cmd;
}

AuthCommand parse_args_auth (int argc, char** argv) {
    AuthCommand cmd = command_auth_default();
    for (size_t i=0; i<argc; i++) {
	if (!strcmp(argv[i],"--alias")) {
	    if (i+1 < argc) cmd.alias = strdup(argv[i+1]);
	}
	else if (!strcmp(argv[i],"--passphrase") || !strcmp(argv[i],"--password")) {
	    if (i+1 < argc) cmd.passphrase = strdup(argv[i+1]);
	}
	else if (!strcmp(argv[i],"--deauth")) {
	    cmd.deauth = true;
	}
    }
    return cmd;
}

void print_help_auth () {
    printf("crad auth (Authenticate) Usage:\n");
    printf("crad auth [--alias <alias>] [--passphrase <passphrase>] [--deauth]\n");
}

int auth_run (Command c) {
    if (c.type == CMD_HELP) {
	print_help_auth();
	return 0;
    }
    else if (profile_load()) {
	AuthCommand cmd = parse_args_auth(c.argc,c.argv);
	if (cmd.deauth) {
	    if (unload_password()) return 1;
	    iprintf("you are now deauthenticated");
	    return 0;
	}
	else if (password_loaded()) {
	    eprintf("you are already authenticated");
	    return 1;
	}
	ssh_key key = 0;
	if (cmd.passphrase) {
	    if (profile_get_privkey(&key,cmd.passphrase)) {
		if (password_loaded()) unload_password();
		return 1;
	    }
	}
	else if (profile_get_privkey(&key,0)) {
	    unload_password();
	    return 1;
	}
	return 0;
    }
    else {
	AuthCommand cmd = parse_args_auth(c.argc,c.argv);
	return auth_init(cmd.alias,cmd.passphrase);
    }
    return 0;
}

int auth_init (const char* cmd_alias, const char* cmd_passphrase) {
    printf("Initializing your radicle identity\n");
    char alias [RAD_BUFSIZ];
    if (cmd_alias) {
	strcpy(alias,cmd_alias);
    }
    else {
	char* env_user = getenv("USER");
	printf("? Enter your alias (%s) ",env_user);
	rad_get_input(alias,RAD_BUFSIZ);
	if (!strlen(alias)) rad_strcpy(alias,env_user,0,RAD_BUFSIZ-1);
    }
    char* passphrase = 0;
    if (cmd_passphrase) {
	passphrase = strdup(cmd_passphrase);
    }
    else {
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
