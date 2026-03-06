#ifndef RADICLE_SOURCE_H
#define RADICLE_SOURCE_H

#include <command.h>

typedef struct {
    char err;
    char* rid;
    char* path;
} SourceCommand;

int source_run (Command c);

#endif
