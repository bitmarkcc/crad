#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

#include <commands/seed.h>
#include <command.h>
#include <profile.h>
#include <util.h>
#include <git.h>
#include <id.h>
#include <repo.h>

SeedCommand command_seed_default() {
    SeedCommand cmd;
    cmd.err = 0;
    cmd.rid = 0;
    return cmd;
}

void print_help_seed () {
    printf("crad seed (Seed Repository) Usage:\n");
    printf("crad seed <rid>\n");
}

SeedCommand parse_args_seed (int argc, char** argv) {
    SeedCommand cmd = command_seed_default();
    for (size_t i=0; i<argc; i++) {
	if (argv[i][0] != '-' && !cmd.rid) {
	    cmd.rid = strdup(argv[i]);
	}
    }
    return cmd;
}

int seed_run (Command c) {
    if (c.type == CMD_HELP) {
	print_help_seed();
	return 0;
    }
    else if (!c.argc) {
	eprintf("crad seed requires an RID argument");
	return 1;
    }
    else if (!profile_load()) {
	eprintf("No profile is loaded. Run `crad auth` to create one.");
	return 1;
    }
    SeedCommand cmd = parse_args_seed(c.argc,c.argv);
    if (cmd.err) {
	return 1;
    }
    if (!cmd.rid) {
	eprintf("crad seed requires an RID argument");
	return 1;
    }

    const char* crad_home = get_rad_home();
    const char* rad_home = get_rad_node_home();

    // Strip rad: prefix if present
    char* rid_str = cmd.rid;
    if (!strncmp(rid_str,"rad:",4))
	rid_str = rid_str + 4;

    // Call rad-seed-wrapped
    char* argv [5];
    char* bin_path = malloc(strlen(crad_home)+32);
    sprintf(bin_path,"%s/bin/rad-seed-wrapped",crad_home);
    char rid_arg [128];
    sprintf(rid_arg,"rad:%s",rid_str);
    argv[0] = bin_path;
    argv[1] = rid_arg;
    argv[2] = 0;
    if (exec_command(bin_path,argv)) {
	eprintf("rad seed (wrapped) command failed");
	return 1;
    }

    // rsync from rad storage to crad storage
    char* rad_repo = malloc(strlen(rad_home)+128);
    sprintf(rad_repo,"%s/storage/%s/",rad_home,rid_str);

    char* crad_repo = malloc(strlen(crad_home)+128);
    sprintf(crad_repo,"%s/storage/%s",crad_home,rid_str);

    argv[0] = "rsync";
    argv[1] = "-a";
    argv[2] = rad_repo;
    argv[3] = crad_repo;
    argv[4] = 0;
    if (exec_command("rsync",argv)) {
	eprintf("rsync failed");
	return 1;
    }

    // Set crad user.name and user.email in the crad repo config
    rad_git_init();
    Storage storage = profile_get_storage();

    git_repository* repo = 0;
    if (git_repository_open(&repo,crad_repo)) {
	eprintf("failed to open git repository at %s",crad_repo);
	return 1;
    }
    git_config* config = 0;
    if (git_repository_config(&config,repo)) {
	eprintf("failed to get the config file for the git repository at %s",crad_repo);
	return 1;
    }
    git_config_set_string(config,"user.name",storage.info.name);
    git_config_set_string(config,"user.email",storage.info.email);
    git_repository_free(repo);

    iprintf("crad seed successful");
    return 0;
}
