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

RepoEntry rad_repo_store (RadRepo rrepo, Oid resource, Oid* related, size_t n_related, Pubkey signer, Create spec);

int rad_repo_update (RadRepo rrepo, Pubkey signer, const char* type_name, Oid obj_id, Oid entry_id);

#endif
