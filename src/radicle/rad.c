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
    res.rid = rid;
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
    printf("doing rad_init_configure\n");
    
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

    printf("cd %s; git push %s %s\n",path,rad_path,refspec);
    
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
    printf("%s\n",reflogmsg);
    git_reference* ref = 0;
    git_reference_create(&ref,repo,refname,&oid,0,reflogmsg);

    /*
      stored.set_remote_identity_root_to(pk, identity)?;
      stored.set_identity_head_to(identity)?;
      stored.set_head()?; // todo check why this is so complex in the rust code
    */
    
    sprintf(refname,"refs/namespaces/%s/refs/rad/root",didcore);

    const size_t HEXSIZ = GIT_OID_SHA1_HEXSIZE+1;
    char buf [HEXSIZ];
    printf("do set-id-root with refname %s and oid %s\n",refname,git_oid_tostr(buf,HEXSIZ,&identity));
    if (git_reference_create(&ref,rrepo.repo,refname,&identity,1,"set-id-root (radicle)")) {
	fprintf(stderr,"Failed to set git reference\n");
	return 1;
    }

    printf("do set-local-branch\n");
    
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
    printf("set local branch again\n");
    
    if (git_reference_create(&ref,rrepo.repo,refname,&oid_head,1,"set-local-branch (radicle)")) {
	fprintf(stderr,"Failed to set git reference\n");
	return 1;
    }

    printf("do set-head\n");
    
    sprintf(refname,"refs/heads/%s",default_branch);
    if (git_reference_symbolic_create(&ref,rrepo.repo,"HEAD",refname,1,"set-head (radicle)")) {
	fprintf(stderr,"Failed to create symbolic git reference\n");
	return 1;
    }
    
    // let signed = stored.sign_refs(signer)?;
    
    return 0;
}
