#include <git2.h>
#include <stdio.h>
#include <string.h>

struct commit {
	git_oid oid;
	git_time_t time;
	struct commit *next;
};

static struct {
	git_repository *repo;
	git_signature *signature;
	struct {
		git_oid oid;
		git_tree *tree;
	} head;
	struct commit *commits;
} destination = { NULL };

typedef int (*walk_func)(git_repository *repo, git_oid *oid, git_commit *commit);

static int walk_repository(git_repository *repo, walk_func f) {

	git_revwalk *walk = NULL;
	git_revwalk_new(&walk, repo);

	int error = git_revwalk_push_glob(walk, "*");
	if (error) {
		const git_error *e = git_error_last();
		fprintf(stderr, "git-squares: %s\n", e->message);
		return 1;
	}

	git_oid oid;
	while (! error && ! git_revwalk_next(&oid, walk)) {
		git_commit *commit;
		error = git_commit_lookup(&commit, repo, &oid);
		if (error) {
			const git_error *e = git_error_last();
			fprintf(stderr, "git-squares: %s\n", e->message);
		} else {
			error = f(repo, &oid, commit);
			git_commit_free(commit);
		}
	}

	git_revwalk_free(walk);

	return error;
}

static int register_commit(git_oid *oid, git_time_t time) {

	struct commit *c = malloc(sizeof(struct commit));
	if (! c) {
		return 1;
	}

	git_oid_cpy(&c->oid, oid);
	c->time = time;
	c->next = destination.commits;

	destination.commits = c;

	return 0;
}

static int load_commit(git_repository *repo, git_oid *oid, git_commit *commit) {

	const char *summary = git_commit_summary(commit);

	/* Skip commits that aren't ours */
	git_oid commit_oid;
	if (strlen(summary) != GIT_OID_HEXSZ ||
		git_oid_fromstrn(&commit_oid, summary, GIT_OID_HEXSZ)) {
		return 0;
	}

	/* Add a new entry to our linked list of known commits */
	return register_commit(&commit_oid, git_commit_time(commit));
}

static int open_repository(const char *path) {

	int error = git_repository_open(&destination.repo, path);
	if (error) {
		const git_error *e = git_error_last();
		fprintf(stderr, "git-squares: %s\n", e->message);
		return 1;
	}

	error = git_signature_default(&destination.signature, destination.repo);
	if (error) {
		const git_error *e = git_error_last();
		fprintf(stderr, "git-squares: %s\n", e->message);
		git_repository_free(destination.repo);
		return 1;
	}

	error = git_reference_name_to_id(&destination.head.oid,
		destination.repo, "HEAD");
	if (error) {
		const git_error *e = git_error_last();
		fprintf(stderr, "git-squares: %s\n", e->message);
		git_signature_free(destination.signature);
		git_repository_free(destination.repo);
		return 1;
	}

	git_commit *commit;
	error = git_commit_lookup(&commit, destination.repo,
		&destination.head.oid);
	if (error) {
		const git_error *e = git_error_last();
		fprintf(stderr, "git-squares: %s\n", e->message);
		git_signature_free(destination.signature);
		git_repository_free(destination.repo);
		return 1;
	}

	error = git_commit_tree(&destination.head.tree, commit);
	if (error) {
		const git_error *e = git_error_last();
		fprintf(stderr, "git-squares: %s\n", e->message);
		git_commit_free(commit);
		git_signature_free(destination.signature);
		git_repository_free(destination.repo);
		return 1;
	}

	git_commit_free(commit);

	error = walk_repository(destination.repo, load_commit);
	if (error) {
		const git_error *e = git_error_last();
		fprintf(stderr, "git-squares: %s\n", e->message);
		git_signature_free(destination.signature);
		git_repository_free(destination.repo);
		return 1;
	}

	return error;
}

static void close_repository() {

	while (destination.commits) {
		struct commit *c = destination.commits->next;
		free(destination.commits);
		destination.commits = c;
	}

	git_tree_free(destination.head.tree);
	git_signature_free(destination.signature);
	git_repository_free(destination.repo);
}

static int import_commit(git_repository *repo, git_oid *oid,
	git_commit *commit) {

	/* Skip if this commit is already known to us */
	for (struct commit *c = destination.commits; c; c = c->next) {
		if (git_oid_equal(oid, &c->oid)) {
			return 0;
		}
	}

	/* Skip if this commit isn't authored by us */
	const git_signature *author = git_commit_author(commit);
	if (strcmp(destination.signature->name, author->name)) {
		return 0;
	}

	char hash[GIT_OID_HEXSZ + 1];
	snprintf(hash, sizeof hash, "%s", git_oid_tostr_s(oid));

	git_commit *parent;
	int error = git_commit_lookup(&parent, destination.repo,
		&destination.head.oid);
	if (error) {
		const git_error *e = git_error_last();
		fprintf(stderr, "git-squares: %s\n", e->message);
		return 1;
	}

	destination.signature->when = (git_time) {
		git_commit_time(commit),
		git_commit_time_offset(commit),
		git_commit_time_offset(commit) < 0 ? '-' : '+'
	};

	git_oid new_commit_oid;
	error = git_commit_create_v(
		&new_commit_oid,
		destination.repo,
		"HEAD",
		destination.signature,
		destination.signature,
		"UTF-8",
		hash,
		destination.head.tree,
		1,
		parent
	);
	if (error) {
		const git_error *e = git_error_last();
		fprintf(stderr, "git-squares: %s\n", e->message);
		return 1;
	}

	git_commit_free(parent);

	git_oid_cpy(&destination.head.oid, &new_commit_oid);

	register_commit(&new_commit_oid, git_commit_time(commit));

	return 0;
}

static int import_repository(const char *path) {

	git_repository *repo = NULL;
	int error = git_repository_open(&repo, path);
	if (error) {
		const git_error *e = git_error_last();
		fprintf(stderr, "git-squares: %s\n", e->message);
		return 1;
	}

	error = walk_repository(repo, import_commit);

	git_repository_free(repo);

	return error;
}

int main(int argc, char **argv) {

	git_libgit2_init();

	int error = open_repository(".");
	if (error) {
		git_libgit2_shutdown();
		return 1;
	}

	for (int i = 1; ! error && i < argc; i++) {
		error = import_repository(argv[i]);
	}

	close_repository();

	git_libgit2_shutdown();

	return error;
}
