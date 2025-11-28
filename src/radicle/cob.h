#ifndef RADICLE_COB_H
#define RADICLE_COB_H

#include <document.h>
#include <repo.h>

typedef struct {
    Oid id;
} Cob;

typedef struct {
    size_t n_actions;
    IdentityAction* actions;
    size_t n_embeds;
    OidEmbed* embeds;
    RadRepo rrepo;
    char* type_name;
} IdentityTransaction;

typedef struct {
    int ret;
    IdentityTransaction tx;
} IdentityTransactionResult;

RepoEntry cob_identity_init (Document doc, RadRepo rrepo, Pubkey signer);

#endif
