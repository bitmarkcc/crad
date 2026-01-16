#ifndef RADICLE_COMMANDS_ID_H
#define RADICLE_COMMANDS_ID_H

#include <command.h>
#include <id.h>
#include <set.h>
#include <document.h>

typedef struct {
    char err;
    char* title;
    char* desc;
    SimpleSet delegate;
    SimpleSet rescind;
    SimpleSet allow;
    SimpleSet disallow;
    size_t threshold;
    Visibility* visibility;
    StrJsonMap payload;
} IDCommand;

int id_run (Command c);
int id_update (char* title, char* desc, SimpleSet* delegate, SimpleSet* rescind, size_t threshold, Visibility* visibility, SimpleSet* allow, SimpleSet* disallow, StrJsonMap payload);

#endif
