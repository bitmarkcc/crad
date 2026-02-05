#ifndef RADICLE_INSPECT_H
#define RADICLE_INSPECT_H

#include <command.h>

typedef struct {
    char err;
    char* rid;
    bool get_rid;
    bool name;
    bool desc;
    bool default_branch;
    bool visibility;
    bool head;
    bool delegates;
    bool allowed;
    bool identity;
} InspectCommand;

int inspect_run (Command c);

#endif
