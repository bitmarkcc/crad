#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <fetch.h>
#include <util.h>

int fetch_run  (Storage storage, RadRepo rrepo, const char* did_raw, const char* oid_str, const char* refstr) {

    //fprintf(stderr,"fetch_run oid %s refstr %s\n",oid_str,refstr);

    //Get path of rad repo
    char* rrepo_path = malloc(strlen(storage.path)+31);
    char* rid_str = oid_to_rid(rrepo.rid);
    sprintf(rrepo_path,"%s/%s",storage.path,rid_str);

    //Get path of current workdir
    git_repository* repo = 0;
    char cwd [1024];
    if (!getcwd(cwd,sizeof(cwd))) {
	fprintf(stderr,"Can't get current working directory\n");
	return 1;
    }
    if (git_repository_open(&repo,cwd)) {
	fprintf(stderr,"Can't open git repository\n");
	return 1;
    }
    const char* repo_path = git_repository_workdir(repo);
    
    char* argv [7];
    argv[0] = "git";
    argv[1] = "-C";
    argv[2] = strdup(repo_path);
    argv[3] = "fetch-pack";
    argv[4] = strdup(rrepo_path);
    argv[5] = strdup(oid_str);
    argv[6] = 0;

    if (exec_command("git",argv)) {
	fprintf(stderr,"git command failed\n");
	return 1;
    }

    printf("\n");
    
    return 0;
}
