#include <string.h>
#include <stdio.h>
#include <sqlite3.h>
#include <time.h>

#include <commands/id.h>
#include <profile.h>
#include <print.h>
#include <git.h>
#include <cob.h>
#include <rad.h>

IDCommand command_id_default () {
    IDCommand cmd;
    cmd.err = 0;
    cmd.title = 0;
    cmd.desc = 0;
    set_init(&cmd.delegate);
    set_init(&cmd.rescind);
    set_init(&cmd.allow);
    set_init(&cmd.disallow);
    cmd.threshold = 0;
    cmd.visibility = 0;
    cmd.payload = str_json_map_new(0);
    return cmd;
}

void print_help_id () {
    printf("crad id (Manage repository identities) Usage:\n");
    printf("crad id update [--title <string>] [--desc <string>] [--delegate <did>] [--rescind <did>] [--threshold <num>] [--visibility <private | public>] [--allow <did>] [--disallow <did>]\n");
}

IDCommand parse_args_id (int argc, char** argv) {
    IDCommand cmd = command_id_default();
    for (size_t i=0; i<argc; i++) {
	if (!strcmp(argv[i],"--title")) {
	    if (i+1 < argc) cmd.title = strdup(argv[i+1]);
	}
	else if (!strcmp(argv[i],"--description") || !strcmp(argv[i],"--desc")) {
	    if (i+1 < argc) cmd.desc = strdup(argv[i+1]);
	}
	else if (!strcmp(argv[i],"--delegate")) {
	    if (i+1 >= argc) {
		eprintf("no parameter to --delegate");
		cmd.err = 1;
		return cmd;
	    }
	    set_add_str(&cmd.delegate,argv[i+1]);
	}
	else if (!strcmp(argv[i],"--rescind")) {
	    if (i+1 >= argc) {
		eprintf("no parameter to --rescind");
		cmd.err = 1;
		return cmd;
	    }
	    set_add_str(&cmd.rescind,argv[i+1]);
	}
	else if (!strcmp(argv[i],"--threshold")) {
	    if (i+1 < argc) {
		cmd.threshold = atoi(argv[i+1]);
		if (!cmd.threshold) {
		    eprintf("invalid --threshold argument");
		    cmd.err = 1;
		    return cmd;
		}
	    }	    
	}
	else if (!strcmp(argv[i],"--visibility")) {
	    if (i+1 < argc) {
		if (!strcmp(argv[i+1],"private")) {
		    cmd.visibility = malloc(sizeof(Visibility));
		    *cmd.visibility = VIS_PRIVATE;
		}
		else if (!strcmp(argv[i+1],"public")) {
		    cmd.visibility = malloc(sizeof(Visibility));
		    *cmd.visibility = VIS_PUBLIC;
		}
		else {
		    eprintf("wrong parameter to --visibility");
		    cmd.err = 1;
		    return cmd;
		}
	    }
	}
	else if (!strcmp(argv[i],"--allow")) {
	    if (i+1 >= argc) {
		eprintf("no parameter to --allow");
		cmd.err = 1;
		return cmd;
	    }
	    set_add_str(&cmd.allow,argv[i+1]);
	}
	else if (!strcmp(argv[i],"--disallow")) {
	    if (i+1 >= argc) {
		eprintf("no parameter to --disallow");
		cmd.err = 1;
		return cmd;
	    }
	    set_add_str(&cmd.disallow,argv[i+1]);
	}
	else if (!strcmp(argv[i],"--payload")) {
	    if (i+3 < argc) {
		if (!cmd.payload.n_keys)
		    cmd.payload = str_json_map_new(1);
		if (cmd.payload.keys[0]) {
		    if (strcmp(argv[i+1],cmd.payload.keys[0])) {
			eprintf("only one payload id at a time is supported for the id update operation");
			cmd.err = 1;
			return cmd;
		    }
		    json_object* obj = cmd.payload.values[0];
		    json_object_object_add(obj,argv[i+2],json_object_new_string(argv[i+3]));
		}
		else {
		    cmd.payload.keys[0] = strdup(argv[i+1]);
		    json_object* obj = json_object_new_object();
		    json_object_object_add(obj,argv[i+2],json_object_new_string(argv[i+3]));
		    cmd.payload.values[0] = obj;
		}
	    }
	    else {
		eprintf("--payload requires 3 arguments");
		cmd.err = 1;
		return cmd;
	    }
	}
    }
    return cmd;
}

int id_run (Command c) {
    if (c.type == CMD_HELP) {
	print_help_id();
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
    else if (c.argc > 0 && !strcmp(c.argv[0],"update")) {
	IDCommand cmd = parse_args_id(c.argc,c.argv);
	if (cmd.err) {
	    return 1;
	}
	return id_update(cmd.title,cmd.desc,&cmd.delegate,&cmd.rescind,cmd.threshold,cmd.visibility,&cmd.allow,&cmd.disallow,cmd.payload);
    }
    else if (c.argc > 0) {
	IDCommand cmd = parse_args_id(c.argc,c.argv);
	if (cmd.err) {
	    return 1;
	}
    }
    return 0;
}

int id_update (char* title, char* desc, SimpleSet* delegate, SimpleSet* rescind, size_t threshold, Visibility* visibility, SimpleSet* allow, SimpleSet* disallow, StrJsonMap payload) {
    char buf [HEXSIZ];
    rad_git_init();
    RadRepo rrepo = rad_repo_default();
    if (get_rad_repo_from_cwd(&rrepo)) {
	eprintf("failed to get rad repo from cwd");
	return 1;
    }
    Pubkey signer = profile_get_pubkey();

    if (!signer_is_delegate(rrepo,signer)) {
	eprintf("the signer doensn't match one of the delegates for the repository");
	return 1;
    }

    // Get former delegates and allowed from identity doc
    SimpleSet former_delegates;
    set_init(&former_delegates);
    SimpleSet former_allowed;
    set_init(&former_allowed);
    StrJsonMap former_payload = str_json_map_new(0);
    Visibility former_visibility = 0;
    if (get_entities_from_identity_doc(&former_delegates,&former_allowed,&former_payload,&former_visibility,rrepo.repo)) {
	eprintf("failed to get entities from identity document");
	return 1;
    }
    // Get new delegates and allowed lists
    SimpleSet delegates1;
    set_init(&delegates1);
    SimpleSet allowed1;
    set_init(&allowed1);
    if (set_difference(&delegates1,&former_delegates,rescind)) {
	eprintf("failed to get difference between delegate sets");
	return 1;
    }
    if (set_difference(&allowed1,&former_allowed,disallow)) {
	eprintf("failed to get difference between allow sets");
	return 1;
    }
    SimpleSet delegates2;
    set_init(&delegates2);
    SimpleSet allowed2;
    set_init(&allowed2);
    if (set_union(&delegates2,&delegates1,delegate)) {
	eprintf("failed to get union of delegate sets");
	return 1;
    }
    if (set_union(&allowed2,&allowed1,allow)) {
	eprintf("failed to get union of allow sets");
	return 1;
    }
    // Get new payload
    if (payload.n_keys) {
	if (strcmp(payload.keys[0],"xyz.radicle.id")) {
	    eprintf("unsupported payload id");
	    return 1;
	}
	json_object* payload_obj_new = former_payload.values[0];
	json_object* payload_obj = payload.values[0];
	json_object_object_foreach(payload_obj,key,val) {
	    json_object_object_add(payload_obj_new,key,val);
	}
	payload.values[0] = payload_obj_new;
    }
    else {
	payload = former_payload;
    }
    // Get new visibility
    Visibility new_visibility = former_visibility;
    if (visibility) {
	new_visibility = *visibility;
    }
    
    RepoEntry re = cob_identity_update(rrepo,signer,title,desc,&delegates2,threshold,new_visibility,&allowed2,payload);
    if (git_oid_is_zero(&re.oid)) {
	eprintf("failed to update id cob");
	return 1;
    }
    if (update_allowed_in_cob_db(rrepo.rid,&delegates2,&allowed2)) {
	eprintf("failed to add id to cob db");
	return 1;
    }
    iprintf("identity cob updated");
    return 0;
}
