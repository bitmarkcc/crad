#ifndef RADICLE_PRINT_H
#define RADICLE_PRINT_H

#include <stdbool.h>

extern bool verbose;

void eprintf (const char* str, ...);
void iprintf (const char* str, ...);
void dbprintf (const char* str, ...);

#endif
