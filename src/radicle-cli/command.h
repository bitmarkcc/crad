#ifndef RADICLE_COMMAND_H
#define RADICLE_COMMAND_H

#include <stdlib.h>
#include <stdbool.h>

typedef enum {
    CMD_OTHER,
    CMD_HELP,
    CMD_VERSION
} CommandType;

typedef struct {
    CommandType type;
    bool json;
    char** argv;
    int argc;
} Command;

void free_command(Command* cmd);

#endif
