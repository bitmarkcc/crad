#include <stdio.h>
#include <string.h>

#include <main.h>
#include <version.h>
#include <command.h>
#include <print.h>
#include <profile.h>
#include <id.h>
#include <util.h>

const int sshd_port = 8777;

Command parse_args(int argc, char** argv) {
    
    Command cmd = {0};

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
	else {
	    eprintf("unexpected argument");
	    exit(1);
	}
    }
    return cmd;
}

void print_help() {
    printf("Usage: radicle-node [OPTIONS]\n");
    printf("Radicle P2P node\n");
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
	ret = node_run();
	break;
    default:
	eprintf("unknown command");
	ret = 1;
	break;
    }

    free_command(&cmd);
	
    return ret;
}

int node_run () {

    if (!profile_load()) {
	eprintf("No profile is loaded. Run `rad auth` to create one");
	return 1;
    }
    else if (!password_loaded()) {
	eprintf("You must be authenticated first. Run `rad auth` to authenticate\n");
	return 1;
    }

    iprintf("Starting node");
    iprintf("Version %s %s (commit %s, timestamp %s)",VERSION.name,VERSION.version,VERSION.commit,VERSION.timestamp);
    iprintf("Loading private key");
    ssh_key key = 0;
    if (profile_get_privkey(&key,0)) {
	eprintf("Unable to load private key");
	return 1;
    }
    iprintf("Node ID is %s",pubkey_to_did(profile_get_pubkey().bytes)+8);
    iprintf("Starting SSHD on port %d",sshd_port);
    const char* rad_home = get_rad_home();
    load_cleartext_privkey_file(rad_home);
    char* sshd_fname = malloc(strlen(rad_home)+18);
    sprintf(sshd_fname,"%s/.ssh/sshd_config",rad_home);
    FILE* f_sshd = fopen(sshd_fname,"w");
    char line [RAD_BUFSIZ];
    sprintf(line,"Port %d\n",sshd_port);
    fputs(line,f_sshd);
    sprintf(line,"HostKey %s/.pw/radicle.key\n",rad_home);
    fputs(line,f_sshd);
    sprintf(line,"AuthorizedKeysFile %s/.ssh/authorized_keys\n",rad_home);
    fputs(line,f_sshd);
    sprintf(line,"PidFile %s/.ssh/sshd.pid\n",rad_home);
    fputs(line,f_sshd);
    fputs("StrictModes no\n",f_sshd);
    fputs("PasswordAuthentication no\n",f_sshd);
    fputs("PubkeyAuthentication yes\n",f_sshd);
    //fputs("KexAlgorithms +ecdh-sha2-secp256k1\n",f_sshd);
    //fputs("HostKeyAlgorithms +ecdsa-sha2-secp256k1\n",f_sshd);
    //fputs("Match User *,!ak\n",f_sshd);
    //fputs("  ForceCommand /bin/bash --login\n",f_sshd);
    fclose(f_sshd);
    
    char* argv [8];
    argv[0] = "/usr/sbin/sshd";
    argv[1] = "-f";
    argv[2] = sshd_fname;
    argv[3] = 0;

    if (exec_command("/usr/sbin/sshd",argv)) {
	eprintf("sshd command failed");
	return 1;
    }
    
    iprintf("Cradicle Node successfully executed");
    
    return 0;
}
