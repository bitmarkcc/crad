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
    cmd.authed = false;
    cmd.alias = false;
    cmd.node_home = false;
    cmd.network_mode = false;
    return cmd;
}

void print_help_self () {
    printf("crad self (Show information about your identity and device) Usage:\n");
    printf("crad self [--did] [--home] [--authed] [--alias] [--node-home] [--network-mode]\n");
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
	else if (!strcmp(argv[i],"--authed")) {
	    cmd.authed = true;
	}
	else if (!strcmp(argv[i],"--alias")) {
	    cmd.alias = true;
	}
	else if (!strcmp(argv[i],"--node-home")) {
	    cmd.node_home = true;
	}
	else if (!strcmp(argv[i],"--network-mode")) {
	    cmd.network_mode = true;
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
	else if (cmd.authed) {
	    if (password_loaded()) {
		printf("yes\n");
		return 0;
	    }
	    else {
		printf("no\n");
		return 1;
	    }
	}
	else if (cmd.alias) {
	    const char* rad_home = get_rad_home();
	    if (!rad_home) {
		eprintf("failed to get rad home");
		return 1;
	    }
	    const char* alias = profile_get_alias(rad_home);
	    if (!alias) {
		eprintf("failed to get alias");
		return 1;
	    }
	    printf("%s\n",alias);
	    return 0;
	}
	else if (cmd.node_home) {
	    const char* node_home = get_rad_node_home();
	    if (!node_home) {
		eprintf("failed to get rad node home");
		return 1;
	    }
	    printf("%s\n",node_home);
	}
	else if (cmd.network_mode) {
	    const char* network_mode = get_rad_network_mode();
	    if (!network_mode) {
		eprintf("failed to get network mode");
		return 1;
	    }
	    printf("%s\n",network_mode);
	}
    }
    return 0;
}
