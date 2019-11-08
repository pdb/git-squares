#include <git2.h>
#include <stdio.h>

static struct {
	git_repository *repo;
	git_signature *signature;
	struct {
		git_oid oid;
		git_tree *tree;
	} head;
} destination = { NULL };

typedef int (*walk_func)(git_repository *repo, git_oid *oid);

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
		error = f(repo, &oid);
	}

	git_revwalk_free(walk);

	return error;
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

	return error;
}

static void close_repository() {

	git_tree_free(destination.head.tree);
	git_signature_free(destination.signature);
	git_repository_free(destination.repo);
}

static int import_commit(git_repository *repo, git_oid *oid) {

	git_commit *commit = NULL;
	int error = git_commit_lookup(&commit, repo, oid);
	if (error) {
		const git_error *e = git_error_last();
		fprintf(stderr, "git-import-squares: %s\n", e->message);
		return 1;
	}

	git_commit_free(commit);

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
