#include <git2.h>
#include <stdio.h>
#include <string.h>

struct commit {
	git_oid oid;
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
		fprintf(stderr, "git-import-squares: %s\n", e->message);
		return 1;
	}

	git_oid oid;
	while (! error && ! git_revwalk_next(&oid, walk)) {
		git_commit *commit;
		error = git_commit_lookup(&commit, repo, &oid);
		if (error) {
			const git_error *e = git_error_last();
			fprintf(stderr, "git-import-squares: %s\n", e->message);
		} else {
			error = f(repo, &oid, commit);
			git_commit_free(commit);
		}
	}

	git_revwalk_free(walk);

	return error;
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
	struct commit *c = malloc(sizeof(struct commit));
	if (! c) {
		return 1;
	}
	git_oid_cpy(&c->oid, &commit_oid);
	c->next = destination.commits;
	destination.commits = c;

	return 0;
}

static int open_repository(const char *path) {

	int error = git_repository_open(&destination.repo, path);
	if (error) {
		const git_error *e = git_error_last();
		fprintf(stderr, "git-import-squares: %s\n", e->message);
		return 1;
	}

	error = git_signature_default(&destination.signature, destination.repo);
	if (error) {
		const git_error *e = git_error_last();
		fprintf(stderr, "git-import-squares: %s\n", e->message);
		git_repository_free(destination.repo);
		return 1;
	}

	error = git_reference_name_to_id(&destination.head.oid,
		destination.repo, "HEAD");
	if (error) {
		const git_error *e = git_error_last();
		fprintf(stderr, "git-import-squares: %s\n", e->message);
		git_signature_free(destination.signature);
		git_repository_free(destination.repo);
		return 1;
	}

	git_commit *commit;
	error = git_commit_lookup(&commit, destination.repo,
		&destination.head.oid);
	if (error) {
		const git_error *e = git_error_last();
		fprintf(stderr, "git-import-squares: %s\n", e->message);
		git_signature_free(destination.signature);
		git_repository_free(destination.repo);
		return 1;
	}

	error = git_commit_tree(&destination.head.tree, commit);
	if (error) {
		const git_error *e = git_error_last();
		fprintf(stderr, "git-import-squares: %s\n", e->message);
		git_commit_free(commit);
		git_signature_free(destination.signature);
		git_repository_free(destination.repo);
		return 1;
	}

	git_commit_free(commit);

	error = walk_repository(destination.repo, load_commit);
	if (error) {
		const git_error *e = git_error_last();
		fprintf(stderr, "git-import-squares: %s\n", e->message);
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

	return 0;
}

static int import_repository(const char *path) {

	git_repository *repo = NULL;
	int error = git_repository_open(&repo, path);
	if (error) {
		const git_error *e = git_error_last();
		fprintf(stderr, "git-import-squares: %s\n", e->message);
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
		error = import_repository(argv[1]);
	}

	close_repository();

	git_libgit2_shutdown();

	return error;
}
