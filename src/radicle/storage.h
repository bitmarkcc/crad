#ifndef RADICLE_STORAGE_H
#define RADICLE_STORAGE_H

typedef struct {
    char* name;
    char* email;
} StorageInfo;

typedef struct {
    char* path;
    StorageInfo info;
} Storage;

StorageInfo rad_storage_info (Storage s);

#endif
