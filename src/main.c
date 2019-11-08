#include <git2.h>
#include <stdio.h>

static int import_repository(const char *path) {

	git_repository *repo = NULL;
	int error = git_repository_open(&repo, path);
	if (error) {
		const git_error *e = git_error_last();
		fprintf(stderr, "git-import-squares: %s\n", e->message);
		return 1;
	}

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
