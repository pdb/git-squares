#include <git2.h>
#include <stdio.h>

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

	int result = 0;
	for (int i = 1; i < argc && ! result; i++) {
		result = import_repository(argv[1]);
	}

	git_libgit2_shutdown();

	return result;
}
