#ifndef RADICLE_ISSUE_H
#define RADICLE_ISSUE_H

#include <command.h>
#include <id.h>

typedef struct {
    char err;
    Oid issue_id;
    size_t issue_id_hexlen;
    char* title;
    char* desc;
    char* message;
    Oid reply_to;
    size_t reply_to_hexlen;
} IssueCommand;

int issue_run (Command c);
int issue_open (char* title, char* desc);
int issue_comment (Oid issue_id, size_t issue_id_hexlen, Oid reply_to, size_t reply_to_hexlen, char* message);

#endif
