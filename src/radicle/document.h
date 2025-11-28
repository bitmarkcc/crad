#ifndef RADICLE_DOCUMENT_H
#define RADICLE_DOCUMENT_H

#include <key.h>
#include <util.h>
#include <project.h>
#include <id.h>
#include <repo.h>

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
    bool visibility;
} Document;

typedef struct {
    Oid oid;
    size_t n_bytes;
    uint8_t* bytes;
    char* sig;
} DocumentEncoding;

extern const uint32_t IDENTITY_VERSION;

Oid document_init (Document doc, RadRepo rrepo, Pubkey signer);

DocumentEncoding document_encode (Document doc);

DocumentEncoding document_sign (Document doc, Pubkey signer);

#endif
