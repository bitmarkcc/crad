#include <json-c/json.h>

#include <cob/identity.h>
#include <util.h>

char* manifest_encode (Manifest manifest) {
    json_object* obj = json_object_new_object();

    json_object_object_add(obj,"type_name",json_object_new_string(manifest.type_name));
    json_object_object_add(obj,"version",json_object_new_int(manifest.version));
    
    return rad_remove_space_json(json_object_to_json_string(obj));
}
