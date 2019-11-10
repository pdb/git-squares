#include "squares.h"

#include <stdio.h>
#include <string.h>

static int import_commit(squares_repo *r, git_repository *repo, git_oid *oid,
	git_commit *commit) {

	/* Skip if this commit is already known to us */
	for (squares_commit *c = r->commits; c; c = c->next) {
		if (git_oid_equal(oid, &c->oid)) {
			return 0;
		}
	}

	/* Skip if this commit isn't authored by us */
	const git_signature *author = git_commit_author(commit);
	if (strcmp(r->signature->name, author->name)) {
		return 0;
	}

	char hash[GIT_OID_HEXSZ + 1];
	snprintf(hash, sizeof hash, "%s", git_oid_tostr_s(oid));

	git_commit *parent;
	int error = git_commit_lookup(&parent, r->repo, &r->head.oid);
	if (error) {
		const git_error *e = git_error_last();
		fprintf(stderr, "git-squares: %s\n", e->message);
		return 1;
	}

	r->signature->when = (git_time) {
		git_commit_time(commit),
		git_commit_time_offset(commit),
		git_commit_time_offset(commit) < 0 ? '-' : '+'
	};

	git_oid new_commit_oid;
	error = git_commit_create_v(
		&new_commit_oid,
		r->repo,
		"HEAD",
		r->signature,
		r->signature,
		"UTF-8",
		hash,
		r->head.tree,
		1,
		parent
	);
	if (error) {
		const git_error *e = git_error_last();
		fprintf(stderr, "git-squares: %s\n", e->message);
		return 1;
	}

	git_commit_free(parent);

	git_oid_cpy(&r->head.oid, &new_commit_oid);

	register_commit(r, &new_commit_oid, git_commit_time(commit));

	return 0;
}

static int import_repository(squares_repo *r, const char *path) {

	git_repository *repo = NULL;
	int error = git_repository_open(&repo, path);
	if (error) {
		const git_error *e = git_error_last();
		fprintf(stderr, "git-squares: %s\n", e->message);
		return 1;
	}

	error = walk_repository(r, repo, import_commit);

	git_repository_free(repo);

	return error;
}

int squares_import(int argc, char **argv) {

	squares_repo *r;
	int error = open_repository(&r, ".");
	if (error) {
		return 1;
	}

	for (int i = 1; ! error && i < argc; i++) {
		error = import_repository(r, argv[i]);
	}

	close_repository(r);

	return error;
}
