#ifndef RADICLE_PUSH_H
#define RADICLE_PUSH_H

#include <json-c/json.h>

#include <storage.h>
#include <repo.h>

extern const size_t HEXSIZ;

int push_run (const char* refspec, Storage storage, RadRepo rrepo, const char* did_raw, json_object* identity_doc);

#endif
