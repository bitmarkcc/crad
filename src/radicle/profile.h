#ifndef RADICLE_PROFILE_H
#define RADICLE_PROFILE_H

#include <stdint.h>
#include <stdbool.h>
#include <libssh/libssh.h>

#include <key.h>
#include <storage.h>

char* get_rad_home ();

char* get_rad_node_home ();

char* get_cob_cache_file ();

bool profile_load ();

bool profile_init (const char* alias, const char* passphrase, const uint8_t* seed);

Pubkey profile_get_pubkey ();

Pubkey profile_get_pubkey_from_privkey ();

int profile_get_privkey (ssh_key* key);

Storage profile_get_storage ();

bool password_loaded ();

#endif
