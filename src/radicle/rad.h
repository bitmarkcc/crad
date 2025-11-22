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
} rad_project_result;

rad_project_result rad_project_init (const git_repository* repo, const char* name, const char* description, const char* default_branch, const Visibility visibility, const Pubkey signer, const Storage storage);

#endif
