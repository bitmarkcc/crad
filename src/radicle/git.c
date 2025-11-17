#include <git2.h>
#include <stdio.h>

void rad_git_init () {
    git_libgit2_init();
}

const char* get_default_branch (git_repository* repo) {
    git_config* config = 0;
    int ret =  git_repository_config(&config,repo);
    if (ret) {
	fprintf(stderr,"Can't get git repository config\n");
	return 0;
    }
    const char* default_branch = 0;
    ret = git_config_get_string(&default_branch,config,"init.defaultbranch");
    if (ret) {
	git_reference* head = 0;
	ret = git_repository_head(&head,repo);
	if (ret) {
	    fprintf(stderr,"Can't get git repository HEAD\n");
	    return 0;
	}
	const char* shorthand = git_reference_shorthand(head);
	git_config_free(config);
	git_reference_free(head);
	return shorthand;
    }
    git_config_free(config);
    return default_branch;
}
