#include "squares.h"

#include <stdio.h>
#include <string.h>

static int import_commit(struct destination *destination, git_repository *repo,
	git_oid *oid, git_commit *commit) {

	/* Skip if this commit is already known to us */
	for (struct commit *c = destination->commits; c; c = c->next) {
		if (git_oid_equal(oid, &c->oid)) {
			return 0;
		}
	}

	/* Skip if this commit isn't authored by us */
	const git_signature *author = git_commit_author(commit);
	if (strcmp(destination->signature->name, author->name)) {
		return 0;
	}

	char hash[GIT_OID_HEXSZ + 1];
	snprintf(hash, sizeof hash, "%s", git_oid_tostr_s(oid));

	git_commit *parent;
	int error = git_commit_lookup(&parent, destination->repo,
		&destination->head.oid);
	if (error) {
		const git_error *e = git_error_last();
		fprintf(stderr, "git-squares: %s\n", e->message);
		return 1;
	}

	destination->signature->when = (git_time) {
		git_commit_time(commit),
		git_commit_time_offset(commit),
		git_commit_time_offset(commit) < 0 ? '-' : '+'
	};

	git_oid new_commit_oid;
	error = git_commit_create_v(
		&new_commit_oid,
		destination->repo,
		"HEAD",
		destination->signature,
		destination->signature,
		"UTF-8",
		hash,
		destination->head.tree,
		1,
		parent
	);
	if (error) {
		const git_error *e = git_error_last();
		fprintf(stderr, "git-squares: %s\n", e->message);
		return 1;
	}

	git_commit_free(parent);

	git_oid_cpy(&destination->head.oid, &new_commit_oid);

	register_commit(destination, &new_commit_oid, git_commit_time(commit));

	return 0;
}

int import_repository(struct destination *destination, const char *path) {

	git_repository *repo = NULL;
	int error = git_repository_open(&repo, path);
	if (error) {
		const git_error *e = git_error_last();
		fprintf(stderr, "git-squares: %s\n", e->message);
		return 1;
	}

	error = walk_repository(destination, repo, import_commit);

	git_repository_free(repo);

	return error;
}
