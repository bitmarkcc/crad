#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>
#include <libgen.h>

#include <commands/init.h>
#include <profile.h>
#include <util.h>
#include <git.h>
#include <rad.h>

int init_run (Command c) {
    if (!profile_load()) {
	fprintf(stderr,"No profile is loaded. Run `rad auth` to load or create one.\n");
	return 1;
    }
    else {
	return init_init();
    }
    return 0;
}

int init_init() {
    char cwd [1024];
    if (!getcwd(cwd,sizeof(cwd))) {
	fprintf(stderr,"Can't get current working directory\n");
	return 1;
    }
    printf("Initializing radicle repository in %s\n",cwd);
    const char* name_default = basename(cwd);
    char name [RAD_BUFSIZ];
    printf("? Name (%s) ",name_default);
    rad_get_input(name,RAD_BUFSIZ);
    if (!strlen(name)) rad_strcpy(name,name_default,0,RAD_BUFSIZ-1);
    char description [RAD_BUFSIZ];
    printf("? Description ");
    while (1) {
	rad_get_input(description,RAD_BUFSIZ);
	if (strlen(description)) break;
    }
    rad_git_init();
    git_repository* repo = 0;
    if (git_repository_open(&repo,cwd)) {
	fprintf(stderr,"Can't open git repository. Make sure your current directory is a git repository.\n");
	return 1;
    }
    const char* branch_default = get_default_branch(repo);
    if (!branch_default) {
	fprintf(stderr,"Can't find the default branch in this repository\n");
	return 1;
    }
    char branch [RAD_BUFSIZ];
    printf("? Default branch (%s) ",branch_default);
    rad_get_input(branch,RAD_BUFSIZ);
    if (!strlen(branch)) rad_strcpy(branch,branch_default,0,RAD_BUFSIZ-1);
    bool public = false;
    char ispublic [RAD_BUFSIZ];
    printf("? Visibility public (no) ");
    rad_get_input(ispublic,RAD_BUFSIZ);
    if (!strlen(ispublic)) rad_strcpy(ispublic,"no",0,RAD_BUFSIZ-1);
    char* ispublic_lower = rad_to_lower(ispublic);
    if (!strcmp(ispublic_lower,"yes")) public = true;
    Visibility visibility = VIS_PRIVATE;
    if (public)
	visibility = VIS_PUBLIC;
    Pubkey signer = profile_get_pubkey();
    Storage storage = profile_get_storage();	
    RadProjectResult res = rad_project_init(repo,name,description,branch,visibility,signer,storage);
    if (res.ret) {
	fprintf(stderr,"Failed to initialize project\n");
	return 1;
    }
    free(ispublic_lower);
    git_repository_free(repo);
    printf("Initialized project\n");
    return 0;
}
