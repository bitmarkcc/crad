#include <stdio.h>
#include <string.h>

#include <commands/clone.h>
#include <command.h>
#include <profile.h>
#include <util.h>
#include <git.h>
#include <id.h>
#include <repo.h>

int clone_run (Command c) {
    if (!c.argc) {
	eprintf("crad clone requires at least one argument");
	return 1;
    }
    if (!profile_load()) {
	fprintf(stderr,"No profile is loaded. Run `crad auth` to create one.\n");
	return 1;
    }
    char* rid_str = strdup(c.argv[0]);
    char* argv [5];
    argv[0] = "rad";
    argv[1] = "clone";
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
    const char* clone_dir = argv[3];

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
    
    char cwd [1024];
    if (!getcwd(cwd,sizeof(cwd))) {
	fprintf(stderr,"Can't get current working directory\n");
	return 1;
    }
    strcat(cwd,"/");
    strcat(cwd,clone_dir);
    rad_git_init();
    Oid rid_valid = rad_repo_validate(cwd);
    Oid rid = rid_to_oid(rid_str+4);
    if (!git_oid_equal(&rid_valid,&rid)) {
	eprintf("repo invalid");
	return 1;
    }
    iprintf("crad clone successful");
    return 0;
}
