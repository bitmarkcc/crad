#include <cob/issue.h>

IssueTransaction transaction_issue_default () {
    IssueTransaction tx;
    tx.n_actions = 0;
    tx.actions = 0;
    tx.n_embeds = 0;
    tx.embeds = 0;
    tx.type_name = "xyz.radicle.issue";
    return tx;
}

IssueAction action_issue_default () {
    IssueAction a;
    a.type = 0;
    a.title = 0;
    a.body = 0;
    Oid zero = {{0}};
    a.reply_to = zero;
    a.assignees = 0;
    return a;
}
