#ifndef RADICLE_PROJECT_H
#define RADICLE_PROJECT_H

#include <json-c/json.h>

typedef struct {
    char* name;
    char* description;
    char* default_branch;
} Project;

json_object* project_to_json_obj (Project project);

#endif
