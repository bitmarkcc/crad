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
    return cmd;
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
	    if (1+1 >= argc) {
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
    odb_obj = 0;
    if (git_odb_read_prefix(&odb_obj,odb,&reply_to,reply_to_hexlen)) {
	eprintf("failed to read prefix from odb");
	return 1;
    }
    reply_to = *git_odb_object_id(odb_obj);
    Pubkey signer = profile_get_pubkey();
    RepoEntry re = cob_issue_comment(rrepo,signer,reply_to,message);
    if (add_comment_to_cob_db(re.oid,issue_id,reply_to)) {
	eprintf("failed to add comment to cob db");
	return 1;
    }
    iprintf("comment %s created",git_oid_tostr(buf,HEXSIZ,&re.oid));
}
