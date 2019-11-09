#include "squares.h"

#include <stdio.h>

int walk_repository(git_repository *repo, walk_func f) {

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
