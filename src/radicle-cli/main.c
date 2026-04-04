#include <stdio.h>
#include <string.h>

#include <version.h>
#include <print.h>
#include <commands/auth.h>
#include <commands/init.h>
#include <commands/clone.h>
#include <commands/validate.h>
#include <commands/issue.h>
#include <commands/id.h>
#include <commands/self.h>
#include <commands/sync.h>
#include <commands/ls.h>
#include <commands/inspect.h>
#include <commands/patch.h>
#include <commands/source.h>
#include <commands/wallet.h>

Command parse_args(int argc, char** argv) {
    
    Command cmd = {0};
    cmd.type = CMD_OTHER;
    cmd.json = false;
    cmd.argv = 0;
    cmd.argc = 0;

    bool command_set = false;

    for (int i=1; i<argc; i++) {
	char* arg = argv[i];
	if (!strcmp(arg,"--json")) {
	    cmd.json = true;
	}
	else if ((!strcmp(arg,"--help") || !strcmp(arg,"-h")) && i==1) {
	    cmd.type = CMD_HELP;
	    command_set = true;
	}
	else if (!strcmp(arg,"--version") || !strcmp(arg,"version")) {
	    cmd.type = CMD_VERSION;
	    command_set = true;
	}
	else if (!command_set) {
	    if (!strcmp(arg,".")) {
		cmd.type = CMD_OTHER;
		cmd.argc = 1;
		cmd.argv = malloc(sizeof(char *));
		cmd.argv[0] = strdup("inspect");
	    } else {
		cmd.type = CMD_OTHER;
		int remaining = argc - i;
		cmd.argc = remaining;
		cmd.argv = malloc(sizeof(char *) * remaining);
		for (int j=0; j<remaining; j++) {
		    cmd.argv[j] = strdup(argv[i+j]);
		}
		break;
	    }
	    command_set = true;
	}
	else {
	    eprintf("unexpected argument");
	    exit(1);
	}
    }
    return cmd;
}

void print_help() {
    printf("Usage: crad [OPTIONS] [COMMAND] [COMMAND OPTIONS]\n");
    printf("Radicle command line interface\n");
    printf("Options:\n");
    printf("  --help, -h       Print help information\n");
    printf("  --version        Print version information\n");
    printf("  --json           Output in JSON format (for version command)\n");
    printf("Commands:\n");
    printf("  auth\n");
    printf("  clone\n");
    printf("  id\n");
    printf("  init\n");
    printf("  inspect\n");
    printf("  issue\n");
    printf("  ls\n");
    printf("  patch\n");
    printf("  self\n");
    printf("  source\n");
    printf("  sync\n");
    printf("  validate\n");
    printf("  wallet\n");
    // Add more help info as needed
}

void print_version(bool json) {
    if (json) {
	printf("{\"name\":\"%s\",\"version\":\"%s\",\"commit\":\"%s\",\"timestamp\":\"%s\"}\n",
	       VERSION.name, VERSION.version, VERSION.commit, VERSION.timestamp);
    } else {
	printf("%s %s (commit %s, timestamp %s)\n",
	       VERSION.name, VERSION.version, VERSION.commit, VERSION.timestamp);
    }
}

int main (int argc, char** argv)  {

    Command cmd = parse_args(argc,argv);

    int ret = 0;

    switch (cmd.type) {
    case CMD_HELP:
	print_help();
	break;
    case CMD_VERSION:
	print_version(cmd.json);
	break;
    case CMD_OTHER:
	if (!cmd.argc) {
	    printf("No command specified\n");
	}
	else {
	    char* exe = cmd.argv[0];
	    Command subcommand = parse_args(cmd.argc,cmd.argv);	    
	    if (!strcmp(exe,"auth")) {
		return auth_run(subcommand);
	    }
	    else if (!strcmp(exe,"init")) {
		return init_run(subcommand);
	    }
	    else if (!strcmp(exe,"clone")) {
		return clone_run(subcommand);
	    }
	    else if (!strcmp(exe,"validate")) {
		return validate_run(subcommand);
	    }
	    else if (!strcmp(exe,"issue")) {
		return issue_run(subcommand);
	    }
	    else if (!strcmp(exe,"id")) {
		return id_run(subcommand);
	    }
	    else if (!strcmp(exe,"self")) {
		return self_run(subcommand);
	    }
	    else if (!strcmp(exe,"sync")) {
		return sync_run(subcommand);
	    }
	    else if (!strcmp(exe,"ls")) {
		return ls_run(subcommand);
	    }
	    else if (!strcmp(exe,"inspect")) {
		return inspect_run(subcommand);
	    }
	    else if (!strcmp(exe,"patch")) {
		return patch_run(subcommand);
	    }
	    else if (!strcmp(exe,"source")) {
		return source_run(subcommand);
	    }
	    else if (!strcmp(exe,"wallet")) {
		return wallet_run(subcommand);
	    }
	    else {
		eprintf("invalid command (%s)",exe);
	    }
	}
	break;
    default:
	fprintf(stderr,"unknown command");
	ret = 1;
	break;
    }

    free_command(&cmd);
	
    return ret;
}
