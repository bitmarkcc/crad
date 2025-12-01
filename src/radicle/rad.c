#include <rad.h>
#include <string.h>
#include <repo.h>
#include <stdio.h>
#include <profile.h>

RadProjectResult rad_project_init (git_repository* repo, const char* name, const char* description, const char* default_branch, const Visibility visibility, Pubkey signer, const Storage storage) {    
    Project project = {strdup(name),strdup(description),strdup(default_branch)};
    json_object* project_obj = project_to_json_obj(project);
    char* keys [1];
    keys[0] = "xyz.radicle.project";
    json_object* values [1];
    values[0] = project_obj;
    StrJsonMap payload = {1,keys,1,values};
    Document doc = {IDENTITY_VERSION,payload,1,&signer,1,visibility};

    RadRepoResult rrepo_result = rad_repo_init(doc,storage,signer);

    rad_init_configure(repo,rrepo_result.rrepo,default_branch,rrepo_result.oid,signer);
    
    RadProjectResult res;
    Oid rid;
    res.rid = rrepo_result.rrepo.rid;
    res.doc = &doc;
    res.ret = 0;
    return res;
}

RadRepoResult rad_repo_init (const Document doc, const Storage s, const Pubkey signer) {
    rad_git_init();
    RadRepoResult rrepo_res;
    rrepo_res.ret = 1;
    DocumentEncoding doc_encoding = document_encode(doc);
    char* rad_home = get_rad_home();
    if (!rad_home) return rrepo_res;
    char* path = malloc(strlen(s.path)+31);
    strcpy(path,s.path);
    strcat(path,"/");
    strcat(path,oid_to_rid(doc_encoding.oid));
    RadRepo rrepo = rad_repo_create(path,doc_encoding.oid,s.info);
    git_odb* odb = 0;
    if (git_repository_odb(&odb,rrepo.repo)) {
	fprintf(stderr,"Failed to get repository odb\n");
	return rrepo_res;
    }
    git_oid oid;

    if (git_odb_write(&oid,odb,doc_encoding.bytes,doc_encoding.n_bytes,GIT_OBJECT_BLOB)) {
	fprintf(stderr,"Failed to write to odb\n");
	return rrepo_res;
    }
    rad_assert_equal(doc_encoding.oid.id,oid.id,20);

    Oid commit = document_init(doc,rrepo,signer);
    
    rrepo_res.ret = 0;
    rrepo_res.rrepo = rrepo;
    rrepo_res.oid = commit;
    return rrepo_res;
}

int rad_init_configure (git_repository* repo, RadRepo rrepo, const char* default_branch, Oid identity, Pubkey signer) {
    
    rad_repo_configure(repo);
    
    char fetchurl [128];
    char pushurl [128];
    char* rid = oid_to_rid(rrepo.rid);
    char* didcore = pubkey_to_did(signer.bytes)+8;
    strcpy(fetchurl,"rad://");
    strcat(fetchurl,rid);
    strcpy(pushurl,"rad://");
    strcat(pushurl,rid);
    strcat(pushurl,"/");
    strcat(pushurl,didcore);
	
    rad_repo_configure_remote(repo,"rad",fetchurl,pushurl);

    const char* path = git_repository_path(repo);
    const char* rad_path = git_repository_path(rrepo.repo);
    char src [128];
    char dst [128];
    strcpy(src,"refs/heads/");
    strcat(src,default_branch);
    strcpy(dst,"refs/namespaces/");
    strcat(dst,didcore);
    strcat(dst,"/refs/heads/");
    strcat(dst,default_branch);
    char refspec [256];
    sprintf(refspec,"%s:%s",src,dst);

    char* argv [7];
    argv[0] = "git";
    argv[1] = "-C";
    argv[2] = strdup(path);
    argv[3] = "push";
    argv[4] = strdup(rad_path);
    argv[5] = strdup(refspec);
    argv[6] = 0;
    
    if (exec_command("git",argv)) {
	fprintf(stderr,"git command failed\n");
	return 1;
    }
   
    char refname [128];
    sprintf(refname,"refs/remotes/rad/%s",default_branch);
    Oid oid = {{0}};
    git_reference_name_to_id(&oid,repo,src);
    char reflogmsg [256];
    sprintf(reflogmsg,"radicle: remote branch rad/%s",default_branch);
    git_reference* ref = 0;
    git_reference_create(&ref,repo,refname,&oid,0,reflogmsg);
    
    sprintf(refname,"refs/namespaces/%s/refs/rad/root",didcore);

    const size_t HEXSIZ = GIT_OID_SHA1_HEXSIZE+1;
    char buf [HEXSIZ];
    if (git_reference_create(&ref,rrepo.repo,refname,&identity,1,"set-id-root (radicle)")) {
	fprintf(stderr,"Failed to set git reference\n");
	return 1;
    }
    
    if (git_reference_create(&ref,rrepo.repo,"refs/rad/id",&identity,1,"set-local-branch (radicle)")) {
	fprintf(stderr,"Failed to set git reference\n");
	return 1;
    }

    sprintf(refname,"refs/heads/%s",default_branch);
    Oid oid_head = {{0}};
    if (git_reference_name_to_id(&oid_head,repo,refname)) {
	fprintf(stderr,"Failed to get oid from git reference name\n");
	return 1;
    }
    
    if (git_reference_create(&ref,rrepo.repo,refname,&oid_head,1,"set-local-branch (radicle)")) {
	fprintf(stderr,"Failed to set git reference\n");
	return 1;
    }
    
    sprintf(refname,"refs/heads/%s",default_branch);
    if (git_reference_symbolic_create(&ref,rrepo.repo,"HEAD",refname,1,"set-head (radicle)")) {
	fprintf(stderr,"Failed to create symbolic git reference\n");
	return 1;
    }

    char refs_str [1024];
    char* identity_str = strdup(git_oid_tostr(buf,HEXSIZ,&identity));
    char* oid_head_str = strdup(git_oid_tostr(buf,HEXSIZ,&oid_head));
    strcpy(refs_str,identity_str);
    strcat(refs_str," refs/cobs/xyz.radicle.id/");
    strcat(refs_str,identity_str);
    strcat(refs_str,"\n");
    strcat(refs_str,oid_head_str);
    strcat(refs_str," refs/heads/");
    strcat(refs_str,default_branch);
    strcat(refs_str,"\n");
    strcat(refs_str,identity_str);
    strcat(refs_str," refs/rad/id\n");
    strcat(refs_str,identity_str);
    strcat(refs_str," refs/rad/root\n");

    git_odb* odb = 0;
    if (git_repository_odb(&odb,rrepo.repo)) {
	fprintf(stderr,"Failed to get repository odb\n");
	return 1;
    }

    if (git_odb_write(&oid,odb,(uint8_t*)refs_str,strlen(refs_str),GIT_OBJECT_BLOB)) {
	fprintf(stderr,"Failed to write refs to odb\n");
	return 1;
    }

    Oid oid_refs = {{0}};
    if (git_oid_cpy(&oid_refs,&oid)) {
	fprintf(stderr,"Failed to copy oid structure\n");
	return 1;
    }

    const char* oid_refs_str = strdup(git_oid_tostr(buf,HEXSIZ,&oid_refs));

    uint8_t* sig_raw = 0;
    size_t sig_raw_len = 0;
    key_sign_raw_unencoded(&sig_raw,&sig_raw_len,signer,refs_str,strlen(refs_str));

    if (git_odb_write(&oid,odb,sig_raw,sig_raw_len,GIT_OBJECT_BLOB)) {
	fprintf(stderr,"Failed to write sig to odb\n");
	return 1;
    }

    Oid oid_sig = {{0}};
    if (git_oid_cpy(&oid_sig,&oid)) {
	fprintf(stderr,"Failed to copy oid structure\n");
	return 1;
    }

    const char* oid_sig_str = strdup(git_oid_tostr(buf,HEXSIZ,&oid_sig));
    
    git_treebuilder* treebuilder = 0;
    if (git_treebuilder_new(&treebuilder,rrepo.repo,0)) {
	fprintf(stderr,"Failed to initialize treebuilder\n");
	return 1;
    }
    const git_tree_entry* tree_entry = 0;
    if (git_treebuilder_insert(&tree_entry,treebuilder,"refs",&oid_refs,GIT_FILEMODE_BLOB)) {
	fprintf(stderr,"Failed to insert to treebuilder\n");
	return 1;
    }
    if (git_treebuilder_insert(&tree_entry,treebuilder,"signature",&oid_sig,GIT_FILEMODE_BLOB)) {
	fprintf(stderr,"Failed to insert to treebuilder\n");
	return 1;
    }

    if (git_treebuilder_write(&oid,treebuilder)) {
	fprintf(stderr,"Failed to write to treebuilder\n");
	return 1;
    }

    Oid* related = 0;
    size_t n_related = 0;
    char** headers = 0;
    size_t n_headers = 0;
    char** trailers = 0;
    size_t n_trailers = 0;
    char* message = "Update signed refs";
    rad_repo_commit(rrepo,oid,related,n_related,headers,n_headers,trailers,n_trailers,message);
    
    return 0;
}
