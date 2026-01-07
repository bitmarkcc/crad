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

RepoEntry cob_issue_comment (RadRepo rrepo, Pubkey signer, Oid issue_id, Oid reply_to, char* message);

RepoEntry cob_issue_assign (RadRepo rrepo, Pubkey signer, Oid issue_id, SimpleSet* assignees);

RepoEntry cob_issue_label (RadRepo rrepo, Pubkey signer, Oid issue_id, SimpleSet* labels);

#endif
