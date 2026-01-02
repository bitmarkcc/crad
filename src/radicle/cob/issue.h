#ifndef RADICLE_COB_ISSUE_H
#define RADICLE_COB_ISSUE_H

#include <stdint.h>

#include <id.h>
#include <cob/common.h>

extern const uint32_t COB_VERSION;
extern const size_t HEXSIZ;

typedef enum {
    ISSUE_ACTION_ASSIGN,
    ISSUE_ACTION_EDIT,
    ISSUE_ACTION_LIFECYCLE,
    ISSUE_ACTION_LABEL,
    ISSUE_ACTION_COMMENT,
    ISSUE_ACTION_COMMENT_EDIT,
    ISSUE_ACTION_COMMENT_REDACT,
    ISSUE_ACTION_COMMENT_REACT
} IssueActionType;

typedef struct {
    IssueActionType type;
    char* title;
    char* body;
    Oid reply_to;
} IssueAction;

typedef struct {
    size_t n_actions;
    IssueAction* actions;
    size_t n_embeds;
    OidEmbed* embeds;
    char* type_name;
} IssueTransaction;

IssueTransaction transaction_issue_default ();

IssueAction action_issue_default ();

#endif
