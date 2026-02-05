#ifndef RADICLE_AUTH_H
#define RADICLE_AUTH_H

#include <command.h>

typedef struct {
    char err;
    char* alias;
    char* passphrase;
    bool deauth;
} AuthCommand;

int auth_run (Command c);
int auth_init();

#endif
