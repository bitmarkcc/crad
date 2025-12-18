#ifndef RADICLE_COB_IDENTITY_H
#define RADICLE_COB_IDENTITY_H

#include <stdint.h>

#include <id.h>
#include <git2.h>
#include <json-c/json.h>

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
    char* name;
    Oid content;
} OidEmbed;

typedef struct {
    char* type_name;
    uint32_t version;
} Manifest;

typedef enum {
    COB_IDENTITY,
    COB_ISSUE,
    COB_PATCH
} CobType;

typedef struct {
    char* type_name;
    uint32_t version;
    char* message;
    size_t n_embeds;
    OidEmbed* embeds;
    size_t n_contents;
    char** contents;
} Create;

char* manifest_encode (Manifest manifest);
json_object* get_identity_document (git_repository* repo);
Oid get_root_identity_doc_oid (git_repository* repo);

#endif
