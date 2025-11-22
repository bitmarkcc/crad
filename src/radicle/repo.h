#ifndef RADICLE_REPO_H
#define RADICLE_REPO_H

#include <document.h>
#include <storage.h>
#include <key.h>
#include <id.h>

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

RadRepoResult rad_repo_init (Document doc, Storage s, Pubkey signer);

RadRepo rad_repo_create(const char* path, const Oid rid, StorageInfo si);

char* get_rad_home ();

#endif
