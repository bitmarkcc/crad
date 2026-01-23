#ifndef RADICLE_SYNC_H
#define RADICLE_SYNC_H

#include <command.h>

typedef struct {
    char err;
    char* seed;
} SyncCommand;

int sync_run (Command c);

#endif
