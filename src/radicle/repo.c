#include <stdio.h>
#include <string.h>

#include <repo.h>
#include <profile.h>
#include <id.h>
#include <git.h>
#include <util.h>

RadRepo rad_repo_default () {
    RadRepo rrepo;
    Oid rid = {{0}};
    rrepo.rid = rid;
    rrepo.repo = 0;
}

RadRepo rad_repo_create (const char* path, const Oid rid, const StorageInfo si) {
    RadRepo rrepo;
    git_repository* repo = 0;
    rrepo.rid = rid;

    git_repository_init_options opts = GIT_REPOSITORY_INIT_OPTIONS_INIT;
    opts.flags |= GIT_REPOSITORY_INIT_BARE;
    opts.flags |= GIT_REPOSITORY_INIT_NO_REINIT;
    opts.flags |= GIT_REPOSITORY_INIT_MKDIR;
    
    if (git_repository_init_ext(&repo, path, &opts)) {
	fprintf(stderr,"failed to initialize git repository at %s\n",path);
	rrepo.repo = 0;
	return rrepo;
    }

    git_config* config = 0;
    if (git_repository_config(&config,repo)) {
	fprintf(stderr,"failed to get the config file for the git repository at %s\n",path);
	rrepo.repo = 0;
	return rrepo;
    }

    git_config_set_string(config,"user.name",si.name);
    git_config_set_string(config,"user.email",si.email);

    rrepo.repo = repo;
    return rrepo;
}

RepoEntry rad_repo_store (RadRepo rrepo, Oid resource, Oid* related, size_t n_related, Pubkey signer, Create spec) {
    /*
    change::Template{type_name,tips,message,embeds,contents} = spec;
    manifest = store_manifest_new(type_name,version_default);
    revision = write_manifest(&manifest,embeds,&contents);
    tree = self.find_tree(revision);
    signature = signer.sign(revision.bytes);
    related.sort();
    related.dedup();
    (id,timestamp) = write_commit(self,reource.map(..),tips.iter(..),message,signature,related,tree);
    return Entry{id,revision,signature,resource,parents,related,manifest,contents,timestamp}*/
    RepoEntry re = {{0}};
    Manifest manifest;
    manifest.type_name = spec.type_name;
    manifest.version = COB_VERSION;
    char* manifest_encoded = manifest_encode(manifest);
    Oid oid;
    git_odb* odb = 0;
    if (git_repository_odb(&odb,rrepo.repo)) {
	fprintf(stderr,"Failed to get repository odb\n");
	return re;
    }
    if (git_odb_write(&oid,odb,(uint8_t*)manifest_encoded,strlen(manifest_encoded),GIT_OBJECT_BLOB)) {
	fprintf(stderr,"Failed to write to ODB\n");
	return re;
    }
    git_treebuilder* treebuilder = 0;
    if (git_treebuilder_new(&treebuilder,rrepo.repo,0)) {
	fprintf(stderr,"Failed to initialize treebuilder\n");
	return re;
    }
    const git_tree_entry* tree_entry = 0;
    if (git_treebuilder_insert(&tree_entry,treebuilder,"manifest",&oid,GIT_FILEMODE_BLOB)) {
	fprintf(stderr,"Failed to insert to treebuilder\n");
	return re;
    }
    for (uint32_t i=0; i<spec.n_contents; i++) {
	char* content = spec.contents[i];
	if (git_odb_write(&oid,odb,(uint8_t*)content,strlen(content),GIT_OBJECT_BLOB)) {
	    fprintf(stderr,"Failed to write to odb\n");
	    return re;
	}
	char i_str [11];
	sprintf(i_str,"%u",i);
	if (git_treebuilder_insert(&tree_entry,treebuilder,i_str,&oid,GIT_FILEMODE_BLOB)) {
	    fprintf(stderr,"Failed to insert to treebuilder\n");
	    return re;
	}
    }

    if (spec.n_embeds) {
	git_treebuilder* treebuilder_embed = 0;
	if (git_treebuilder_new(&treebuilder_embed,rrepo.repo,0)) {
	    fprintf(stderr,"Failed to initialize embed treebuilder\n");
	    return re;
	}
	const git_tree_entry* tree_entry_embed = 0;
	for (uint32_t i=0; i<spec.n_embeds; i++) {
	    OidEmbed embed = spec.embeds[i];
	    oid = embed.content;
	    if (git_treebuilder_insert(&tree_entry_embed,treebuilder_embed,embed.name,&oid,GIT_FILEMODE_BLOB)) {
		fprintf(stderr,"Failed to insert to embed treebuilder\n");
		return re;
	    }
	}
	if (git_treebuilder_write(&oid,treebuilder_embed)) {
	    fprintf(stderr,"Failed to write the embeds tree\n");
	    return re;
	}
	if (git_treebuilder_insert(&tree_entry,treebuilder,"embeds",&oid,GIT_FILEMODE_TREE)) {
	    fprintf(stderr,"Failed to insert to main tree\n");
	}
    }
    
    if (git_treebuilder_write(&oid,treebuilder)) {
	fprintf(stderr,"Failed to write to treebuilder\n");
	return re;
    }

    char* sig_raw = 0;
    char* sig_full = 0;
    key_sign(&sig_raw,&sig_full,signer,oid.id,20);

    oids_dedup(&related,&n_related);
    oids_sort(&related,n_related);

    //write_commit(resource,tips,message,sig_full,related,oid);

    char* header = malloc(2048); //todo set right size
    strcpy(header,"gpgsig ");
    strcat(header,sig_full);    
    char** headers = &header;
    size_t n_headers = 1;
    char** trailers = 0;
    size_t n_trailers = 0;
    
    // turn this to a new function
    
    char commit_str [4096]; //todo set right size
    const size_t HEXSIZ = GIT_OID_SHA1_HEXSIZE+1;
    char buf [HEXSIZ];
    git_signature* gitsig = 0;
    if (git_signature_default(&gitsig,rrepo.repo)) {
	fprintf(stderr,"Failed to get git signature\n");
	return re;
    }
    char* author_name = gitsig->name;
    char* author_email = gitsig->email;
    char author_time [20];
    sprintf(author_time,"%ld",gitsig->when.time);
    char author_sign [2];
    sprintf(author_sign,"%c",gitsig->when.sign);
    strcpy(commit_str,"tree ");
    strcat(commit_str,strdup(git_oid_tostr(buf,HEXSIZ,&oid)));
    strcat(commit_str,"\n");
    for (size_t i=0; i<n_related; i++) {
	if (git_oid_is_zero(related+i)) continue;
	strcat(commit_str,"parent ");
	strcat(commit_str,strdup(git_oid_tostr(buf,HEXSIZ,related+i)));
	strcat(commit_str,"\n");
    }
    strcat(commit_str,"author ");
    strcat(commit_str,author_name);
    strcat(commit_str," <");
    strcat(commit_str,author_email);
    strcat(commit_str,"> ");
    strcat(commit_str,author_time);
    strcat(commit_str," ");
    strcat(commit_str,author_sign);
    strcat(commit_str,time_offset(gitsig->when.offset));
    strcat(commit_str,"\n");
    strcat(commit_str,"committer ");
    strcat(commit_str,author_name);
    strcat(commit_str," <");
    strcat(commit_str,author_email);
    strcat(commit_str,"> ");
    strcat(commit_str,author_time);
    strcat(commit_str," ");
    strcat(commit_str,author_sign);
    strcat(commit_str,time_offset(gitsig->when.offset));
    strcat(commit_str,"\n");
    for (size_t i=0; i<n_headers; i++) {
	strcat(commit_str,headers[i]);
	strcat(commit_str,"\n");
    }
    strcat(commit_str,"\n");
    strcat(commit_str,spec.message);
    strcat(commit_str,"\n");

    if (n_trailers)
	strcat(commit_str,"\n");
    
    for (size_t i=0; i<n_trailers; i++) {
	strcat(commit_str,trailers[i]);
    }

    if (git_odb_write(&oid,odb,(uint8_t*)commit_str,strlen(commit_str),GIT_OBJECT_COMMIT)) {
	fprintf(stderr,"Failed to write commit to odb\n");
	return re;
    }

    re.oid = oid;
    return re;
}


int rad_repo_update (RadRepo rrepo, Pubkey signer, const char* type_name, Oid obj_id, Oid entry_id) {
    git_reference* ref = 0;
    char refname [256]; //todo get right size here and below
    char reflogmsg [256];
    const size_t HEXSIZ = GIT_OID_SHA1_HEXSIZE+1;
    char buf [HEXSIZ];
    char* obj_id_str = strdup(git_oid_tostr(buf,HEXSIZ,&obj_id));
    char* entry_id_str = strdup(git_oid_tostr(buf,HEXSIZ,&entry_id));
    strcpy(refname,"refs/namespaces/");
    strcat(refname,pubkey_to_did(signer.bytes)+8);
    strcat(refname,"/refs/cobs/");
    strcat(refname,type_name);
    strcat(refname,"/");
    strcat(refname,obj_id_str);
    sprintf(reflogmsg,"Updating collaborative object '%s/%s' with new entry %s\n",type_name,obj_id_str,entry_id_str);
    if (git_reference_create(&ref,rrepo.repo,refname,&entry_id,1,reflogmsg)) {
	fprintf(stderr,"Failed to create git reference\n");
	return 1;
    }
    return 0;
}
