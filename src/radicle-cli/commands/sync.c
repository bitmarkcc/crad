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
    cmd.rid = 0;
    return cmd;
}

void print_help_sync () {
    printf("crad sync (Sync Repository) Usage:\n");
    printf("crad sync [-s <seed>] [-R <rid>]\n");
}

SyncCommand parse_args_sync (int argc, char** argv) {
    SyncCommand cmd = command_sync_default();
    for (size_t i=0; i<argc; i++) {
	if (!strcmp(argv[i],"--seed") || !strcmp(argv[i],"-s")) {
	    if (i+1 < argc) cmd.seed = strdup(argv[i+1]);
	}
	else if (!strcmp(argv[i],"-R"))
	    if (i+1 < argc) cmd.rid = strdup(argv[i+1]);
    }
    return cmd;
}

int sync_run (Command c) { // todo: sync refs/rad/id
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
    const char* crad_home = get_rad_home();
    Oid rid = {{0}};
    char* rid_str = 0;
    char cwd [1024];
    git_repository* repo = 0;
    rad_git_init();
    if (cmd.rid) {
	rid_str = cmd.rid;
	rid = rid_to_oid(rid_str);
	cwd[0] = 0;
    }
    else {
	// get rid of repo in cwd
	if (!getcwd(cwd,sizeof(cwd))) {
	    fprintf(stderr,"Can't get current working directory\n");
	    return 1;
	}
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
    }

    //backup rad repo dir
    char* dst = malloc(strlen(crad_home)+64);
    sprintf(dst,"%s/storage/%s/",crad_home,rid_str);
    char* dst_bak = malloc(strlen(dst)+32);
    sprintf(dst_bak,"%s/storage/%s.bak.%ld/",crad_home,rid_str,time(0));
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
	char* config_dir = malloc(strlen(crad_home)+6);
	sprintf(config_dir,"%s/.ssh",crad_home);
	if (access(config_dir,F_OK)) {
	    if (mkdir(config_dir,0755)) {
		eprintf("Can't create ssh directory");
		return 1;
	    }
	}
	char* config_fname = malloc(strlen(crad_home)+13);
	sprintf(config_fname,"%s/.ssh/config",crad_home);
	FILE* f = fopen(config_fname,"w");
	char line [256];
	fputs("Host seed\n",f);
	sprintf(line,"  HostName %s\n",host);
	fputs(line,f);
	sprintf(line,"  Port %s\n",port);
	fputs(line,f);
	sprintf(line,"  User %s\n",user);
	fputs(line,f);
	sprintf(line,"  IdentityFile %s/.pw/radicle.key\n",crad_home);
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
	char* rsh = malloc(strlen(crad_home)+20);
	sprintf(rsh,"ssh -F %s/.ssh/config",crad_home);
	argv[6] = rsh;
	char* src = malloc(strlen(cmd.seed)+strlen(rid_str)+2);
	sprintf(src,"seed:%s",rid_str);
	argv[7] = src;
	char* dsttmp = malloc(strlen(crad_home)+128);
	sprintf(dsttmp,"%s/storage/%s.tmp/",crad_home,rid_str);
	argv[8] = dsttmp;
	argv[9] = 0;
	//iprintf("exec: %s -- %s -- %s -- %s -- %s -- %s -- %s -- %s -- %s",argv[0],argv[1],argv[2],argv[3],argv[4],argv[5],argv[6],argv[7],argv[8]);
	
	if (load_cleartext_privkey_file(crad_home)) {
	    eprintf("failed to load cleartext privkey file");
	    return 1;
	}	
	if (exec_command("rsync",argv)) {
	    eprintf("rsync command failed");
	    return 1;
	}
	if (unload_cleartext_privkey_file(crad_home)) {
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

	// sync namespaces from seed temp to crad using git fetch
	// (handles both packed and loose refs, unlike rsync of ref files)
	argv[0] = "git";
	argv[1] = "--git-dir";
	argv[2] = dst;
	argv[3] = "fetch";
	argv[4] = dsttmp;
	argv[5] = "+refs/namespaces/*:refs/namespaces/*";
	argv[6] = 0;
	exec_command("git",argv);

	// sync canonical head
	
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

	    //iprintf("exec: %s -- %s -- %s -- %s",argv[0],argv[1],argv[2],argv[3]);	
	    if (exec_command("rsync",argv)) {
		eprintf("rsync command failed");
		return 1;
	    }
	}

	// sync identity document

	Oid rad_id = {{0}};
	if (git_reference_name_to_id(&rad_id,repo_dst,"refs/rad/id")) {
	    eprintf("failed to lookup rad oid");
	    return 1;
	}
	commit = 0;
	if (git_commit_lookup(&commit,repo_dst,&rad_id)) {
	    eprintf("failed to lookup git commit");
	    return 1;
	}
	Oid rad_id_tmp = {{0}};
	if (git_reference_name_to_id(&rad_id_tmp,repo_dsttmp,"refs/rad/id")) {
	    eprintf("failed to lookup rad oid");
	    return 1;
	}
	commit_tmp = 0;
	if (git_commit_lookup(&commit_tmp,repo_dsttmp,&rad_id_tmp)) {
	    eprintf("failed to lookup git commit");
	    return 1;
	}
	if (git_commit_time(commit_tmp) > git_commit_time(commit)) {
	    char* src_cur = malloc(strlen(dsttmp)+16);
	    sprintf(src_cur,"%srefs/rad/id",dsttmp);
	    char* dst_cur = malloc(strlen(dst)+16);
	    sprintf(dst_cur,"%srefs/rad/",dst);
	    argv[0] = "rsync";
	    argv[1] = "-a";
	    argv[2] = src_cur;
	    argv[3] = dst_cur;
	    argv[4] = 0;
	    //iprintf("exec: %s -- %s -- %s -- %s",argv[0],argv[1],argv[2],argv[3]);	
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

	if (strlen(cwd)) {
	    // Update remote tracking branches for patches from storage
	    char patch_glob [128];
	    sprintf(patch_glob,"refs/namespaces/%s/refs/heads/patches/*",namespace_me);
	    git_reference_iterator* patch_it = 0;
	    if (!git_reference_iterator_glob_new(&patch_it,repo_dst,patch_glob)) {
		const char* patch_refname = 0;
		while (!git_reference_next_name(&patch_refname,patch_it)) {
		    // Extract patch ID from refs/namespaces/<nid>/refs/heads/patches/<id>
		    const char* patch_id_str = strrchr(patch_refname,'/');
		    if (!patch_id_str) continue;
		    patch_id_str++;
		    Oid patch_head = {{0}};
		    if (git_reference_name_to_id(&patch_head,repo_dst,patch_refname)) continue;
		    // Create refs/remotes/rad/patches/<id> in working copy
		    char remote_ref [128];
		    sprintf(remote_ref,"refs/remotes/rad/patches/%s",patch_id_str);
		    git_reference* tracking_ref = 0;
		    git_reference_create(&tracking_ref,repo,remote_ref,&patch_head,1,"sync patch tracking branch");
		}
		git_reference_iterator_free(patch_it);
	    }

	    // Get default branch name from storage repo HEAD
	    git_reference* head_ref_dst = 0;
	    const char* default_branch = "main";
	    if (!git_reference_lookup(&head_ref_dst,repo_dst,"HEAD")) {
		git_reference* resolved = 0;
		if (!git_reference_resolve(&resolved,head_ref_dst)) {
		    const char* refname = git_reference_name(resolved);
		    if (refname && !strncmp(refname,"refs/heads/",11))
			default_branch = refname + 11;
		}
	    }

	    // pull from radrepo (explicit branch to avoid patch upstream issues)
	    argv[0] = "git";
	    argv[1] = "-C";
	    argv[2] = strdup(cwd);
	    argv[3] = "pull";
	    argv[4] = "rad";
	    argv[5] = strdup(default_branch);
	    argv[6] = 0;
	    //iprintf("exec: %s -- %s -- %s -- %s -- %s -- %s",argv[0],argv[1],argv[2],argv[3],argv[4],argv[5]);
	    if (exec_command("git",argv)) {
		eprintf("git command failed");
		return 1;
	    }
	}
    }
    else { // use rad
	char* argv [10];
	char* bin_path = malloc(strlen(crad_home)+32);
	sprintf(bin_path,"%s/bin/rad-sync-wrapped",crad_home);
	argv[0] = bin_path;
	argv[1] = rid_str;
	argv[2] = 0;
	//iprintf("exec: %s -- %s",argv[0],argv[1]);
	if (exec_command(bin_path,argv)) {
	    eprintf("rad sync (wrapped) command failed");
	    return 1;
	}
	
	const char* rad_home = get_rad_node_home();

	// sync the objects into crad storage

	char* src = malloc(strlen(rad_home)+128);
	sprintf(src,"%s/storage/%s",rad_home,rid_str);
	
	char* src_objects = malloc(strlen(src)+9);
	sprintf(src_objects,"%s/objects",src);

	char* dst = malloc(strlen(crad_home)+128);
	sprintf(dst,"%s/storage/%s/",crad_home,rid_str);
	
	argv[0] = "rsync";
	argv[1] = "-a";
	argv[2] = src_objects;
	argv[3] = dst;
	argv[4] = 0;

	//iprintf("exec %s -- %s -- %s -- %s",argv[0],argv[1],argv[2],argv[3]);
	
	if (exec_command("rsync",argv)) {
	    eprintf("rsync command failed");
	    return 1;
	}

	// sync namespaces from rad to crad using git fetch
	// (handles both packed and loose refs, unlike rsync of ref files)

	Pubkey signer = profile_get_pubkey();
	const char* namespace_me = pubkey_to_did(signer.bytes)+8;

	argv[0] = "git";
	argv[1] = "--git-dir";
	argv[2] = dst;
	argv[3] = "fetch";
	argv[4] = src;
	argv[5] = "+refs/namespaces/*:refs/namespaces/*";
	argv[6] = 0;
	exec_command("git",argv);

	// sync canonical HEAD
	
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
	git_commit* commit = 0;
	if (git_commit_lookup(&commit,repo_dst,&head_id)) {
	    eprintf("failed to lookup git commit");
	    return 1;
	}
	git_repository* repo_src = 0;
	if (git_repository_open(&repo_src,src)) {
	    eprintf("failed to open git repository at %s",src);
	    return 1;
	}
	Oid head_id_src = {{0}};
	if (git_reference_name_to_id(&head_id_src,repo_src,"HEAD")) {
	    eprintf("failed to lookup HEAD oid");
	    return 1;
	}
	git_commit* commit_src = 0;
	if (git_commit_lookup(&commit_src,repo_src,&head_id_src)) {
	    eprintf("failed to lookup git commit");
	    return 1;
	}
	if (git_commit_time(commit_src) > git_commit_time(commit)) {
	    git_reference* ref_src = 0;
	    if (git_reference_lookup(&ref_src,repo_src,"HEAD")) {
		eprintf("failed to lookup git reference HEAD");
		return 1;
	    }
	    git_reference* ref_src_resolved = 0;	    
	    if (git_reference_resolve(&ref_src_resolved,ref_src)) {
		eprintf("failed to resolve git reference");
		return 1;
	    }
	    const char* ref_src_name = git_reference_name(ref_src_resolved);
	    char* src_cur = malloc(strlen(src)+strlen(ref_src_name)+2);
	    sprintf(src_cur,"%s/%s",src,ref_src_name);
	    char* dst_cur = malloc(strlen(dst)+16);
	    sprintf(dst_cur,"%srefs/heads/",dst);
	    
	    argv[0] = "rsync";
	    argv[1] = "-a";
	    argv[2] = src_cur;
	    argv[3] = dst_cur;
	    argv[4] = 0;
	    
	    //iprintf("exec: %s -- %s -- %s -- %s",argv[0],argv[1],argv[2],argv[3]);	
	    if (exec_command("rsync",argv)) {
		eprintf("rsync command failed");
		return 1;
	    }
	}

	// sync refs/rad/id
	
	Oid rad_id = {{0}};
	if (git_reference_name_to_id(&rad_id,repo_dst,"refs/rad/id")) {
	    eprintf("failed to lookup rad oid");
	    return 1;
	}
	commit = 0;
	if (git_commit_lookup(&commit,repo_dst,&rad_id)) {
	    eprintf("failed to lookup git commit");
	    return 1;
	}
	Oid rad_id_src = {{0}};
	if (git_reference_name_to_id(&rad_id_src,repo_src,"refs/rad/id")) {
	    eprintf("failed to lookup rad oid");
	    return 1;
	    }
	commit_src = 0;
	if (git_commit_lookup(&commit_src,repo_src,&rad_id_src)) {
	    eprintf("failed to lookup git commit");
	    return 1;
	}
	if (git_commit_time(commit_src) > git_commit_time(commit)) {
	    char* src_cur = malloc(strlen(src)+16);
	    sprintf(src_cur,"%s/refs/rad/id",src);
	    char* dst_cur = malloc(strlen(dst)+16);
	    sprintf(dst_cur,"%srefs/rad/",dst);
	    argv[0] = "rsync";
	    argv[1] = "-a";
	    argv[2] = src_cur;
	    argv[3] = dst_cur;
	    argv[4] = 0;	    
	    //iprintf("exec: %s -- %s -- %s -- %s",argv[0],argv[1],argv[2],argv[3]);	
	    if (exec_command("rsync",argv)) {
		eprintf("rsync command failed");
		return 1;
	    }
	}
    }	

    Oid rid_valid = {{0}};
    if (strlen(cwd))
	rid_valid = rad_repo_validate(cwd);
    else
	rid_valid = rad_repo_validate(dst);
    if (!git_oid_equal(&rid_valid,&rid)) {
	eprintf("repo invalid");
	return 1;
    }
    iprintf("crad sync successful");
    return 0;
}
