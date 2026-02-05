#ifndef RADICLE_DOCUMENT_H
#define RADICLE_DOCUMENT_H

#include <key.h>
#include <util.h>
#include <id.h>

extern const size_t HEXSIZ;

typedef enum {
    VIS_PUBLIC,
    VIS_PRIVATE
} Visibility;

typedef struct {
    uint32_t version;
    StrJsonMap payload;
    size_t n_delegates;
    Pubkey* delegates;
    size_t threshold;
    Visibility visibility;
    size_t n_allow;
    Pubkey* allow;
} Document;

typedef struct {
    Oid oid;
    size_t n_bytes;
    uint8_t* bytes;
    char* sig;
} DocumentEncoding;

extern const uint32_t IDENTITY_VERSION;

Oid document_init (Document doc, git_repository* repo, Pubkey signer);

DocumentEncoding document_encode (Document doc);

DocumentEncoding document_sign (Document doc, Pubkey signer);

char* visibility_to_str (Visibility visibility);

#endif
