#include <git2.h>
#include <stdio.h>

static int import_commit(git_oid *oid) {

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

	git_revwalk *walk = NULL;
	git_revwalk_new(&walk, repo);

	error = git_revwalk_push_glob(walk, "*");
	if (error) {
		const git_error *e = git_error_last();
		fprintf(stderr, "git-import-squares: %s\n", e->message);
		return 1;
	}

	git_oid oid;
	while (! git_revwalk_next(&oid, walk) && ! import_commit(&oid)) { }

	git_revwalk_free(walk);
	git_repository_free(repo);

	return 0;
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
