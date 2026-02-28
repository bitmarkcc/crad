#ifndef RADICLE_PATCH_H
#define RADICLE_PATCH_H

#include <command.h>
#include <id.h>

typedef struct {
    char err;
    Oid patch_id;
    size_t patch_id_hexlen;
    char* rid;
    bool json;
} PatchCommand;

int patch_run (Command c);
int patch_list (const char* rid);
int patch_show (Oid patch_id, size_t patch_id_hexlen, const char* rid, bool json);

#endif
