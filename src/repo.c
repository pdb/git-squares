#include "squares.h"

#include <stdio.h>
#include <string.h>

int register_commit(squares_repo *r, git_oid *oid, git_time_t time) {

	squares_commit *c = malloc(sizeof(squares_commit));
	if (! c) {
		return 1;
	}

	git_oid_cpy(&c->oid, oid);
	c->time = time;
	c->next = r->commits;

	r->commits = c;

	return 0;
}

static int load_commit(git_repository *repo, git_oid *oid, git_commit *commit,
	void *closure) {

	squares_repo *r = closure;

	const char *summary = git_commit_summary(commit);

	/* Skip commits that aren't ours */
	git_oid commit_oid;
	if (strlen(summary) != GIT_OID_HEXSZ ||
		git_oid_fromstrn(&commit_oid, summary, GIT_OID_HEXSZ)) {
		return 0;
	}

	/* Add a new entry to our linked list of known commits */
	return register_commit(r, &commit_oid, git_commit_time(commit));
}

int open_repository(squares_repo **_r, const char *path, const char *ref) {

	squares_repo *r = *_r = malloc(sizeof(squares_repo));
	if (! r) {
		fprintf(stderr, "git-squares: malloc failed\n");
		return 1;
	}

	memset(r, 0, sizeof(squares_repo));

	int error = git_repository_open(&r->repo, path);
	if (error) {
		const git_error *e = git_error_last();
		fprintf(stderr, "git-squares: %s\n", e->message);
		return 1;
	}

	error = git_signature_default(&r->signature, r->repo);
	if (error) {
		const git_error *e = git_error_last();
		fprintf(stderr, "git-squares: %s\n", e->message);
		git_repository_free(r->repo);
		return 1;
	}

	if (ref) {
		char buffer[1024];
		snprintf(buffer, sizeof buffer, "refs/heads/%s", ref);
		r->ref.name = strdup(buffer);
	} else {
		r->ref.name = strdup("HEAD");
	}

	error = git_reference_name_to_id(&r->ref.oid, r->repo, r->ref.name);
	if (error) {
		const git_error *e = git_error_last();
		fprintf(stderr, "git-squares: %s\n", e->message);
		git_signature_free(r->signature);
		git_repository_free(r->repo);
		free(r->ref.name);
		return 1;
	}

	git_commit *commit;
	error = git_commit_lookup(&commit, r->repo, &r->ref.oid);
	if (error) {
		const git_error *e = git_error_last();
		fprintf(stderr, "git-squares: %s\n", e->message);
		git_signature_free(r->signature);
		git_repository_free(r->repo);
		free(r->ref.name);
		return 1;
	}

	error = git_commit_tree(&r->ref.tree, commit);
	if (error) {
		const git_error *e = git_error_last();
		fprintf(stderr, "git-squares: %s\n", e->message);
		git_commit_free(commit);
		git_signature_free(r->signature);
		git_repository_free(r->repo);
		free(r->ref.name);
		return 1;
	}

	git_commit_free(commit);

	error = walk_repository(r->repo, load_commit, r);
	if (error) {
		const git_error *e = git_error_last();
		fprintf(stderr, "git-squares: %s\n", e->message);
		git_signature_free(r->signature);
		git_repository_free(r->repo);
		free(r->ref.name);
		return 1;
	}

	return error;
}

void close_repository(squares_repo *r) {

	while (r->commits) {
		squares_commit *c = r->commits->next;
		free(r->commits);
		r->commits = c;
	}

	git_tree_free(r->ref.tree);
	git_signature_free(r->signature);
	git_repository_free(r->repo);
	free(r->ref.name);
	free(r);
}
