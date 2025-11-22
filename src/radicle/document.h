#ifndef RADICLE_DOCUMENT_H
#define RADICLE_DOCUMENT_H

#include <key.h>
#include <util.h>
#include <project.h>
#include <id.h>

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
} DocumentEncoding;

extern const uint32_t IDENTITY_VERSION;

Document document_init (Project project, Pubkey delegate, Visibility visibility);

DocumentEncoding document_encode (Document doc);

#endif
