#include <rad.h>
#include <string.h>
#include <repo.h>
#include <stdio.h>

rad_project_result rad_project_init (const git_repository* repo, const char* name, const char* description, const char* default_branch, const Visibility visibility, Pubkey signer, const Storage storage) {    
    Project project = {strdup(name),strdup(description),strdup(default_branch)};
    json_object* project_obj = project_to_json_obj(project);
    char* keys [1];
    keys[0] = "xyz.radicle.project";
    json_object* values [1];
    values[0] = project_obj;
    StrJsonMap payload = {1,keys,1,values};
    Document doc = {IDENTITY_VERSION,payload,1,&signer,1,visibility};

    RadRepoResult rrepo_result = rad_repo_init(doc,storage,signer);
    
    rad_project_result res;
    Rid rid;
    res.rid = rid;
    res.doc = &doc;
    res.ret = 0;
    return res;
}
