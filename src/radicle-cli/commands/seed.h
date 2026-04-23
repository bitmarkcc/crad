#ifndef RADICLE_SEED_H
#define RADICLE_SEED_H

#include <command.h>

typedef struct {
    char err;
    char* rid;
} SeedCommand;

int seed_run (Command c);

#endif
