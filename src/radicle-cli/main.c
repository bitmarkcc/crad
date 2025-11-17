#include <stdio.h>
#include <string.h>

#include <version.h>
#include <commands/auth.h>
#include <commands/init.h>

void print_error(const char *msg) {
    fprintf(stderr, "rad: %s\n", msg);
}

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
	else if (!strcmp(arg,"--help") || !strcmp(arg,"-h")) {
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
	    print_error("unexpected argument");
	    exit(1);
	}
    }
    return cmd;
}

void print_help() {
    printf("Usage: rad [OPTIONS] [COMMAND]\n");
    printf("Radicle command line interface\n");
    printf("Options:\n");
    printf("  --help, -h       Print help information\n");
    printf("  --version        Print version information\n");
    printf("  --json           Output in JSON format (for version command)\n");
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
