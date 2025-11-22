#include <stdio.h>
#include <string.h>

#include <repo.h>
#include <profile.h>
#include <id.h>
#include <git.h>

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
    if (!rrepo.repo) printf("!rrepo.repo\n");
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
    rrepo_res.ret = 0;
    rrepo_res.rrepo = rrepo;
    rrepo_res.oid = oid;
    return rrepo_res;
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
