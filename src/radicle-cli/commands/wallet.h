#ifndef RADICLE_WALLET_H
#define RADICLE_WALLET_H

#include <command.h>
#include <id.h>
#include <cob/wallet.h>

typedef struct {
    char err;
    char* currency;
    char* address;
    char* rid;
    bool json;
} WalletCommand;

int wallet_run (Command c);
int wallet_add (char* currency, char* address, const char* rid);
int wallet_remove (char* currency, const char* rid);
int wallet_list (const char* rid, bool json);

#endif
