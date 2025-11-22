#include <string.h>
#include <stdio.h>
#include <git2.h>

#include <document.h>
#include <id.h>

const uint32_t IDENTITY_VERSION = 1;

Document document_init (Project project, Pubkey delegate, Visibility visibility) {
    Document doc;
    return doc;
}

DocumentEncoding document_encode (Document doc) {
    DocumentEncoding encoding;
    
    json_object* obj = json_object_new_object();

    json_object* delegates = json_object_new_array();
    json_object_array_add(delegates,json_object_new_string(pubkey_to_did(doc.delegates[0].bytes)));
    json_object_object_add(obj,"delegates",delegates);

    StrJsonMap payload = doc.payload;
    json_object* payload_obj = json_object_new_object();
    json_object_object_add(payload_obj,payload.keys[0],payload.values[0]);
    json_object_object_add(obj,"payload",payload_obj);

    json_object_object_add(obj,"threshold",json_object_new_int(1));
    
    if (IDENTITY_VERSION > 1) {
	json_object_object_add(obj,"version",json_object_new_int(IDENTITY_VERSION));
    }

    char* obj_str = rad_remove_space_json(json_object_to_json_string(obj));
    
    encoding.n_bytes = strlen(obj_str);
    encoding.bytes = (uint8_t*)obj_str;
    git_odb_hash(&encoding.oid,encoding.bytes,encoding.n_bytes,GIT_OBJECT_BLOB);
    
    return encoding;
}
