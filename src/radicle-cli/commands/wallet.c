#include <string.h>
#include <stdio.h>

#include <commands/wallet.h>
#include <profile.h>
#include <print.h>
#include <git.h>
#include <cob.h>
#include <rad.h>

WalletCommand command_wallet_default() {
    WalletCommand cmd;
    cmd.err = 0;
    cmd.currency = 0;
    cmd.address = 0;
    cmd.rid = 0;
    cmd.json = false;
    return cmd;
}

void print_help_wallet () {
    printf("crad wallet (Manage wallet addresses) Usage:\n");
    printf("crad wallet add --currency <name> --address <addr> [-R <rid>]\n");
    printf("crad wallet remove --currency <name> [-R <rid>]\n");
    printf("crad wallet list [-R <rid>] [--json]\n");
}

WalletCommand parse_args_wallet (int argc, char** argv) {
    WalletCommand cmd = command_wallet_default();
    for (size_t i=0; i<argc; i++) {
	if (!strcmp(argv[i],"--currency")) {
	    if (i+1 >= argc) {
		eprintf("no parameter to --currency");
		cmd.err = 1;
		return cmd;
	    }
	    cmd.currency = strdup(argv[i+1]);
	}
	else if (!strcmp(argv[i],"--address")) {
	    if (i+1 >= argc) {
		eprintf("no parameter to --address");
		cmd.err = 1;
		return cmd;
	    }
	    cmd.address = strdup(argv[i+1]);
	}
	else if (!strcmp(argv[i],"-R")) {
	    if (i+1 >= argc) {
		eprintf("no parameter to -R");
		cmd.err = 1;
		return cmd;
	    }
	    cmd.rid = strdup(argv[i+1]);
	}
	else if (!strcmp(argv[i],"--json")) {
	    cmd.json = true;
	}
    }
    return cmd;
}

int wallet_run (Command c) {
    if (c.type == CMD_HELP) {
	print_help_wallet();
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
    else if (c.argc > 0 && !strcmp(c.argv[0],"add")) {
	WalletCommand cmd = parse_args_wallet(c.argc,c.argv);
	if (cmd.err) return 1;
	if (!cmd.currency) {
	    eprintf("--currency is required");
	    return 1;
	}
	if (!cmd.address) {
	    eprintf("--address is required");
	    return 1;
	}
	return wallet_add(cmd.currency,cmd.address,cmd.rid);
    }
    else if (c.argc > 0 && !strcmp(c.argv[0],"remove")) {
	WalletCommand cmd = parse_args_wallet(c.argc,c.argv);
	if (cmd.err) return 1;
	if (!cmd.currency) {
	    eprintf("--currency is required");
	    return 1;
	}
	return wallet_remove(cmd.currency,cmd.rid);
    }
    else if (c.argc > 0 && !strcmp(c.argv[0],"list")) {
	WalletCommand cmd = parse_args_wallet(c.argc,c.argv);
	if (cmd.err) return 1;
	return wallet_list(cmd.rid,cmd.json);
    }
    else {
	print_help_wallet();
    }
    return 0;
}

int wallet_add (char* currency, char* address, const char* rid) {
    char buf [HEXSIZ];
    const char* rad_home = get_rad_home();
    rad_git_init();
    RadRepo rrepo = rad_repo_default();
    if (rid) {
	rrepo.rid = rid_to_oid(rid);
	git_repository* repo = 0;
	char* path = malloc(strlen(rad_home)+64);
	sprintf(path,"%s/storage/%s",rad_home,rid);
	if (git_repository_open(&repo,path)) {
	    eprintf("failed to open git repository at path %s",path);
	    return 1;
	}
	rrepo.repo = repo;
    }
    else if (get_rad_repo_from_cwd(&rrepo)) {
	eprintf("failed to get rad repo from cwd");
	return 1;
    }
    Pubkey signer = profile_get_pubkey();
    RepoEntry re = cob_wallet_add(rrepo,signer,currency,address);
    if (git_oid_is_zero(&re.oid)) {
	eprintf("failed to add wallet entry");
	return 1;
    }
    iprintf("wallet %s address added: %s",currency,address);
    return 0;
}

int wallet_remove (char* currency, const char* rid) {
    char buf [HEXSIZ];
    const char* rad_home = get_rad_home();
    rad_git_init();
    RadRepo rrepo = rad_repo_default();
    if (rid) {
	rrepo.rid = rid_to_oid(rid);
	git_repository* repo = 0;
	char* path = malloc(strlen(rad_home)+64);
	sprintf(path,"%s/storage/%s",rad_home,rid);
	if (git_repository_open(&repo,path)) {
	    eprintf("failed to open git repository at path %s",path);
	    return 1;
	}
	rrepo.repo = repo;
    }
    else if (get_rad_repo_from_cwd(&rrepo)) {
	eprintf("failed to get rad repo from cwd");
	return 1;
    }
    Pubkey signer = profile_get_pubkey();
    RepoEntry re = cob_wallet_remove(rrepo,signer,currency);
    if (git_oid_is_zero(&re.oid)) {
	eprintf("failed to remove wallet entry");
	return 1;
    }
    iprintf("wallet %s address removed",currency);
    return 0;
}

int wallet_list (const char* rid, bool json) {
    char buf [HEXSIZ];
    rad_git_init();
    RadRepo rrepo = rad_repo_default();
    if (rid) {
	rrepo.rid = rid_to_oid(rid);
	git_repository* repo = 0;
	const char* rad_home = get_rad_home();
	char* path = malloc(strlen(rad_home)+64);
	sprintf(path,"%s/storage/%s",rad_home,rid);
	if (git_repository_open(&repo,path)) {
	    eprintf("failed to open git repository at path %s",path);
	    return 1;
	}
	rrepo.repo = repo;
    }
    else if (get_rad_repo_from_cwd(&rrepo)) {
	eprintf("failed to get rad repo from cwd");
	return 1;
    }

    // Iterate all refs to find wallet COB entries
    git_reference_iterator* refit = 0;
    if (git_reference_iterator_glob_new(&refit,rrepo.repo,"refs/namespaces/*")) {
	return 0;
    }

    // Collect wallet refs: namespace (DID) and latest entry OID per namespace
    size_t n_wallets = 0;
    size_t cap_wallets = 8;
    char** wallet_ns = malloc(cap_wallets * sizeof(char*));
    Oid* wallet_oids = malloc(cap_wallets * sizeof(Oid));
    uint64_t* wallet_times = malloc(cap_wallets * sizeof(uint64_t));

    const char* refname = 0;
    int ret = 0;
    while (!(ret = git_reference_next_name(&refname,refit))) {
	if (!strstr(refname,"/refs/cobs/xyz.radicle.wallet/")) continue;

	// Extract namespace
	char* rcopy = strdup(refname);
	char* p = rcopy + strlen("refs/namespaces/");
	char* ns_end = strchr(p,'/');
	if (!ns_end) { free(rcopy); continue; }
	*ns_end = '\0';
	char* ns = strdup(p);
	free(rcopy);

	// Get the commit this ref points to
	Oid ref_oid = {{0}};
	if (git_reference_name_to_id(&ref_oid,rrepo.repo,refname)) {
	    free(ns);
	    continue;
	}
	git_commit* commit = 0;
	if (git_commit_lookup(&commit,rrepo.repo,&ref_oid)) {
	    free(ns);
	    continue;
	}
	uint64_t ctime = git_commit_time(commit);

	// Find or add namespace entry, keeping the latest
	bool found = false;
	for (size_t i = 0; i < n_wallets; i++) {
	    if (!strcmp(wallet_ns[i],ns)) {
		if (ctime > wallet_times[i]) {
		    wallet_oids[i] = ref_oid;
		    wallet_times[i] = ctime;
		}
		found = true;
		break;
	    }
	}
	if (!found) {
	    if (n_wallets >= cap_wallets) {
		cap_wallets *= 2;
		wallet_ns = realloc(wallet_ns,cap_wallets * sizeof(char*));
		wallet_oids = realloc(wallet_oids,cap_wallets * sizeof(Oid));
		wallet_times = realloc(wallet_times,cap_wallets * sizeof(uint64_t));
	    }
	    wallet_ns[n_wallets] = ns;
	    wallet_oids[n_wallets] = ref_oid;
	    wallet_times[n_wallets] = ctime;
	    n_wallets++;
	    ns = 0; // prevent free below
	}
	free(ns);
    }

    if (!n_wallets) {
	free(wallet_ns);
	free(wallet_oids);
	free(wallet_times);
	return 0;
    }

    if (json) printf("[");
    bool first_json = true;
    bool printed_header = false;

    for (size_t wi = 0; wi < n_wallets; wi++) {
	Oid entry_oid = wallet_oids[wi];

	// Walk the COB chain from latest to root, collecting all commits
	git_commit* commit = 0;
	if (git_commit_lookup(&commit,rrepo.repo,&entry_oid)) continue;

	size_t n_commits = 0;
	size_t cap_commits = 16;
	git_commit** commits = malloc(cap_commits * sizeof(git_commit*));
	commits[n_commits++] = commit;

	while (git_commit_parentcount(commit) >= 2) {
	    git_commit* parent = 0;
	    if (git_commit_parent(&parent,commit,0)) break;
	    if (n_commits >= cap_commits) {
		cap_commits *= 2;
		commits = realloc(commits,cap_commits * sizeof(git_commit*));
	    }
	    commits[n_commits++] = parent;
	    commit = parent;
	}

	// Build wallet state by replaying actions from oldest to newest
	size_t n_entries = 0;
	size_t cap_entries = 16;
	char** entry_currencies = malloc(cap_entries * sizeof(char*));
	char** entry_addresses = malloc(cap_entries * sizeof(char*));

	for (int ci = n_commits - 1; ci >= 0; ci--) {
	    git_tree* tree = 0;
	    if (git_commit_tree(&tree,commits[ci])) continue;
	    for (size_t ai = 0; ai < 16; ai++) {
		char ai_str [4];
		sprintf(ai_str,"%zu",ai);
		git_tree_entry* te = 0;
		if (git_tree_entry_bypath(&te,tree,ai_str)) break;
		const Oid* blob_oid = git_tree_entry_id(te);
		if (!blob_oid) continue;
		git_blob* blob = 0;
		if (git_blob_lookup(&blob,rrepo.repo,blob_oid)) continue;
		const void* raw = git_blob_rawcontent(blob);
		git_object_size_t rawsz = git_blob_rawsize(blob);
		char* content = malloc(rawsz + 1);
		memcpy(content,raw,rawsz);
		content[rawsz] = '\0';
		json_object* jobj = json_tokener_parse(content);
		free(content);
		if (!jobj) continue;
		json_object* val_type = 0;
		json_object_object_get_ex(jobj,"type",&val_type);
		if (!val_type) continue;
		const char* type = json_object_get_string(val_type);
		if (!strcmp(type,"add")) {
		    json_object* val_currency = 0;
		    json_object* val_address = 0;
		    json_object_object_get_ex(jobj,"currency",&val_currency);
		    json_object_object_get_ex(jobj,"address",&val_address);
		    if (!val_currency || !val_address) continue;
		    const char* cur = json_object_get_string(val_currency);
		    const char* addr = json_object_get_string(val_address);
		    bool dup = false;
		    for (size_t ei = 0; ei < n_entries; ei++) {
			if (!strcmp(entry_currencies[ei],cur)) {
			    free(entry_addresses[ei]);
			    entry_addresses[ei] = strdup(addr);
			    dup = true;
			    break;
			}
		    }
		    if (!dup) {
			if (n_entries >= cap_entries) {
			    cap_entries *= 2;
			    entry_currencies = realloc(entry_currencies,cap_entries * sizeof(char*));
			    entry_addresses = realloc(entry_addresses,cap_entries * sizeof(char*));
			}
			entry_currencies[n_entries] = strdup(cur);
			entry_addresses[n_entries] = strdup(addr);
			n_entries++;
		    }
		}
		else if (!strcmp(type,"remove")) {
		    json_object* val_currency = 0;
		    json_object_object_get_ex(jobj,"currency",&val_currency);
		    if (!val_currency) continue;
		    const char* cur = json_object_get_string(val_currency);
		    for (size_t ei = 0; ei < n_entries; ei++) {
			if (!strcmp(entry_currencies[ei],cur)) {
			    free(entry_currencies[ei]);
			    free(entry_addresses[ei]);
			    for (size_t k = ei; k < n_entries - 1; k++) {
				entry_currencies[k] = entry_currencies[k+1];
				entry_addresses[k] = entry_addresses[k+1];
			    }
			    n_entries--;
			    break;
			}
		    }
		}
	    }
	}

	if (n_entries > 0) {
	    char did_str[72];
	    snprintf(did_str,sizeof(did_str),"did:key:%s",wallet_ns[wi]);
	    if (json) {
		for (size_t ei = 0; ei < n_entries; ei++) {
		    if (!first_json) printf(",");
		    printf("{\"did\":\"%s\",\"currency\":\"%s\",\"address\":\"%s\"}",
			   did_str,entry_currencies[ei],entry_addresses[ei]);
		    first_json = false;
		}
	    } else {
		if (!printed_header) {
		    printf("DID----------------------Currency---------Address\n");
		    printed_header = true;
		}
		for (size_t ei = 0; ei < n_entries; ei++) {
		    char did_str_cut [25];
		    memcpy(did_str_cut,did_str,24);
		    did_str_cut[24] = 0;
		    printf("%s ",did_str_cut);
		    char* currency_cut = strdup(entry_currencies[ei]);
		    if (strlen(currency_cut)>16)
			currency_cut[16] = 0;
		    rad_replace(currency_cut,' ','_');
		    printf("%s",currency_cut);
		    size_t currency_cut_len = strlen(currency_cut);
		    for (size_t j=0; j<17-currency_cut_len; j++)
			printf(" ");
		    printf("%s\n",entry_addresses[ei]);
		}
	    }
	}

	for (size_t ei = 0; ei < n_entries; ei++) {
	    free(entry_currencies[ei]);
	    free(entry_addresses[ei]);
	}
	free(entry_currencies);
	free(entry_addresses);
	free(commits);
    }

    if (json) printf("]\n");

    for (size_t i = 0; i < n_wallets; i++) free(wallet_ns[i]);
    free(wallet_ns);
    free(wallet_oids);
    free(wallet_times);

    return 0;
}
