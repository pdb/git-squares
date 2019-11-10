#include "squares.h"

#include <getopt.h>
#include <stdio.h>
#include <string.h>

typedef struct {
	squares_repo *r;
	const char *path;
	char *author;
} import_job;

static int import_commit(git_repository *repo, git_oid *oid,
	git_commit *commit, void *closure) {

	import_job *job = closure;

	/* Skip if this commit is already known to us */
	for (squares_commit *c = job->r->commits; c; c = c->next) {
		if (git_oid_equal(oid, &c->oid)) {
			return 0;
		}
	}

	/* Skip if this commit isn't authored by us */
	const git_signature *author = git_commit_author(commit);
	if (strcmp(job->author, author->name) &&
		strcmp(job->author, author->email)) {
		return 0;
	}

	char hash[GIT_OID_HEXSZ + 1];
	snprintf(hash, sizeof hash, "%s", git_oid_tostr_s(oid));

	git_commit *parent;
	int error = git_commit_lookup(&parent, job->r->repo, &job->r->head.oid);
	if (error) {
		const git_error *e = git_error_last();
		fprintf(stderr, "git-squares: %s\n", e->message);
		return 1;
	}

	job->r->signature->when = (git_time) {
		git_commit_time(commit),
		git_commit_time_offset(commit),
		git_commit_time_offset(commit) < 0 ? '-' : '+'
	};

	git_oid new_commit_oid;
	error = git_commit_create_v(
		&new_commit_oid,
		job->r->repo,
		"HEAD",
		job->r->signature,
		job->r->signature,
		"UTF-8",
		hash,
		job->r->head.tree,
		1,
		parent
	);
	if (error) {
		const git_error *e = git_error_last();
		fprintf(stderr, "git-squares: %s\n", e->message);
		return 1;
	}

	git_commit_free(parent);

	git_oid_cpy(&job->r->head.oid, &new_commit_oid);

	register_commit(job->r, &new_commit_oid, git_commit_time(commit));

	return 0;
}

static int import_repository(import_job *job) {

	git_repository *repo = NULL;
	int error = git_repository_open(&repo, job->path);
	if (error) {
		const git_error *e = git_error_last();
		fprintf(stderr, "git-squares: %s\n", e->message);
		return 1;
	}

	error = walk_repository(repo, import_commit, job);

	git_repository_free(repo);

	return error;
}

static struct option long_options[] = {
	{ "author",		required_argument,	NULL, 'a' },
	{ NULL, 0, NULL, 0 }
};

int squares_import(int argc, char **argv) {

	import_job job = { NULL };

	while (1) {
		int option_index = 0;
		int c = getopt_long(argc, argv, "a:", long_options,
			&option_index);
		if (c == -1) {
			break;
		}
		switch (c) {
			case 'a':
				free(job.author);
				job.author = strdup(optarg);
				break;
			default:
				free(job.author);
				return 1;
		}
	}

	if (argc == optind) {
		fprintf(stderr, "usage: "
			"git squares import [--author AUTHOR] REPO ...\n");
		free(job.author);
		return 1;
	}

	int error = open_repository(&job.r, ".");
	if (error) {
		return 1;
	}

	if (! job.author) {
		job.author = strdup(job.r->signature->name);
	}

	for (int i = optind; ! error && i < argc; i++) {
		job.path = argv[i];
		error = import_repository(&job);
	}

	close_repository(job.r);
	free(job.author);

	return error;
}
