#ifndef RADICLE_GIT_H
#define RADICLE_GIT_H

#include <git2.h>

void rad_git_init ();

const char* get_default_branch (git_repository* repo);

#endif
