#ifndef RADICLE_GIT_H
#define RADICLE_GIT_H

#include <git2.h>

void rad_git_init ();

char* get_default_branch (git_repository* repo);

char* rad_refname_relative (const char* name);

char* rad_namespace_from_ref (const char* refname);

char* rad_sigref_entry_oid (const char* entry);

char* rad_sigref_entry_name (const char* entry);

#endif
