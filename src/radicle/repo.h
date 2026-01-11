#ifndef RADICLE_REPO_H
#define RADICLE_REPO_H

#include <storage.h>
#include <key.h>
#include <id.h>
#include <cob/identity.h>

typedef struct {
    Oid rid;
    git_repository* repo;
} RadRepo;

typedef struct {
    int ret;
    RadRepo rrepo;
    Oid oid;
} RadRepoResult;

typedef struct {
    Oid oid;
    uint8_t* bytes;
} OidBytes;

typedef struct {
    Oid oid;
} RepoEntry;

RadRepo rad_repo_default ();

RadRepo rad_repo_create (const char* path, const Oid rid, StorageInfo si);

RepoEntry rad_repo_store (git_repository* repo, Oid resource, Oid* related, size_t n_related, Pubkey signer, Create spec);

int rad_repo_update (git_repository* repo, Pubkey signer, const char* type_name, Oid obj_id, Oid entry_id);

int rad_repo_configure (git_repository* repo);

int rad_repo_configure_remote (git_repository* repo, char* name, char* fetchurl, char* pushurl);

RepoEntry rad_repo_commit (git_repository* repo, Oid tree_oid, Oid* related, size_t n_related, char** headers, size_t n_headers, char** trailers, size_t n_trailers, char* message); 

Oid rad_repo_sign_refs (RadRepo rrepo, Pubkey signer);

int rad_repo_set_upstream (git_repository* repo, const char* branch);

Oid rid_of_rad_remote (git_repository* repo);

Oid rad_repo_validate (const char* path);

int get_rad_repo_from_cwd (RadRepo* out);

int create_sigrefs_commit (RadRepo rrepo, Pubkey signer, Oid tree_oid);

#endif
