#ifndef RADICLE_ISSUE_H
#define RADICLE_ISSUE_H

#include <command.h>
#include <id.h>
#include <set.h>
#include <cob/issue.h>

typedef struct {
    char err;
    Oid issue_id;
    size_t issue_id_hexlen;
    char* title;
    char* desc;
    char* message;
    Oid reply_to;
    size_t reply_to_hexlen;
    Oid edit;
    size_t edit_hexlen;
    SimpleSet add;
    SimpleSet delete;
    char emoji [4];
    IssueState state;
    SimpleSet assigned;
    char* rid;
    bool json;
} IssueCommand;

int issue_run (Command c);
int issue_open (char* title, char* desc, const char* rid);
int issue_comment (Oid issue_id, size_t issue_id_hexlen, Oid reply_to, size_t reply_to_hexlen, char* message, Oid edit, size_t edit_hexlen, const char* rid);
int issue_assign (Oid issue_id, size_t issue_id_hexlen, SimpleSet* add, SimpleSet* delete, const char* rid);
int issue_label (Oid issue_id, size_t issue_id_hexlen, SimpleSet* add, SimpleSet* delete, const char* rid);
int issue_react (Oid issue_id, size_t issue_id_hexlen, Oid reply_to, size_t reply_to_hexlen, char emoji [4]);
int issue_state (Oid issue_id, size_t issue_id_hexlen, IssueState state);
int issue_delete (Oid issue_id, size_t issue_id_hexlen);
int issue_edit (Oid issue_id, size_t issue_id_hexlen, char* title, char* desc);
int issue_list (SimpleSet* assigned, IssueState state, const char* rid);
int issue_show (Oid issue_id, size_t issue_id_hexlen, const char* rid, bool json);
bool issue_entry_in_cob_db (Oid issue_entry);

#endif
