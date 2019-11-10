#include "squares.h"

int main(int argc, char **argv) {

	git_libgit2_init();

	squares_repo *r;
	int error = open_repository(&r, ".");
	if (error) {
		git_libgit2_shutdown();
		return 1;
	}

	for (int i = 1; ! error && i < argc; i++) {
		error = import_repository(r, argv[i]);
	}

	close_repository(r);

	git_libgit2_shutdown();

	return error;
}
