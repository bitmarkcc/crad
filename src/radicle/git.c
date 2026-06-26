#include <stdio.h>
#include <string.h>

#include <git.h>
#include <print.h>

void rad_git_init () {
    git_libgit2_init();
}

char* get_default_branch (git_repository* repo) {
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
	char* shorthand = strdup(git_reference_shorthand(head));
	git_config_free(config);
	git_reference_free(head);
	return shorthand;
    }
    git_config_free(config);
    return strdup(default_branch);
}

char* rad_refname_relative (const char* name) { // assume starts with refs/namespaces/
    size_t len = strlen(name);
    if (len < 16 || len>1024) return 0;
    const char* it = name+16;
    while (*it != '/') {
	it++;
    }
    return strdup(it+1);
}

char* rad_namespace_from_ref (const char* refname) { // assume starts with refs/namespaces
    size_t len = strlen(refname);
    if (len < 16 || len > 1024) return 0;
    char* out = malloc(len+1);
    char* it = out;
    refname += 16;
    while (*refname != '/') {
	*it = *refname;
	refname++;
	it++;
    }
    *it = 0;
    return out;
}

char* rad_sigref_entry_oid (const char* entry) {
    char* out = strdup(entry);
    char* it = out;
    while (*it != ' ') {
	it++;
    }
    *it = 0;
    return out;
}

char* rad_sigref_entry_name (const char* entry) {
    while (*entry != ' ') {
	entry++;
    }
    char* out = strdup(entry+1);
    char* it = out;
    while (*it != ' ') {
	it++;
    }
    *it = 0;
    return out;
}

char* rad_sigref_entry_name_raw (const char* entry) {
    while (*entry && *entry != ' ') {
	entry++;
    }
    if (*entry) entry++;
    return strdup(entry);
}

char* rad_sigref_entry_namespace (const char* entry) {
    while (*entry != ' ') {
	entry++;
    }
    entry++;
    while (*entry != ' ') {
	entry++;
    }
    return strdup(entry+1);
}

/*char* rad_namespace_from_ref (const char* refname) {
    char* token = strtok(refname,"/");
    if (token && !strcmp(token,"refs")) {
	token = strtok(0,"/");
	if (token && !strcmp(token,"namespaces")) {
	    token = strtok(0,"/");
	    if (token) return strdup(token);
	}
    }
    return 0;
    }*/
