#include <stdio.h>

#include <commands/validate.h>
#include <command.h>
#include <profile.h>
#include <repo.h>
#include <git.h>

void print_help_validate () {
    printf("crad validate (Validate a radicle-initialized repository in the working directory) Usage:\n");
    printf("crad validate\n");
}

int validate_run (Command c) {
    if (c.type == CMD_HELP) {
	print_help_validate();
	return 0;
    }
    else if (!profile_load()) {
	fprintf(stderr,"No profile is loaded. Run `crad auth` to create one.\n");
	return 1;
    }
    char cwd [1024];
    if (!getcwd(cwd,sizeof(cwd))) {
	fprintf(stderr,"Can't get current working directory\n");
	return 1;
    }
    rad_git_init();
    Oid rid = rad_repo_validate(cwd);
    if (git_oid_is_zero(&rid)) {
	eprintf("repo invalid");
	return 1;
    }
    iprintf("repo valid with rid %s",oid_to_rid(rid));
    return 0;
}
