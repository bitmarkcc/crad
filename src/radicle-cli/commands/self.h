#ifndef RADICLE_SELF_H
#define RADICLE_SELF_H

#include <command.h>

typedef struct {
    char err;
    bool did;
    bool home;
    bool authed;
    bool alias;
    bool node_home;
} SelfCommand;

int self_run (Command c);

#endif
