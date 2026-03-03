#include <string.h>

#include <cob/patch.h>

PatchTransaction transaction_patch_default () {
    PatchTransaction tx;
    tx.n_actions = 0;
    tx.actions = 0;
    tx.n_embeds = 0;
    tx.embeds = 0;
    tx.type_name = "xyz.radicle.patch";
    return tx;
}

PatchAction action_patch_default () {
    PatchAction a;
    a.type = PATCH_ACTION_REVISION;
    a.description = 0;
    Oid zero = {{0}};
    a.base = zero;
    a.oid = zero;
    a.title = 0;
    a.target = 0;
    a.assignees = 0;
    a.labels = 0;
    a.state = 0;
    return a;
}
