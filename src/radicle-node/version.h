#ifndef RADICLE_VERSION_H
#define RADICLE_VERSION_H

#define NAME "C-Radicle"
#define RADICLE_VERSION "0.1"
#define GIT_HEAD "deadbeef1234"
#define TIMESTAMP "1763053651"

typedef struct {
    const char* name;
    const char* version;
    const char* commit;
    const char* timestamp;
} Version;

const Version VERSION = {
    NAME,
    RADICLE_VERSION,
    GIT_HEAD,
    TIMESTAMP
};

#endif
