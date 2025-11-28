#include <rad.h>
#include <string.h>
#include <repo.h>
#include <stdio.h>
#include <profile.h>

RadProjectResult rad_project_init (const git_repository* repo, const char* name, const char* description, const char* default_branch, const Visibility visibility, Pubkey signer, const Storage storage) {    
    Project project = {strdup(name),strdup(description),strdup(default_branch)};
    json_object* project_obj = project_to_json_obj(project);
    char* keys [1];
    keys[0] = "xyz.radicle.project";
    json_object* values [1];
    values[0] = project_obj;
    StrJsonMap payload = {1,keys,1,values};
    Document doc = {IDENTITY_VERSION,payload,1,&signer,1,visibility};

    RadRepoResult rrepo_result = rad_repo_init(doc,storage,signer);
    
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
    rrepo_res.oid = oid;
    return rrepo_res;
}
