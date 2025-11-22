#include <project.h>
#include <stdio.h>

json_object* project_to_json_obj (Project project) {
    json_object* obj = json_object_new_object();
    json_object_object_add(obj,"defaultBranch",json_object_new_string(project.default_branch));
    json_object_object_add(obj,"description",json_object_new_string(project.description));
    json_object_object_add(obj,"name",json_object_new_string(project.name));
    if (!obj) fprintf(stderr,"Can't create json object\n");
    return obj;
}
