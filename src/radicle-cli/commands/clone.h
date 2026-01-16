#ifndef RADICLE_CLONE_H
#define RADICLE_CLONE_H

#include <command.h>

typedef struct {
    char err;
    char* seed;
} CloneCommand;

int clone_run (Command c);

#endif
