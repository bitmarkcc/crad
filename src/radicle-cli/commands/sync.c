#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>

#include <commands/sync.h>
#include <command.h>
#include <profile.h>
#include <util.h>
#include <git.h>
#include <id.h>
#include <repo.h>

SyncCommand command_sync_default() {
    SyncCommand cmd;
    cmd.err = 0;
    cmd.seed = 0;
    return cmd;
}

void print_help_sync () {
    printf("crad sync (Sync Repository) Usage:\n");
    printf("crad sync [-s <seed>]\n");
}

SyncCommand parse_args_sync (int argc, char** argv) {
    SyncCommand cmd = command_sync_default();
    for (size_t i=0; i<argc; i++) {
	if (!strcmp(argv[i],"--seed") || !strcmp(argv[i],"-s")) {
	    if (i+1 < argc) cmd.seed = strdup(argv[i+1]);
	}
    }
    return cmd;
}

int sync_run (Command c) {
    if (c.type == CMD_HELP) {
	print_help_sync();
	return 0;
    }
    else if (!profile_load()) {
	eprintf("No profile is loaded. Run `crad auth` to create one.");
	return 1;
    }
    SyncCommand cmd = parse_args_sync(c.argc,c.argv);
    if (cmd.err) {
	return 1;
    }
    Oid rid = {{0}};
    char* rid_str = 0;
    char cwd [1024];
    if (cmd.seed) { // sync from the seed using crad
	if (!password_loaded()) {
	    eprintf("You must be authenticated first. Run `crad auth` to authenticate");
	    return 1;
	}
	// get rid of repo in cwd
	if (!getcwd(cwd,sizeof(cwd))) {
	    fprintf(stderr,"Can't get current working directory\n");
	    return 1;
	}
	rad_git_init();
	git_repository* repo = 0;
	if (git_repository_open(&repo,cwd)) {
	    eprintf("failed to open git repository at %s",cwd);
	    return 1;
	}
	rid = rid_of_rad_remote(repo);
	if (git_oid_is_zero(&rid)) {
	    eprintf("failed to get rid from CWD");
	    return 1;
	}
	rid_str = oid_to_rid(rid);
	
	const char* userhost = strtok(strdup(cmd.seed),":");
	const char* port = strtok(0,":");
	const char* user = strtok(strdup(userhost),"@");
	const char* host = strtok(0,"@");
	// create ssh config file
	const char* rad_home = get_rad_home();
	char* config_dir = malloc(strlen(rad_home)+6);
	sprintf(config_dir,"%s/.ssh",rad_home);
	if (access(config_dir,F_OK)) {
	    if (mkdir(config_dir,0755)) {
		eprintf("Can't create ssh directory");
		return 1;
	    }
	}
	char* config_fname = malloc(strlen(rad_home)+13);
	sprintf(config_fname,"%s/.ssh/config",rad_home);
	FILE* f = fopen(config_fname,"w");
	char line [256];
	fputs("Host seed\n",f);
	sprintf(line,"  HostName %s\n",host);
	fputs(line,f);
	sprintf(line,"  Port %s\n",port);
	fputs(line,f);
	sprintf(line,"  User %s\n",user);
	fputs(line,f);
	sprintf(line,"  IdentityFile %s/.pw/radicle.key\n",rad_home);
	fputs(line,f);
	fputs("  ProxyCommand ncat --proxy-type socks5 --proxy 127.0.0.1:9050 %h %p\n",f);
	fputs("  StrictHostKeyChecking no\n",f);
	fputs("  UserKnownHostsFile /dev/null\n",f);
	fclose(f);
	char* argv [10];
	argv[0] = "rsync";
	argv[1] = "-az";
	argv[2] = "--no-o";
	argv[3] = "--no-g";
	argv[4] = "--old-args";
	argv[5] = "-e";
	char* rsh = malloc(strlen(rad_home)+20);
	sprintf(rsh,"ssh -F %s/.ssh/config",rad_home);
	argv[6] = rsh;
	char* src = malloc(strlen(cmd.seed)+strlen(rid_str)+2);
	sprintf(src,"seed:%s",rid_str);
	argv[7] = src;
	char* dst = malloc(strlen(rad_home)+128);
	sprintf(dst,"%s/storage/%s/",rad_home,rid_str);
	argv[8] = dst;
	argv[9] = 0;
	iprintf("exec: %s -- %s -- %s -- %s -- %s -- %s -- %s -- %s -- %s",argv[0],argv[1],argv[2],argv[3],argv[4],argv[5],argv[6],argv[7],argv[8]);

	char* dst_dir = strdup(dst);
	dst_dir[strlen(dst_dir)-1] = 0;
	if (!access(dst_dir,F_OK)) {
	    char* dst_dir_bak = malloc(strlen(dst_dir)+64);
	    sprintf(dst_dir_bak,"%s.bak.%ld",dst_dir,time(0));
	    if (rename(dst_dir,dst_dir_bak)) {
		eprintf("failed to rename file");
		return 1;
	    }
	}

	if (load_cleartext_privkey_file(rad_home)) {
	    eprintf("failed to load cleartext privkey file");
	    return 1;
	}	
	if (exec_command("rsync",argv)) {
	    eprintf("rsync command failed");
	    return 1;
	}
	if (unload_cleartext_privkey_file(rad_home)) {
	    eprintf("failed to unload cleartext privkey file");
	    return 1;
	}

	// pull from radrepo
	argv[0] = "git";
	argv[1] = "-C";
	argv[2] = strdup(cwd);
	argv[3] = "pull";
	argv[4] = 0;
	//iprintf("exec: %s -- %s -- %s -- %s -- %s",argv[0],argv[1],argv[2],argv[3],argv[4]);
	if (exec_command("git",argv)) {
	    eprintf("git command failed");
	    return 1;
	}
    }
    else { // use rad
	char* argv [3];
	argv[0] = "rad";
	argv[1] = "sync";
	argv[2] = 0;
	if (exec_command("rad",argv)) {
	    eprintf("rad sync command failed");
	    return 1;
	}
	
	const char* rad_home = get_rad_node_home();
	const char* crad_home = get_rad_home();
	char* rad_repo_path = malloc(strlen(rad_home)+128);
	char* crad_repo_path = malloc(strlen(crad_home)+128);

	if (!getcwd(cwd,sizeof(cwd))) {
	    eprintf("can't get current working directory");
	    return 1;
	}
	rad_git_init();
	git_repository* repo = 0;
	if (git_repository_open(&repo,cwd)) {
	    eprintf("failed to open git repository at %s",cwd);
	    return 1;
	}
	rid = rid_of_rad_remote(repo);
	if (git_oid_is_zero(&rid)) {
	    eprintf("failed to get rid from CWD");
	    return 1;
	}
	rid_str = oid_to_rid(rid);
	
	sprintf(rad_repo_path,"%s/storage/%s",rad_home,rid_str);
	sprintf(crad_repo_path,"%s/storage/%s",crad_home,rid_str);
	
	argv[0] = "cp";
	argv[1] = "-a";
	argv[2] = rad_repo_path;
	argv[3] = crad_repo_path;
	argv[4] = 0;
	if (exec_command("cp",argv)) {
	    eprintf("cp command failed");
	    return 1;
	}

	if (!getcwd(cwd,sizeof(cwd))) {
	    fprintf(stderr,"Can't get current working directory\n");
	    return 1;
	}
	rad_git_init();
    }

    Oid rid_valid = rad_repo_validate(cwd);
    if (!git_oid_equal(&rid_valid,&rid)) {
	eprintf("repo invalid");
	return 1;
    }
    iprintf("crad sync successful");
    return 0;
}
