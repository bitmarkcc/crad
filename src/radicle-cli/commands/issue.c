#include <string.h>
#include <stdio.h>
#include <sqlite3.h>
#include <time.h>

#include <commands/issue.h>
#include <profile.h>
#include <print.h>
#include <git.h>
#include <cob.h>
#include <rad.h>

IssueCommand command_issue_default() {
    IssueCommand cmd;
    cmd.err = 0;
    cmd.title = 0;
    cmd.desc = 0;
    cmd.message = 0;
    Oid zero = {{0}};
    cmd.reply_to = zero;
    cmd.reply_to_hexlen = 0;
    cmd.issue_id = zero;
    cmd.issue_id_hexlen = 0;
    cmd.edit = zero;
    cmd.edit_hexlen = 0;
    set_init(&cmd.add);
    set_init(&cmd.delete);
    memset(cmd.emoji,0,4);
    IssueState state = {0};
    cmd.state = state;
    set_init(&cmd.assigned);
    return cmd;
}

void print_help_issue () {
    printf("crad issue (Manage issues) Usage:\n");
    printf("crad issue open [--title <title>] [--desc <text>]\n");
    printf("crad issue comment <issue-id> [--message <message>] [--reply-to <comment-id>] [--edit <comment-id>]\n");
    printf("crad issue assign <issue-id> [--add <did>] [--delete <did>]\n");
    printf("crad issue label <issue-id> [--add <label>] [--delete <label>]\n");
    printf("crad issue react <issue-id> [--emoji <char>] [--to <comment>]\n");
    printf("crad issue state <issue-id> [--closed --open --solved]\n");
    printf("crad issue delete <issue-id>\n");
    printf("crad issue edit <issue-id> [--title <title>] [--desc <text>]\n");
    printf("crad issue list [--assigned <did>] [--closed | --open | --solved]\n");
    printf("crad issue show <issue-id>\n");
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
	else if (!strcmp(argv[i],"--reply-to") || !strcmp(argv[i],"--to")) {
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
	else if (!strcmp(argv[i],"--edit")) {
	    if (i+1 >= argc) {
		eprintf("no parameter to --edit");
		cmd.err = 1;
		return cmd;
	    }
	    Oid edit = {{0}};
	    if (git_oid_fromstrp(&edit,argv[i+1])) {
		eprintf("failed to parse edit oid");
		cmd.err = 1;
		return cmd;
	    }
	    cmd.edit = edit;
	    cmd.edit_hexlen = strlen(argv[i+1]);
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
	else if (!strcmp(argv[i],"--emoji")) {
	    if (i+1 < argc) memcpy(cmd.emoji,argv[i+1],4);
	}
	else if (!strcmp(argv[i],"--closed")) {
	    //todo
	    cmd.state.reason = 0;
	    cmd.state.status = strdup("closed");
	}
	else if (!strcmp(argv[i],"--open")) {
	    //todo
	    cmd.state.reason = 0;
	    cmd.state.status = strdup("open");
	}
	else if (!strcmp(argv[i],"--solved")) {
	    cmd.state.reason = strdup("solved");
	    cmd.state.status = strdup("closed");
	}
	else if (!strcmp(argv[i],"--assigned")) {
	    if (i+1 >= argc) {
		eprintf("no parameter to --assigned");
		cmd.err = 1;
		return cmd;
	    }
	    set_add_str(&cmd.assigned,argv[i+1]);
	}
    }
    return cmd;
}

int issue_run (Command c) {
    if (c.type == CMD_HELP) {
	print_help_issue();
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
	return issue_comment(issue_id,issue_id_hexlen,cmd.reply_to,cmd.reply_to_hexlen,cmd.message,cmd.edit,cmd.edit_hexlen);
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
    else if (c.argc > 1 && !strcmp(c.argv[0],"label")) {
	IssueCommand cmd = parse_args_issue(c.argc,c.argv);
	if (cmd.err) return 1;
	Oid issue_id = {{0}};
	if (git_oid_fromstrp(&issue_id,c.argv[1])) {
	    eprintf("failed to parse issue id");
	    return 1;
	}
	size_t issue_id_hexlen = strlen(c.argv[1]);
	return issue_label(issue_id,issue_id_hexlen,&cmd.add,&cmd.delete);
    }
    else if (c.argc > 1 && !strcmp(c.argv[0],"react")) {
	IssueCommand cmd = parse_args_issue(c.argc,c.argv);
	if (cmd.err) return 1;
	Oid issue_id = {{0}};
	if (git_oid_fromstrp(&issue_id,c.argv[1])) {
	    eprintf("failed to parse issue id");
	    return 1;
	}
	size_t issue_id_hexlen = strlen(c.argv[1]);
	return issue_react(issue_id,issue_id_hexlen,cmd.reply_to,cmd.reply_to_hexlen,cmd.emoji);
    }
    else if (c.argc > 1 && !strcmp(c.argv[0],"state")) {
	IssueCommand cmd = parse_args_issue(c.argc,c.argv);
	if (cmd.err) return 1;
	Oid issue_id = {{0}};
	if (git_oid_fromstrp(&issue_id,c.argv[1])) {
	    eprintf("failed to parse issue id");
	    return 1;
	}
	size_t issue_id_hexlen = strlen(c.argv[1]);
	return issue_state(issue_id,issue_id_hexlen,cmd.state);
    }
    else if (c.argc > 1 && !strcmp(c.argv[0],"delete")) {
	Oid issue_id = {{0}};
	if (git_oid_fromstrp(&issue_id,c.argv[1])) {
	    eprintf("failed to parse issue id");
	    return 1;
	}
	size_t issue_id_hexlen = strlen(c.argv[1]);
	return issue_delete(issue_id,issue_id_hexlen);
    }
    else if (c.argc > 1 && !strcmp(c.argv[0],"edit")) {
	IssueCommand cmd = parse_args_issue(c.argc,c.argv);
	if (cmd.err) return 1;
	Oid issue_id = {{0}};
	if (git_oid_fromstrp(&issue_id,c.argv[1])) {
	    eprintf("failed to parse issue id");
	    return 1;
	}
	size_t issue_id_hexlen = strlen(c.argv[1]);
	return issue_edit(issue_id,issue_id_hexlen,cmd.title,cmd.desc);
    }
    else if (c.argc > 0 && !strcmp(c.argv[0],"list")) {
	IssueCommand cmd = parse_args_issue(c.argc,c.argv);
	if (cmd.err) return 1;
	return issue_list(&cmd.assigned,cmd.state);
    }
    else if (c.argc > 1 && !strcmp(c.argv[0],"show")) {
	Oid issue_id = {{0}};
	if (git_oid_fromstrp(&issue_id,c.argv[1])) {
	    eprintf("failed to parse issue id");
	    return 1;
	}
	size_t issue_id_hexlen = strlen(c.argv[1]);
	return issue_show(issue_id,issue_id_hexlen);
    }
    else if (c.argc > 0) {
	IssueCommand cmd = parse_args_issue(c.argc,c.argv);
	if (cmd.err) {
	    return 1;
	}
    }
    return 0;
}

int add_comment_to_cob_db (Oid comment_id, Oid issue_id, Oid reply_to, uint64_t commit_time, const char* author, const char* alias) {    
    sqlite3* db = 0;
    sqlite3_stmt* stmt = 0;
    const char* db_file = get_cob_cache_file();
    sqlite3_open(db_file,&db);
    if (!db) {
	eprintf("failed to open cob db");
	return 1;
    }
    const char* sql = "INSERT INTO Comments (ID, EditID, Time, Issue, ReplyTo, Author, Alias) VALUES (?, ?, ?, ?, ?, ?, ?);";
    if (sqlite3_prepare_v2(db,sql,-1,&stmt,0)) {
	eprintf("failed to prepare sql statement");
	return 1;
    }
    sqlite3_bind_blob(stmt,1,comment_id.id,20,SQLITE_TRANSIENT);
    sqlite3_bind_blob(stmt,2,comment_id.id,20,SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt,3,commit_time);
    sqlite3_bind_blob(stmt,4,issue_id.id,20,SQLITE_TRANSIENT);
    sqlite3_bind_blob(stmt,5,reply_to.id,20,SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt,6,author,-1,SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt,7,alias,-1,SQLITE_TRANSIENT);
    if (sqlite3_step(stmt) != SQLITE_DONE) {
	eprintf("SQL execution failed: %s",sqlite3_errmsg(db));
	return 1;
    }
    sqlite3_finalize(stmt);
    sql = "UPDATE Issues SET EntryID = ? WHERE ID = ?;";
    if (sqlite3_prepare_v2(db,sql,-1,&stmt,0)) {
	eprintf("failed to prepare sql statement");
	return 1;
    }
    sqlite3_bind_blob(stmt,1,comment_id.id,20,SQLITE_TRANSIENT);
    sqlite3_bind_blob(stmt,2,issue_id.id,20,SQLITE_TRANSIENT);
    if (sqlite3_step(stmt) != SQLITE_DONE) {
	eprintf("SQL execution failed: %s",sqlite3_errmsg(db));
	return 1;
    }
    sqlite3_finalize(stmt);
    sqlite3_close(db);
    return 0;
}

int add_reaction_to_cob_db (Oid reaction_id, Oid issue_id, Oid reply_to) {
    sqlite3* db = 0;
    sqlite3_stmt* stmt = 0;
    const char* db_file = get_cob_cache_file();
    sqlite3_open(db_file,&db);
    if (!db) {
	eprintf("failed to open cob db");
	return 1;
    }
    const char* sql = "INSERT INTO Reactions (ID, Time, Issue, ReplyTo) VALUES (?, ?, ?, ?);";
    if (sqlite3_prepare_v2(db,sql,-1,&stmt,0)) {
	eprintf("failed to prepare sql statement");
	return 1;
    }
    sqlite3_bind_blob(stmt,1,reaction_id.id,20,SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt,2,time(0));
    sqlite3_bind_blob(stmt,3,issue_id.id,20,SQLITE_TRANSIENT);
    sqlite3_bind_blob(stmt,4,reply_to.id,20,SQLITE_TRANSIENT);
    if (sqlite3_step(stmt) != SQLITE_DONE) {
	eprintf("SQL execution failed: %s",sqlite3_errmsg(db));
	return 1;
    }
    sqlite3_finalize(stmt);
    sql = "UPDATE Issues SET EntryID = ? WHERE ID = ?;";
    if (sqlite3_prepare_v2(db,sql,-1,&stmt,0)) {
	eprintf("failed to prepare sql statement");
	return 1;
    }
    sqlite3_bind_blob(stmt,1,reaction_id.id,20,SQLITE_TRANSIENT);
    sqlite3_bind_blob(stmt,2,issue_id.id,20,SQLITE_TRANSIENT);
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

int get_labels_from_cob_db (SimpleSet* labels, Oid issue_id) {
    sqlite3* db = 0;
    sqlite3_stmt* stmt = 0;
    const char* db_file = get_cob_cache_file();
    sqlite3_open(db_file,&db);
    if (!db) {
	eprintf("failed to open cob db");
	return 1;
    }
    const char* sql = "SELECT Label FROM Labels WHERE Issue = ?;";
    if (sqlite3_prepare_v2(db,sql,-1,&stmt,0)) {
	eprintf("failed to prepare sql statement");
	return 1;
    }
    sqlite3_bind_blob(stmt,1,issue_id.id,20,SQLITE_TRANSIENT);
    int ret = 0;
    while (1) {
	ret = sqlite3_step(stmt);
	if (ret == SQLITE_ROW) set_add_str(labels,sqlite3_column_text(stmt,0));
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

int add_issue_to_cob_db (Oid issue_id, Oid repo_oid, uint64_t commit_time, const char* author, const char* status) {
    sqlite3* db = 0;
    sqlite3_stmt* stmt = 0;
    const char* db_file = get_cob_cache_file();
    sqlite3_open(db_file,&db);
    if (!db) {
	eprintf("failed to open cob db");
	return 1;
    }
    const char* sql = "INSERT INTO Issues (ID, RID, EntryID, EditID, Time, Author, Status) VALUES (?, ?, ?, ?, ?, ?, ?);";
    if (sqlite3_prepare_v2(db,sql,-1,&stmt,0)) {
	eprintf("failed to prepare sql statement");
	return 1;
    }
    sqlite3_bind_blob(stmt,1,issue_id.id,20,SQLITE_TRANSIENT);
    sqlite3_bind_blob(stmt,2,repo_oid.id,20,SQLITE_TRANSIENT);
    sqlite3_bind_blob(stmt,3,issue_id.id,20,SQLITE_TRANSIENT);
    sqlite3_bind_blob(stmt,4,issue_id.id,20,SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt,5,commit_time);
    sqlite3_bind_text(stmt,6,author,-1,SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt,7,status,-1,SQLITE_TRANSIENT);
    if (sqlite3_step(stmt) != SQLITE_DONE) {
	eprintf("SQL execution failed: %s",sqlite3_errmsg(db));
	return 1;
    }
    sqlite3_finalize(stmt);
    sqlite3_close(db);
    return 0;
}

int edit_issue_in_cob_db (Oid issue_id, Oid edit_id) {
    sqlite3* db = 0;
    sqlite3_stmt* stmt = 0;
    const char* db_file = get_cob_cache_file();
    sqlite3_open(db_file,&db);
    if (!db) {
	eprintf("failed to open cob db");
	return 1;
    }
    const char* sql = "UPDATE Issues SET EntryID = ?, EditID = ? WHERE ID = ?;";
    if (sqlite3_prepare_v2(db,sql,-1,&stmt,0)) {
	eprintf("failed to prepare sql statement");
	return 1;
    }
    sqlite3_bind_blob(stmt,1,edit_id.id,20,SQLITE_TRANSIENT);
    sqlite3_bind_blob(stmt,2,edit_id.id,20,SQLITE_TRANSIENT);
    sqlite3_bind_blob(stmt,3,issue_id.id,20,SQLITE_TRANSIENT);
    if (sqlite3_step(stmt) != SQLITE_DONE) {
	eprintf("SQL execution failed: %s",sqlite3_errmsg(db));
	return 1;
    }
    sqlite3_finalize(stmt);
    sqlite3_close(db);
    return 0;
}

int edit_comment_in_cob_db (Oid comment_id, Oid edit_id, uint64_t commit_time) {
    sqlite3* db = 0;
    sqlite3_stmt* stmt = 0;
    const char* db_file = get_cob_cache_file();
    sqlite3_open(db_file,&db);
    if (!db) {
	eprintf("failed to open cob db");
	return 1;
    }
    const char* sql = "UPDATE Comments SET EditID = ? WHERE ID = ?;";
    if (sqlite3_prepare_v2(db,sql,-1,&stmt,0)) {
	eprintf("failed to prepare sql statement");
	return 1;
    }
    sqlite3_bind_blob(stmt,1,edit_id.id,20,SQLITE_TRANSIENT);
    sqlite3_bind_blob(stmt,2,comment_id.id,20,SQLITE_TRANSIENT);
    if (sqlite3_step(stmt) != SQLITE_DONE) {
	eprintf("SQL execution failed: %s",sqlite3_errmsg(db));
	return 1;
    }
    sqlite3_finalize(stmt);

    Oid issue_id = {{0}};
    sql = "SELECT Issue FROM Comments WHERE ID = ?;";
    if (sqlite3_prepare_v2(db,sql,-1,&stmt,0)) {
	eprintf("failed to prepare sql statement");
	return 1;
    }
    sqlite3_bind_blob(stmt,1,comment_id.id,20,SQLITE_TRANSIENT);
    int ret = 0;
    while (1) {
	ret = sqlite3_step(stmt);
	if (ret == SQLITE_ROW) memcpy(issue_id.id,sqlite3_column_blob(stmt,0),20);
	else break;
    }
    if (ret != SQLITE_DONE) {
	eprintf("SQL execution failed: %s",sqlite3_errmsg(db));
	return 1;
    }
    sqlite3_finalize(stmt);
    
    sql = "UPDATE Issues SET EntryID = ? WHERE ID = ?;";
    if (sqlite3_prepare_v2(db,sql,-1,&stmt,0)) {
	eprintf("failed to prepare sql statement");
	return 1;
    }
    sqlite3_bind_blob(stmt,1,edit_id.id,20,SQLITE_TRANSIENT);
    sqlite3_bind_blob(stmt,2,issue_id.id,20,SQLITE_TRANSIENT);
    if (sqlite3_step(stmt) != SQLITE_DONE) {
	eprintf("SQL execution failed: %s",sqlite3_errmsg(db));
	return 1;
    }
    sqlite3_finalize(stmt);
    sqlite3_close(db);
    return 0;
}

int delete_issue_from_cob_db (Oid issue_id) {
    sqlite3* db = 0;
    sqlite3_stmt* stmt = 0;
    const char* db_file = get_cob_cache_file();
    sqlite3_open(db_file,&db);
    if (!db) {
	eprintf("failed to open cob db");
	return 1;
    }
    const char* sql = "DELETE FROM Issues WHERE id = ?;";
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
    sqlite3_close(db);
    return 0;
}

int update_assignees_in_cob_db (Oid issue_id, SimpleSet* assignees, Oid entry_id) {
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
    sql = "UPDATE Issues SET EntryID = ? WHERE ID = ?;";
    if (sqlite3_prepare_v2(db,sql,-1,&stmt,0)) {
	eprintf("failed to prepare sql statement");
	return 1;
    }
    sqlite3_bind_blob(stmt,1,entry_id.id,20,SQLITE_TRANSIENT);
    sqlite3_bind_blob(stmt,2,issue_id.id,20,SQLITE_TRANSIENT);
    if (sqlite3_step(stmt) != SQLITE_DONE) {
	eprintf("SQL execution failed: %s",sqlite3_errmsg(db));
	return 1;
    }
    sqlite3_finalize(stmt);
    sqlite3_close(db);
    return 0;
}

int update_labels_in_cob_db (Oid issue_id, SimpleSet* labels, Oid entry_id) {
    sqlite3* db = 0;
    sqlite3_stmt* stmt = 0;
    const char* db_file = get_cob_cache_file();
    sqlite3_open(db_file,&db);
    if (!db) {
	eprintf("failed to open cob db");
	return 1;
    }
    const char* sql = "DELETE FROM Labels WHERE Issue = ?;";
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
    sql = "INSERT INTO Labels (Label, Issue) VALUES (?, ?);";
    if (sqlite3_prepare_v2(db,sql,-1,&stmt,0)) {
	eprintf("failed to prepare sql statement");
	return 1;
    }
    size_t n_labels = 0;
    char** labels_list = set_to_array(labels,&n_labels);
    for (size_t i=0; i<n_labels; i++) {
	sqlite3_bind_text(stmt,1,labels_list[i],-1,SQLITE_TRANSIENT);
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
    sql = "UPDATE Issues SET EntryID = ? WHERE ID = ?;";
    if (sqlite3_prepare_v2(db,sql,-1,&stmt,0)) {
	eprintf("failed to prepare sql statement");
	return 1;
    }
    sqlite3_bind_blob(stmt,1,entry_id.id,20,SQLITE_TRANSIENT);
    sqlite3_bind_blob(stmt,2,issue_id.id,20,SQLITE_TRANSIENT);
    if (sqlite3_step(stmt) != SQLITE_DONE) {
	eprintf("SQL execution failed: %s",sqlite3_errmsg(db));
	return 1;
    }
    sqlite3_finalize(stmt);
    sqlite3_close(db);
    return 0;
}

int update_state_in_cob_db (Oid issue_id, IssueState state, Oid entry_id) {
    sqlite3* db = 0;
    sqlite3_stmt* stmt = 0;
    const char* db_file = get_cob_cache_file();
    sqlite3_open(db_file,&db);
    if (!db) {
	eprintf("failed to open cob db");
	return 1;
    }
    const char* sql = "UPDATE Issues SET Status = ?, Reason = ?, EntryID = ? WHERE ID = ?;";
    if (sqlite3_prepare_v2(db,sql,-1,&stmt,0)) {
	eprintf("failed to prepare sql statement");
	return 1;
    }
    sqlite3_bind_text(stmt,1,state.status,-1,SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt,2,state.reason,-1,SQLITE_TRANSIENT);
    sqlite3_bind_blob(stmt,3,entry_id.id,20,SQLITE_TRANSIENT);
    sqlite3_bind_blob(stmt,4,issue_id.id,20,SQLITE_TRANSIENT);
    if (sqlite3_step(stmt) != SQLITE_DONE) {
	eprintf("SQL execution failed: %s",sqlite3_errmsg(db));
	return 1;
    }
    sqlite3_finalize(stmt);
    sqlite3_close(db);
    return 0;
}

int update_issue_in_cob_db (Oid issue_entry, RadRepo rrepo) {
    char buf [HEXSIZ];
    if (issue_entry_in_cob_db(issue_entry)) return 0;

    Oid entry = issue_entry;
    size_t entries_to_apply_capacity = 16;
    Oid* entries_to_apply = malloc(entries_to_apply_capacity*sizeof(Oid));
    size_t n_entries_to_apply = 0;
    bool passed_entry_in_cob_db = false;
    Oid issue_id = {{0}};

    do {
	if (!passed_entry_in_cob_db) {
	    if (n_entries_to_apply>=entries_to_apply_capacity) {
		entries_to_apply_capacity *= 2;
		entries_to_apply = realloc(entries_to_apply,entries_to_apply_capacity*sizeof(Oid));
	    }
	    entries_to_apply[n_entries_to_apply] = entry;
	    n_entries_to_apply++;
	}
	git_commit* commit = 0;
	if (git_commit_lookup(&commit,rrepo.repo,&entry)) {
	    eprintf("failed to lookup git commit");
	    return 1;
	}
	unsigned int parent_count = git_commit_parentcount(commit);
	if (parent_count < 2) {
	    issue_id = entry;
	    break;
	}
	const Oid* pparent_oid = git_commit_parent_id(commit,0);
	if (!pparent_oid) {
	    eprintf("failed to get parent oid of an issue entry");
	    return 1;
	}
	entry = *pparent_oid;
	if (!passed_entry_in_cob_db && issue_entry_in_cob_db(entry)) {
	    passed_entry_in_cob_db = true;
	}
    } while (1);
    
    for (int i=n_entries_to_apply-1; i>=0; i--) {
	Oid entry_id = entries_to_apply[i];
	git_commit* commit = 0;
	if (git_commit_lookup(&commit,rrepo.repo,&entry_id)) {
	    eprintf("failed to lookup git commit");
	    return 1;
	}
	unsigned int parent_count = git_commit_parentcount(commit);
	if (parent_count < 2) { // new issue
	    const git_signature* git_author = git_commit_author(commit);
	    char author [128];
	    sprintf(author,"did:key:%s",rad_email_get_domain(git_author->email));
	    if (add_issue_to_cob_db(entry_id,rrepo.rid,git_commit_time(commit),author,"open")) {
		eprintf("failed to add issue to cob db");
		return 1;
	    }
	}
	else {	
	    git_tree* tree = 0;
	    if (git_commit_tree(&tree,commit)) {
		eprintf("failed to get tree from git commit");
		return 1;
	    }
	    git_tree_entry* tree_entry = 0;
	    if (git_tree_entry_bypath(&tree_entry,tree,"0")) {
		eprintf("Can't find the git tree entry 0");
		return 1;
	    }
	    const Oid* poid_0 = git_tree_entry_id(tree_entry);
	    if (!poid_0) {
		eprintf("Can't find oid of git tree entry");
		return 1;
	    }
	    git_blob* blob = 0;
	    if (git_blob_lookup(&blob,rrepo.repo,poid_0)) {
		eprintf("can't lookup blob corresponding to git oid");
		return 1;
	    }
	    const uint8_t* blob_content = git_blob_rawcontent(blob);
	    //iprintf("blob_content %s",(char*)blob_content);
	    json_object* content_0 = json_tokener_parse((char*)blob_content);	    
	    json_object* val_type = 0;
	    json_object_object_get_ex(content_0,"type",&val_type);
	    const char* type = json_object_get_string(val_type);
	    
	    if (!strcmp(type,"comment")) {
		json_object* val_reply_to = 0;
		json_object_object_get_ex(content_0,"reply_to",&val_reply_to);
		Oid reply_to = {{0}};
		if (git_oid_fromstr(&reply_to,json_object_get_string(val_reply_to))) {
		    eprintf("failed to convert string to git oid");
		    return 1;
		}
		const git_signature* git_author = git_commit_author(commit);
		char author [128];
		sprintf(author,"did:key:%s",rad_email_get_domain(git_author->email));
		const char* alias = rad_email_get_user(git_author->email);
		if (add_comment_to_cob_db(entry_id,issue_id,reply_to,git_commit_time(commit),author,alias)) {
		    eprintf("failed to add comment to cob db");
		    return 1;
		}
	    }
	    else if (!strcmp(type,"assign")) {
		json_object* val_assignees = 0;
		json_object_object_get_ex(content_0,"assignees",&val_assignees);
		size_t n_assignees = json_object_array_length(val_assignees);
		SimpleSet assignees;
		set_init(&assignees);
		for (size_t j=0; j<n_assignees; j++)
		    set_add_str(&assignees,json_object_get_string(json_object_array_get_idx(val_assignees,j)));
		if (update_assignees_in_cob_db(issue_id,&assignees,entry_id)) {
		    eprintf("failed to update assignees in cob db");
		    return 1;
		}
	    }
	    else if (!strcmp(type,"label")) {
		json_object* val_labels = 0;
		json_object_object_get_ex(content_0,"labels",&val_labels);
		size_t n_labels = json_object_array_length(val_labels);
		SimpleSet labels;
		set_init(&labels);
		for (size_t j=0; j<n_labels; j++)
		    set_add_str(&labels,json_object_get_string(json_object_array_get_idx(val_labels,j)));
		if (update_labels_in_cob_db(issue_id,&labels,entry_id)) {
		    eprintf("failed to update labels in cob db");
		    return 1;
		}
	    }
	    else if (!strcmp(type,"comment.react")) {
		json_object* val_reply_to = 0;
		json_object_object_get_ex(content_0,"id",&val_reply_to);
		Oid reply_to = {{0}};
		if (git_oid_fromstr(&reply_to,json_object_get_string(val_reply_to))) {
		    eprintf("failed to convert string to git oid");
		    return 1;
		}
		if (add_reaction_to_cob_db(entry_id,issue_id,reply_to)) {
		    eprintf("failed to add react to cob db");
		    return 1;
		}
	    }
	    else if (!strcmp(type,"lifecycle")) {
		json_object* val_state = 0;
		json_object_object_get_ex(content_0,"state",&val_state);
		json_object* val_reason = 0;
		json_object_object_get_ex(val_state,"reason",&val_reason);
		json_object* val_status = 0;
		json_object_object_get_ex(val_state,"status",&val_status);
		const char* reason = json_object_get_string(val_reason);
		const char* status = json_object_get_string(val_status);
		IssueState state;
		state.reason = strdup(reason);
		state.status = strdup(status);
		if (update_state_in_cob_db(issue_id,state,entry_id)) {
		    eprintf("failed to update state in cob db");
		    return 1;
		}
	    }
	    else if (!strcmp(type,"edit")) {
		if (edit_issue_in_cob_db(issue_id,entry_id)) {
		    eprintf("failed to edit issue in cob db");
		    return 1;
		}
	    }
	    else if (!strcmp(type,"comment.edit")) {
		json_object* val_edit = 0;
		json_object_object_get_ex(content_0,"id",&val_edit);
		Oid edit = {{0}};
		if (git_oid_fromstr(&edit,json_object_get_string(val_edit))) {
		    eprintf("failed to convert string to git oid");
		    return 1;
		}
		if (edit_comment_in_cob_db(edit,entry_id,git_commit_time(commit))) {
		    eprintf("failed to edit comment in cob db");
		    return 1;
		}
	    }
	    /*else if () {
		if (delete_issue_from_cob_db(issue_id)) {
		    eprintf("failed to delete issue from cob db");
		    return 1;
		}
		}*/
	}
    }
  
    // make issue ref point to the entry ??
    return 0;
}

bool issue_entry_in_cob_db (Oid issue_entry) {
    char buf [HEXSIZ];
    sqlite3* db = 0;
    sqlite3_stmt* stmt = 0;
    const char* db_file = get_cob_cache_file();
    sqlite3_open(db_file,&db);
    if (!db) {
	eprintf("failed to open cob db");
	return 1;
    }
    const char* sql = "SELECT * FROM Issues WHERE EntryID = ?;";
    if (sqlite3_prepare_v2(db,sql,-1,&stmt,0)) {
	eprintf("failed to prepare sql statement");
	return 1;
    }
    sqlite3_bind_blob(stmt,1,issue_entry.id,20,SQLITE_TRANSIENT);
    int ret = 0;
    bool match = false;
    while (1) {
	ret = sqlite3_step(stmt);
	if (ret == SQLITE_ROW) match = true;
	else break;
    }
    if (ret != SQLITE_DONE) {
	eprintf("SQL execution failed: %s",sqlite3_errmsg(db));
	return 1;
    }
    sqlite3_finalize(stmt);
    sqlite3_close(db);
    return match;
}

Oid get_issue_id (Oid entry_id) {
    Oid zero = {{0}};
    sqlite3* db = 0;
    sqlite3_stmt* stmt = 0;
    const char* db_file = get_cob_cache_file();
    sqlite3_open(db_file,&db);
    if (!db) {
	eprintf("failed to open cob db");
	return zero;
    }
    const char* sql = "SELECT ID FROM Issues WHERE EntryID = ?;";
    if (sqlite3_prepare_v2(db,sql,-1,&stmt,0)) {
	eprintf("failed to prepare sql statement");
	return zero;
    }
    sqlite3_bind_blob(stmt,1,entry_id.id,20,SQLITE_TRANSIENT);
    int ret = 0;
    Oid issue_id = {{0}};
    while (1) {
	ret = sqlite3_step(stmt);
	if (ret == SQLITE_ROW) memcpy(issue_id.id,sqlite3_column_blob(stmt,0),20);
	else break;
    }
    if (ret != SQLITE_DONE) {
	eprintf("SQL execution failed: %s",sqlite3_errmsg(db));
	return zero;
    }
    sqlite3_finalize(stmt);
    sqlite3_close(db);
    return issue_id;
}

char* get_issue_title (RadRepo rrepo, Oid issue_id) {
    Oid edit_id = {{0}};
    sqlite3* db = 0;
    sqlite3_stmt* stmt = 0;
    const char* db_file = get_cob_cache_file();
    sqlite3_open(db_file,&db);
    if (!db) {
	eprintf("failed to open cob db");
	return 0;
    }
    const char* sql = "SELECT EditID FROM Issues WHERE ID = ?;";
    if (sqlite3_prepare_v2(db,sql,-1,&stmt,0)) {
	eprintf("failed to prepare sql statement");
	return 0;
    }
    sqlite3_bind_blob(stmt,1,issue_id.id,20,SQLITE_TRANSIENT);
    int ret = 0;
    while (1) {
	ret = sqlite3_step(stmt);
	if (ret == SQLITE_ROW) memcpy(edit_id.id,sqlite3_column_blob(stmt,0),20);
	else break;
    }
    if (ret != SQLITE_DONE) {
	eprintf("SQL execution failed: %s",sqlite3_errmsg(db));
	return 0;
    }
    sqlite3_finalize(stmt);
    sqlite3_close(db);
    git_commit* commit = 0;
    if (git_commit_lookup(&commit,rrepo.repo,&edit_id)) {
	eprintf("failed to lookup git commit");
	return 0;
    }
    git_tree* tree = 0;
    if (git_commit_tree(&tree,commit)) {
	eprintf("failed to get tree from git commit");
	return 0;
    }
    git_tree_entry* tree_entry = 0;
    if (git_tree_entry_bypath(&tree_entry,tree,"0")) {
	eprintf("Can't find the git tree entry 0");
	return 0;
    }
    const Oid* poid = git_tree_entry_id(tree_entry);
    if (!poid) {
	eprintf("Can't find oid of git tree entry");
	return 0;
    }
    git_blob* blob = 0;
    if (git_blob_lookup(&blob,rrepo.repo,poid)) {
	eprintf("can't lookup blob corresponding to git oid");
	return 0;
    }
    const uint8_t* blob_content = git_blob_rawcontent(blob);
    json_object* content_0 = json_tokener_parse((char*)blob_content);
    tree_entry = 0;
    if (git_tree_entry_bypath(&tree_entry,tree,"1")) {
	eprintf("Can't find the git tree entry 1");
	return 0;
    }
    poid = git_tree_entry_id(tree_entry);
    if (!poid) {
	eprintf("Can't find oid of git tree entry");
	return 0;
    }
    blob = 0;
    if (git_blob_lookup(&blob,rrepo.repo,poid)) {
	eprintf("can't lookup blob corresponding to git oid");
	return 0;
    }
    blob_content = git_blob_rawcontent(blob);
    json_object* content_1 = json_tokener_parse((char*)blob_content);
    json_object* val_type = 0;
    json_object_object_get_ex(content_0,"type",&val_type);
    const char* type = json_object_get_string(val_type);
    json_object* content = 0;
    if (!strcmp(type,"edit")) {
	content = content_0;
    }
    else {
	val_type = 0;
	json_object_object_get_ex(content_1,"type",&val_type);
	type = json_object_get_string(val_type);
	if (!strcmp(type,"edit")) {
	    content = content_1;
	}
	else {
	    eprintf("can't find edit type");
	    return 0;
	}
    }
    json_object* val_title = 0;
    json_object_object_get_ex(content,"title",&val_title);
    const char* title = json_object_get_string(val_title);
    char* title_ret = malloc(strlen(title)+1);
    strcpy(title_ret,title);
    return title_ret;
}

char* get_issue_description (RadRepo rrepo, Oid issue_id) {
    Oid edit_id = {{0}};
    sqlite3* db = 0;
    sqlite3_stmt* stmt = 0;
    const char* db_file = get_cob_cache_file();
    sqlite3_open(db_file,&db);
    if (!db) {
	eprintf("failed to open cob db");
	return 0;
    }
    const char* sql = "SELECT EditID FROM Issues WHERE ID = ?;";
    if (sqlite3_prepare_v2(db,sql,-1,&stmt,0)) {
	eprintf("failed to prepare sql statement");
	return 0;
    }
    sqlite3_bind_blob(stmt,1,issue_id.id,20,SQLITE_TRANSIENT);
    int ret = 0;
    while (1) {
	ret = sqlite3_step(stmt);
	if (ret == SQLITE_ROW) memcpy(edit_id.id,sqlite3_column_blob(stmt,0),20);
	else break;
    }
    if (ret != SQLITE_DONE) {
	eprintf("SQL execution failed: %s",sqlite3_errmsg(db));
	return 0;
    }
    sqlite3_finalize(stmt);
    sqlite3_close(db);
    git_commit* commit = 0;
    if (git_commit_lookup(&commit,rrepo.repo,&edit_id)) {
	eprintf("failed to lookup git commit");
	return 0;
    }
    git_tree* tree = 0;
    if (git_commit_tree(&tree,commit)) {
	eprintf("failed to get tree from git commit");
	return 0;
    }
    git_tree_entry* tree_entry = 0;
    if (git_tree_entry_bypath(&tree_entry,tree,"0")) {
	eprintf("Can't find the git tree entry 0");
	return 0;
    }
    const Oid* poid = git_tree_entry_id(tree_entry);
    if (!poid) {
	eprintf("Can't find oid of git tree entry");
	return 0;
    }
    git_blob* blob = 0;
    if (git_blob_lookup(&blob,rrepo.repo,poid)) {
	eprintf("can't lookup blob corresponding to git oid");
	return 0;
    }
    const uint8_t* blob_content = git_blob_rawcontent(blob);
    json_object* content_0 = json_tokener_parse((char*)blob_content);
    tree_entry = 0;
    if (git_tree_entry_bypath(&tree_entry,tree,"1")) {
	eprintf("Can't find the git tree entry 1");
	return 0;
    }
    poid = git_tree_entry_id(tree_entry);
    if (!poid) {
	eprintf("Can't find oid of git tree entry");
	return 0;
    }
    blob = 0;
    if (git_blob_lookup(&blob,rrepo.repo,poid)) {
	eprintf("can't lookup blob corresponding to git oid");
	return 0;
    }
    blob_content = git_blob_rawcontent(blob);
    json_object* content_1 = json_tokener_parse((char*)blob_content);
    json_object* val_type = 0;
    json_object_object_get_ex(content_0,"type",&val_type);
    const char* type = json_object_get_string(val_type);
    json_object* content = 0;
    if (!strcmp(type,"comment") || !strcmp(type,"comment.edit")) {
	content = content_0;
    }
    else {
	val_type = 0;
	json_object_object_get_ex(content_1,"type",&val_type);
	type = json_object_get_string(val_type);
	if (!strcmp(type,"comment") || !strcmp(type,"comment.edit")) {
	    content = content_1;
	}
	else {
	    eprintf("can't find edit type");
	    return 0;
	}
    }
    json_object* val_desc = 0;
    json_object_object_get_ex(content,"body",&val_desc);
    const char* desc = json_object_get_string(val_desc);
    char* desc_ret = malloc(strlen(desc)+1);
    strcpy(desc_ret,desc);
    return desc_ret;
}

char* get_issue_author (RadRepo rrepo, Oid issue_id) {
    sqlite3* db = 0;
    sqlite3_stmt* stmt = 0;
    const char* db_file = get_cob_cache_file();
    sqlite3_open(db_file,&db);
    if (!db) {
	eprintf("failed to open cob db");
	return 0;
    }
    const char* sql = "SELECT Author FROM Issues WHERE ID = ?;";
    if (sqlite3_prepare_v2(db,sql,-1,&stmt,0)) {
	eprintf("failed to prepare sql statement");
	return 0;
    }
    sqlite3_bind_blob(stmt,1,issue_id.id,20,SQLITE_TRANSIENT);
    int ret = 0;
    char* author = 0;
    while (1) {
	ret = sqlite3_step(stmt);
	if (ret == SQLITE_ROW) author = strdup(sqlite3_column_text(stmt,0));
	else break;
    }
    if (ret != SQLITE_DONE) {
	eprintf("SQL execution failed: %s",sqlite3_errmsg(db));
	return 0;
    }
    sqlite3_finalize(stmt);
    sqlite3_close(db);
    return author;
}

int get_issue_labels (SimpleSet* labels, Oid issue_id) {
    sqlite3* db = 0;
    sqlite3_stmt* stmt = 0;
    const char* db_file = get_cob_cache_file();
    sqlite3_open(db_file,&db);
    if (!db) {
	eprintf("failed to open cob db");
	return 1;
    }
    const char* sql = "SELECT Label FROM Labels WHERE Issue = ?;";
    if (sqlite3_prepare_v2(db,sql,-1,&stmt,0)) {
	eprintf("failed to prepare sql statement");
	return 1;
    }
    sqlite3_bind_blob(stmt,1,issue_id.id,20,SQLITE_TRANSIENT);
    int ret = 0;
    while (1) {
	ret = sqlite3_step(stmt);
	if (ret == SQLITE_ROW) set_add_str(labels,sqlite3_column_text(stmt,0));
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

int get_issue_assignees (SimpleSet* assignees, Oid issue_id) {
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

int get_issue_opening_time (RadRepo rrepo, Oid issue_id) {
    git_commit* commit = 0;
    if (git_commit_lookup(&commit,rrepo.repo,&issue_id)) {
	eprintf("failed to lookup git commit");
	return -1;
    }
    return git_commit_time(commit);
}

IssueState get_issue_state (Oid issue_id) {
    IssueState state = {0};
    sqlite3* db = 0;
    sqlite3_stmt* stmt = 0;
    const char* db_file = get_cob_cache_file();
    sqlite3_open(db_file,&db);
    if (!db) {
	eprintf("failed to open cob db");
	return state;
    }
    const char* sql = "SELECT Status, Reason FROM Issues WHERE ID = ?;";
    if (sqlite3_prepare_v2(db,sql,-1,&stmt,0)) {
	eprintf("failed to prepare sql statement");
	return state;
    }
    sqlite3_bind_blob(stmt,1,issue_id.id,20,SQLITE_TRANSIENT);
    int ret = 0;
    char* status = 0;
    char* reason = 0;
    while (1) {
	ret = sqlite3_step(stmt);
	if (ret == SQLITE_ROW) {
	    const char* col = sqlite3_column_text(stmt,0);
	    if (col)
		status = strdup(col);
	    col = sqlite3_column_text(stmt,1);
	    if (col)
		reason = strdup(col);
	}
	else break;
    }
    if (ret != SQLITE_DONE) {
	eprintf("SQL execution failed: %s",sqlite3_errmsg(db));
	return state;
    }
    sqlite3_finalize(stmt);
    sqlite3_close(db);
    state.status = status;
    state.reason = reason;
    return state;
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
    git_commit* commit = 0;
    if (git_commit_lookup(&commit,rrepo.repo,&re.oid)) {
	eprintf("failed to lookup git commit");
	return 1;
    }
    git_time_t commit_time = git_commit_time(commit);
    if (add_issue_to_cob_db(re.oid,rrepo.rid,commit_time,pubkey_to_did(signer.bytes),"open")) {
	eprintf("failed to add issue to cob db");
	return 1;
    }
    iprintf("issue %s opened",git_oid_tostr(buf,HEXSIZ,&re.oid));
    return 0;
}

int issue_comment (Oid issue_id, size_t issue_id_hexlen, Oid reply_to, size_t reply_to_hexlen, char* message, Oid edit, size_t edit_hexlen) {
    char buf [HEXSIZ];
    Oid zero = {{0}};
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
    if (edit_hexlen) {
	odb_obj = 0;
	if (git_odb_read_prefix(&odb_obj,odb,&edit,edit_hexlen)) {
	    eprintf("failed to read prefix from odb");
	    return 1;
	}
	edit = *git_odb_object_id(odb_obj);
    }
    else {
	edit = zero;
    }
    Pubkey signer = profile_get_pubkey();
    RepoEntry re = cob_issue_comment(rrepo,signer,issue_id,reply_to,message,edit);
    if (git_oid_is_zero(&re.oid)) {
	eprintf("failed to comment on cob issue");
	return 1;
    }
    // get commit time
    git_commit* commit = 0;
    if (git_commit_lookup(&commit,rrepo.repo,&re.oid)) {
	eprintf("failed to lookup git commit");
	return 1;
    }
    git_time_t commit_time = git_commit_time(commit);
    //int time_offset = git_commit_time_offset(commit); //todo: not needed?
    const git_signature* git_author = git_commit_author(commit);
    char author [128];
    sprintf(author,"did:key:%s",rad_email_get_domain(git_author->email));
    const char* alias = rad_email_get_user(git_author->email);
    if (edit_hexlen) {
	if (edit_comment_in_cob_db(edit,re.oid,commit_time)) {
	    eprintf("failed to edit comment in cob db");
	    return 1;
	}
	iprintf("comment %s edited",git_oid_tostr(buf,HEXSIZ,&edit));
    }
    else {
	if (add_comment_to_cob_db(re.oid,issue_id,reply_to,commit_time,author,alias)) { // todo handle case of edit
	    eprintf("failed to add comment to cob db");
	    return 1;
	} //todo update Issues table with EditID
	iprintf("comment %s created",git_oid_tostr(buf,HEXSIZ,&re.oid));
    }
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

    if (!signer_is_delegate(rrepo,signer)) {
	eprintf("the signer doesn't match one of the delegates for the repository");
	return 1;
    }
    
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
    if (update_assignees_in_cob_db(issue_id,&assignees2,re.oid)) {
	eprintf("failed to update assignees in cob db");
	return 1;
    }
    iprintf("assigned issue %s",git_oid_tostr(buf,HEXSIZ,&issue_id));
    return 0;    
}

int issue_label (Oid issue_id, size_t issue_id_hexlen, SimpleSet* add, SimpleSet* delete) {
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

    if (!signer_is_delegate(rrepo,signer)) {
	eprintf("the signer doesn't match one of the delegates for the repository");
	return 1;
    }
    
    // Get former labels from cob db
    SimpleSet former_labels;
    set_init(&former_labels);
    if (get_labels_from_cob_db(&former_labels,issue_id)) {
	eprintf("failed to get labels from cob db");
	return 1;
    }
    SimpleSet labels1;
    set_init(&labels1);
    if (set_difference(&labels1,&former_labels,delete)) {
	eprintf("failed to get difference between label sets");
	return 1;
    }
    SimpleSet labels2;
    set_init(&labels2);
    if (set_union(&labels2,&labels1,add)) {
	eprintf("failed to get union of label sets");
	return 1;
    }
    RepoEntry re = cob_issue_label(rrepo,signer,issue_id,&labels2);
    if (git_oid_is_zero(&re.oid)) {
	eprintf("failed to label cob issue");
	return 1;
    }
    if (update_labels_in_cob_db(issue_id,&labels2,re.oid)) {
	eprintf("failed to update labels in cob db");
	return 1;
    }
    iprintf("labeled issue %s",git_oid_tostr(buf,HEXSIZ,&issue_id));
    return 0;    
}

int issue_react (Oid issue_id, size_t issue_id_hexlen, Oid reply_to, size_t reply_to_hexlen, char emoji [4]) {
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
    RepoEntry re = cob_issue_react(rrepo,signer,issue_id,reply_to,emoji);
    if (git_oid_is_zero(&re.oid)) {
	eprintf("failed to react on cob issue");
	return 1;
    }
    if (add_reaction_to_cob_db(re.oid,issue_id,reply_to)) {
	eprintf("failed to add react to cob db");
	return 1;
    }
    iprintf("reaction %s created",git_oid_tostr(buf,HEXSIZ,&re.oid));
    return 0;
}

int issue_state (Oid issue_id, size_t issue_id_hexlen, IssueState state) {
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
    if (!signer_is_delegate(rrepo,signer)) {
	eprintf("the signer doesn't match one of the delegates for the repository");
	return 1;
    }
    RepoEntry re = cob_issue_state(rrepo,signer,issue_id,state);
    if (git_oid_is_zero(&re.oid)) {
	eprintf("failed to react on cob issue");
	return 1;
    }
    if (update_state_in_cob_db(issue_id,state,re.oid)) {
	eprintf("failed to update state in cob db");
	return 1;
    }
    iprintf("state %s set",git_oid_tostr(buf,HEXSIZ,&re.oid));
    return 0;
}

int issue_delete (Oid issue_id, size_t issue_id_hexlen) {
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
    if (!signer_is_delegate(rrepo,signer)) {
	eprintf("the signer doesn't match one of the delegates for the repository");
	return 1;
    }
    if (cob_issue_delete(rrepo,signer,issue_id)) {
	eprintf("failed to delete cob issue");
	return 1;
    }
    if (delete_issue_from_cob_db(issue_id)) {
	eprintf("failed to delete issue from cob db");
	return 1;
    }
    iprintf("issue %s deleted",git_oid_tostr(buf,HEXSIZ,&issue_id));
    return 0;
}

int issue_edit (Oid issue_id, size_t issue_id_hexlen, char* title, char* desc) {
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
    if (!signer_is_delegate(rrepo,signer)) {
	eprintf("the signer doesn't match one of the delegates for the repository");
	return 1;
    }
    RepoEntry re = cob_issue_edit(rrepo,signer,issue_id,title,desc);
    if (git_oid_is_zero(&re.oid)) {
	eprintf("failed to edit cob issue");
	return 1;
    }
    if (edit_issue_in_cob_db(issue_id,re.oid)) {
	eprintf("failed to edit issue in cob db");
	return 1;
    }
    iprintf("edit %s performed",git_oid_tostr(buf,HEXSIZ,&re.oid));
    return 0;
}

int issue_list (SimpleSet* assigned, IssueState state) {
    char buf [HEXSIZ];
    rad_git_init();
    RadRepo rrepo = rad_repo_default();
    if (get_rad_repo_from_cwd(&rrepo)) {
	eprintf("failed to get rad repo from cwd");
	return 1;
    }
    //Pubkey signer = profile_get_pubkey();
    SimpleSet issues;
    set_init(&issues);
    if (get_cobs(&issues,COB_ISSUE,rrepo)) {
	eprintf("failed to get list of issues");
	return 1;
    }
    size_t n_issues = 0;
    char** issues_list = set_to_array(&issues,&n_issues);
    if (n_issues)
	printf("ID------Title------------Author------------Labels------------Assignees------------Opened\n");
    for (int i=0; i<n_issues; i++) {
	Oid issue_entry = {{0}};
	if (git_oid_fromstr(&issue_entry,issues_list[i])) {
	    eprintf("failed to get Oid from hex string");
	    return 1;
	}
	if (git_oid_is_zero(&issue_entry)) {
	    eprintf("invalid issue entry");
	    return 1;
	}
	if (!issue_entry_in_cob_db(issue_entry)) { // if not, update the db
	    if (update_issue_in_cob_db(issue_entry,rrepo)) {
		eprintf("failed to update issue in cob db");
		return 1;
	    }
	}
	//iprintf("issue_entry %s",git_oid_tostr(buf,HEXSIZ,&issue_entry));
	Oid issue_id = get_issue_id(issue_entry);
	if (git_oid_is_zero(&issue_id)) {
	    eprintf("failed to get issue id");
	    return 1;
	}

	SimpleSet assignees;
	set_init(&assignees);
	if (get_issue_assignees(&assignees,issue_id)) {
	    eprintf("failed to get assignees for issue");
	    return 1;
	}

	size_t n_assigned = 0;
	char** assigned_list = set_to_array(assigned,&n_assigned);
	if (n_assigned) {
	    SimpleSet intersection;
	    set_init(&intersection);
	    if (set_intersection(&intersection,&assignees,assigned)) {
		eprintf("failed to get intersection between assignee sets");
		return 1;
	    }
	    size_t n_intersection = 0;
	    char** intersection_list = set_to_array(&intersection,&n_intersection);
	    if (!n_intersection) continue;
	}

	//get issue state and compare with requested state
	if (state.status || state.reason) {
	    IssueState issue_state = get_issue_state(issue_id);
	    if (state.status) {
		if (!issue_state.status || strcmp(state.status,issue_state.status)) {
		    continue;
		}
	    }
	    if (state.reason) {
		if (!issue_state.reason || strcmp(state.reason,issue_state.reason)) {
		    continue;
		}
	    }
	}
	
	char* issue_id_str = git_oid_tostr(buf,HEXSIZ,&issue_id);
	char id [8];
	memcpy(id,issue_id_str,7);
	id[7] = 0;
	printf("%s ",id);
	char* title = get_issue_title(rrepo,issue_id);
	if (!title) {
	    eprintf("failed to get title for issue");
	    return 1;
	}
	if (strlen(title)>16)
	    title[16] = 0;
	printf("%s",title);
	size_t title_len = strlen(title);
	for (size_t j=0; j<17-title_len; j++)
	    printf(" ");
	char* author = get_issue_author(rrepo,issue_id);
	if (!author) {
	    eprintf("failed to get author for issue");
	    return 1;
	}
	if (strlen(author)>17)
	    author[17] = 0;
	printf("%s",author);
	size_t author_len = strlen(author);
	for (size_t j=0; j<18-author_len; j++)
	    printf(" ");
	SimpleSet labels;
	set_init(&labels);
	if (get_issue_labels(&labels,issue_id)) {
	    eprintf("failed to get labels for issue");
	    return 1;
	}
	size_t n_labels = 0;
	char** labels_list = set_to_array(&labels,&n_labels);
	char* labels_str = malloc(n_labels*256); //todo set correct size
	strcpy(labels_str,"");
	for (size_t j=0; j<n_labels; j++) {
	    strcat(labels_str,labels_list[j]);
	    if (j<n_labels-1)
		strcat(labels_str,",");
	}
	if (strlen(labels_str)>17)
	    labels_str[17] = 0;
	printf("%s",labels_str);
	size_t labels_str_len = strlen(labels_str);
	for (size_t j=0; j<18-labels_str_len; j++)
	    printf(" ");
	
	size_t n_assignees = 0;
	char** assignees_list = set_to_array(&assignees,&n_assignees);
	char* assignees_str = malloc(n_assignees*128); //todo set correct size
	strcpy(assignees_str,"");
	for (size_t j=0; j<n_assignees; j++) {
	    strcat(assignees_str,assignees_list[i]);
	    if (j<n_assignees-1)
		strcat(assignees_str,",");
	}
	if (strlen(assignees_str)>20)
	    assignees_str[20] = 0;
	printf("%s",assignees_str);
	size_t assignees_str_len = strlen(assignees_str);
	for (size_t j=0; j<21-assignees_str_len; j++)
	    printf(" ");
	int64_t time_opened = get_issue_opening_time(rrepo,issue_id);
	if (time_opened<0) {
	    eprintf("failed to get issue opening time");
	    return 1;
	}
	time_t time_opened_tt = time_opened;
	printf("%s",ctime(&time_opened_tt));
    }
}

int issue_show (Oid issue_id, size_t issue_id_hexlen) {
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

    printf("Title   %s\n",get_issue_title(rrepo,issue_id));
    printf("Issue   %s\n",git_oid_tostr(buf,HEXSIZ,&issue_id));
    printf("Author  %s\n",get_issue_author(rrepo,issue_id));
    IssueState state = get_issue_state(issue_id);
    printf("Status  %s",state.status);
    if (state.reason && strlen(state.reason))
	printf(" (%s)",state.reason);
    printf("\n\n");
    printf("%s\n",get_issue_description(rrepo,issue_id));
    
}
