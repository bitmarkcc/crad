#ifndef RADICLE_ISSUE_H
#define RADICLE_ISSUE_H

#include <command.h>
#include <id.h>
#include <set.h>

typedef struct {
    char err;
    Oid issue_id;
    size_t issue_id_hexlen;
    char* title;
    char* desc;
    char* message;
    Oid reply_to;
    size_t reply_to_hexlen;
    SimpleSet add;
    SimpleSet delete;
} IssueCommand;

int issue_run (Command c);
int issue_open (char* title, char* desc);
int issue_comment (Oid issue_id, size_t issue_id_hexlen, Oid reply_to, size_t reply_to_hexlen, char* message);
int issue_assign (Oid issue_id, size_t issue_id_hexlen, SimpleSet* add, SimpleSet* delete);
int issue_label (Oid issue_id, size_t issue_id_hexlen, SimpleSet* add, SimpleSet* delete);

#endif
