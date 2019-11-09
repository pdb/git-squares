#include "squares.h"

struct destination destination = { NULL };

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
