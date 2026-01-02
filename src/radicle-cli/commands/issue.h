#ifndef RADICLE_ISSUE_H
#define RADICLE_ISSUE_H

#include <command.h>

typedef struct {
    char* title;
    char* desc;
} IssueCommand;

int issue_run (Command c);
int issue_open ();

#endif
