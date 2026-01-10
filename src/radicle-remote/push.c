#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include <push.h>
#include <util.h>
#include <profile.h>

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

int push_run (const char* refspec, Storage storage, RadRepo rrepo, const char* did_raw, json_object* identity_doc) {

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
    char* namespace_prefix = malloc(len_namespace_prefix+1);
    sprintf(namespace_prefix,"refs/namespaces/%s/",did_raw);
    char* dst_full = malloc(len_namespace_prefix+strlen(dst)+1);
    if (strlen(dst) < 16+strlen(did_raw) || strcmp(rad_substr(dst,0,len_namespace_prefix),namespace_prefix)) {
	sprintf(dst_full,"%s%s",namespace_prefix,dst);
    }
    
    //fprintf(stderr,"dst_full %s\n",dst_full);
    char* refspec_full = malloc(strlen(src_full)+strlen(dst_full)+3);
    if (force)
	sprintf(refspec_full,"+%s:%s",src_full,dst_full);
    else
	sprintf(refspec_full,"%s:%s",src_full,dst_full);

    char* refspec_canon = malloc(strlen(src_full)+strlen(dst)+3);
    if (force)
	sprintf(refspec_canon,"+%s:%s",src_full,dst);
    else
	sprintf(refspec_canon,"%s:%s",src_full,dst);

    const char* repo_path = git_repository_workdir(repo);
    
    //fprintf(stderr,"for send-pack repo_path %s rrepo_path %s refspec %s\n",repo_path,rrepo_path,refspec_full);


    //get default branch and delegate for this repo
    char* default_branch = 0;
    char** delegates = 0;
    size_t n_delegates = 0;
    json_object_object_foreach(identity_doc,key,val) {
	if (!strcmp(key,"payload")) {
	    json_object_object_foreach(val,key2,val2) {
		if (!strcmp(key2,"xyz.radicle.project")) {
		    json_object_object_foreach(val2,key3,val3) {
			if (!strcmp(key3,"defaultBranch"))
			    default_branch = rad_strip('"',json_object_to_json_string(val3));
		    }
		}
	    }
	}
	else if (!strcmp(key,"delegates")) {
	    n_delegates = json_object_array_length(val);
	    delegates = malloc(n_delegates*sizeof(char*));
	    for (size_t i=0; i<n_delegates; i++) {
		json_object* delegate_obj = json_object_array_get_idx(val,i);
		if (!delegate_obj) {
		    fprintf(stderr,"Can't find the delegates array object\n");
		    return 1;
		}
		delegates[i] = rad_strip('"',json_object_to_json_string(delegate_obj));
	    }
	}
    }
    //fprintf(stderr,"default_branch %s delegate %s\n",default_branch,delegate);
    
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
	  return 1;
	}
	
	// if dst branch matches the default branch, and did matches the delegate, push to the canonical ref
	size_t len_default_branch = strlen(default_branch);
	//fprintf(stderr,"dst %s substring %s\n",dst,rad_substr(dst,-len_default_branch,len_default_branch));
	if (!strcmp(rad_substr(dst,-len_default_branch,len_default_branch),default_branch)) {
	    //fprintf(stderr,"dst branch matches default branch\n");
	    bool signer_match = false;
	    for (size_t i=0; i<n_delegates; i++) {
		if (!strcmp(delegates[i]+8,did_raw)) {
		    signer_match = true;
		    break;
		}
	    }
	    if (signer_match) {
		//fprintf(stderr,"delegate matches current signer\n");
		//make sure current signer controls the privkey
		Pubkey pubkey_from_privkey = profile_get_pubkey_from_privkey();
		if (!pubkey_from_privkey.bytes) {
		    fprintf(stderr,"Failed to get pubkey from privkey\n");
		    return 1;
		}
		const char* did_from_privkey = pubkey_to_did(pubkey_from_privkey.bytes)+8;
		if (!strcmp(did_raw,did_from_privkey)) {
		    argv[5] = strdup(refspec_canon);
		    if (exec_command("git",argv)) {
			fprintf(stderr,"git command failed\n");
			return 1;
		    }
		}
	    }
	}
	
	// sign refs

	Pubkey signer;
	signer.bytes = raw_did_to_pubkey(did_raw);	
	Oid oid = rad_repo_sign_refs(rrepo,signer);

	// update sigrefs

	char update_ref [128];
	sprintf(update_ref,"refs/namespaces/%s/refs/rad/sigrefs",did_raw);
	git_signature* gitsig = 0;
	if (git_signature_default(&gitsig,rrepo.repo)) {
	    fprintf(stderr,"Failed to get git signature\n");
	    return 1;
	}
	git_tree* tree = 0;
	if (git_tree_lookup(&tree,rrepo.repo,&oid)) {
	    fprintf(stderr,"Failed to lookup git tree\n");
	    return 1;
	}
	Oid oid_parent = {{0}};
	git_commit* parent = 0;
	if (git_reference_name_to_id(&oid_parent,rrepo.repo,update_ref)) { //todo check if this is ok
	    //fprintf(stderr,"Failed to get oid of git reference name\n");
	    // no error printed because it may be the first commit for the did namespace
	}
	else if (git_commit_lookup(&parent,rrepo.repo,&oid_parent)) {
	    fprintf(stderr,"Failed to lookup git commit\n");
	    return 1;
	}
	const git_commit* parents [1];
	parents[0] = parent;
	if (git_commit_create(&oid,rrepo.repo,update_ref,gitsig,gitsig,0,"Update signed refs\n",tree,1,parents)) {
	    fprintf(stderr,"Error creating git commit\n");
	    return 1;
	}
	printf("ok %s\n\n",dst);
	return 0;
    }
    return 1;
}
