#include <stdio.h>
#include <string.h>

#include <commands/self.h>
#include <command.h>
#include <profile.h>
#include <id.h>

SelfCommand command_self_default () {
    SelfCommand cmd;
    cmd.err = 0;
    cmd.did = false;
    cmd.home = false;
    return cmd;
}

void print_help_self () {
    printf("crad self (Show information about your identity and device) Usage:\n");
    printf("crad self [--did] [--home]\n");
}

SelfCommand parse_args_self (int argc, char** argv) {
    SelfCommand cmd = command_self_default();
    for (size_t i=0; i<argc; i++) {
	if (!strcmp(argv[i],"--did")) {
	    cmd.did = true;
	}
	else if (!strcmp(argv[i],"--home")) {
	    cmd.home = true;
	}
    }
    return cmd;
}

int self_run (Command c) {
    if (c.type == CMD_HELP) {
	print_help_self();
	return 0;
    }
    else if (!profile_load()) {
	eprintf("No profile is loaded. Run `crad auth` to create one.");
	return 1;
    }
    else {
	SelfCommand cmd = parse_args_self(c.argc,c.argv);
	if (cmd.err) {
	    return 1;
	}
	if (cmd.did) {
	    Pubkey pubkey = profile_get_pubkey();
	    if (!pubkey.bytes) {
		eprintf("failed to get pubkey");
		return 1;
	    }
	    const char* did = pubkey_to_did(pubkey.bytes);
	    if (!did) {
		eprintf("failed to get did");
		return 1;
	    }
	    printf("%s\n",did);
	}
	else if (cmd.home) {
	    const char* rad_home = get_rad_home();
	    if (!rad_home) {
		eprintf("failed to get rad home");
		return 1;
	    }
	    printf("%s\n",rad_home);
	}
    }
    return 0;
}
