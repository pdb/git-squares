#ifndef SQUARES_H_
#define SQUARES_H_

#include <git2.h>

typedef struct commit squares_commit;

typedef struct commit {
	git_oid oid;
	git_time_t time;
	squares_commit *next;
} squares_commit;

typedef struct {
	git_repository *repo;
	git_signature *signature;
	struct {
		git_oid oid;
		git_tree *tree;
	} head;
	squares_commit *commits;
} squares_repo;

int register_commit(squares_repo *r, git_oid *oid, git_time_t time);
int open_repository(squares_repo **r, const char *path);
void close_repository(squares_repo *r);

int squares_import(int argc, char **argv);

typedef int (*walk_func)(git_repository *repo, git_oid *oid,
	git_commit *commit, void *closure);

int walk_repository(git_repository *repo, walk_func f, void *closure);

#endif
