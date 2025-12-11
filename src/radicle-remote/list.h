#ifndef RADICLE_LIST_H
#define RADICLE_LIST_H

#include <repo.h>

extern const size_t HEXSIZ;

int list_for_push (RadRepo rrepo, const char* did_raw);

int list_for_fetch (RadRepo rrepo, const char* did_raw);

#endif
