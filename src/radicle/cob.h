#ifndef RADICLE_COB_H
#define RADICLE_COB_H

#include <document.h>
#include <repo.h>

typedef struct {
    Oid id;
} Cob;

typedef struct {
    int ret;
    IdentityTransaction tx;
} IdentityTransactionResult;

RepoEntry cob_identity_init (Document doc, RadRepo rrepo, Pubkey signer);

RepoEntry cob_issue_init (RadRepo rrepo, Pubkey signer, char* title, char* desc);

#endif
