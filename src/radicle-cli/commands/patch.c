#include <string.h>
#include <stdio.h>
#include <time.h>

#include <commands/patch.h>
#include <profile.h>
#include <print.h>
#include <git.h>
#include <cob.h>
#include <rad.h>

typedef struct {
    Oid patch_id;
    char* title;
    char* description;
    Oid base;
    Oid head;
    char* author;
    char* alias;
    int64_t timestamp;
} PatchInfo;

// Parse actions from a single COB entry commit tree
static void parse_patch_tree (PatchInfo* info, git_repository* repo, git_commit* commit) {
    git_tree* tree = 0;
    if (git_commit_tree(&tree,commit)) return;
    for (size_t i=0; i<16; i++) {
	char i_str [3];
	sprintf(i_str,"%lu",i);
	git_tree_entry* tree_entry = 0;
	if (git_tree_entry_bypath(&tree_entry,tree,i_str))
	    break;
	const Oid* poid = git_tree_entry_id(tree_entry);
	if (!poid) break;
	git_blob* blob = 0;
	if (git_blob_lookup(&blob,repo,poid)) break;
	const char* blob_content = (const char*)git_blob_rawcontent(blob);
	json_object* content = json_tokener_parse(blob_content);
	if (!content) continue;
	json_object* val_type = 0;
	json_object_object_get_ex(content,"type",&val_type);
	const char* type = json_object_get_string(val_type);
	if (!type) continue;
	if (!strcmp(type,"revision")) {
	    json_object* val_base = 0;
	    json_object_object_get_ex(content,"base",&val_base);
	    if (val_base)
		git_oid_fromstr(&info->base,json_object_get_string(val_base));
	    json_object* val_oid = 0;
	    json_object_object_get_ex(content,"oid",&val_oid);
	    if (val_oid)
		git_oid_fromstr(&info->head,json_object_get_string(val_oid));
	    json_object* val_desc = 0;
	    json_object_object_get_ex(content,"description",&val_desc);
	    if (info->description) free(info->description);
	    info->description = strdup(val_desc ? json_object_get_string(val_desc) : "");
	}
	else if (!strcmp(type,"edit")) {
	    json_object* val_title = 0;
	    json_object_object_get_ex(content,"title",&val_title);
	    if (info->title) free(info->title);
	    info->title = strdup(val_title ? json_object_get_string(val_title) : "Untitled");
	}
    }
}

// Walk the COB commit chain from latest entry back to root.
// Latest entry provides head/base (revision), root entry provides title (edit).
// The root entry OID (commit with < 2 parents) is the stable patch ID.
static int parse_patch_entry (PatchInfo* info, git_repository* repo, Oid entry_oid) {
    // Collect all entries in the chain (latest first)
    size_t entries_capacity = 8;
    Oid* entries = malloc(entries_capacity*sizeof(Oid));
    size_t n_entries = 0;
    Oid cur = entry_oid;

    while (1) {
	if (n_entries >= entries_capacity) {
	    entries_capacity *= 2;
	    entries = realloc(entries,entries_capacity*sizeof(Oid));
	}
	entries[n_entries++] = cur;
	git_commit* commit = 0;
	if (git_commit_lookup(&commit,repo,&cur)) {
	    eprintf("failed to lookup patch commit");
	    free(entries);
	    return 1;
	}
	unsigned int parent_count = git_commit_parentcount(commit);
	if (parent_count < 2) {
	    // This is the root entry (create commit)
	    info->patch_id = cur;
	    break;
	}
	// First parent is the previous entry in the COB chain
	const Oid* parent = git_commit_parent_id(commit,0);
	if (!parent) break;
	cur = *parent;
    }

    // Apply entries from oldest to newest so latest revision overwrites earlier ones
    for (int i=n_entries-1; i>=0; i--) {
	git_commit* commit = 0;
	if (git_commit_lookup(&commit,repo,&entries[i])) {
	    eprintf("failed to lookup patch commit");
	    free(entries);
	    return 1;
	}
	// Get author and timestamp from the root (create) entry
	if (i == (int)n_entries-1) {
	    info->timestamp = git_commit_time(commit);
	    const git_signature* author = git_commit_author(commit);
	    if (author && author->email) {
		info->alias = strdup(rad_email_get_user(author->email));
		char did [128];
		sprintf(did,"did:key:%s",rad_email_get_domain(author->email));
		info->author = strdup(did);
	    } else {
		info->alias = strdup("unknown");
		info->author = strdup("unknown");
	    }
	}
	parse_patch_tree(info,repo,commit);
    }

    free(entries);
    if (!info->title) info->title = strdup("Untitled");
    if (!info->description) info->description = strdup("");
    return 0;
}

PatchCommand command_patch_default () {
    PatchCommand cmd;
    cmd.err = 0;
    Oid zero = {{0}};
    cmd.patch_id = zero;
    cmd.patch_id_hexlen = 0;
    cmd.rid = 0;
    cmd.json = false;
    return cmd;
}

void print_help_patch () {
    printf("crad patch (Manage patches) Usage:\n");
    printf("crad patch list [-R <rid>]\n");
    printf("crad patch show <patch-id> [-R <rid>] [--json]\n");
}

PatchCommand parse_args_patch (int argc, char** argv) {
    PatchCommand cmd = command_patch_default();
    for (int i=0; i<argc; i++) {
	if (!strcmp(argv[i],"-R")) {
	    if (i+1 >= argc) {
		eprintf("no parameter to -R");
		cmd.err = 1;
		return cmd;
	    }
	    cmd.rid = strdup(argv[i+1]);
	    i++;
	}
	else if (!strcmp(argv[i],"--json")) {
	    cmd.json = true;
	}
    }
    return cmd;
}

static int open_rrepo (RadRepo* rrepo, const char* rid) {
    rad_git_init();
    *rrepo = rad_repo_default();
    if (rid) {
	rrepo->rid = rid_to_oid(rid);
	git_repository* repo = 0;
	const char* rad_home = get_rad_home();
	char* path = malloc(strlen(rad_home)+64);
	sprintf(path,"%s/storage/%s",rad_home,rid);
	if (git_repository_open(&repo,path)) {
	    eprintf("failed to open git repository at path %s",path);
	    return 1;
	}
	rrepo->repo = repo;
    }
    else if (get_rad_repo_from_cwd(rrepo)) {
	eprintf("failed to get rad repo from cwd");
	return 1;
    }
    return 0;
}

int patch_list (const char* rid) {
    char buf [HEXSIZ];
    RadRepo rrepo;
    if (open_rrepo(&rrepo,rid)) return 1;

    SimpleSet patches;
    set_init(&patches);
    if (get_cobs(&patches,COB_PATCH,rrepo)) {
	eprintf("failed to get list of patches");
	return 1;
    }
    size_t n_patches = 0;
    char** patches_list = set_to_array(&patches,&n_patches);
    if (n_patches)
	printf("ID------Title-------------------Author-------Head----Opened\n");
    for (size_t i=0; i<n_patches; i++) {
	Oid patch_entry = {{0}};
	if (git_oid_fromstr(&patch_entry,patches_list[i])) {
	    eprintf("failed to get Oid from hex string");
	    return 1;
	}
	if (git_oid_is_zero(&patch_entry)) {
	    eprintf("invalid patch entry");
	    return 1;
	}
	PatchInfo info = {0};
	if (parse_patch_entry(&info,rrepo.repo,patch_entry)) {
	    eprintf("failed to parse patch entry");
	    return 1;
	}
	// ID (7 chars, root patch ID)
	char* patch_id_str = git_oid_tostr(buf,HEXSIZ,&info.patch_id);
	char id [8];
	memcpy(id,patch_id_str,7);
	id[7] = 0;
	printf("%s ",id);

	// Title (20 chars)
	char* title = info.title;
	if (strlen(title) > 23)
	    title[23] = 0;
	rad_replace(title,' ','_');
	printf("%s",title);
	size_t title_len = strlen(title);
	for (size_t j=0; j<24-title_len; j++)
	    printf(" ");

	// Author (12 chars, DID suffix)
	char* author = info.author+8; // skip "did:key:"
	if (strlen(author) > 12)
	    author[12] = 0;
	printf("%s ",author);

	// Head (7 chars)
	char head_str [8];
	git_oid_tostr(head_str,8,&info.head);
	printf("%s ",head_str);

	// Opened
	time_t time_opened = info.timestamp;
	char* time_str = ctime(&time_opened);
	if (time_str)
	    rad_replace(time_str,' ','_');
	printf("%s",time_str ? time_str : "unknown\n");
    }
    return 0;
}

// Find the latest COB entry for a given root patch ID
static int find_latest_patch_entry (Oid* latest, git_repository* repo, Oid patch_id) {
    char buf [HEXSIZ];
    char glob [128];
    sprintf(glob,"refs/namespaces/*/refs/cobs/xyz.radicle.patch/%s",git_oid_tostr(buf,HEXSIZ,&patch_id));
    git_reference_iterator* refit = 0;
    if (git_reference_iterator_glob_new(&refit,repo,glob)) {
	eprintf("failed to create glob iterator for patch");
	return 1;
    }
    const char* refname = 0;
    int ret = 0;
    uint64_t latest_time = 0;
    while (!(ret = git_reference_next_name(&refname,refit))) {
	Oid entry_id = {{0}};
	if (git_reference_name_to_id(&entry_id,repo,refname)) continue;
	git_commit* commit = 0;
	if (git_commit_lookup(&commit,repo,&entry_id)) continue;
	git_time_t t = git_commit_time(commit);
	if (t > latest_time) {
	    latest_time = t;
	    *latest = entry_id;
	}
    }
    return git_oid_is_zero(latest) ? 1 : 0;
}

int patch_show (Oid patch_id, size_t patch_id_hexlen, const char* rid, bool json) {
    char buf [HEXSIZ];
    RadRepo rrepo;
    if (open_rrepo(&rrepo,rid)) return 1;

    // Resolve prefix to full OID
    git_odb* odb = 0;
    if (git_repository_odb(&odb,rrepo.repo)) {
	eprintf("failed to get repository odb");
	return 1;
    }
    git_odb_object* odb_obj = 0;
    if (git_odb_read_prefix(&odb_obj,odb,&patch_id,patch_id_hexlen)) {
	eprintf("failed to find patch with given id prefix");
	return 1;
    }
    patch_id = *git_odb_object_id(odb_obj);

    // Find the latest entry for this patch (may differ from root after updates)
    Oid latest_entry = {{0}};
    if (find_latest_patch_entry(&latest_entry,rrepo.repo,patch_id)) {
	eprintf("failed to find patch entry in storage");
	return 1;
    }

    PatchInfo info = {0};
    if (parse_patch_entry(&info,rrepo.repo,latest_entry)) {
	eprintf("failed to parse patch entry");
	return 1;
    }

    if (json) {
	json_object* obj = json_object_new_object();
	json_object_object_add(obj,"title",json_object_new_string(info.title));
	json_object_object_add(obj,"patch",json_object_new_string(git_oid_tostr(buf,HEXSIZ,&info.patch_id)));
	json_object_object_add(obj,"author",json_object_new_string(info.author));
	json_object_object_add(obj,"alias",json_object_new_string(info.alias));
	json_object_object_add(obj,"head",json_object_new_string(git_oid_tostr(buf,HEXSIZ,&info.head)));
	json_object_object_add(obj,"base",json_object_new_string(git_oid_tostr(buf,HEXSIZ,&info.base)));
	json_object_object_add(obj,"status",json_object_new_string("open"));
	json_object_object_add(obj,"description",json_object_new_string(info.description));
	json_object_object_add(obj,"timestamp",json_object_new_int64(info.timestamp));
	printf("%s\n",json_object_to_json_string(obj));
    }
    else {
	printf("Title   %s\n",info.title);
	printf("Patch   %s\n",git_oid_tostr(buf,HEXSIZ,&info.patch_id));
	printf("Author  %s %s\n",info.alias,info.author);
	printf("Head    %s\n",git_oid_tostr(buf,HEXSIZ,&info.head));
	printf("Base    %s\n",git_oid_tostr(buf,HEXSIZ,&info.base));
	printf("Status  open\n");
	if (info.description && strlen(info.description))
	    printf("\n%s\n",info.description);
    }
    return 0;
}

int patch_run (Command c) {
    if (c.type == CMD_HELP) {
	print_help_patch();
	return 0;
    }
    else if (!profile_load()) {
	eprintf("No profile is loaded. Run `crad auth` to create one");
	return 1;
    }
    else if (!password_loaded()) {
	eprintf("You must be authenticated first. Run `crad auth` to authenticate");
	return 1;
    }
    else if (c.argc > 0 && !strcmp(c.argv[0],"list")) {
	PatchCommand cmd = parse_args_patch(c.argc,c.argv);
	if (cmd.err) return 1;
	return patch_list(cmd.rid);
    }
    else if (c.argc > 1 && !strcmp(c.argv[0],"show")) {
	Oid patch_id = {{0}};
	if (git_oid_fromstrp(&patch_id,c.argv[1])) {
	    eprintf("failed to parse patch id");
	    return 1;
	}
	size_t patch_id_hexlen = strlen(c.argv[1]);
	PatchCommand cmd = parse_args_patch(c.argc,c.argv);
	if (cmd.err) return 1;
	return patch_show(patch_id,patch_id_hexlen,cmd.rid,cmd.json);
    }
    else {
	print_help_patch();
    }
    return 0;
}
