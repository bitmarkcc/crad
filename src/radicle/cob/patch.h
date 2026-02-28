#ifndef RADICLE_COB_PATCH_H
#define RADICLE_COB_PATCH_H

#include <stdint.h>

#include <id.h>
#include <cob/common.h>

extern const uint32_t COB_VERSION;
extern const size_t HEXSIZ;

typedef enum {
    PATCH_ACTION_REVISION,
    PATCH_ACTION_EDIT
} PatchActionType;

typedef struct {
    PatchActionType type;
    // revision fields
    char* description;
    Oid base;
    Oid oid;
    // edit fields
    char* title;
    char* target; // "delegates"
} PatchAction;

typedef struct {
    size_t n_actions;
    PatchAction* actions;
    size_t n_embeds;
    OidEmbed* embeds;
    char* type_name;
} PatchTransaction;

PatchTransaction transaction_patch_default ();
PatchAction action_patch_default ();

#endif
