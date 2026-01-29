#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <dirent.h>

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
    char buf [HEXSIZ];
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

    //backup rad repo dir
    const char* rad_home = get_rad_home();
    char* dst = malloc(strlen(rad_home)+64);
    sprintf(dst,"%s/storage/%s/",rad_home,rid_str);
    char* dst_bak = malloc(strlen(dst)+5);
    sprintf(dst_bak,"%s/storage/%s.bak.%ld/",rad_home,rid_str,time(0));
    char* argv [10];
    argv[0] = "cp";
    argv[1] = "-a";
    argv[2] = dst;
    argv[3] = dst_bak;
    argv[4] = 0;
    //iprintf("exec: %s -- %s -- %s -- %s",argv[0],argv[1],argv[2],argv[3]);	
    if (exec_command("cp",argv)) {
	eprintf("cp command failed");
	return 1;
    }
    
    if (cmd.seed) { // sync from the seed using crad
	if (!password_loaded()) {
	    eprintf("You must be authenticated first. Run `crad auth` to authenticate");
	    return 1;
	}	
	const char* userhost = strtok(strdup(cmd.seed),":");
	const char* port = strtok(0,":");
	const char* user = strtok(strdup(userhost),"@");
	const char* host = strtok(0,"@");
	// create ssh config file
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
	char* dsttmp = malloc(strlen(rad_home)+128);
	sprintf(dsttmp,"%s/storage/%s.tmp/",rad_home,rid_str);
	argv[8] = dsttmp;
	argv[9] = 0;
	//iprintf("exec: %s -- %s -- %s -- %s -- %s -- %s -- %s -- %s -- %s",argv[0],argv[1],argv[2],argv[3],argv[4],argv[5],argv[6],argv[7],argv[8]);
	
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

	src = malloc(strlen(dsttmp)+32);
	sprintf(src,"%sobjects",dsttmp);
	
	argv[0] = "rsync";
	argv[1] = "-a";
	argv[2] = src;
	argv[3] = dst;
	argv[4] = 0;

	//iprintf("exec: %s -- %s -- %s -- %s",argv[0],argv[1],argv[2],argv[3]);	
	if (exec_command("rsync",argv)) {
	    eprintf("rsync command failed");
	    return 1;
	}

	Pubkey signer = profile_get_pubkey();
	const char* namespace_me = pubkey_to_did(signer.bytes)+8;
	
	char* namespaces_src = malloc(strlen(dsttmp)+128);
	sprintf(namespaces_src,"%srefs/namespaces",dsttmp);
	char* namespaces_dst = malloc(strlen(dst));
	sprintf(namespaces_dst,"%srefs/namespaces",dst);
	if (!access(namespaces_src,F_OK) && !access(namespaces_dst,F_OK)) {
	    DIR* d = opendir(namespaces_src);
	    struct dirent* dir = 0;
	    if (!d) {
		eprintf("failed to open directory %s",namespaces_src);
		return 1;
	    }
	    while (dir = readdir(d)) {
		if (strlen(dir->d_name)>2) {
		    //iprintf("namespace dir file %s",dir->d_name);
		    if (!strcmp(dir->d_name,namespace_me)) continue;
		    char* namespaces_src_cur = malloc(strlen(namespaces_src)+64);
		    sprintf(namespaces_src_cur,"%s/%s",namespaces_src,dir->d_name);
		    char* namespaces_dst_cur = malloc(strlen(namespaces_dst)+2);
		    sprintf(namespaces_dst_cur,"%s/",namespaces_dst);
		    argv[0] = "rsync";
		    argv[1] = "-a";
		    argv[2] = namespaces_src_cur;
		    argv[3] = namespaces_dst_cur;
		    argv[4] = 0;
		    //iprintf("exec: %s -- %s -- %s -- %s",argv[0],argv[1],argv[2],argv[3]);	
		    if (exec_command("rsync",argv)) {
			eprintf("rsync command failed");
			return 1;
		    }
		}
	    }
	    closedir(d);
	}

	git_repository* repo_dst = 0;
	if (git_repository_open(&repo_dst,dst)) {
	    eprintf("failed to open git repository at %s",dst);
	    return 1;
	}
	Oid head_id = {{0}};
	if (git_reference_name_to_id(&head_id,repo_dst,"HEAD")) {
	    eprintf("failed to lookup HEAD oid");
	    return 1;
	}
	//iprintf("HEAD oid %s",git_oid_tostr(buf,HEXSIZ,&head_id));
	git_commit* commit = 0;
	if (git_commit_lookup(&commit,repo_dst,&head_id)) {
	    eprintf("failed to lookup git commit");
	    return 1;
	}
	git_repository* repo_dsttmp = 0;
	if (git_repository_open(&repo_dsttmp,dsttmp)) {
	    eprintf("failed to open git repository at %s",dsttmp);
	    return 1;
	}
	Oid head_id_tmp = {{0}};
	if (git_reference_name_to_id(&head_id_tmp,repo_dsttmp,"HEAD")) {
	    eprintf("failed to lookup HEAD oid");
	    return 1;
	}
	git_commit* commit_tmp = 0;
	if (git_commit_lookup(&commit_tmp,repo_dsttmp,&head_id_tmp)) {
	    eprintf("failed to lookup git commit");
	    return 1;
	}
	if (git_commit_time(commit_tmp) > git_commit_time(commit)) {
	    git_reference* ref_tmp = 0;
	    if (git_reference_lookup(&ref_tmp,repo_dsttmp,"HEAD")) {
		eprintf("failed to lookup git reference HEAD");
		return 1;
	    }
	    git_reference* ref_tmp_resolved = 0;	    
	    if (git_reference_resolve(&ref_tmp_resolved,ref_tmp)) {
		eprintf("failed to resolve git reference");
		return 1;
	    }
	    const char* ref_tmp_name = git_reference_name(ref_tmp_resolved);
	    char* src_cur = malloc(strlen(dsttmp)+strlen(ref_tmp_name)+8);
	    sprintf(src_cur,"%s%s",dsttmp,ref_tmp_name);
	    char* dst_cur = malloc(strlen(dst)+16);
	    sprintf(dst_cur,"%srefs/heads/",dst);
	    
	    argv[0] = "rsync";
	    argv[1] = "-a";
	    argv[2] = src_cur;
	    argv[3] = dst_cur;
	    argv[4] = 0;

	    iprintf("exec: %s -- %s -- %s -- %s",argv[0],argv[1],argv[2],argv[3]);	
	    if (exec_command("rsync",argv)) {
		eprintf("rsync command failed");
		return 1;
	    }
	}
	
	argv[0] = "rm";
	argv[1] = "-rf";
	argv[2] = dsttmp;
	argv[3] = 0;
	//iprintf("exec: %s -- %s -- %s",argv[0],argv[1],argv[2]);
	/*if (exec_command("rm",argv)) {
	    eprintf("rm command failed");
	    return 1;
	    }*/
	
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
