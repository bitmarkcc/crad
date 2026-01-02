#include <stdio.h>
#include <string.h>

#include <cob.h>
#include <cob/identity.h>
#include <cob/issue.h>
#include <document.h>

const uint32_t COB_VERSION = 1;

int transaction_identity_add_revision (IdentityTransaction* tx, char* title, char* desc, Document doc, Oid parent, RadRepo rrepo, Pubkey signer) {
    DocumentEncoding encoding = document_sign(doc,signer);
    Oid oid;
    git_odb* odb = 0;
    if (git_repository_odb(&odb,rrepo.repo)) {
	fprintf(stderr,"Failed to get repository odb\n");
	return 1;
    }
    if (git_odb_write(&oid,odb,encoding.bytes,encoding.n_bytes,GIT_OBJECT_BLOB)) {
	fprintf(stderr,"Failed to write to ODB\n");
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
    return 0;
}

int transaction_issue_add_new (IssueTransaction* tx, char* title, char* desc) {
    IssueAction action = action_issue_default();
    action.type = ISSUE_ACTION_COMMENT;
    action.body = desc;
    rad_push_array(&tx->n_actions,(void**)&tx->actions,sizeof(action),&action);
    action = action_issue_default();
    action.type = ISSUE_ACTION_EDIT;
    action.title = title;
    rad_push_array(&tx->n_actions,(void**)&tx->actions,sizeof(action),&action);
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

char** identity_actions_to_json_strings (const IdentityAction* actions, size_t n) {
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
	    json_object_object_add(obj,"type",json_object_new_string("revision"));
	} //todo cover all cases for the action.type
	jsons[i] = rad_remove_space_json(json_object_to_json_string(obj));
    }
    return jsons;    
}

char** issue_actions_to_json_strings (const IssueAction* actions, size_t n) {
    if (!n) return 0;
    char** jsons = malloc(n*sizeof(char*));
    const size_t HEXSIZ = GIT_OID_SHA1_HEXSIZE+1;
    char buf[HEXSIZ];
    for (int i=0; i<n; i++) {
	json_object* obj = json_object_new_object();
	IssueAction action = actions[i];
	if (action.type == ISSUE_ACTION_EDIT) {
	    json_object_object_add(obj,"title",json_object_new_string(action.title));
	    json_object_object_add(obj,"type",json_object_new_string("edit"));
	}
	else if (action.type == ISSUE_ACTION_COMMENT) { // todo implement embeds
	    json_object_object_add(obj,"body",json_object_new_string(action.body));
	    //if (!git_oid_is_zero(action.embeds)) json_object_object_add(obj,"reply_to",json_object_new_string(git_oid_tostr(buf,HEXSIZ,&action.embeds)));
	    if (!git_oid_is_zero(&action.reply_to)) json_object_object_add(obj,"reply_to",json_object_new_string(git_oid_tostr(buf,HEXSIZ,&action.reply_to)));
	    json_object_object_add(obj,"type",json_object_new_string("comment"));
	} //todo cover all cases for the action.type
	jsons[i] = rad_remove_space_json(json_object_to_json_string(obj));
    }
    return jsons;    
}

RepoEntry create_cob_identity (RadRepo rrepo, char* message, IdentityAction* actions, size_t n_actions, OidEmbed* embeds, size_t n_embeds, Pubkey signer) {
    Oid* parents = rad_get_parents(actions,n_actions);
    size_t n_parents = n_actions; // todo correct?
    char** contents = identity_actions_to_json_strings(actions,n_actions);
    Create create;
    create.type_name = "xyz.radicle.id";
    create.version = COB_VERSION;
    create.message = message;
    create.n_embeds = n_embeds;
    create.embeds = embeds;
    create.n_contents = n_actions;
    create.contents = contents;
    Oid resource = {{0}};
    return cob_create(rrepo,signer,resource,parents,n_parents,create);
}

RepoEntry create_cob_issue (RadRepo rrepo, char* message, IssueAction* actions, size_t n_actions, OidEmbed* embeds, size_t n_embeds, Pubkey signer) {
    Oid parents [1] = {get_root_identity_commit_oid(rrepo.repo)};
    size_t n_parents = 1;
    char** contents = issue_actions_to_json_strings(actions,n_actions);
    Create create;
    create.type_name = "xyz.radicle.issue";
    create.version = COB_VERSION;
    create.message = message;
    create.n_embeds = n_embeds;
    create.embeds = embeds;
    create.n_contents = n_actions;
    create.contents = contents;
    Oid resource = parents[0];
    return cob_create(rrepo,signer,resource,parents,n_parents,create);
}

RepoEntry transaction_identity_init (char* message, RadRepo rrepo, Pubkey signer, Document doc) {
    RepoEntry re;
    IdentityTransaction tx = transaction_identity_default();
    Oid oid = {{0}};
    transaction_identity_add_revision(&tx,"Initial revision","",doc,oid,rrepo,signer);
    return create_cob_identity(rrepo,message,tx.actions,tx.n_actions,tx.embeds,tx.n_embeds,signer);
}

RepoEntry transaction_issue_init (char* message, RadRepo rrepo, Pubkey signer, char* title, char* desc) {
    RepoEntry re;
    IssueTransaction tx = transaction_issue_default();
    Oid oid = {{0}};
    transaction_issue_add_new(&tx,title,desc);
    return create_cob_issue(rrepo,message,tx.actions,tx.n_actions,tx.embeds,tx.n_embeds,signer);
}

RepoEntry cob_identity_init (Document doc, RadRepo rrepo, Pubkey signer) {
    return transaction_identity_init("Initialize identity",rrepo,signer,doc);
}

RepoEntry cob_issue_init (RadRepo rrepo, Pubkey signer, char* title, char* desc) {
    return transaction_issue_init("Create issue",rrepo,signer,title,desc);
}
