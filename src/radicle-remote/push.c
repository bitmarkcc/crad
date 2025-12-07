#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include <push.h>
#include <util.h>

int refspec_parse (char** src, char** dst, bool* force, const char* refspec) {
    if (!refspec) return 1;
    size_t len = strlen(refspec);
    *src = malloc(len);
    *dst = malloc(len);
    if (*refspec == '+') {
	*force = true;
	refspec++;
    }
    size_t i = 0;
    while (*refspec && *refspec != ':') {
	(*src)[i] = *refspec;
	refspec++;
	i++;
    }
    if (*refspec != ':') return 1;
    (*src)[i] = 0;
    refspec++;
    i = 0;
    while (*refspec) {
	(*dst)[i] = *refspec;
	refspec++;
	i++;
    }
    (*dst)[i] = 0;
    return 0;
}

int push_run (const char* refspec, Storage storage, RadRepo rrepo, const char* did_raw) {

    char* src = 0;
    char* dst = 0;
    bool force = false;
    if (refspec_parse(&src,&dst,&force,refspec)) {
	fprintf(stderr,"Failed to parse refspec\n");
	return 1;
    }
    bool delete = false;
    if (!src[0]) delete = true;

    char* rrepo_path = malloc(strlen(storage.path)+31);
    char* rid_str = oid_to_rid(rrepo.rid);
    sprintf(rrepo_path,"%s/%s",storage.path,rid_str);

    // Get oid corresponding to src
    git_repository* repo = 0;
    char cwd [1024];
    if (!getcwd(cwd,sizeof(cwd))) {
	fprintf(stderr,"Can't get current working directory\n");
	return 1;
    }
    if (git_repository_open(&repo,cwd)) {
	fprintf(stderr,"Can't open git repository\n");
	return 1;
    }
    git_object* obj = 0;
    if (git_revparse_single(&obj,repo,src)) {
	fprintf(stderr,"Can't find git object specified by revision string\n");
	return 1;
    }
    char buf [HEXSIZ];
    if (!git_oid_tostr(buf,HEXSIZ,git_object_id(obj))) {
	fprintf(stderr,"Failed to convert oid to a string\n");
	return 1;
    }
    char* src_full = strdup(buf);
        
    // Add the namespace prefix to the dst if needed
    size_t len_namespace_prefix = 17+strlen(did_raw);
    char* namespace_prefix = malloc(len_namespace_prefix);
    sprintf(namespace_prefix,"refs/namespaces/%s/",did_raw);
    char* dst_full = strdup(dst);
    if (strlen(dst) < 16+strlen(did_raw) || strcmp(rad_substr(dst,0,len_namespace_prefix),namespace_prefix)) {
	sprintf(dst_full,"%s%s",namespace_prefix,dst);
    }
    //fprintf(stderr,"dst_full %s\n",dst_full);
    //char* refspec_full = malloc();

    char* refspec_full = malloc(strlen(src_full)+strlen(dst_full)+3);
    if (force)
	sprintf(refspec_full,"+%s:%s",src_full,dst_full);
    else
	sprintf(refspec_full,"%s:%s",src_full,dst_full);

    const char* repo_path = git_repository_workdir(repo);
    
    //fprintf(stderr,"for send-pack repo_path %s rrepo_path %s refspec %s\n",repo_path,rrepo_path,refspec_full);
    
    if (delete) { // todo implement push --delete
	fprintf(stderr,"delete a ref (to implement)\n");
	return 1;
    }
    else if (!strcmp(dst,"refs/patches")) { // todo implement push to patches
	fprintf(stderr,"open a patch (to implement)\n");
	return 1;
    }
    else if (strlen(dst)>18 && !strcmp(rad_substr(dst,0,19),"refs/heads/patches/")) {
	fprintf(stderr,"update a patch (to implement)\n");
	return 1;
    }
    else {
	char* argv [7];
	argv[0] = "git";
	argv[1] = "-C";
	argv[2] = strdup(repo_path);
	argv[3] = "send-pack";
	argv[4] = strdup(rrepo_path);
	argv[5] = strdup(refspec_full);
	argv[6] = 0;

	if (exec_command("git",argv)) {
	    fprintf(stderr,"git command failed\n");
	}

	printf("ok %s\n",dst);
	
	return 0;
    }
    return 1;
}
