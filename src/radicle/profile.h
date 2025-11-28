#ifndef RADICLE_PROFILE_H
#define RADICLE_PROFILE_H

#include <stdint.h>
#include <stdbool.h>
#include <libssh/libssh.h>

#include <key.h>
#include <storage.h>

char* get_rad_home ();

bool profile_load ();

bool profile_init (const char* alias, const char* passphrase, const uint8_t* seed);

Pubkey profile_get_pubkey ();

ssh_key profile_get_privkey ();

Storage profile_get_storage ();

#endif
