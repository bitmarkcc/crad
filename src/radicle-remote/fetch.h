#ifndef RADICLE_FETCH_H
#define RADICLE_FETCH_H

#include <storage.h>
#include <repo.h>

extern const size_t HEXSIZ;

int fetch_run (Storage storage, RadRepo rrepo, const char* did_raw, const char* oid_str, const char* refstr);

#endif
