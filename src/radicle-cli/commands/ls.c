#include <string.h>
#include <stdio.h>
#include <sqlite3.h>
#include <time.h>
#include <dirent.h>

#include <commands/ls.h>
#include <profile.h>
#include <print.h>
#include <git.h>
#include <cob.h>
#include <rad.h>

LsCommand command_ls_default () {
    LsCommand cmd;
    cmd.err = 0;
    cmd.public = false;
    cmd.private = false;
    return cmd;
}

void print_help_ls () {
    printf("crad ls (List repositories) Usage:\n");
    printf("crad ls [--public] [--private]\n");
}

LsCommand parse_args_ls (int argc, char** argv) {
    LsCommand cmd = command_ls_default();
    for (size_t i=0; i<argc; i++) {
	if (!strcmp(argv[i],"--public")) {
	    cmd.public = true;
	}
	else if (!strcmp(argv[i],"--private")) {
	    cmd.private = true;
	}
    }
    return cmd;
}

int ls_run (Command c) {
    if (c.type == CMD_HELP) {
	print_help_ls();
	return 0;
    }
    else if (!profile_load()) {
	eprintf("No profile is loaded. Run `crad auth` to create one");
	return 1;
    }
    else {
	LsCommand cmd = parse_args_ls(c.argc,c.argv);
	if (cmd.err) {
	    return 1;
	}
	char buf [HEXSIZ];
	Storage s = profile_get_storage();
	DIR* d = opendir(s.path);
	struct dirent* dir = 0;
	if (!d) {
	    eprintf("failed to open directory %s",s.path);
	    return 1;
	}
	rad_git_init();
	bool have_dir = false;
	while (dir = readdir(d)) {
	    if (strlen(dir->d_name)>2 && strlen(dir->d_name)<40) { // todo handle edge cases
		if (!have_dir) {
		    printf("Name-------------RID---------------------------Visibility-Head----Description\n");
		    have_dir = true;
		}
		char* repo_path = malloc(strlen(s.path)+64);
		sprintf(repo_path,"%s/%s",s.path,dir->d_name);
		git_repository* repo = 0;
		if (git_repository_open(&repo,repo_path)) {
		    eprintf("failed to open git repository at %s",repo_path);
		    return 1;
		}
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

		if (cmd.public && visibility != VIS_PUBLIC)
		    continue;

		if (cmd.private && visibility != VIS_PRIVATE)
		    continue;
		
		json_object* payload_val = payload.values[0];
		json_object* name_val = 0;
		json_object* desc_val = 0;
		json_object_object_get_ex(payload_val,"name",&name_val);
		json_object_object_get_ex(payload_val,"description",&desc_val);
		const char* name_str = json_object_get_string(name_val);
		const char* desc_str = json_object_get_string(desc_val);

		Oid head_id = {{0}};
		if (git_reference_name_to_id(&head_id,repo,"HEAD")) {
		    eprintf("failed to lookup HEAD oid");
		    return 1;
		}
		git_oid_tostr(buf,HEXSIZ,&head_id);
		buf[7] = 0;
		
		char* name = strdup(name_str);
		if (strlen(name)>16)
		    name[16] = 0;
		rad_replace(name,' ','_');
		printf("%s",name);
		size_t name_len = strlen(name);
		for (size_t j=0; j<17-name_len; j++)
		    printf(" ");
		printf("%s ",dir->d_name);
		size_t rid_len = strlen(dir->d_name);
		for (size_t j=0; j<29-rid_len; j++)
		    printf(" ");
		
		char* visibility_str = visibility_to_str(visibility);
		if (strlen(visibility_str)>10)
		    visibility_str[10] = 0;
		printf("%s",visibility_str);
		size_t visibility_len = strlen(visibility_str);
		for (size_t j=0; j<11-visibility_len; j++)
		    printf(" ");

		printf("%s ",buf);

		char* desc = strdup(desc_str);
		if (strlen(desc)>32)
		    desc[32] = 0;
		rad_replace(desc,' ','_');
		printf("%s\n",desc);
		
	    }
	}
	closedir(d);
    }
    return 0;
}
