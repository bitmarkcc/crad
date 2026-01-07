#include <string.h>
#include <stdio.h>
#include <sqlite3.h>
#include <time.h>

#include <commands/issue.h>
#include <profile.h>
#include <print.h>
#include <git.h>
#include <cob.h>

IssueCommand command_issue_default() {
    IssueCommand cmd;
    cmd.err = 0;
    cmd.title = 0;
    cmd.desc = 0;
    cmd.message = 0;
    Oid reply_to = {{0}};
    cmd.reply_to = reply_to;
    cmd.reply_to_hexlen = 0;
    Oid issue_id = {{0}};
    cmd.issue_id = issue_id;
    cmd.issue_id_hexlen = 0;
    set_init(&cmd.add);
    set_init(&cmd.delete);
    return cmd;
}

void print_help_issue () {
    printf("crad issue (Manage issues) Usage:\n");
    printf("crad issue open [--title <title>] [--desc <text>]\n");
    printf("crad issue comment <issue-id> [--message <message>] [--reply-to <comment-id>]\n");
    printf("crad issue assign <issue-id> [--add <did>] [--delete <did>]\n");
}

IssueCommand parse_args_issue (int argc, char** argv) {
    IssueCommand cmd = command_issue_default();
    for (size_t i=0; i<argc; i++) {
	if (!strcmp(argv[i],"--title")) {
	    if (i+1 < argc) cmd.title = strdup(argv[i+1]);
	}
	else if (!strcmp(argv[i],"--description") || !strcmp(argv[i],"--desc")) {
	    if (i+1 < argc) cmd.desc = strdup(argv[i+1]);
	}
	else if (!strcmp(argv[i],"--message")) {
	    if (i+1 < argc) cmd.message = strdup(argv[i+1]);
	}
	else if (!strcmp(argv[i],"--reply-to")) {
	    if (i+1 >= argc) {
		eprintf("no parameter to --reply-to");
		cmd.err = 1;
		return cmd;
	    }
	    Oid reply_to = {{0}};
	    if (git_oid_fromstrp(&reply_to,argv[i+1])) {
		eprintf("failed to parse reply-to oid");
		cmd.err = 1;
		return cmd;
	    }
	    cmd.reply_to = reply_to;
	    cmd.reply_to_hexlen = strlen(argv[i+1]);
	}
	else if (!strcmp(argv[i],"--add")) {
	    if (i+1 >= argc) {
		eprintf("no parameter to --add");
		cmd.err = 1;
		return cmd;
	    }
	    set_add_str(&cmd.add,argv[i+1]);
	}
	else if (!strcmp(argv[i],"--delete")) {
	    if (i+1 >= argc) {
		eprintf("no parameter to --delete");
		cmd.err = 1;
		return cmd;
	    }
	    set_add_str(&cmd.delete,argv[i+1]);
	}
    }
    return cmd;
}

int issue_run (Command c) {
    if (!profile_load()) {
	eprintf("No profile is loaded. Run `crad auth` to create one");
	return 1;
    }
    else if (!password_loaded()) {
	eprintf("You must be authenticated first. Run `crad auth` to authenticate");
	return 1;
    }
    else if (c.type == CMD_HELP) {
	print_help_issue();
	return 0;
    }
    else if (c.argc > 0 && !strcmp(c.argv[0],"open")) {
	IssueCommand cmd = parse_args_issue(c.argc,c.argv);
	if (cmd.err) {
	    return 1;
	}
	return issue_open(cmd.title,cmd.desc);
    }
    else if (c.argc > 1 && !strcmp(c.argv[0],"comment")) {
	IssueCommand cmd = parse_args_issue(c.argc,c.argv);
	if (cmd.err) {
	    return 1;
	}
	Oid issue_id = {{0}};
	if (git_oid_fromstrp(&issue_id,c.argv[1])) {
	    eprintf("failed to parse issue id");
	    return 1;
	}
	size_t issue_id_hexlen = strlen(c.argv[1]);
	return issue_comment(issue_id,issue_id_hexlen,cmd.reply_to,cmd.reply_to_hexlen,cmd.message);
    }
    else if (c.argc > 1 && !strcmp(c.argv[0],"assign")) {
	IssueCommand cmd = parse_args_issue(c.argc,c.argv);
	if (cmd.err) return 1;
	Oid issue_id = {{0}};
	if (git_oid_fromstrp(&issue_id,c.argv[1])) {
	    eprintf("failed to parse issue id");
	    return 1;
	}
	size_t issue_id_hexlen = strlen(c.argv[1]);
	return issue_assign(issue_id,issue_id_hexlen,&cmd.add,&cmd.delete);
    }
    else if (c.argc > 0) {
	IssueCommand cmd = parse_args_issue(c.argc,c.argv);
	if (cmd.err) {
	    return 1;
	}
    }
    return 0;
}

int add_comment_to_cob_db (Oid comment_id, Oid issue_id, Oid reply_to) {
    sqlite3* db = 0;
    sqlite3_stmt* stmt = 0;
    const char* db_file = get_cob_cache_file();
    sqlite3_open(db_file,&db);
    if (!db) {
	eprintf("failed to open cob db");
	return 1;
    }
    const char* sql = "INSERT INTO Comments (ID, Time, Issue, ReplyTo) VALUES (?, ?, ?, ?);";
    if (sqlite3_prepare_v2(db,sql,-1,&stmt,0)) {
	eprintf("failed to prepare sql statement");
	return 1;
    }
    sqlite3_bind_blob(stmt,1,comment_id.id,20,SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt,2,time(0));
    sqlite3_bind_blob(stmt,3,issue_id.id,20,SQLITE_TRANSIENT);
    sqlite3_bind_blob(stmt,4,reply_to.id,20,SQLITE_TRANSIENT);
    if (sqlite3_step(stmt) != SQLITE_DONE) {
	eprintf("SQL execution failed: %s",sqlite3_errmsg(db));
	return 1;
    }
    sqlite3_finalize(stmt);
    sqlite3_close(db);
    return 0;
}

int get_assignees_from_cob_db (SimpleSet* assignees, Oid issue_id) {
    sqlite3* db = 0;
    sqlite3_stmt* stmt = 0;
    const char* db_file = get_cob_cache_file();
    sqlite3_open(db_file,&db);
    if (!db) {
	eprintf("failed to open cob db");
	return 1;
    }
    const char* sql = "SELECT DID FROM Assignees WHERE Issue = ?;";
    if (sqlite3_prepare_v2(db,sql,-1,&stmt,0)) {
	eprintf("failed to prepare sql statement");
	return 1;
    }
    sqlite3_bind_blob(stmt,1,issue_id.id,20,SQLITE_TRANSIENT);
    int ret = 0;
    while (1) {
	ret = sqlite3_step(stmt);
	if (ret == SQLITE_ROW) set_add_str(assignees,sqlite3_column_text(stmt,0));
	else break;
    }
    if (ret != SQLITE_DONE) {
	eprintf("SQL execution failed: %s",sqlite3_errmsg(db));
	return 1;
    }
    sqlite3_finalize(stmt);
    sqlite3_close(db);
    return 0;
}

int add_issue_to_cob_db (Oid issue_id, const char* author, const char* status) {
    sqlite3* db = 0;
    sqlite3_stmt* stmt = 0;
    const char* db_file = get_cob_cache_file();
    sqlite3_open(db_file,&db);
    if (!db) {
	eprintf("failed to open cob db");
	return 1;
    }
    const char* sql = "INSERT INTO Issues (ID, Author, Status) VALUES (?, ?, ?);";
    if (sqlite3_prepare_v2(db,sql,-1,&stmt,0)) {
	eprintf("failed to prepare sql statement");
	return 1;
    }
    sqlite3_bind_blob(stmt,1,issue_id.id,20,SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt,2,author,-1,SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt,3,status,-1,SQLITE_TRANSIENT);
    if (sqlite3_step(stmt) != SQLITE_DONE) {
	eprintf("SQL execution failed: %s",sqlite3_errmsg(db));
	return 1;
    }
    sqlite3_finalize(stmt);
    sqlite3_close(db);
    return 0;
}

int update_assignees_in_cob_db (Oid issue_id, SimpleSet* assignees) {
    sqlite3* db = 0;
    sqlite3_stmt* stmt = 0;
    const char* db_file = get_cob_cache_file();
    sqlite3_open(db_file,&db);
    if (!db) {
	eprintf("failed to open cob db");
	return 1;
    }
    const char* sql = "DELETE FROM Assignees WHERE Issue = ?;";
    if (sqlite3_prepare_v2(db,sql,-1,&stmt,0)) {
	eprintf("failed to prepare sql statement");
	return 1;
    }
    sqlite3_bind_blob(stmt,1,issue_id.id,20,SQLITE_TRANSIENT);
    if (sqlite3_step(stmt) != SQLITE_DONE) {
	eprintf("SQL execution failed: %s",sqlite3_errmsg(db));
	return 1;
    }
    sqlite3_finalize(stmt);
    sql = "INSERT INTO Assignees (DID, Issue) VALUES (?, ?);";
    if (sqlite3_prepare_v2(db,sql,-1,&stmt,0)) {
	eprintf("failed to prepare sql statement");
	return 1;
    }
    size_t n_assignees = 0;
    char** assignees_list = set_to_array(assignees,&n_assignees);
    for (size_t i=0; i<n_assignees; i++) {
	sqlite3_bind_text(stmt,1,assignees_list[i],-1,SQLITE_TRANSIENT);
	sqlite3_bind_blob(stmt,2,issue_id.id,20,SQLITE_TRANSIENT);
	if (sqlite3_step(stmt) != SQLITE_DONE) {
	    eprintf("SQL execution failed: %s",sqlite3_errmsg(db));
	    return 1;
	}
	if (sqlite3_reset(stmt) != SQLITE_OK) {
	    eprintf("SQL execution failed: %s",sqlite3_errmsg(db));
	    return 1;
	}
    }
    sqlite3_finalize(stmt);
    sqlite3_close(db);
    return 0;
}

int issue_open (char* title, char* desc) {
    char buf [HEXSIZ];
    rad_git_init();
    RadRepo rrepo = rad_repo_default();
    if (get_rad_repo_from_cwd(&rrepo)) {
	eprintf("failed to get rad repo from cwd");
	return 1;
    }
    Pubkey signer = profile_get_pubkey();    
    RepoEntry re = cob_issue_init(rrepo,signer,title,desc);
    if (git_oid_is_zero(&re.oid)) {
	eprintf("failed to open cob issue");
	return 1;
    }
    if (add_issue_to_cob_db(re.oid,pubkey_to_did(signer.bytes),"open")) {
	eprintf("failed to add issue to cob db");
	return 1;
    }
    iprintf("issue %s opened",git_oid_tostr(buf,HEXSIZ,&re.oid));
    return 0;
}

int issue_comment (Oid issue_id, size_t issue_id_hexlen, Oid reply_to, size_t reply_to_hexlen, char* message) {
    char buf [HEXSIZ];
    rad_git_init();
    RadRepo rrepo = rad_repo_default();
    if (get_rad_repo_from_cwd(&rrepo)) {
	eprintf("failed to get rad repo from cwd");
	return 1;
    }
    git_odb* odb = 0;
    if (git_repository_odb(&odb,rrepo.repo)) {
	eprintf("failed to get repository odb");
	return 1;
    }
    git_odb_object* odb_obj = 0;
    if (git_odb_read_prefix(&odb_obj,odb,&issue_id,issue_id_hexlen)) {
	eprintf("failed to read prefix from odb");
	return 1;
    }
    issue_id = *git_odb_object_id(odb_obj);
    if (reply_to_hexlen) {
	odb_obj = 0;
	if (git_odb_read_prefix(&odb_obj,odb,&reply_to,reply_to_hexlen)) {
	    eprintf("failed to read prefix from odb");
	    return 1;
	}
	reply_to = *git_odb_object_id(odb_obj);
    }
    else {
	reply_to = issue_id;
    }
    Pubkey signer = profile_get_pubkey();
    RepoEntry re = cob_issue_comment(rrepo,signer,issue_id,reply_to,message);
    if (git_oid_is_zero(&re.oid)) {
	eprintf("failed to comment on cob issue");
	return 1;
    }
    if (add_comment_to_cob_db(re.oid,issue_id,reply_to)) {
	eprintf("failed to add comment to cob db");
	return 1;
    }
    iprintf("comment %s created",git_oid_tostr(buf,HEXSIZ,&re.oid));
    return 0;
}

int issue_assign (Oid issue_id, size_t issue_id_hexlen, SimpleSet* add, SimpleSet* delete) {
    char buf [HEXSIZ];
    rad_git_init();
    RadRepo rrepo = rad_repo_default();
    if (get_rad_repo_from_cwd(&rrepo)) {
	eprintf("failed to get rad repo from cwd");
	return 1;
    }
    git_odb* odb = 0;
    if (git_repository_odb(&odb,rrepo.repo)) {
	eprintf("failed to get repository odb");
	return 1;
    }
    git_odb_object* odb_obj = 0;
    if (git_odb_read_prefix(&odb_obj,odb,&issue_id,issue_id_hexlen)) {
	eprintf("failed to read prefix from odb");
	return 1;
    }
    issue_id = *git_odb_object_id(odb_obj);
    Pubkey signer = profile_get_pubkey();

    // Get former assignees from cob db
    SimpleSet former_assignees;
    set_init(&former_assignees);
    if (get_assignees_from_cob_db(&former_assignees,issue_id)) {
	eprintf("failed to get assignees from cob db");
	return 1;
    }
    SimpleSet assignees1;
    set_init(&assignees1);
    if (set_difference(&assignees1,&former_assignees,delete)) {
	eprintf("failed to get difference between assignee sets");
	return 1;
    }
    SimpleSet assignees2;
    set_init(&assignees2);
    if (set_union(&assignees2,&assignees1,add)) {
	eprintf("failed to get union of assignee sets");
	return 1;
    }
    RepoEntry re = cob_issue_assign(rrepo,signer,issue_id,&assignees2);
    if (git_oid_is_zero(&re.oid)) {
	eprintf("failed to assign cob issue");
	return 1;
    }
    if (update_assignees_in_cob_db(issue_id,&assignees2)) {
	eprintf("failed to update assignees in cob db");
	return 1;
    }
    iprintf("assigned issue %s",git_oid_tostr(buf,HEXSIZ,&issue_id));
    return 0;    
}
