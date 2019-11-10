#include "squares.h"

#include <stdio.h>
#include <string.h>

static void usage(const char *app) {

	fprintf(stderr, "usage: %s <command> [<args>]\n", app);
}

int main(int argc, char **argv) {

	if (argc == 1) {
		usage(argv[0]);
		return 1;
	}

	git_libgit2_init();

	int result;
	if (strcmp(argv[1], "import") == 0) {
		result = squares_import(argc - 1, argv + 1);
	} else {
		usage(argv[0]);
		result = 1;
	}

	git_libgit2_shutdown();

	return result;
}
