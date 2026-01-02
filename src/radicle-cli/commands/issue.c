#include <string.h>
#include <stdio.h>

#include <commands/issue.h>
#include <profile.h>
#include <print.h>
#include <git.h>
#include <cob.h>

IssueCommand command_issue_default() {
    IssueCommand cmd;
    cmd.title = 0;
    cmd.desc = 0;
    return cmd;
}

IssueCommand parse_args_issue (int argc, char** argv) {
    IssueCommand cmd = command_issue_default();
    for (size_t i=0; i<argc; i++) {
	if (!strcmp(argv[i],"--title")) {
	    if (i+1 < argc) cmd.title = strdup(argv[i+1]);
	}
	else if (!strcmp(argv[i],"--description") || !strcmp(argv[i],"--desc")) {
	    if (i+1 < argc) cmd.desc = strdup(argv[i+1]);
	}
    }
    return cmd;
}

int issue_run (Command c) {
    if (!profile_load()) {
	eprintf("No profile is loaded. Run `crad auth` to create one");
	return 1;
    }
    else if (!password_loaded()) {
	eprintf("You must be authenticated first. Run `crad auth` to authenticate");
	return 1;
    }
    else if (c.argc > 0 && !strcmp(c.argv[0],"open")) {
	IssueCommand cmd = parse_args_issue(c.argc,c.argv);
	return issue_open(cmd.title,cmd.desc);
    }
    return 0;
}

int issue_open (char* title, char* desc) {
    char buf [HEXSIZ];
    char cwd [1024];
    if (!getcwd(cwd,sizeof(cwd))) {
	eprintf("Can't get current working directory");
	return 1;
    }
    rad_git_init();
    git_repository* repo = 0;
    if (git_repository_open(&repo,cwd)) {
	eprintf("Can't open git repository. Make sure your current directory is a git repository.");
	return 1;
    }
    Oid rid = rid_of_rad_remote(repo);
    if (git_oid_is_zero(&rid)) {
	eprintf("failed to get rid of rad remote");
	return 1;
    }
    RadRepo rrepo;
    rrepo.repo = 0;
    rrepo.rid = rid;
    git_repository* repo_rad = 0;
    Storage storage = profile_get_storage();
    char* path_rad = malloc(strlen(storage.path)+64);
    sprintf(path_rad,"%s/%s",storage.path,oid_to_rid(rid));
    if (git_repository_open(&repo_rad,path_rad)) {
	eprintf("failed to open git repository at %s",path_rad);
	return 1;
    }
    rrepo.repo = repo_rad;
    Pubkey signer = profile_get_pubkey();
    
    iprintf("doing issue open with title %s, desc %s",title,desc);
    RepoEntry re = cob_issue_init(rrepo,signer,title,desc);
    iprintf("did cob_issue_init with oid %s",git_oid_tostr(buf,HEXSIZ,&re.oid));
    
    return 0;
}
