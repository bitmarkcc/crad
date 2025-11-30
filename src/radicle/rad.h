#ifndef RADICLE_RAD_H
#define RADICLE_RAD_H

#include <git.h>
#include <project.h>
#include <document.h>
#include <id.h>
#include <storage.h>

typedef struct {
    Oid rid;
    Document* doc;
    int ret; // return code
} RadProjectResult;

RadProjectResult rad_project_init (git_repository* repo, const char* name, const char* description, const char* default_branch, const Visibility visibility, const Pubkey signer, const Storage storage);

RadRepoResult rad_repo_init (Document doc, Storage s, Pubkey signer);

int rad_init_configure (git_repository* repo, RadRepo rrepo, const char* default_branch, Oid identity, Pubkey signer);

#endif
