#ifndef RADICLE_COB_H
#define RADICLE_COB_H

#include <document.h>
#include <repo.h>
#include <cob/issue.h>

char* cob_type_name (CobType type);
RepoEntry cob_identity_init (Document doc, git_repository* repo, Pubkey signer);
RepoEntry cob_identity_update (RadRepo rrepo, Pubkey signer, char* title, char* desc, SimpleSet* delegates, size_t threshold, Visibility visibility, SimpleSet* allowed, StrJsonMap payload);
RepoEntry cob_issue_init (RadRepo rrepo, Pubkey signer, char* title, char* desc);
RepoEntry cob_issue_comment (RadRepo rrepo, Pubkey signer, Oid issue_id, Oid reply_to, char* message, Oid edit);
RepoEntry cob_issue_assign (RadRepo rrepo, Pubkey signer, Oid issue_id, SimpleSet* assignees);
RepoEntry cob_issue_label (RadRepo rrepo, Pubkey signer, Oid issue_id, SimpleSet* labels);
RepoEntry cob_issue_react (RadRepo rrepo, Pubkey signer, Oid issue_id, Oid reply_to, char emoji [4]);
RepoEntry cob_issue_state (RadRepo rrepo, Pubkey signer, Oid issue_id, IssueState state);
int cob_issue_delete (RadRepo rrepo, Pubkey signer, Oid issue_id);
RepoEntry cob_issue_edit (RadRepo rrepo, Pubkey signer, Oid issue_id, char* title, char* desc);
int get_cobs (SimpleSet* cobs, CobType type, RadRepo rrepo);

#endif
