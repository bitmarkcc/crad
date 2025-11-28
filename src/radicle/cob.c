#include <stdio.h>
#include <string.h>

#include <cob.h>
#include <cob/identity.h>
#include <document.h>

const uint32_t COB_VERSION = 1;

IdentityTransaction transaction_identity_default() {
    IdentityTransaction tx;
    tx.n_actions = 0;
    tx.actions = 0;
    tx.n_embeds = 0;
    tx.embeds = 0;
    RadRepo rrepo = rad_repo_default();
    tx.type_name = "xyz.radicle.id";
    return tx;
}

int transaction_identity_add_revision(IdentityTransaction* tx, char* title, char* desc, Document doc, Oid parent, RadRepo rrepo, Pubkey signer) {
    DocumentEncoding encoding = document_sign(doc,signer);
    Oid oid;
    git_odb* odb = 0;
    if (git_repository_odb(&odb,rrepo.repo)) {
	fprintf(stderr,"Failed to get repository odb\n");
	return 1;
    }
    if (git_odb_write(&oid,odb,encoding.bytes,encoding.n_bytes,GIT_OBJECT_BLOB)) {
	fprintf(stderr,"Failed to right to ODB\n");
	return 1;
    }
    OidEmbed embed;
    embed.name = "radicle.json";
    embed.content = oid;
    rad_push_array(&tx->n_embeds,(void**)&tx->embeds,sizeof(embed),&embed);
    IdentityAction action;
    action.type = IDENTITY_ACTION_REVISION;
    action.title = title;
    action.desc = desc;
    action.oid = oid;
    action.parent = parent;
    action.sig = encoding.sig;
    rad_push_array(&tx->n_actions,(void**)&tx->actions,sizeof(action),&action);
    if (!tx->actions)
	printf("add_revision: tx.actions null\n");
    tx->rrepo = rrepo;
    return 0;
}

RepoEntry cob_create (RadRepo rrepo, Pubkey signer, Oid resource, Oid* related, size_t n_related, Create args) {
    const char* type_name = args.type_name;
    int32_t version = args.version;
    RepoEntry re = rad_repo_store(rrepo,resource,related,n_related,signer,args);
    rad_repo_update(rrepo,signer,type_name,re.oid,re.oid);
    return re;
}

Oid* rad_get_parents (const IdentityAction* actions, size_t n) {
    Oid* parents = malloc(n*sizeof(Oid));
    for (int i=0; i<n; i++) {
	parents[i] = actions[i].parent;
    }
    return parents;
}

char** rad_actions_to_json_strings (const IdentityAction* actions, size_t n) {
    if (!n) return 0;
    char** jsons = malloc(n*sizeof(char*));

    const size_t HEXSIZ = GIT_OID_SHA1_HEXSIZE+1;
    char buf[HEXSIZ];
    
    for (int i=0; i<n; i++) {
	json_object* obj = json_object_new_object();
	IdentityAction action = actions[i];
	if (action.type == IDENTITY_ACTION_REVISION) {
	    json_object_object_add(obj,"blob",json_object_new_string(git_oid_tostr(buf,HEXSIZ,&action.oid)));
	    if (action.desc && strlen(action.desc))
		json_object_object_add(obj,"description",json_object_new_string(action.desc));
	    if (oid_is_null(action.parent))
		json_object_object_add(obj,"parent",0);
	    else
		json_object_object_add(obj,"parent",json_object_new_string(git_oid_tostr(buf,HEXSIZ,&action.parent)));
	    json_object_object_add(obj,"signature",json_object_new_string(action.sig));
	    json_object_object_add(obj,"title",json_object_new_string(action.title));
	} //todo cover all cases for the action.type
	jsons[i] = rad_remove_space_json(json_object_to_json_string(obj));
    }
    return jsons;    
}

RepoEntry create_cob_identity (RadRepo rrepo, char* message, IdentityAction* actions, size_t n_actions, OidEmbed* embeds, size_t n_embeds, Pubkey signer) {
    Oid* parents = rad_get_parents(actions,n_actions);
    size_t n_parents = n_actions;
    char** contents = rad_actions_to_json_strings(actions,n_actions);
    Create create;
    create.type_name = "xyz.radicle.id";
    create.version = COB_VERSION;
    create.message = message;
    create.n_embeds = n_embeds;
    create.embeds = embeds;
    create.n_contents = n_actions;
    create.contents = contents;

    Oid repo_oid = {{0}};
    
    RepoEntry re = cob_create(rrepo,signer,repo_oid,parents,n_parents,create);
    //return (cob.id, cob.object);
    return re;
}

RepoEntry transaction_identity_init(char* message, RadRepo rrepo, Pubkey signer, Document doc) {
    RepoEntry re;
    IdentityTransaction tx = transaction_identity_default();
    Oid oid = {{0}};
    transaction_identity_add_revision(&tx,"Initial revision","",doc,oid,rrepo,signer);
    re = create_cob_identity(rrepo,message,tx.actions,tx.n_actions,tx.embeds,tx.n_embeds,signer);
    return re;
}

RepoEntry cob_identity_init (Document doc, RadRepo rrepo, Pubkey signer) {
    //store = store::open(store);
    RepoEntry re = transaction_identity_init("Initialize identity",rrepo,signer,doc);
    // identity_mut(id,identity,stor);
    return re;
}
