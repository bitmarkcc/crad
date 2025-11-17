#ifndef RADICLE_PROFILE_H
#define RADICLE_PROFILE_H

#include <stdint.h>

bool profile_load();

bool profile_init(const char* alias, const char* passphrase, const uint8_t* seed);

#endif
