#include "squares.h"

#include <stdio.h>
#include <string.h>

int register_commit(squares_repo *destination, git_oid *oid, git_time_t time) {

	squares_commit *c = malloc(sizeof(squares_commit));
	if (! c) {
		return 1;
	}

	git_oid_cpy(&c->oid, oid);
	c->time = time;
	c->next = destination->commits;

	destination->commits = c;

	return 0;
}

static int load_commit(squares_repo *destination, git_repository *repo,
	git_oid *oid, git_commit *commit) {

	const char *summary = git_commit_summary(commit);

	/* Skip commits that aren't ours */
	git_oid commit_oid;
	if (strlen(summary) != GIT_OID_HEXSZ ||
		git_oid_fromstrn(&commit_oid, summary, GIT_OID_HEXSZ)) {
		return 0;
	}

	/* Add a new entry to our linked list of known commits */
	return register_commit(destination, &commit_oid,
		git_commit_time(commit));
}

int open_repository(squares_repo **_destination, const char *path) {

	squares_repo *destination =
		*_destination = malloc(sizeof(squares_repo));
	if (! destination) {
		fprintf(stderr, "git-squares: malloc failed\n");
		return 1;
	}

	memset(destination, 0, sizeof(squares_repo));

	int error = git_repository_open(&destination->repo, path);
	if (error) {
		const git_error *e = git_error_last();
		fprintf(stderr, "git-squares: %s\n", e->message);
		return 1;
	}

	error = git_signature_default(&destination->signature,
		destination->repo);
	if (error) {
		const git_error *e = git_error_last();
		fprintf(stderr, "git-squares: %s\n", e->message);
		git_repository_free(destination->repo);
		return 1;
	}

	error = git_reference_name_to_id(&destination->head.oid,
		destination->repo, "HEAD");
	if (error) {
		const git_error *e = git_error_last();
		fprintf(stderr, "git-squares: %s\n", e->message);
		git_signature_free(destination->signature);
		git_repository_free(destination->repo);
		return 1;
	}

	git_commit *commit;
	error = git_commit_lookup(&commit, destination->repo,
		&destination->head.oid);
	if (error) {
		const git_error *e = git_error_last();
		fprintf(stderr, "git-squares: %s\n", e->message);
		git_signature_free(destination->signature);
		git_repository_free(destination->repo);
		return 1;
	}

	error = git_commit_tree(&destination->head.tree, commit);
	if (error) {
		const git_error *e = git_error_last();
		fprintf(stderr, "git-squares: %s\n", e->message);
		git_commit_free(commit);
		git_signature_free(destination->signature);
		git_repository_free(destination->repo);
		return 1;
	}

	git_commit_free(commit);

	error = walk_repository(destination, destination->repo, load_commit);
	if (error) {
		const git_error *e = git_error_last();
		fprintf(stderr, "git-squares: %s\n", e->message);
		git_signature_free(destination->signature);
		git_repository_free(destination->repo);
		return 1;
	}

	return error;
}

void close_repository(squares_repo *destination) {

	while (destination->commits) {
		squares_commit *c = destination->commits->next;
		free(destination->commits);
		destination->commits = c;
	}

	git_tree_free(destination->head.tree);
	git_signature_free(destination->signature);
	git_repository_free(destination->repo);

	free(destination);
}
