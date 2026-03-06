#include <stdio.h>
#include <string.h>

#include <commands/source.h>
#include <profile.h>
#include <print.h>
#include <git.h>
#include <repo.h>
#include <json-c/json.h>

SourceCommand command_source_default () {
    SourceCommand cmd;
    cmd.err = 0;
    cmd.rid = 0;
    cmd.path = 0;
    return cmd;
}

void print_help_source () {
    printf("crad source (Browse repository source code) Usage:\n");
    printf("crad source tree [-R <rid>] [--path <path>]\n");
    printf("crad source blob [-R <rid>] --path <path>\n");
}

SourceCommand parse_args_source (int argc, char** argv) {
    SourceCommand cmd = command_source_default();
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
	else if (!strcmp(argv[i],"--path")) {
	    if (i+1 >= argc) {
		eprintf("no parameter to --path");
		cmd.err = 1;
		return cmd;
	    }
	    cmd.path = strdup(argv[i+1]);
	    i++;
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

static int get_head_tree (git_tree** out, git_repository* repo) {
    Oid head_id = {{0}};
    if (git_reference_name_to_id(&head_id,repo,"HEAD")) {
	eprintf("failed to lookup HEAD");
	return 1;
    }
    git_commit* commit = 0;
    if (git_commit_lookup(&commit,repo,&head_id)) {
	eprintf("failed to lookup HEAD commit");
	return 1;
    }
    if (git_commit_tree(out,commit)) {
	eprintf("failed to get commit tree");
	git_commit_free(commit);
	return 1;
    }
    git_commit_free(commit);
    return 0;
}

int source_tree (const char* rid, const char* path) {
    RadRepo rrepo;
    if (open_rrepo(&rrepo,rid)) return 1;

    git_tree* root_tree = 0;
    if (get_head_tree(&root_tree,rrepo.repo)) return 1;

    git_tree* target_tree = root_tree;
    git_tree_entry* path_entry = 0;
    if (path && strlen(path) && strcmp(path,"/") && strcmp(path,".")) {
	if (git_tree_entry_bypath(&path_entry,root_tree,path)) {
	    eprintf("path not found: %s",path);
	    return 1;
	}
	if (git_tree_entry_type(path_entry) != GIT_OBJECT_TREE) {
	    eprintf("path is not a directory: %s",path);
	    return 1;
	}
	const git_oid* tree_oid = git_tree_entry_id(path_entry);
	target_tree = 0;
	if (git_tree_lookup(&target_tree,rrepo.repo,tree_oid)) {
	    eprintf("failed to lookup tree for path %s",path);
	    return 1;
	}
    }

    size_t count = git_tree_entrycount(target_tree);
    json_object* arr = json_object_new_array();

    // directories first
    for (size_t i=0; i<count; i++) {
	const git_tree_entry* te = git_tree_entry_byindex(target_tree,i);
	if (git_tree_entry_type(te) == GIT_OBJECT_TREE) {
	    json_object* obj = json_object_new_object();
	    json_object_object_add(obj,"name",
		json_object_new_string(git_tree_entry_name(te)));
	    json_object_object_add(obj,"kind",
		json_object_new_string("tree"));
	    json_object_array_add(arr,obj);
	}
    }
    // then files
    for (size_t i=0; i<count; i++) {
	const git_tree_entry* te = git_tree_entry_byindex(target_tree,i);
	if (git_tree_entry_type(te) != GIT_OBJECT_TREE) {
	    json_object* obj = json_object_new_object();
	    json_object_object_add(obj,"name",
		json_object_new_string(git_tree_entry_name(te)));
	    json_object_object_add(obj,"kind",
		json_object_new_string("blob"));
	    json_object_array_add(arr,obj);
	}
    }

    printf("%s\n",json_object_to_json_string(arr));
    json_object_put(arr);
    return 0;
}

int source_blob (const char* rid, const char* path) {
    if (!path || !strlen(path)) {
	eprintf("--path is required for blob subcommand");
	return 1;
    }

    RadRepo rrepo;
    if (open_rrepo(&rrepo,rid)) return 1;

    git_tree* root_tree = 0;
    if (get_head_tree(&root_tree,rrepo.repo)) return 1;

    git_tree_entry* entry = 0;
    if (git_tree_entry_bypath(&entry,root_tree,path)) {
	eprintf("file not found: %s",path);
	return 1;
    }
    if (git_tree_entry_type(entry) != GIT_OBJECT_BLOB) {
	eprintf("path is not a file: %s",path);
	return 1;
    }

    const git_oid* blob_oid = git_tree_entry_id(entry);
    git_blob* blob = 0;
    if (git_blob_lookup(&blob,rrepo.repo,blob_oid)) {
	eprintf("failed to lookup blob");
	return 1;
    }

    const void* content = git_blob_rawcontent(blob);
    git_object_size_t size = git_blob_rawsize(blob);
    fwrite(content,1,size,stdout);

    git_blob_free(blob);
    return 0;
}

int source_run (Command c) {
    if (c.type == CMD_HELP) {
	print_help_source();
	return 0;
    }
    else if (!profile_load()) {
	eprintf("No profile is loaded. Run `crad auth` to create one");
	return 1;
    }
    else if (c.argc > 0 && !strcmp(c.argv[0],"tree")) {
	SourceCommand cmd = parse_args_source(c.argc,c.argv);
	if (cmd.err) return 1;
	return source_tree(cmd.rid,cmd.path);
    }
    else if (c.argc > 0 && !strcmp(c.argv[0],"blob")) {
	SourceCommand cmd = parse_args_source(c.argc,c.argv);
	if (cmd.err) return 1;
	if (!cmd.path) {
	    eprintf("--path is required for blob subcommand");
	    return 1;
	}
	return source_blob(cmd.rid,cmd.path);
    }
    else {
	print_help_source();
    }
    return 0;
}
