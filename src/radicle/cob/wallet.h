#ifndef RADICLE_COB_WALLET_H
#define RADICLE_COB_WALLET_H

#include <stdint.h>

#include <id.h>
#include <cob/common.h>
#include <set.h>

extern const uint32_t COB_VERSION;
extern const size_t HEXSIZ;

typedef enum {
    WALLET_ACTION_ADD,
    WALLET_ACTION_REMOVE
} WalletActionType;

typedef struct {
    WalletActionType type;
    char* currency;
    char* address;
} WalletAction;

typedef struct {
    size_t n_actions;
    WalletAction* actions;
    size_t n_embeds;
    OidEmbed* embeds;
    char* type_name;
} WalletTransaction;

WalletTransaction transaction_wallet_default ();
WalletAction action_wallet_default ();

#endif
