#ifndef RADICLE_COMMANDS_LS_H
#define RADICLE_COMMANDS_LS_H

#include <command.h>

typedef struct {
    char err;
    bool public;
    bool private;
    bool json;
    bool refresh;
    int limit;
    char* query;
} LsCommand;

int ls_run (Command c);

#endif
