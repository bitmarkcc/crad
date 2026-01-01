#include <stdio.h>
#include <libssh/libssh.h>
#include <string.h>

#include <cob/identity.h>
#include <util.h>
#include <print.h>
#include <key.h>

char* manifest_encode (Manifest manifest) {
    json_object* obj = json_object_new_object();

    json_object_object_add(obj,"typeName",json_object_new_string(manifest.type_name));
    json_object_object_add(obj,"version",json_object_new_int(manifest.version));
    
    return rad_remove_space_json(json_object_to_json_string(obj));
}

json_object* get_identity_document (git_repository* repo) {
    Oid oid;
    if (git_reference_name_to_id(&oid,repo,"refs/rad/id")) {
	fprintf(stderr,"Failed to get oid of reference name\n");
	return 0;
    }
    git_commit* commit = 0;
    if (git_commit_lookup(&commit,repo,&oid)) {
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
    const Oid* poid = git_tree_entry_id(tree_entry);
    if (!poid) {
	fprintf(stderr,"Can't find oid of git tree entry\n");
	return 0;
    }
    oid = *poid;
    char buf [HEXSIZ];
    iprintf("id document oid is %s oid to rid is %s",git_oid_tostr(buf,HEXSIZ,&oid),oid_to_rid(oid));
    git_blob* blob = 0;
    if (git_blob_lookup(&blob,repo,poid)) {
	fprintf(stderr,"Can't lookup blob corresponding to git oid\n");
	return 0;
    }
    const uint8_t* blob_content = git_blob_rawcontent(blob);
    return json_tokener_parse((char*)blob_content);
}

Oid get_root_identity_doc_oid (git_repository* repo) { // also validate sigs
    Oid ret = {{0}};
    char buf [HEXSIZ];

    //get commit for latest id doc
    Oid oid_commit = {{0}};
    if (git_reference_name_to_id(&oid_commit,repo,"refs/rad/id")) {
	fprintf(stderr,"Failed to get oid of reference name\n");
	return ret;
    }
    git_commit* commit = 0;
    if (git_commit_lookup(&commit,repo,&oid_commit)) {
	fprintf(stderr,"Failed to lookup commit from git repo\n");
	return ret;
    }
    
    git_commit* parent = 0;
    size_t n_commit_oids = 0;
    size_t commit_oids_capacity = 8;
    Oid* commit_oids = malloc(commit_oids_capacity*sizeof(Oid));
    commit_oids[0] = oid_commit;
    n_commit_oids++;
    while (1) { // Follow the parents until you reach the root commit. Save each commit in a list.
	if (git_commit_parentcount(commit)) {   
	    if (git_commit_parent(&parent,commit,0)) { // todo handle multiple parents
		fprintf(stderr,"Failed to get tree associated with a git commit\n");
		return ret;
	    }
	    commit = parent;
	    parent = 0;
	    if (n_commit_oids > commit_oids_capacity) {
		commit_oids_capacity *= 2;
		commit_oids = realloc(commit_oids,commit_oids_capacity*sizeof(Oid));
	    }
	    commit_oids[n_commit_oids] = *git_commit_id(commit);
	    n_commit_oids++;
	}
	else {
	    break;
	}
    }
    
    Oid root_doc_oid = {{0}};
    for (size_t i=n_commit_oids-1; i>=1; i--) { // Now backtrack and check sigs
	if (git_commit_lookup(&parent,repo,commit_oids+i)) {
	    eprintf("failed to lookup commit from git repo");
	    return ret;
	}
	if (git_commit_lookup(&commit,repo,commit_oids+i-1)) {
	    eprintf("failed to lookup commit from git repo");
	    return ret;
	}

	// verify that ssh sig is made by committer of commit 
	git_tree* tree = 0;
	if (git_commit_tree(&tree,commit)) {
	    fprintf(stderr,"Failed to get tree associated with a git commit\n");
	    return ret;
	}
	Oid oid_tree = *git_tree_id(tree);
	git_buf sig = {0};
	git_buf signed_data = {0};
	oid_commit = *git_commit_id(commit);
	if (git_commit_extract_signature(&sig,&signed_data,repo,&oid_commit,0)) {
	    eprintf("failed to extract signature from commit");
	    return ret;
	}
	const git_signature* gitsig = git_commit_committer(commit);
	const char* email = gitsig->email;
	Pubkey committer = {0};
	committer.bytes = raw_did_to_pubkey(rad_email_get_domain(email));
	if (rad_sshsig_verify(oid_tree.id,20,sig.ptr,committer)) {
	    eprintf("failed to validate ssh signature");
	    return ret;
	}

	// get the delegates set by the parent and check that one of them is the signer of `commit`
	tree = 0;
	if (git_commit_tree(&tree,parent)) {
	    eprintf("failed to get tree associated with a git commit");
	    return ret;
	}
	if (i == n_commit_oids-1) { // check the parent sig just for the root commit, as others overlap with `commit`
	    Oid oid_tree = *git_tree_id(tree);
	    git_buf sig = {0};
	    git_buf signed_data = {0};
	    oid_commit = *git_commit_id(parent);
	    if (git_commit_extract_signature(&sig,&signed_data,repo,commit_oids+i,0)) {
		eprintf("failed to extract signature from commit");
		return ret;
	    }
	    const git_signature* gitsig = git_commit_committer(parent);
	    const char* email = gitsig->email;
	    Pubkey committer_root = {0};
	    committer_root.bytes = raw_did_to_pubkey(rad_email_get_domain(email));
	    if (rad_sshsig_verify(oid_tree.id,20,sig.ptr,committer_root)) {
		eprintf("failed to validate ssh signature of root commit");
		return ret;
	    }
	}
	git_tree_entry* tree_entry = 0;
	if (git_tree_entry_bypath(&tree_entry,tree,"embeds/radicle.json")) {
	    fprintf(stderr,"Can't find the git tree entry embeds/radicle.json for the root rad/id ref\n");
	    return ret;
	}
	const Oid* poid_entry = git_tree_entry_id(tree_entry);
	if (!poid_entry) {
	    fprintf(stderr,"Can't find oid of git tree entry\n");
	    return ret;
	}
	if (i==n_commit_oids-1) root_doc_oid = *poid_entry;;
	git_blob* blob = 0;
	if (git_blob_lookup(&blob,repo,poid_entry)) {
	    fprintf(stderr,"Can't lookup blob corresponding to git oid\n");
	    return ret;
	}
	const uint8_t* blob_content = git_blob_rawcontent(blob);
	json_object* identity_doc = json_tokener_parse((char*)blob_content);
	json_object_object_foreach(identity_doc,key,val) {
	    if (!strcmp(key,"delegates")) {
		size_t n_delegates = json_object_array_length(val);
		bool signer_match = false;
		for (size_t i=0; i<n_delegates; i++) {
		    json_object* delegate_obj = json_object_array_get_idx(val,i);
		    Pubkey delegate = {0};
		    delegate.bytes = did_to_pubkey(rad_strip('"',json_object_to_json_string(delegate_obj)));
		    if (!memcmp(delegate.bytes,committer.bytes,32)) {
			signer_match = true;
			break;
		    }
		}
		if (!signer_match) {
		    eprintf("The committer is not an authorized delegate");
		    return ret;
		}
	    }
	    else if (!strcmp(key,"threshold")) {
		int threshold = json_object_get_int(val);
		if (threshold != 1) {
		    eprintf("This program currently only supports a signing threshold of 1");
		    return ret;
		}
	    }
	}
    }
    return root_doc_oid;
}
