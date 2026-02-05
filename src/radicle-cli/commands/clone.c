#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>

#include <commands/clone.h>
#include <command.h>
#include <profile.h>
#include <util.h>
#include <git.h>
#include <id.h>
#include <repo.h>

CloneCommand command_clone_default() {
    CloneCommand cmd;
    cmd.err = 0;
    cmd.seed = 0;
    return cmd;
}

void print_help_clone () {
    printf("crad clone (Clone Repository) Usage:\n");
    printf("crad clone [-s <seed>] <rid> [<directory>]\n");
}

CloneCommand parse_args_clone (int argc, char** argv) {
    CloneCommand cmd = command_clone_default();
    for (size_t i=0; i<argc; i++) {
	if (!strcmp(argv[i],"--seed") || !strcmp(argv[i],"-s")) {
	    if (i+1 < argc) cmd.seed = strdup(argv[i+1]);
	}
    }
    return cmd;
}

int clone_run (Command c) {
    if (c.type == CMD_HELP) {
	print_help_clone();
	return 0;
    }
    else if (!c.argc) {
	eprintf("crad clone requires at least one argument");
	return 1;
    }
    else if (!profile_load()) {
	eprintf("No profile is loaded. Run `crad auth` to create one.");
	return 1;
    }
    CloneCommand cmd = parse_args_clone(c.argc,c.argv);
    if (cmd.err) {
	return 1;
    }
    const char* clone_dir = 0;
    char* rid_str = 0;
    char cwd [1024];
    if (cmd.seed) { // clone directory from the seed using _crad_
	if (!password_loaded()) {
	    eprintf("You must be authenticated first. Run `crad auth` to authenticate");
	    return 1;
	}
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
	rid_str = strdup(c.argv[2]);
	char* src = malloc(strlen(cmd.seed)+strlen(rid_str)+2);
	sprintf(src,"seed:%s",rid_str+4);
	argv[7] = src;
	char* dst = malloc(strlen(rad_home)+128);
	sprintf(dst,"%s/storage/%s/",rad_home,rid_str+4);
	argv[8] = dst;
	argv[9] = 0;
	//iprintf("exec: %s -- %s -- %s -- %s -- %s -- %s -- %s -- %s",argv[0],argv[1],argv[2],argv[3],argv[4],argv[5],argv[6],argv[7]);

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

	if (c.argc > 3) {
	    clone_dir = c.argv[3];
	}
	else {
	    clone_dir = strdup(rid_str+4);
	}
	if (!getcwd(cwd,sizeof(cwd))) {
	    fprintf(stderr,"Can't get current working directory\n");
	    return 1;
	}
	strcat(cwd,"/");
	strcat(cwd,clone_dir);
	rad_git_init();
	git_repository* repo = 0;
	if (git_repository_init(&repo,cwd,0)) {
	    eprintf("failed to initialize git repository at %s",cwd);
	    return 1;
	}
	if (rad_repo_configure(repo)) {
	    eprintf("failed to configure git repository");
	    return 1;
	}
	char fetchurl [128];
	char pushurl [128];
	const char* did_raw = pubkey_to_did(profile_get_pubkey().bytes)+8;
	strcpy(fetchurl,"rad://");
	strcat(fetchurl,rid_str+4);
	strcpy(pushurl,"rad://");
	strcat(pushurl,rid_str+4);
	strcat(pushurl,"/");
	strcat(pushurl,did_raw);
	
	if (rad_repo_configure_remote(repo,"rad",fetchurl,pushurl)) {
	    eprintf("failed to configure remote for git repository");
	    return 1;
	}

	// init RadRepo to get the default branch and set it in the config
	Oid zero = {{0}};
	RadRepo rrepo;
	rrepo.rid = rid_to_oid(rid_str+4);
	rrepo.repo = 0;
	char* rrepo_path = malloc(strlen(dst)+1);
	sprintf(rrepo_path,"%s",dst);
	rrepo_path[strlen(dst)-1] = 0;
	if (git_repository_open(&rrepo.repo,rrepo_path)) {
	    eprintf("failed to open git repository with path: %s",rrepo_path);
	    return 1;
	}
	json_object* identity_doc = get_identity_document(rrepo.repo);
	if (!identity_doc) {
	    eprintf("failed to get identity document");
	    return 1;
	}
	char* default_branch = 0;
	json_object_object_foreach(identity_doc,key,val) {
	    if (!strcmp(key,"payload")) {
		json_object_object_foreach(val,key2,val2) {
		    if (!strcmp(key2,"xyz.radicle.project")) {
			json_object_object_foreach(val2,key3,val3) {
			    if (!strcmp(key3,"defaultBranch"))
				default_branch = rad_strip('"',json_object_to_json_string(val3));
			}
		    }
		}
	    }
	}	
	if (rad_repo_set_upstream(repo,default_branch)) {
	    eprintf("failed to set upstream");
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
    else { // use _rad_ , todo: set correct alias and did in config for rad repo
	iprintf("cloning using rad");
	char* argv [5];
	argv[0] = "rad";
	argv[1] = "clone";
	rid_str = strdup(c.argv[0]);
	argv[2] = rid_str; // must be in format rad:zyx...cba
	if (c.argc > 1) {
	    argv[3] = strdup(c.argv[1]);
	}
	else {
	    argv[3] = strdup(rid_str+4);
	}
	argv[4] = 0;
	if (exec_command("rad",argv)) {
	    eprintf("rad clone command failed");
	    return 1;
	}
	clone_dir = argv[3];
	
	const char* rad_home = get_rad_node_home();
	const char* crad_home = get_rad_home();
	char* rad_repo_path = malloc(strlen(rad_home)+128);
	char* crad_repo_path = malloc(strlen(crad_home)+128);
	sprintf(rad_repo_path,"%s/storage/%s",rad_home,argv[2]+4);
	sprintf(crad_repo_path,"%s/storage/%s",crad_home,argv[2]+4);
	
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
	if (!clone_dir) {
	    eprintf("no clone dir");
	    return 1;
	}
	strcat(cwd,"/");
	strcat(cwd,clone_dir);
	rad_git_init();
    }

    Oid rid_valid = rad_repo_validate(cwd);
    Oid rid = rid_to_oid(rid_str+4);
    if (!git_oid_equal(&rid_valid,&rid)) {
	eprintf("repo invalid");
	return 1;
    }
    iprintf("crad clone successful");
    return 0;
}
