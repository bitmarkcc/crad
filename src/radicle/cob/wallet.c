#include <string.h>

#include <cob/wallet.h>

WalletTransaction transaction_wallet_default () {
    WalletTransaction tx;
    tx.n_actions = 0;
    tx.actions = 0;
    tx.n_embeds = 0;
    tx.embeds = 0;
    tx.type_name = "xyz.radicle.wallet";
    return tx;
}

WalletAction action_wallet_default () {
    WalletAction a;
    a.type = 0;
    a.currency = 0;
    a.address = 0;
    return a;
}
