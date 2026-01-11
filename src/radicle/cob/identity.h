#ifndef RADICLE_COB_IDENTITY_H
#define RADICLE_COB_IDENTITY_H

#include <stdint.h>
#include <git2.h>
#include <json-c/json.h>

#include <id.h>
#include <cob/common.h>
#include <set.h>
#include <document.h>

extern const uint32_t COB_VERSION;
extern const size_t HEXSIZ;

typedef enum {
    IDENTITY_ACTION_REVISION,
    IDENTITY_ACTION_REVISION_EDIT,
    IDENTITY_ACTION_REVISION_ACCEPT,
    IDENTITY_ACTION_REVISION_REJECT,
    IDENTITY_ACTION_REVISION_REDACT
} IdentityActionType;

typedef struct {
    IdentityActionType type;
    char* title;
    char* desc;
    Oid oid;
    Oid parent;
    uint8_t* sig;
} IdentityAction;

typedef struct {
    size_t n_actions;
    IdentityAction* actions;
    size_t n_embeds;
    OidEmbed* embeds;
    char* type_name;
} IdentityTransaction;

IdentityTransaction transaction_identity_default ();
char* manifest_encode (Manifest manifest);
json_object* get_identity_document (git_repository* repo);
int get_entities_from_identity_doc (SimpleSet* delegates, SimpleSet* allowed, StrJsonMap* payload, Visibility* visibility, git_repository* repo);
Oid get_root_identity_doc_oid (git_repository* repo);
Oid get_identity_commit_oid (git_repository* repo);
Oid get_root_identity_commit_oid (git_repository* repo);

#endif
