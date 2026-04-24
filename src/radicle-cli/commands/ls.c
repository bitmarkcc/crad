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
#include <json-c/json.h>

LsCommand command_ls_default () {
    LsCommand cmd;
    cmd.err = 0;
    cmd.public = false;
    cmd.private = false;
    cmd.json = false;
    cmd.refresh = false;
    cmd.limit = 0;
    cmd.query = 0;
    return cmd;
}

void print_help_ls () {
    printf("crad ls (List repositories) Usage:\n");
    printf("crad ls [--public] [--private] [--json] [--refresh] [--limit N] [--query <text>]\n");
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
	else if (!strcmp(argv[i],"--json")) {
	    cmd.json = true;
	}
	else if (!strcmp(argv[i],"--refresh")) {
	    cmd.refresh = true;
	}
	else if (!strcmp(argv[i],"--limit") || !strcmp(argv[i],"-n")) {
	    if (i+1 < argc) cmd.limit = atoi(argv[++i]);
	}
	else if (!strcmp(argv[i],"--query") || !strcmp(argv[i],"-q")) {
	    if (i+1 < argc) cmd.query = strdup(argv[++i]);
	}
    }
    return cmd;
}

static char* get_repo_cache_file () {
    char* rad_home = get_rad_home();
    char* path = malloc(strlen(rad_home)+20);
    sprintf(path,"%s/cobs/repos.db",rad_home);
    return path;
}

static int ensure_repo_cache_table (sqlite3* db) {
    const char* sql =
	"CREATE TABLE IF NOT EXISTS Repos ("
	"  rid TEXT PRIMARY KEY,"
	"  name TEXT,"
	"  description TEXT,"
	"  visibility TEXT,"
	"  head TEXT"
	");";
    char* errmsg = 0;
    if (sqlite3_exec(db,sql,0,0,&errmsg)) {
	eprintf("failed to create repos table: %s",errmsg);
	sqlite3_free(errmsg);
	return 1;
    }
    return 0;
}

static int refresh_repo_cache (Storage s, sqlite3* db) {
    DIR* d = opendir(s.path);
    struct dirent* dir = 0;
    if (!d) {
	eprintf("failed to open directory %s",s.path);
	return 1;
    }
    char buf [HEXSIZ];
    rad_git_init();

    sqlite3_exec(db,"BEGIN TRANSACTION;",0,0,0);

    sqlite3_stmt* stmt = 0;
    const char* sql = "INSERT OR REPLACE INTO Repos (rid,name,description,visibility,head) VALUES (?,?,?,?,?);";
    if (sqlite3_prepare_v2(db,sql,-1,&stmt,0)) {
	eprintf("failed to prepare sql statement");
	closedir(d);
	return 1;
    }

    while ((dir = readdir(d))) {
	if (strlen(dir->d_name) <= 2 || strlen(dir->d_name) >= 32)
	    continue;

	char* repo_path = malloc(strlen(s.path)+64);
	sprintf(repo_path,"%s/%s",s.path,dir->d_name);
	git_repository* repo = 0;
	if (git_repository_open(&repo,repo_path)) {
	    free(repo_path);
	    continue;
	}

	SimpleSet delegates;
	set_init(&delegates);
	SimpleSet allowed;
	set_init(&allowed);
	StrJsonMap payload = str_json_map_new(0);
	Visibility visibility = 0;
	if (get_entities_from_identity_doc(&delegates,&allowed,&payload,&visibility,repo)) {
	    git_repository_free(repo);
	    free(repo_path);
	    continue;
	}

	json_object* payload_val = payload.values[0];
	json_object* name_val = 0;
	json_object* desc_val = 0;
	json_object_object_get_ex(payload_val,"name",&name_val);
	json_object_object_get_ex(payload_val,"description",&desc_val);
	const char* name_str = json_object_get_string(name_val);
	const char* desc_str = json_object_get_string(desc_val);

	Oid head_id = {{0}};
	if (git_reference_name_to_id(&head_id,repo,"HEAD")) {
	    git_repository_free(repo);
	    free(repo_path);
	    continue;
	}
	git_oid_tostr(buf,HEXSIZ,&head_id);

	sqlite3_bind_text(stmt,1,dir->d_name,-1,SQLITE_TRANSIENT);
	sqlite3_bind_text(stmt,2,name_str ? name_str : "",-1,SQLITE_TRANSIENT);
	sqlite3_bind_text(stmt,3,desc_str ? desc_str : "",-1,SQLITE_TRANSIENT);
	sqlite3_bind_text(stmt,4,visibility_to_str(visibility),-1,SQLITE_TRANSIENT);
	sqlite3_bind_text(stmt,5,buf,-1,SQLITE_TRANSIENT);
	sqlite3_step(stmt);
	sqlite3_reset(stmt);

	git_repository_free(repo);
	free(repo_path);
    }
    closedir(d);
    sqlite3_finalize(stmt);
    sqlite3_exec(db,"COMMIT;",0,0,0);
    return 0;
}

static int query_repo_cache (sqlite3* db, LsCommand cmd, bool use_json) {
    // Build SQL query
    char sql [512];
    int n = 0;
    n += sprintf(sql+n,"SELECT rid,name,description,visibility,head FROM Repos WHERE 1=1");
    if (cmd.public)
	n += sprintf(sql+n," AND visibility='public'");
    if (cmd.private)
	n += sprintf(sql+n," AND visibility='private'");
    if (cmd.query)
	n += sprintf(sql+n," AND (name LIKE ? OR description LIKE ?)");
    n += sprintf(sql+n," ORDER BY name");
    if (cmd.limit > 0)
	n += sprintf(sql+n," LIMIT %d",cmd.limit);
    n += sprintf(sql+n,";");

    sqlite3_stmt* stmt = 0;
    if (sqlite3_prepare_v2(db,sql,-1,&stmt,0)) {
	eprintf("failed to prepare query: %s",sqlite3_errmsg(db));
	return 1;
    }

    if (cmd.query) {
	char* pattern = malloc(strlen(cmd.query)+3);
	sprintf(pattern,"%%%s%%",cmd.query);
	sqlite3_bind_text(stmt,1,pattern,-1,SQLITE_TRANSIENT);
	sqlite3_bind_text(stmt,2,pattern,-1,SQLITE_TRANSIENT);
	free(pattern);
    }

    bool have_header = false;
    json_object* arr = use_json ? json_object_new_array() : 0;

    while (sqlite3_step(stmt) == SQLITE_ROW) {
	const char* rid = (const char*)sqlite3_column_text(stmt,0);
	const char* name = (const char*)sqlite3_column_text(stmt,1);
	const char* desc = (const char*)sqlite3_column_text(stmt,2);
	const char* vis = (const char*)sqlite3_column_text(stmt,3);
	const char* head = (const char*)sqlite3_column_text(stmt,4);

	if (use_json) {
	    json_object* obj = json_object_new_object();
	    json_object_object_add(obj,"name",json_object_new_string(name));
	    json_object_object_add(obj,"rid",json_object_new_string(rid));
	    json_object_object_add(obj,"visibility",json_object_new_string(vis));
	    json_object_object_add(obj,"head",json_object_new_string(head));
	    json_object_object_add(obj,"description",json_object_new_string(desc));
	    json_object_array_add(arr,obj);
	} else {
	    if (!have_header) {
		printf("Name-------------RID---------------------------Visibility-Head----Description\n");
		have_header = true;
	    }
	    char* name_trunc = strdup(name);
	    if (strlen(name_trunc)>16)
		name_trunc[16] = 0;
	    rad_replace(name_trunc,' ','_');
	    printf("%s",name_trunc);
	    size_t name_len = strlen(name_trunc);
	    for (size_t j=0; j<17-name_len; j++)
		printf(" ");
	    free(name_trunc);

	    printf("%s ",rid);
	    size_t rid_len = strlen(rid);
	    for (size_t j=0; j<29-rid_len; j++)
		printf(" ");

	    char* vis_trunc = strdup(vis);
	    if (strlen(vis_trunc)>10)
		vis_trunc[10] = 0;
	    printf("%s",vis_trunc);
	    size_t vis_len = strlen(vis_trunc);
	    for (size_t j=0; j<11-vis_len; j++)
		printf(" ");
	    free(vis_trunc);

	    char head_short [8];
	    strncpy(head_short,head,7);
	    head_short[7] = 0;
	    printf("%s ",head_short);

	    char* desc_trunc = strdup(desc);
	    if (strlen(desc_trunc)>32)
		desc_trunc[32] = 0;
	    rad_replace(desc_trunc,' ','_');
	    printf("%s\n",desc_trunc);
	    free(desc_trunc);
	}
    }
    sqlite3_finalize(stmt);

    if (use_json) {
	printf("%s\n",json_object_to_json_string(arr));
	json_object_put(arr);
    }
    return 0;
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
	bool use_json = cmd.json || c.json;
	Storage s = profile_get_storage();

	const char* db_file = get_repo_cache_file();
	sqlite3* db = 0;
	if (sqlite3_open(db_file,&db)) {
	    eprintf("failed to open repo cache db");
	    return 1;
	}
	if (ensure_repo_cache_table(db)) {
	    sqlite3_close(db);
	    return 1;
	}

	// Auto-refresh if cache is empty or --refresh requested
	if (cmd.refresh) {
	    iprintf("refreshing repo cache...");
	    if (refresh_repo_cache(s,db)) {
		sqlite3_close(db);
		return 1;
	    }
	    iprintf("repo cache refreshed");
	} else {
	    sqlite3_stmt* count_stmt = 0;
	    sqlite3_prepare_v2(db,"SELECT COUNT(*) FROM Repos;",-1,&count_stmt,0);
	    int count = 0;
	    if (sqlite3_step(count_stmt) == SQLITE_ROW)
		count = sqlite3_column_int(count_stmt,0);
	    sqlite3_finalize(count_stmt);
	    if (count == 0) {
		iprintf("building repo cache (first run)...");
		if (refresh_repo_cache(s,db)) {
		    sqlite3_close(db);
		    return 1;
		}
		iprintf("repo cache built");
	    }
	}

	int ret = query_repo_cache(db,cmd,use_json);
	sqlite3_close(db);
	return ret;
    }
    return 0;
}
