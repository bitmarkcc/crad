#include <string.h>
#include <stdio.h>
#include <git2.h>

#include <document.h>
#include <id.h>
#include <cob.h>
#include <key.h>

const uint32_t IDENTITY_VERSION = 1;

Oid document_init (const Document doc, const RadRepo rrepo, const Pubkey signer) {
    Oid commit;

    RepoEntry re = cob_identity_init(doc,rrepo,signer);
    // Create symbolic link ../refs/rad/id -> ../refs/cobs/xyz.radicle.id/<id>
    git_reference* ref = 0;
    const char* refname = "refs/rad/id";
    char reftarget [256];
    char reflogmsg [256];
    const size_t HEXSIZ = GIT_OID_SHA1_HEXSIZE+1;
    char buf[HEXSIZ];
    char* cob_id_str = strdup(git_oid_tostr(buf,HEXSIZ,&re.oid));
    strcpy(reftarget,"refs/namespaces/");
    strcat(reftarget,pubkey_to_did(signer.bytes)+8);
    strcat(reftarget,"/xyz.radicle.id/");
    strcat(reftarget,cob_id_str);
    strcpy(reflogmsg,"Create `rad/id` reference to point to new identity COB");
    
    if (git_reference_symbolic_create(&ref,rrepo.repo,refname,reftarget,0,reflogmsg)) {
	fprintf(stderr,"Failed to create symbolic git reference\n");
	return commit;
    }
    return commit;
}

DocumentEncoding document_encode (Document doc) {
    DocumentEncoding encoding;
    encoding.sig = 0;
    
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

DocumentEncoding document_sign (Document doc, Pubkey signer) {
    DocumentEncoding encoding = document_encode(doc);
    char* sig_full = 0;
    key_sign(&encoding.sig,&sig_full,signer,encoding.oid.id,20);
    return encoding;
}
