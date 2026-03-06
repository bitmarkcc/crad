#include <stdio.h>
#include <string.h>

#include <commands/inspect.h>
#include <command.h>
#include <profile.h>
#include <id.h>
#include <git.h>
#include <repo.h>

InspectCommand command_inspect_default () {
    InspectCommand cmd;
    cmd.err = 0;
    cmd.rid = 0;
    cmd.get_rid = false;
    cmd.name = false;
    cmd.desc = false;
    cmd.default_branch = false;
    cmd.visibility = false;
    cmd.head = false;
    cmd.delegates = false;
    cmd.allowed = false;
    cmd.identity = false;
    return cmd;
}

void print_help_inspect () {
    printf("crad inspect (Show information about the repository in the current directory, or the one given with the -R option) Usage:\n");
    printf("crad inspect [-R <rid>] [--rid] [--name] [--description] [--default-branch] [--visibility] [--head] [--delegates] [--allowed] [--identity]\n");
}

InspectCommand parse_args_inspect (int argc, char** argv) {
    InspectCommand cmd = command_inspect_default();
    for (size_t i=0; i<argc; i++) {
	if (!strcmp(argv[i],"-R")) {
	    if (i+1 >= argc) {
		eprintf("no parameter to -R");
		cmd.err = 1;
		return cmd;
	    }
	    cmd.rid = strdup(argv[i+1]);
	}
	if (!strcmp(argv[i],"--rid")) {
	    cmd.get_rid = true;
	}
	else if (!strcmp(argv[i],"--name")) {
	    cmd.name = true;
	}
	else if (!strcmp(argv[i],"--description") || !strcmp(argv[i],"--desc")) {
	    cmd.desc = true;
	}
	else if (!strcmp(argv[i],"--default-branch") || !strcmp(argv[i],"--branch")) {
	    cmd.default_branch = true;
	}
	else if (!strcmp(argv[i],"--visibility")) {
	    cmd.visibility = true;
	}
	else if (!strcmp(argv[i],"--head")) {
	    cmd.head = true;
	}
	else if (!strcmp(argv[i],"--delegates")) {
	    cmd.delegates = true;
	}
	else if (!strcmp(argv[i],"--allowed")) {
	    cmd.allowed = true;
	}
	else if (!strcmp(argv[i],"--identity")) {
	    cmd.identity = true;
	}
    }
    return cmd;
}

int inspect_run (Command c) {
    if (c.type == CMD_HELP) {
	print_help_inspect();
	return 0;
    }
    else if (!profile_load()) {
	eprintf("No profile is loaded. Run `crad auth` to create one.");
	return 1;
    }
    else {
	InspectCommand cmd = parse_args_inspect(c.argc,c.argv);
	if (cmd.err) {
	    return 1;
	}
	const char* rad_home = get_rad_home();
	const char* rid_str = 0;
	git_repository* repo = 0;
	rad_git_init();	
	if (cmd.rid) {
	    rid_str = cmd.rid;
	    char* path = malloc(strlen(rad_home)+64);
	    sprintf(path,"%s/storage/%s",rad_home,rid_str);
	    if (git_repository_open(&repo,path)) {
		eprintf("failed to open git repository at %s",path);
		return 1;
	    }
	}
	else {
	    char cwd [1024];
	    if (!getcwd(cwd,sizeof(cwd))) {
		eprintf("can't get current working directory");
		return 1;
	    }
	    if (git_repository_open(&repo,cwd)) {
		eprintf("failed to open git repository at %s",cwd);
		return 1;
	    }
	    Oid rid = rid_of_rad_remote(repo);
	    if (git_oid_is_zero(&rid)) {
		eprintf("failed to get candidate rid");
		return 1;
	    }
	    rid_str = oid_to_rid(rid);
	    char* path = malloc(strlen(rad_home)+64);
	    sprintf(path,"%s/storage/%s",rad_home,rid_str);
	    repo = 0;
	    if (git_repository_open(&repo,path)) {
		eprintf("failed to open git repository at %s",path);
		return 1;
	    }
	}

	char buf [HEXSIZ];
	
	if (cmd.identity) {
	    json_object* identity_doc = get_identity_document(repo);
	    char* identity_doc_str = rad_remove_space_json(json_object_to_json_string(identity_doc));
	    printf("%s\n",identity_doc_str);
	}
	else {
	    SimpleSet delegates;
	    set_init(&delegates);
	    SimpleSet allowed;
	    set_init(&allowed);
	    StrJsonMap payload = str_json_map_new(0);
	    Visibility visibility = 0;
	    if (get_entities_from_identity_doc(&delegates,&allowed,&payload,&visibility,repo)) {
		eprintf("failed to get entities from identity document");
		return 1;
	    }
	    
	    json_object* payload_val = payload.values[0];
	    json_object* name_val = 0;
	    json_object* desc_val = 0;
	    json_object* branch_val = 0;
	    json_object_object_get_ex(payload_val,"name",&name_val);
	    json_object_object_get_ex(payload_val,"description",&desc_val);
	    json_object_object_get_ex(payload_val,"defaultBranch",&branch_val);
	    const char* name_str = json_object_get_string(name_val);
	    const char* desc_str = json_object_get_string(desc_val);
	    const char* branch_str = json_object_get_string(branch_val);
	    char* name = strdup(name_str);
	    char* desc = strdup(desc_str);
	    char* branch = strdup(branch_str);
	    rad_replace(name,' ','_');
	    rad_replace(desc,' ','_');
	    rad_replace(branch,' ','_');
	    
	    Oid head_id = {{0}};
	    if (git_reference_name_to_id(&head_id,repo,"HEAD")) {
		eprintf("failed to lookup HEAD oid");
		return 1;
	    }
	    git_oid_tostr(buf,HEXSIZ,&head_id);
	    const char* head = strdup(buf);
	    
	    if (cmd.get_rid) {
		printf("rid rad:%s\n",rid_str);
	    }
	    
	    if (cmd.name) {
		printf("name %s\n",name);
	    }
	    
	    if (cmd.desc) {
		printf("desc %s\n",desc);
	    }
	    
	    if (cmd.default_branch) {
		printf("branch %s\n",branch);
	    }
	    
	    if (cmd.visibility) {
		printf("visibility %s\n",visibility_to_str(visibility));
	    }
	    
	    if (cmd.head) {
		Oid head_id = {{0}};
		if (git_reference_name_to_id(&head_id,repo,"HEAD")) {
		    eprintf("failed to lookup HEAD oid");
		    return 1;
		}
		git_oid_tostr(buf,HEXSIZ,&head_id);
		git_commit* head_commit = 0;
		if (!git_commit_lookup(&head_commit,repo,&head_id)) {
		    git_time_t t = git_commit_time(head_commit);
		    printf("head %s %ld\n",buf,(long)t);
		    git_commit_free(head_commit);
		} else {
		    printf("head %s\n",buf);
		}
	    }
	    if (cmd.delegates) {
		size_t n_delegates = 0;
		char** delegates_list = set_to_array(&delegates,&n_delegates);
		printf("delegates ");
		char* delegates_str = malloc(n_delegates*64);
		delegates_str[0] = 0;
		for (size_t i=0; i<n_delegates; i++) {
		    if (i>0)
			strcat(delegates_str,",");
		    strcat(delegates_str,delegates_list[i]);
		}
		printf("%s\n",delegates_str);
	    }
	    if (cmd.allowed) {
		size_t n_allowed = 0;
		char** allowed_list = set_to_array(&allowed,&n_allowed);
		printf("allowed ");
		char* allowed_str = malloc(n_allowed*64);
		allowed_str[0] = 0;
		for (size_t i=0; i<n_allowed; i++) {
		    if (i>0)
			strcat(allowed_str,",");
		    strcat(allowed_str,allowed_list[i]);
		}
		printf("%s\n",allowed_str);
	    }
	}
    }
    return 0;
}
