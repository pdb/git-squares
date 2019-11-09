#include "squares.h"

int main(int argc, char **argv) {

	git_libgit2_init();

	struct destination *destination;
	int error = open_repository(&destination, ".");
	if (error) {
		git_libgit2_shutdown();
		return 1;
	}

	for (int i = 1; ! error && i < argc; i++) {
		error = import_repository(destination, argv[i]);
	}

	close_repository(destination);

	git_libgit2_shutdown();

	return error;
}
