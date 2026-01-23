#include <stdio.h>
#include <string.h>
#include <libgen.h>

#include <cob.h>
#include <cob/identity.h>
#include <cob/issue.h>
#include <document.h>
#include <print.h>

const uint32_t COB_VERSION = 1;

char* cob_type_name (CobType type) {
    switch (type) {
    case COB_IDENTITY:
	return strdup("xyz.radicle.id");
    case COB_ISSUE:
	return strdup("xyz.radicle.issue");
    case COB_PATCH:
	return strdup("xyz.radicle.patch");
    default:
	return strdup("xyz.radicle.unknown");
    }
}

int transaction_identity_add_revision (IdentityTransaction* tx, char* title, char* desc, Document doc, Oid parent, git_repository* repo, Pubkey signer) {
    DocumentEncoding encoding = document_sign(doc,signer);
    Oid oid;
    git_odb* odb = 0;
    if (git_repository_odb(&odb,repo)) {
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

int transaction_issue_add_comment (IssueTransaction* tx, Oid reply_to, char* body) {
    IssueAction action = action_issue_default();
    action.type = ISSUE_ACTION_COMMENT;
    action.body = body;
    action.reply_to = reply_to;
    rad_push_array(&tx->n_actions,(void**)&tx->actions,sizeof(action),&action);
    return 0;
}

int transaction_issue_add_assign (IssueTransaction* tx, Oid issue_id, SimpleSet* assignees) {
    IssueAction action = action_issue_default();
    action.type = ISSUE_ACTION_ASSIGN;
    action.assignees = assignees;
    rad_push_array(&tx->n_actions,(void**)&tx->actions,sizeof(action),&action);
    return 0;
}

int transaction_issue_add_label (IssueTransaction* tx, Oid issue_id, SimpleSet* labels) {
    IssueAction action = action_issue_default();
    action.type = ISSUE_ACTION_LABEL;
    action.labels = labels;
    rad_push_array(&tx->n_actions,(void**)&tx->actions,sizeof(action),&action);
    return 0;
}

int transaction_issue_add_react (IssueTransaction* tx, Oid reply_to, char emoji [4]) {
    IssueAction action = action_issue_default();
    action.type = ISSUE_ACTION_COMMENT_REACT;
    memcpy(action.emoji,emoji,4);
    action.reply_to = reply_to;
    rad_push_array(&tx->n_actions,(void**)&tx->actions,sizeof(action),&action);
    return 0;
}

int transaction_issue_add_state (IssueTransaction* tx, IssueState state) {
    IssueAction action = action_issue_default();
    action.type = ISSUE_ACTION_LIFECYCLE;
    action.state = state;
    rad_push_array(&tx->n_actions,(void**)&tx->actions,sizeof(action),&action);
    return 0;
}

int transaction_issue_add_edit (IssueTransaction* tx, Oid issue_id, char* title, char* desc) {
    IssueAction action = action_issue_default();
    action.type = ISSUE_ACTION_EDIT;
    action.title = title;
    rad_push_array(&tx->n_actions,(void**)&tx->actions,sizeof(action),&action);
    action = action_issue_default();
    action.type = ISSUE_ACTION_COMMENT_EDIT;
    action.body = desc;
    action.id = issue_id;
    rad_push_array(&tx->n_actions,(void**)&tx->actions,sizeof(action),&action);
    return 0;
}

int transaction_issue_add_comment_edit (IssueTransaction* tx, Oid comment_id, char* body) {
    IssueAction action = action_issue_default();
    action.type = ISSUE_ACTION_COMMENT_EDIT;
    action.body = body;
    action.id = comment_id;
    rad_push_array(&tx->n_actions,(void**)&tx->actions,sizeof(action),&action);
    return 0;
}

RepoEntry cob_create (git_repository* repo, Pubkey signer, Oid resource, Oid* related, size_t n_related, Create args, Oid root_id) {
    Oid zero = {{0}};
    const char* type_name = args.type_name;
    RepoEntry re = rad_repo_store(repo,resource,related,n_related,signer,args);
    if (git_oid_is_zero(&re.oid)) {
	eprintf("failed to store to rad repo");
	return re;
    }
    if (git_oid_is_zero(&root_id)) root_id = re.oid;
    if (rad_repo_update(repo,signer,type_name,root_id,re.oid)) {
	eprintf("failed to update repo with new cob");
	re.oid = zero;
    }
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
	    //if (!git_oid_is_zero(action.embeds)) json_object_object_add();
	    if (!git_oid_is_zero(&action.reply_to)) json_object_object_add(obj,"reply_to",json_object_new_string(git_oid_tostr(buf,HEXSIZ,&action.reply_to)));
	    json_object_object_add(obj,"type",json_object_new_string("comment"));
	} //todo cover all cases for the action.type
	else if (action.type == ISSUE_ACTION_ASSIGN) {
	    json_object* assignees = json_object_new_array();
	    size_t n_assignees = 0;
	    char** assignees_list = set_to_array(action.assignees,&n_assignees);
	    for (size_t i=0; i<n_assignees; i++) {
		json_object_array_add(assignees,json_object_new_string(assignees_list[i]));
	    }
	    json_object_object_add(obj,"assignees",assignees);
	    json_object_object_add(obj,"type",json_object_new_string("assign"));
	}
	else if (action.type == ISSUE_ACTION_LABEL) {
	    json_object* labels = json_object_new_array();
	    size_t n_labels = 0;
	    char** labels_list = set_to_array(action.labels,&n_labels);
	    for (size_t i=0; i<n_labels; i++) {
		json_object_array_add(labels,json_object_new_string(labels_list[i]));
	    }
	    json_object_object_add(obj,"labels",labels);
	    json_object_object_add(obj,"type",json_object_new_string("label"));
	}
	else if (action.type == ISSUE_ACTION_COMMENT_REACT) {
	    json_object_object_add(obj,"active",json_object_new_boolean(true));
	    if (!git_oid_is_zero(&action.reply_to)) json_object_object_add(obj,"id",json_object_new_string(git_oid_tostr(buf,HEXSIZ,&action.reply_to)));
	    json_object_object_add(obj,"reaction",json_object_new_string(action.emoji));
	    json_object_object_add(obj,"type",json_object_new_string("comment.react"));
	}
	else if (action.type == ISSUE_ACTION_LIFECYCLE) {
	    json_object* obj_state = json_object_new_object();
	    json_object_object_add(obj_state,"reason",json_object_new_string(action.state.reason));
	    json_object_object_add(obj_state,"status",json_object_new_string(action.state.status));
	    json_object_object_add(obj,"state",obj_state);
	    json_object_object_add(obj,"type",json_object_new_string("lifecycle"));
	}
	else if (action.type == ISSUE_ACTION_COMMENT_EDIT) {
	    json_object_object_add(obj,"body",json_object_new_string(action.body));
	    json_object* embeds = json_object_new_array();
	    // todo handle embeds
	    json_object_object_add(obj,"embeds",embeds);
	    json_object_object_add(obj,"id",json_object_new_string(git_oid_tostr(buf,HEXSIZ,&action.id)));
	    json_object_object_add(obj,"type",json_object_new_string("comment.edit"));
	}
	jsons[i] = rad_remove_space_json(json_object_to_json_string(obj));
    }
    return jsons;    
}

RepoEntry create_cob_identity (git_repository* repo, char* message, IdentityAction* actions, size_t n_actions, OidEmbed* embeds, size_t n_embeds, Pubkey signer) {
    //Oid* parents = rad_get_parents(actions,n_actions); // todo: correct?
    //size_t n_parents = n_actions; // todo correct?
    Oid* parents = 0;
    size_t n_parents = 0;
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
    Oid root_id = {{0}};
    return cob_create(repo,signer,resource,parents,n_parents,create,root_id);
}

RepoEntry update_cob_identity (git_repository* repo, char* message, IdentityAction* actions, size_t n_actions, OidEmbed* embeds, size_t n_embeds, Pubkey signer, Oid root_oid) {
    char buf [HEXSIZ];
    Oid zero = {{0}};
    RepoEntry re;
    re.oid = zero;
    // get parent
    char refname [128];
    const char* did_raw = pubkey_to_did(signer.bytes)+8;
    Oid parent_oid = {{0}};
    sprintf(refname,"refs/namespaces/%s/refs/cobs/xyz.radicle.id/%s",did_raw,git_oid_tostr(buf,HEXSIZ,&root_oid));
    if (git_reference_name_to_id(&parent_oid,repo,refname)) {
	eprintf("failed to lookup oid from git reference name %s",refname);
	return re;
    }
    Oid parents [1] = {parent_oid};
    size_t n_parents = 1;
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
    return cob_create(repo,signer,resource,parents,n_parents,create,root_oid);
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
    Oid root_id = {{0}};
    return cob_create(rrepo.repo,signer,resource,parents,n_parents,create,root_id);
}

RepoEntry update_cob_issue (RadRepo rrepo, char* message, IssueAction* actions, size_t n_actions, OidEmbed* embeds, size_t n_embeds, Pubkey signer, Oid issue_id) {
    char buf [HEXSIZ];
    Oid zero = {{0}};
    RepoEntry re;
    re.oid = zero;
    // get first parent
    char refname [128];
    const char* did_raw = pubkey_to_did(signer.bytes)+8;
    Oid parent_oid = {{0}};
    sprintf(refname,"refs/namespaces/%s/refs/cobs/xyz.radicle.issue/%s",did_raw,git_oid_tostr(buf,HEXSIZ,&issue_id));
    if (git_reference_name_to_id(&parent_oid,rrepo.repo,refname)) {
	eprintf("failed to lookup oid from git reference name %s",refname);
	return re;
    }
    Oid parents [2] = {parent_oid,get_root_identity_commit_oid(rrepo.repo)};
    size_t n_parents = 2;
    char** contents = issue_actions_to_json_strings(actions,n_actions);
    Create create;
    create.type_name = "xyz.radicle.issue";
    create.version = COB_VERSION;
    create.message = message;
    create.n_embeds = n_embeds;
    create.embeds = embeds;
    create.n_contents = n_actions;
    create.contents = contents;
    Oid resource = parents[n_parents-1];
    return cob_create(rrepo.repo,signer,resource,parents,n_parents,create,issue_id);
}

RepoEntry transaction_identity_init (char* message, git_repository* repo, Pubkey signer, Document doc) {
    RepoEntry re;
    IdentityTransaction tx = transaction_identity_default();
    Oid oid = {{0}};
    transaction_identity_add_revision(&tx,"Initial revision","",doc,oid,repo,signer);
    return create_cob_identity(repo,message,tx.actions,tx.n_actions,tx.embeds,tx.n_embeds,signer);
}

RepoEntry transaction_identity_update (char* message, git_repository* repo, Pubkey signer, char* title, char* desc, SimpleSet* delegates, size_t threshold, Visibility visibility, SimpleSet* allowed, StrJsonMap payload) {
    Oid zero = {{0}};
    RepoEntry re;
    IdentityTransaction tx = transaction_identity_default();
    Oid parent_oid = get_identity_commit_oid(repo);
    Oid root_oid = get_root_identity_commit_oid(repo);
    Document doc;
    doc.version = IDENTITY_VERSION;
    doc.payload = payload;
    size_t n_delegates = 0;
    char** delegates_list = set_to_array(delegates,&n_delegates);
    doc.n_delegates = n_delegates;
    doc.delegates = malloc(n_delegates*sizeof(Pubkey));
    for (size_t i=0; i<n_delegates; i++) {
	Pubkey pubkey;
	pubkey.bytes = did_to_pubkey(delegates_list[i]);
	doc.delegates[i] = pubkey;
    }
    if (threshold > 1) { // todo handle greater thresholds
	eprintf("threshold greater than one currently not supported");
	re.oid = zero;
	return re;
    }
    else {
	doc.threshold = 1;
    }
    doc.visibility = visibility;
    size_t n_allowed = 0;
    char** allowed_list = set_to_array(allowed,&n_allowed);
    doc.n_allow = n_allowed;
    doc.allow = malloc(n_allowed*sizeof(Pubkey));
    for (size_t i=0; i<n_allowed; i++) {
	Pubkey pubkey;
	pubkey.bytes = did_to_pubkey(allowed_list[i]);
	doc.allow[i] = pubkey;
    }
    transaction_identity_add_revision(&tx,title,desc,doc,parent_oid,repo,signer);
    return update_cob_identity(repo,message,tx.actions,tx.n_actions,tx.embeds,tx.n_embeds,signer,root_oid);
}

RepoEntry transaction_issue_init (char* message, RadRepo rrepo, Pubkey signer, char* title, char* desc) {
    RepoEntry re;
    IssueTransaction tx = transaction_issue_default();
    Oid oid = {{0}};
    transaction_issue_add_new(&tx,title,desc);
    return create_cob_issue(rrepo,message,tx.actions,tx.n_actions,tx.embeds,tx.n_embeds,signer);
}

RepoEntry transaction_issue_comment (char* message, RadRepo rrepo, Pubkey signer, Oid issue_id, Oid reply_to, char* body) {
    RepoEntry re;
    IssueTransaction tx = transaction_issue_default();
    Oid oid = {{0}};
    transaction_issue_add_comment(&tx,reply_to,body);
    return update_cob_issue(rrepo,message,tx.actions,tx.n_actions,tx.embeds,tx.n_embeds,signer,issue_id);
}

RepoEntry transaction_issue_comment_edit (char* message, RadRepo rrepo, Pubkey signer, Oid issue_id, Oid edit, char* body) {
    RepoEntry re;
    IssueTransaction tx = transaction_issue_default();
    Oid oid = {{0}};
    transaction_issue_add_comment_edit(&tx,edit,body);
    return update_cob_issue(rrepo,message,tx.actions,tx.n_actions,tx.embeds,tx.n_embeds,signer,issue_id);
}

RepoEntry transaction_issue_assign (char* message, RadRepo rrepo, Pubkey signer, Oid issue_id, SimpleSet* assignees) {
    RepoEntry re;
    IssueTransaction tx = transaction_issue_default();
    Oid oid = {{0}};
    transaction_issue_add_assign(&tx,issue_id,assignees);
    return update_cob_issue(rrepo,message,tx.actions,tx.n_actions,tx.embeds,tx.n_embeds,signer,issue_id);
}

RepoEntry transaction_issue_label (char* message, RadRepo rrepo, Pubkey signer, Oid issue_id, SimpleSet* labels) {
    RepoEntry re;
    IssueTransaction tx = transaction_issue_default();
    Oid oid = {{0}};
    transaction_issue_add_label(&tx,issue_id,labels);
    return update_cob_issue(rrepo,message,tx.actions,tx.n_actions,tx.embeds,tx.n_embeds,signer,issue_id);
}

RepoEntry transaction_issue_react (char* message, RadRepo rrepo, Pubkey signer, Oid issue_id, Oid reply_to, char emoji [4]) {
    RepoEntry re;
    IssueTransaction tx = transaction_issue_default();
    Oid oid = {{0}};
    transaction_issue_add_react(&tx,reply_to,emoji);
    return update_cob_issue(rrepo,message,tx.actions,tx.n_actions,tx.embeds,tx.n_embeds,signer,issue_id);
}

RepoEntry transaction_issue_state (char* message, RadRepo rrepo, Pubkey signer, Oid issue_id, IssueState state) {
    RepoEntry re;
    IssueTransaction tx = transaction_issue_default();
    Oid oid = {{0}};
    transaction_issue_add_state(&tx,state);
    return update_cob_issue(rrepo,message,tx.actions,tx.n_actions,tx.embeds,tx.n_embeds,signer,issue_id);
}

RepoEntry transaction_issue_edit (char* message, RadRepo rrepo, Pubkey signer, Oid issue_id, char* title, char* desc) {
    RepoEntry re;
    IssueTransaction tx = transaction_issue_default();
    Oid oid = {{0}};
    transaction_issue_add_edit(&tx,issue_id,title,desc);
    return update_cob_issue(rrepo,message,tx.actions,tx.n_actions,tx.embeds,tx.n_embeds,signer,issue_id);
}

RepoEntry cob_identity_init (Document doc, git_repository* repo, Pubkey signer) {
    return transaction_identity_init("Initialize identity",repo,signer,doc);
}

RepoEntry cob_identity_update (RadRepo rrepo, Pubkey signer, char* title, char* desc, SimpleSet* delegates, size_t threshold, Visibility visibility, SimpleSet* allowed, StrJsonMap payload) {
    Oid zero = {{0}};
    RepoEntry re = transaction_identity_update("Propose revision",rrepo.repo,signer,title,desc,delegates,threshold,visibility,allowed,payload);
    if (git_oid_is_zero(&re.oid)) {
	eprintf("transaction to update identity document failed");
	return re;
    }
    Oid oid = rad_repo_sign_refs(rrepo,signer);
    if (git_oid_is_zero(&oid)) {
	re.oid = zero;
	return re;
    }
    if (create_sigrefs_commit(rrepo,signer,oid)) {
	eprintf("failed to create new sigrefs commit");
	re.oid = zero;
    }
    // set canonical rad/id reference
    git_reference* ref = 0;
    if (git_reference_create(&ref,rrepo.repo,"refs/rad/id",&re.oid,1,"set-local-branch (radicle)")) {
	eprintf("failed to set git reference");
	re.oid = zero;
	return re;
    }
    return re;
}

RepoEntry cob_issue_init (RadRepo rrepo, Pubkey signer, char* title, char* desc) {
    Oid zero = {{0}};
    RepoEntry re = transaction_issue_init("Create issue",rrepo,signer,title,desc);
    if (git_oid_is_zero(&re.oid)) {
	eprintf("transaction to open issue failed");
	return re;
    }
    Oid oid = rad_repo_sign_refs(rrepo,signer);
    if (git_oid_is_zero(&oid)) {
	eprintf("failed to sign refs");
	re.oid = zero;
	return re;
    }
    if (create_sigrefs_commit(rrepo,signer,oid)) {
	eprintf("failed to create new sigrefs commit");
	re.oid = zero;
    }
    return re;
}

RepoEntry cob_issue_comment (RadRepo rrepo, Pubkey signer, Oid issue_id, Oid reply_to, char* message, Oid edit) {
    Oid zero = {{0}};
    RepoEntry re;
    re.oid = zero;
    if (git_oid_is_zero(&edit))
	re = transaction_issue_comment("Comment",rrepo,signer,issue_id,reply_to,message);
    else
	re = transaction_issue_comment_edit("Edit comment",rrepo,signer,issue_id,edit,message);
    if (git_oid_is_zero(&re.oid)) {
	eprintf("transaction to comment on issue failed");
	return re;
    }
    Oid oid = rad_repo_sign_refs(rrepo,signer);
    if (git_oid_is_zero(&oid)) {
	re.oid = zero;
	return re;
    }
    if (create_sigrefs_commit(rrepo,signer,oid)) {
	eprintf("failed to create new sigrefs commit");
	re.oid = zero;
    }
    return re;
}

RepoEntry cob_issue_assign (RadRepo rrepo, Pubkey signer, Oid issue_id, SimpleSet* assignees) {
    Oid zero = {{0}};
    RepoEntry re = transaction_issue_assign("Assign",rrepo,signer,issue_id,assignees);
    if (git_oid_is_zero(&re.oid)) {
	eprintf("transaction to assign issue failed");
	return re;
    }
    Oid oid = rad_repo_sign_refs(rrepo,signer);
    if (git_oid_is_zero(&oid)) {
	re.oid = zero;
	return re;
    }
    if (create_sigrefs_commit(rrepo,signer,oid)) {
	eprintf("failed to create new sigrefs commit");
	re.oid = zero;
    }
    return re;
}

RepoEntry cob_issue_label (RadRepo rrepo, Pubkey signer, Oid issue_id, SimpleSet* labels) {
    Oid zero = {{0}};
    RepoEntry re = transaction_issue_label("Label",rrepo,signer,issue_id,labels);
    if (git_oid_is_zero(&re.oid)) {
	eprintf("transaction to label issue failed");
	return re;
    }
    Oid oid = rad_repo_sign_refs(rrepo,signer);
    if (git_oid_is_zero(&oid)) {
	re.oid = zero;
	return re;
    }
    if (create_sigrefs_commit(rrepo,signer,oid)) {
	eprintf("failed to create new sigrefs commit");
	re.oid = zero;
    }
    return re;
}

RepoEntry cob_issue_react (RadRepo rrepo, Pubkey signer, Oid issue_id, Oid reply_to, char emoji [4]) {
    Oid zero = {{0}};
    RepoEntry re = transaction_issue_react("React",rrepo,signer,issue_id,reply_to,emoji);
    if (git_oid_is_zero(&re.oid)) {
	eprintf("transaction to react on issue failed");
	return re;
    }
    Oid oid = rad_repo_sign_refs(rrepo,signer);
    if (git_oid_is_zero(&oid)) {
	re.oid = zero;
	return re;
    }
    if (create_sigrefs_commit(rrepo,signer,oid)) {
	eprintf("failed to create new sigrefs commit");
	re.oid = zero;
    }
    return re;
}

RepoEntry cob_issue_state (RadRepo rrepo, Pubkey signer, Oid issue_id, IssueState state) {
    Oid zero = {{0}};
    RepoEntry re = transaction_issue_state("Lifecycle",rrepo,signer,issue_id,state);
    if (git_oid_is_zero(&re.oid)) {
	eprintf("transaction to label issue failed");
	return re;
    }
    Oid oid = rad_repo_sign_refs(rrepo,signer);
    if (git_oid_is_zero(&oid)) {
	re.oid = zero;
	return re;
    }
    if (create_sigrefs_commit(rrepo,signer,oid)) {
	eprintf("failed to create new sigrefs commit");
	re.oid = zero;
    }
    return re;
}

int cob_issue_delete (RadRepo rrepo, Pubkey signer, Oid issue_id) {
    char refname [256];
    char buf [HEXSIZ];
    char* issue_id_str = strdup(git_oid_tostr(buf,HEXSIZ,&issue_id));
    sprintf(refname,"refs/namespaces/%s/refs/cobs/xyz.radicle.issue/%s",pubkey_to_did(signer.bytes)+8,issue_id_str);
    if (git_reference_remove(rrepo.repo,refname)) {
	eprintf("failed to remove git reference %s",refname);
    }
    Oid oid = rad_repo_sign_refs(rrepo,signer);
    if (git_oid_is_zero(&oid)) {
	eprintf("failed to sign refs");
	return 1;
    }
    if (create_sigrefs_commit(rrepo,signer,oid)) {
	eprintf("failed to create new sigrefs commit");
	return 1;
    }
    return 0;
}

RepoEntry cob_issue_edit (RadRepo rrepo, Pubkey signer, Oid issue_id, char* title, char* desc) {
    Oid zero = {{0}};
    RepoEntry re = transaction_issue_edit("Edit",rrepo,signer,issue_id,title,desc);
    if (git_oid_is_zero(&re.oid)) {
	eprintf("transaction to edit issue failed");
	return re;
    }
    Oid oid = rad_repo_sign_refs(rrepo,signer);
    if (git_oid_is_zero(&oid)) {
	re.oid = zero;
	return re;
    }
    if (create_sigrefs_commit(rrepo,signer,oid)) {
	eprintf("failed to create new sigrefs commit");
	re.oid = zero;
    }
    return re;
}

int get_cobs (SimpleSet* cobs, CobType type, RadRepo rrepo) {
    char buf [HEXSIZ];
    const char* type_name = cob_type_name(type);
    const char* glob = "refs/namespaces/*";
    git_reference_iterator* refit = 0;
    if (git_reference_iterator_glob_new(&refit,rrepo.repo,glob)) {
	eprintf("failed to create glob iterator");
	return 1;
    }
    const char* name = 0;
    int ret = 0;
    SimpleSet root_cobs;
    set_init(&root_cobs);
    SimpleSet cob_refs;
    set_init(&cob_refs);
    while (!(ret = git_reference_next_name(&name,refit))) {
	if (!strcmp(rad_basename_dirname(name),type_name)) {
	    set_add_str(&root_cobs,basename(strdup(name)));
	    set_add_str(&cob_refs,name);
	}
    }
    size_t n_root_cobs = 0;
    char** root_cobs_list = set_to_array(&root_cobs,&n_root_cobs);
    size_t n_cob_refs = 0;
    char** cob_refs_list = set_to_array(&cob_refs,&n_cob_refs);
    for (size_t i=0; i<n_root_cobs; i++) {
	Oid latest_entry = {{0}};
	uint64_t latest_entry_time = 0;
	for (size_t j=0; j<n_cob_refs; j++) {
	    if (!strcmp(root_cobs_list[i],basename(strdup(cob_refs_list[j])))) {
		Oid entry_oid = {{0}};
		if (git_reference_name_to_id(&entry_oid,rrepo.repo,cob_refs_list[j])) {
		    eprintf("failed to get oid from reference name: %s",cob_refs_list[j]);
		    return 1;
		}
		git_commit* commit = 0;
		if (git_commit_lookup(&commit,rrepo.repo,&entry_oid)) {
		    eprintf("failed to lookup git commit");
		    return 1;
		}
		git_time_t commit_time = git_commit_time(commit);
		if (commit_time > latest_entry_time) {
		    latest_entry_time = commit_time;
		    latest_entry = entry_oid;
		}
	    }
	}
	set_add_str(cobs,git_oid_tostr(buf,HEXSIZ,&latest_entry));
	
    }

    return 0;
}
