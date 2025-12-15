#include <stdio.h>

#include <command.h>
#include <profile.h>

int clone_run (Command c) {
    if (!profile_load()) {
	fprintf(stderr,"No profile is loaded. Run `crad auth` to create one.\n");
	return 1;
    }

    
    
    return 0;
}
