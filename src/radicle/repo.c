#include <stdio.h>
#include <string.h>

#include <repo.h>
#include <profile.h>
#include <id.h>
#include <git.h>
#include <util.h>
#include <print.h>
#include <set.h>

const size_t HEXSIZ = GIT_OID_SHA1_HEXSIZE+1;

RadRepo rad_repo_default () {
    RadRepo rrepo;
    Oid rid = {{0}};
    rrepo.rid = rid;
    rrepo.repo = 0;
}

RadRepo rad_repo_create (const char* path, const Oid rid, const StorageInfo si) {
    RadRepo rrepo;
    git_repository* repo = 0;
    rrepo.rid = rid;

    git_repository_init_options opts = GIT_REPOSITORY_INIT_OPTIONS_INIT;
    opts.flags |= GIT_REPOSITORY_INIT_BARE;
    opts.flags |= GIT_REPOSITORY_INIT_NO_REINIT;
    opts.flags |= GIT_REPOSITORY_INIT_MKDIR;
    
    if (git_repository_init_ext(&repo, path, &opts)) {
	fprintf(stderr,"failed to initialize git repository at %s\n",path);
	rrepo.repo = 0;
	return rrepo;
    }

    git_config* config = 0;
    if (git_repository_config(&config,repo)) {
	fprintf(stderr,"failed to get the config file for the git repository at %s\n",path);
	rrepo.repo = 0;
	return rrepo;
    }

    git_config_set_string(config,"user.name",si.name);
    git_config_set_string(config,"user.email",si.email);

    rrepo.repo = repo;
    return rrepo;
}

RepoEntry rad_repo_commit (RadRepo rrepo, Oid tree_oid, Oid* related, size_t n_related, char** headers, size_t n_headers, char** trailers, size_t n_trailers, char* message) {

    Oid oid;
    RepoEntry re;
    re.oid = oid;
    git_odb* odb = 0;
    if (git_repository_odb(&odb,rrepo.repo)) {
	fprintf(stderr,"Failed to get repository odb\n");
	return re;
    }
    
    char commit_str [4096]; //todo set right size
    char buf [HEXSIZ];
    git_signature* gitsig = 0;
    if (git_signature_default(&gitsig,rrepo.repo)) {
	fprintf(stderr,"Failed to get git signature\n");
	return re;
    }
    char* author_name = gitsig->name;
    char* author_email = gitsig->email;
    char author_time [20];
    sprintf(author_time,"%ld",gitsig->when.time);
    char author_sign [2];
    sprintf(author_sign,"%c",gitsig->when.sign);
    strcpy(commit_str,"tree ");
    strcat(commit_str,strdup(git_oid_tostr(buf,HEXSIZ,&tree_oid)));
    strcat(commit_str,"\n");
    for (size_t i=0; i<n_related; i++) {
	if (git_oid_is_zero(related+i)) continue;
	strcat(commit_str,"parent ");
	strcat(commit_str,strdup(git_oid_tostr(buf,HEXSIZ,related+i)));
	strcat(commit_str,"\n");
    }
    strcat(commit_str,"author ");
    strcat(commit_str,author_name);
    strcat(commit_str," <");
    strcat(commit_str,author_email);
    strcat(commit_str,"> ");
    strcat(commit_str,author_time);
    strcat(commit_str," ");
    strcat(commit_str,author_sign);
    strcat(commit_str,time_offset(gitsig->when.offset));
    strcat(commit_str,"\n");
    strcat(commit_str,"committer ");
    strcat(commit_str,author_name);
    strcat(commit_str," <");
    strcat(commit_str,author_email);
    strcat(commit_str,"> ");
    strcat(commit_str,author_time);
    strcat(commit_str," ");
    strcat(commit_str,author_sign);
    strcat(commit_str,time_offset(gitsig->when.offset));
    strcat(commit_str,"\n");
    for (size_t i=0; i<n_headers; i++) {
	strcat(commit_str,rad_indent_str(headers[i]));
	strcat(commit_str,"\n");
    }
    strcat(commit_str,"\n");
    strcat(commit_str,message);
    strcat(commit_str,"\n");

    if (n_trailers)
	strcat(commit_str,"\n");
    
    for (size_t i=0; i<n_trailers; i++) {
	strcat(commit_str,trailers[i]);
    }

    if (git_odb_write(&oid,odb,(uint8_t*)commit_str,strlen(commit_str),GIT_OBJECT_COMMIT)) {
	fprintf(stderr,"Failed to write commit to odb\n");
	return re;
    }

    re.oid = oid;
    return re;
}

RepoEntry rad_repo_store (RadRepo rrepo, Oid resource, Oid* related, size_t n_related, Pubkey signer, Create spec) {

    RepoEntry re = {{0}};
    Manifest manifest;
    manifest.type_name = spec.type_name;
    manifest.version = COB_VERSION;
    char* manifest_encoded = manifest_encode(manifest);
    Oid oid;
    git_odb* odb = 0;
    if (git_repository_odb(&odb,rrepo.repo)) {
	fprintf(stderr,"Failed to get repository odb\n");
	return re;
    }
    if (git_odb_write(&oid,odb,(uint8_t*)manifest_encoded,strlen(manifest_encoded),GIT_OBJECT_BLOB)) {
	fprintf(stderr,"Failed to write to ODB\n");
	return re;
    }
    git_treebuilder* treebuilder = 0;
    if (git_treebuilder_new(&treebuilder,rrepo.repo,0)) {
	fprintf(stderr,"Failed to initialize treebuilder\n");
	return re;
    }
    const git_tree_entry* tree_entry = 0;
    if (git_treebuilder_insert(&tree_entry,treebuilder,"manifest",&oid,GIT_FILEMODE_BLOB)) {
	fprintf(stderr,"Failed to insert to treebuilder\n");
	return re;
    }
    for (uint32_t i=0; i<spec.n_contents; i++) {
	char* content = spec.contents[i];
	if (git_odb_write(&oid,odb,(uint8_t*)content,strlen(content),GIT_OBJECT_BLOB)) {
	    fprintf(stderr,"Failed to write to odb\n");
	    return re;
	}
	char i_str [11];
	sprintf(i_str,"%u",i);
	if (git_treebuilder_insert(&tree_entry,treebuilder,i_str,&oid,GIT_FILEMODE_BLOB)) {
	    fprintf(stderr,"Failed to insert to treebuilder\n");
	    return re;
	}
    }

    if (spec.n_embeds) {
	git_treebuilder* treebuilder_embed = 0;
	if (git_treebuilder_new(&treebuilder_embed,rrepo.repo,0)) {
	    fprintf(stderr,"Failed to initialize embed treebuilder\n");
	    return re;
	}
	const git_tree_entry* tree_entry_embed = 0;
	for (uint32_t i=0; i<spec.n_embeds; i++) {
	    OidEmbed embed = spec.embeds[i];
	    oid = embed.content;
	    if (git_treebuilder_insert(&tree_entry_embed,treebuilder_embed,embed.name,&oid,GIT_FILEMODE_BLOB)) {
		fprintf(stderr,"Failed to insert to embed treebuilder\n");
		return re;
	    }
	}
	if (git_treebuilder_write(&oid,treebuilder_embed)) {
	    fprintf(stderr,"Failed to write the embeds tree\n");
	    return re;
	}
	if (git_treebuilder_insert(&tree_entry,treebuilder,"embeds",&oid,GIT_FILEMODE_TREE)) {
	    fprintf(stderr,"Failed to insert to main tree\n");
	}
    }
    
    if (git_treebuilder_write(&oid,treebuilder)) {
	fprintf(stderr,"Failed to write to treebuilder\n");
	return re;
    }

    uint8_t sig_bytes [64] = {0};
    key_sign_bytes(sig_bytes,signer,oid.id,20);
    const char* sig_ssh = rad_sig_to_ssh_format(sig_bytes,signer);

    oids_dedup(&related,&n_related);
    oids_sort(&related,n_related);

    char* header = malloc(2048); //todo set right size
    strcpy(header,"gpgsig ");
    strcat(header,sig_ssh);
    char** headers = &header;
    size_t n_headers = 1;
    char** trailers = 0;
    size_t n_trailers = 0;
    
    return rad_repo_commit(rrepo,oid,related,n_related,headers,n_headers,trailers,n_trailers,spec.message);
}


int rad_repo_update (RadRepo rrepo, Pubkey signer, const char* type_name, Oid obj_id, Oid entry_id) {
    git_reference* ref = 0;
    char refname [256]; //todo get right size here and below
    char reflogmsg [256];
    const size_t HEXSIZ = GIT_OID_SHA1_HEXSIZE+1;
    char buf [HEXSIZ];
    char* obj_id_str = strdup(git_oid_tostr(buf,HEXSIZ,&obj_id));
    char* entry_id_str = strdup(git_oid_tostr(buf,HEXSIZ,&entry_id));
    strcpy(refname,"refs/namespaces/");
    strcat(refname,pubkey_to_did(signer.bytes)+8);
    strcat(refname,"/refs/cobs/");
    strcat(refname,type_name);
    strcat(refname,"/");
    strcat(refname,obj_id_str);
    sprintf(reflogmsg,"Updating collaborative object '%s/%s' with new entry %s\n",type_name,obj_id_str,entry_id_str);
    if (git_reference_create(&ref,rrepo.repo,refname,&entry_id,1,reflogmsg)) {
	fprintf(stderr,"Failed to create git reference\n");
	return 1;
    }
    return 0;
}

int rad_repo_configure (git_repository* repo) { // for git repo
    git_config* config = 0;
    if (git_repository_config(&config,repo)) {
	fprintf(stderr,"failed to get the config file for the git repository\n");
	return 1;
    }
    git_config_set_string(config,"push.default","upstream");
}

int rad_repo_configure_remote (git_repository* repo, char* name, char* fetchurl, char* pushurl) { // for git repo
    
    char fetchspec [128];

    sprintf(fetchspec,"+refs/heads/*:refs/remotes/%s/*",name);
    git_remote* remote = 0;
    if(git_remote_create_with_fetchspec(&remote,repo,name,fetchurl,fetchspec)) {
	fprintf(stderr,"Failed to create fetch remote\n");
	return 1;
    }

    sprintf(fetchspec,"+refs/tags/*:refs/remotes/%s/tags/*",name);
    if (git_remote_add_fetch(repo,name,fetchspec)) {
	fprintf(stderr,"Failed to add fetchspec to remote %s\n",name);
	return 1;
    }
    
    if (strcmp(name,"rad")) {
	git_config* config = 0;
	if (git_repository_config(&config,repo)) {
	    fprintf(stderr,"failed to get the config file for the git repository\n");
	    return 1;
	}
	char* buf = malloc(strlen(name)+18);
	strcpy(buf,"remote.");
	strcat(buf,name);
	strcat(buf,".pruneTags");
	git_config_set_bool(config,buf,0);
	strcpy(buf,"remote.");
	strcat(buf,name);
	strcat(buf,".tagOpt");
	git_config_set_string(config,buf,"--no-tags");	
    }

    if (strcmp(pushurl,fetchurl)) {
	if (git_remote_set_pushurl(repo,name,pushurl)) {
	    fprintf(stderr,"Failed to set push url for remote %s\n",name);
	    return 1;
	}
    }
    return 0;
}

Oid rad_repo_sign_refs  (RadRepo rrepo, Pubkey signer) {
    Oid oid_ret = {{0}};
    const size_t HEXSIZ = GIT_OID_SHA1_HEXSIZE+1;
    char buf [HEXSIZ];
    char refs_str [1024]; //todo set the right size
    *refs_str = 0;
    git_reference_iterator* it = 0;
    char glob [128];
    const char* did_raw = pubkey_to_did(signer.bytes)+8;
    sprintf(glob,"refs/namespaces/%s/*",did_raw);
    if (git_reference_iterator_glob_new(&it,rrepo.repo,glob)) {
	fprintf(stderr,"Failed to create glob iterator\n");
	return oid_ret;
    }
    const char* name = 0;
    int ret = 0;
    Oid oid;
    while (!(ret = git_reference_next_name(&name,it))) {
	if (git_reference_name_to_id(&oid,rrepo.repo,name)) {
	    eprintf("failed to get oid from reference name");
	    return oid_ret;
	}
	char* oid_str = strdup(git_oid_tostr(buf,HEXSIZ,&oid));
	char* short_name = rad_substr(name,17+strlen(did_raw),0); // remove the refs/namespaces/<did>/ part
	// add oid and short_name to list of refs to sign
	strcat(refs_str,oid_str);
	strcat(refs_str," ");
	strcat(refs_str,short_name);
	strcat(refs_str,"\n");
    }
    if (ret != GIT_ITEROVER) {
	fprintf(stderr,"Error iterating over glob reference names\n");
	return oid_ret;
    }
    
    git_odb* odb = 0;
    if (git_repository_odb(&odb,rrepo.repo)) {
	fprintf(stderr,"Failed to get repository odb\n");
	return oid_ret;
    }
    iprintf("sign %s with len %u",refs_str,strlen(refs_str));
    if (git_odb_write(&oid,odb,(uint8_t*)refs_str,strlen(refs_str),GIT_OBJECT_BLOB)) {
	fprintf(stderr,"Failed to write refs to odb\n");
	return oid_ret;
    }
    Oid oid_refs = {{0}};
    if (git_oid_cpy(&oid_refs,&oid)) {
	fprintf(stderr,"Failed to copy oid structure\n");
	return oid_ret;
    }
    const char* oid_refs_str = strdup(git_oid_tostr(buf,HEXSIZ,&oid_refs));
    uint8_t sig_bytes [64] = {0};
    key_sign_bytes(sig_bytes,signer,(uint8_t*)refs_str,strlen(refs_str));
    if (git_odb_write(&oid,odb,sig_bytes,64,GIT_OBJECT_BLOB)) {
	fprintf(stderr,"Failed to write sig to odb\n");
	return oid_ret;
    }
    if (rad_sig_verify(refs_str,strlen(refs_str),sig_bytes,signer)) {
	eprintf("failed to verify sig (test)\n");
	return oid_ret;
    }
    Oid oid_sig = {{0}};
    if (git_oid_cpy(&oid_sig,&oid)) {
	fprintf(stderr,"Failed to copy oid structure\n");
	return oid_ret;
    }
    const char* oid_sig_str = strdup(git_oid_tostr(buf,HEXSIZ,&oid_sig));
    git_treebuilder* treebuilder = 0;
    if (git_treebuilder_new(&treebuilder,rrepo.repo,0)) {
	fprintf(stderr,"Failed to initialize treebuilder\n");
	return oid_ret;
    }
    const git_tree_entry* tree_entry = 0;
    if (git_treebuilder_insert(&tree_entry,treebuilder,"refs",&oid_refs,GIT_FILEMODE_BLOB)) {
	fprintf(stderr,"Failed to insert to treebuilder\n");
	return oid_ret;
    }
    if (git_treebuilder_insert(&tree_entry,treebuilder,"signature",&oid_sig,GIT_FILEMODE_BLOB)) {
	fprintf(stderr,"Failed to insert to treebuilder\n");
	return oid_ret;
    }
    if (git_treebuilder_write(&oid,treebuilder)) {
	fprintf(stderr,"Failed to write to treebuilder\n");
	return oid_ret;
    }

    return oid;
}

int rad_repo_set_upstream (git_repository* repo, const char* branch) {
    git_config* config = 0;
    if (git_repository_config(&config,repo)) {
	fprintf(stderr,"Failed to get the config file for the git repository\n");
	return 1;
    }

    char* branch_remote = malloc(strlen(branch)+15);
    sprintf(branch_remote,"branch.%s.remote",branch);
    char* branch_merge = malloc(strlen(branch)+14);
    sprintf(branch_merge,"branch.%s.merge",branch);
    char* branch_merge_value = malloc(12+strlen(branch));
    sprintf(branch_merge_value,"refs/heads/%s",branch);
    
    git_config_set_multivar(config,branch_remote,".*","rad");
    git_config_set_multivar(config,branch_merge,".*",branch_merge_value);
}

Oid rad_repo_validate (const char* path) {
    Oid rid = {{0}};
    char buf [HEXSIZ];

    //check local repo with git fsck
    char* argv [5];
    argv[0] = "git";
    argv[1] = "-C";
    argv[2] = strdup(path);
    argv[3] = "fsck";
    argv[4] = 0;
    if (exec_command("git",argv)) {
	eprintf("git fsck command failed on %s",path);
	return rid;
    }

    // get candidate rid from the git config for the rad remote
    git_repository* repo = 0;
    if (git_repository_open(&repo,path)) {
	eprintf("failed to open git repository at %s",path);
	return rid;
    }
    git_config* config = 0;
    if (git_repository_config(&config,repo)) {
	eprintf("failed to get the config file for the git repository at %s\n",path);
	return rid;
    }
    git_config_iterator* it = 0;
    if (git_config_multivar_iterator_new(&it,config,"remote.rad.url",0)) {
	eprintf("failed to get git config iterator");
	return rid;
    }
    git_config_entry* entry = 0;
    Oid rid_candidate = {{0}};
    while (!git_config_next(&entry,it)) {
	rid_candidate = rid_to_oid(strdup(entry->value)+6);
	break; // todo assume only one entry?
    }
    const char* rid_candidate_str = oid_to_rid(rid_candidate);

    // setup rad repo struct and get the radicle storage path for the candidate rid
    RadRepo rrepo;
    rrepo.repo = 0;
    rrepo.rid = rid_candidate;
    git_repository* repo_rad = 0;
    Storage storage = profile_get_storage();
    char* path_rad = malloc(strlen(storage.path)+64);
    sprintf(path_rad,"%s/%s",storage.path,rid_candidate_str);

    // run git fsck on the rad repo
    argv[2] = strdup(path_rad);
    if (exec_command("git",argv)) {
	eprintf("git fsck command failed on %s",path_rad);
	return rid;
    }

    // open the git repo associated with the rad repo and set the RadRepo repo property
    if (git_repository_open(&repo_rad,path_rad)) {
	eprintf("failed to open git repository at %s",path_rad);
	return rid;
    }
    rrepo.repo = repo_rad;

    // check if rid dervied from id doc matches the rid candidate
    Oid rid_from_id_doc = get_root_identity_doc_oid(rrepo.repo);
    if (!git_oid_equal(&rid_from_id_doc,&rid_candidate)) {
	eprintf("rid from id doc doesn't match the candidate rid");
	return rid;
    }

    // get list of all branches in git repo
    const char* repo_path = git_repository_path(repo);
    char* heads_path = "refs/heads";
    SimpleSet files;
    set_init(&files);
    if (rad_dir_list_recursive(repo_path,heads_path,&files)) {
	eprintf("failed to get list of refs");
	return rid;
    }
    size_t n_files = 0;
    char** files_list = set_to_array(&files,&n_files);    
    // get corresponding list of oids for these branches
    Oid* oids_list = malloc(n_files*sizeof(Oid));
    for (size_t i=0; i<n_files; i++) {
	if (git_reference_name_to_id(oids_list+i,repo,files_list[i])) {
	    eprintf("failed to get the oid of a reference name: %s",files_list[i]);
	    return rid;
	}
	iprintf("file %s oid %s",files_list[i],git_oid_tostr(buf,HEXSIZ,oids_list+i));
    }

    // make a list of the refs/oids that are in the rad repo. Also make list of all namespaces.
    git_reference_iterator* refit = 0;
    const char* glob = "refs/namespaces/*";
    if (git_reference_iterator_glob_new(&refit,rrepo.repo,glob)) {
	eprintf("failed to create glob iterator");
	return rid;
    }
    const char* refname = 0;
    int ret = 0;
    Oid oid;
    SimpleSet rrepo_files;
    set_init(&rrepo_files);
    SimpleSet namespaces;
    set_init(&namespaces);
    while (!(ret = git_reference_next_name(&refname,refit))) {
	set_add_str(&rrepo_files,strdup(refname));
	iprintf("added %s to rrepo_files set",refname);
	if (set_contains_str(&rrepo_files,"refs/namespaces/z6Mkuq9mgy1DgxbCLEb5fAdMWf55nLmiyosL3uVfwHT52eK6/refs/heads/feature/sync")) {
	    eprintf("rrepo_files set doesn't contain the sync feature");
	    //return rid;
	}
	set_add_str(&namespaces,rad_namespace_from_ref(refname));
    }    
    if (ret != GIT_ITEROVER) {
	eprintf("failed to iterate over glob reference names");
	return rid;
    }
    size_t n_rrepo_files = 0;
    char** rrepo_files_list = set_to_array(&rrepo_files,&n_rrepo_files);
    for (size_t i=0; i<n_rrepo_files; i++) iprintf("rrepo_files_list[%u] = %s\n",i,rrepo_files_list[i]);
    Oid* rrepo_oids_list = malloc(n_rrepo_files*sizeof(Oid));
    iprintf("n_rrepo_files = %u",n_rrepo_files);
    for (size_t i=0; i<n_rrepo_files; i++) {
	iprintf("rrepo_files_list[%u] = %s\n",i,rrepo_files_list[i]);
	if (git_reference_name_to_id(rrepo_oids_list+i,rrepo.repo,rrepo_files_list[i])) {
	    eprintf("failed to get the oid of a reference name: %s",rrepo_files_list[i]);
	    return rid;
	}
	iprintf("rrepo file %s oid %s",rrepo_files_list[i],git_oid_tostr(buf,HEXSIZ,rrepo_oids_list+i));
    }
    size_t n_namespaces = 0;
    char** namespaces_list = set_to_array(&namespaces,&n_namespaces);
    SimpleSet sigref_entries;
    set_init(&sigref_entries);
    char* sigrefname = malloc(128);
    for (size_t i=0; i<n_namespaces; i++) {
	iprintf("namespace %s",namespaces_list[i]);
	sprintf(sigrefname,"refs/namespaces/%s/refs/rad/sigrefs",namespaces_list[i]);
	//open sigrefs commit for the namespace
	Oid sigrefs_oid = {{0}};
	if (git_reference_name_to_id(&sigrefs_oid,rrepo.repo,sigrefname)) {
	    eprintf("failed to get id of reference %s",sigrefname);
	    return rid;
	}
	git_commit* commit = 0;
	if (git_commit_lookup(&commit,rrepo.repo,&sigrefs_oid)) {
	    eprintf("failed to lookup git commit");
	    return rid;
	}
	git_tree* tree = 0;
	if (git_commit_tree(&tree,commit)) {
	    fprintf(stderr,"Failed to get tree associated with a git commit\n");
	    return rid;
	}
	git_tree_entry* tree_entry = 0;
	if (git_tree_entry_bypath(&tree_entry,tree,"refs")) {
	    eprintf("Can't find the git tree entry refs for the rad/sigrefs ref\n");
	    return rid;
	}
	const Oid* poid_refs = git_tree_entry_id(tree_entry);
	if (!poid_refs) {
	    eprintf("Can't find oid of git tree entry\n");
	    return rid;
	}
	git_blob* blob = 0;
	if (git_blob_lookup(&blob,rrepo.repo,poid_refs)) {
	    eprintf("Can't lookup blob corresponding to git oid");
	    return rid;
	}
	const uint8_t* blob_content = git_blob_rawcontent(blob);
	size_t refs_size = git_blob_rawsize(blob);
	const uint8_t* refs_content = strdup(blob_content);
	char* token = strtok((char*)blob_content,"\n");
	while (token) {
	    set_add_str(&sigref_entries,token);
	    token = strtok(0,"\n");
	}
	//verify signature
	tree_entry = 0;
	if (git_tree_entry_bypath(&tree_entry,tree,"signature")) {
	    eprintf("Can't find the git tree entry `signature` for the rad/sigrefs ref\n");
	    return rid;
	}
	poid_refs = git_tree_entry_id(tree_entry);
	if (!poid_refs) {
	    eprintf("Can't find oid of git tree entry\n");
	    return rid;
	}
	blob = 0;
	if (git_blob_lookup(&blob,rrepo.repo,poid_refs)) {
	    eprintf("Can't lookup blob corresponding to git oid");
	    return rid;
	}
	const uint8_t* sig = git_blob_rawcontent(blob);
	Pubkey signer;
	signer.bytes = raw_did_to_pubkey(namespaces_list[i]);
	iprintf("verify %s with len %lu",refs_content,refs_size);
	if (rad_sig_verify(refs_content,refs_size,sig,signer)) {
	    eprintf("signature verification failed for sigrefs with namespace %s",namespaces_list[i]);
	    return rid;
	}
    }

    size_t n_sigref_entries = 0;
    char** sigref_entries_list = set_to_array(&sigref_entries,&n_sigref_entries);

    // Compare the files/oids list with the rrepo files/oids list to make sure rrepo contains each of the files/oids in the local repo list. Also check if the files/oids list matches with the sigref entries list.
    bool allmatch = true;
    for (size_t i=0; i<n_files; i++) {
	iprintf("check %s",files_list[i]);
	bool matches = false;
	for (size_t j=0; j<n_rrepo_files; j++) {
	    if (git_oid_equal(oids_list+i,rrepo_oids_list+j)) {
		if (!strcmp(files_list[i],rad_refname_relative(rrepo_files_list[j]))) {
		    matches = true;
		    break;
		}
	    }
	}
	if (!matches) {
	    allmatch = false;
	    eprintf("a ref from local repo doesn't match with one in the rad repo");
	    break;
	}
	matches = false;
	for (size_t j=0; j<n_sigref_entries; j++) {
	    Oid oid_entry;
	    if (git_oid_fromstr(&oid_entry,rad_sigref_entry_oid(sigref_entries_list[j]))) {
		eprintf("failed to parse git oid from string");
		return rid;
	    }
	    if (git_oid_equal(oids_list+i,&oid_entry)) {
		if (!strcmp(files_list[i],rad_sigref_entry_name(sigref_entries_list[j]))) {
		    matches = true;
		    break;
		}
	    }
	}
	if (!matches) {
	    allmatch = false;
	    eprintf("a ref from local repo doesn't match with a sigref entry in the rad repo");
	    break;
	}
    }
    if (!allmatch) {
	return rid;
    }
  
    rid = rid_candidate;
    iprintf("repo valid with rid %s",oid_to_rid(rid));
    return rid;
}
