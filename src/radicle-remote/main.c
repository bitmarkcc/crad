#include <stdio.h>
#include <string.h>

#include <profile.h>
#include <util.h>
#include <repo.h>
#include <git.h>
#include <push.h>
#include <list.h>

const size_t HEXSIZ = GIT_OID_SHA1_HEXSIZE+1;

void print_err (const char* str) {
    fprintf(stderr,"%s\n",str);
}

void print_help() {
    print_err("Usage: git-remote-rad <repository> [<url>]");
    print_err("If only one argment is specified, <repository> is a url.");
    print_err("Otherwise, repository is the name of the remote");
}

int parse_url (char** rid_str, char** did_raw, const char* url) {
    if (strcmp(rad_substr(url,0,6),"rad://")) {
	fprintf(stderr,"Not a rad url\n");
	return 1;
    }
    url += 6;
    size_t len = strlen(url);
    *rid_str = malloc(len+1);
    *did_raw = 0;
    size_t i = 0;
    while (*url && *url != '/') {
	(*rid_str)[i] = *url;
	url++;
	i++;
    }
    (*rid_str)[i] = 0;
    if (*url) {
	*did_raw = malloc(len);
	url++;
	i = 0;
	while (*url) {
	    (*did_raw)[i] = *url;
	    url++;
	    i++;
	}
	(*did_raw)[i] = 0;
    }
    return 0;
}

json_object* get_identity_document (RadRepo rrepo) {
    Oid oid;
    if (git_reference_name_to_id(&oid,rrepo.repo,"refs/rad/id")) {
	fprintf(stderr,"Failed to get oid of reference name\n");
	return 0;
    }
    git_commit* commit = 0;
    if (git_commit_lookup(&commit,rrepo.repo,&oid)) {
	fprintf(stderr,"Failed to lookup commit from git repo\n");
	return 0;
    }
    git_tree* tree = 0;
    if (git_commit_tree(&tree,commit)) {
	fprintf(stderr,"Failed to get tree associated with a git commit\n");
	return 0;
    }
    git_tree_entry* tree_entry = 0;
    if (git_tree_entry_bypath(&tree_entry,tree,"embeds/radicle.json")) {
	fprintf(stderr,"Can't find the git tree entry embeds/radicle.json for the rad/id ref\n");
	return 0;
    }
    Oid* poid = git_tree_entry_id(tree_entry);
    if (!poid) {
	fprintf(stderr,"Can't find oid of git tree entry\n");
	return 0;
    }
    oid = *poid;
    git_blob* blob = 0;
    if (git_blob_lookup(&blob,rrepo.repo,poid)) {
	fprintf(stderr,"Can't lookup blob corresponding to git oid\n");
	return 0;
    }
    uint8_t* blob_content = git_blob_rawcontent(blob);
    return json_tokener_parse((char*)blob_content);
}

int main (int argc, char** argv)  {
    
    char* url = 0;
    char* name = 0;
    
    if (argc == 2) {
	url = strdup(argv[1]);
    }
    else if (argc == 3) {
	name = strdup(argv[1]);
	url = strdup(argv[2]);
    }
    else {
	print_help();
	return 1;
    }

    //fprintf(stderr,"url %s name %s\n",url,name);

    Storage storage = profile_get_storage();
    char* rid_str = 0;
    char* did_raw = 0;
    if (parse_url(&rid_str,&did_raw,url)) {
	fprintf(stderr,"Failed to parse url %s\n",url);
	return 1;
    }
    //fprintf(stderr,"rid_str = %s, did_raw = %s\n",rid_str,did_raw);
    rad_git_init();
    RadRepo rrepo;
    rrepo.repo = 0;
    rrepo.rid = rid_to_oid(rid_str);
    char* repo_path = malloc(strlen(storage.path)+31);
    sprintf(repo_path,"%s/%s",storage.path,rid_str);
    if (git_repository_open(&rrepo.repo,repo_path)) {
	fprintf(stderr,"Failed to open git repository at path %s\n",repo_path);
	return 1;
    }

    json_object* identity_doc = get_identity_document(rrepo);
    if (!identity_doc) return 1;    
    char request [RAD_BUFSIZ2];
    while (1) {
	rad_get_input(request,RAD_BUFSIZ2);
	//fprintf(stderr,"request: %s\n",request);
	size_t request_len = strlen(request);
	if (!strcmp(request,"capabilities")) {
	    printf("option\npush\nfetch\n\n");
	    fflush(stdout);
	}
	else if (request_len>3 && !strcmp(rad_substr(request,0,4),"push")) {
	    char* refspec = rad_substr(request,5,0);
	    //fprintf(stderr,"refspec %s\n",refspec);
	    if (push_run(refspec,storage,rrepo,did_raw,identity_doc)) {
		fprintf(stderr,"Failed to push %s\n",refspec);
		return 1;
	    }
	    fflush(stdout);
	}
	else if (request_len>5 && !strcmp(rad_substr(request,0,6),"option")) {
	    if (request_len>14 && !strcmp(rad_substr(request,7,8),"progress")) {
		printf("unsupported\n");
		fflush(stdout);
	    }
	    else if (request_len>15 && !strcmp(rad_substr(request,7,9),"verbosity")) {
		printf("ok\n");
		fflush(stdout);
	    }
	    else {
		printf("unsupported\n");
		fflush(stdout);
	    }
	}
	else if (request_len>3 && !strcmp(rad_substr(request,0,4),"list")) {
	    if (request_len>12 && !strcmp(rad_substr(request,5,8),"for-push")) {
		if (list_for_push(rrepo,did_raw)) {
		    return 1;
		}
		printf("\n");
		fflush(stdout);
	    }
	    else if (!strcmp(request,"list")) {
		if (list_for_fetch(rrepo,did_raw)) {
		    return 1;
		}
		printf("\n");
		fflush(stdout);
	    }
	    else {
		printf("unsupported\n");
		fflush(stdout);
	    }
	}
	else if (!request_len) {
	    return 0;
	}
	else {
	    print_err("unknown request");
	    return 1;
	}
    }
    return 0;
}
