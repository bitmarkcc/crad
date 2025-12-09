#include <stdio.h>
#include <string.h>

#include <list.h>
#include <storage.h>
#include <repo.h>
#include <util.h>

int list_for_push (Storage storage, RadRepo rrepo, const char* did_raw) {
    char buf [HEXSIZ];
    git_reference_iterator* it = 0;
    char glob [128];
    sprintf(glob,"refs/namespaces/%s/*",did_raw);
    if (git_reference_iterator_glob_new(&it,rrepo.repo,glob)) {
	fprintf(stderr,"Failed to create glob iterator\n");
	return 1;
    }
    const char* name = 0;
    int ret = 0;
    while (!(ret = git_reference_next_name(&name,it))) {
	Oid oid;
	git_reference_name_to_id(&oid,rrepo.repo,name);
	char* oid_str = strdup(git_oid_tostr(buf,HEXSIZ,&oid));
	char* short_name = rad_substr(name,17+strlen(did_raw),0); // remove the refs/namespaces/<did>/ part
	if (!strcmp(rad_substr(short_name,0,10),"refs/heads") || !strcmp(rad_substr(short_name,0,9),"refs/tags")) {
	    printf("%s %s\n",oid_str,short_name);
	}
    }
    if (ret != GIT_ITEROVER) {
	fprintf(stderr,"Error iterating over glob reference names\n");
	return 1;
    }
    return 0;
}

int list_for_fetch () {
    return 1;
}
