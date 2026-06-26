#ifndef RADICLE_VERSION_H
#define RADICLE_VERSION_H

#define NAME "C-Radicle"
#define RADICLE_VERSION "0.2"
#define GIT_HEAD "91421c29698a"
#define TIMESTAMP "1782508720"

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
