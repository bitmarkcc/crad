#include <stdio.h>

#include <commands/validate.h>
#include <command.h>
#include <profile.h>
#include <repo.h>
#include <git.h>

int validate_run (Command c) {
    if (!profile_load()) {
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
	eprintf("rid is zero");
	return 1;
    }
    return 0;
}
