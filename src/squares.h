#ifndef SQUARES_H_
#define SQUARES_H_

#include <git2.h>

struct commit {
	git_oid oid;
	git_time_t time;
	struct commit *next;
};

typedef struct {
	git_repository *repo;
	git_signature *signature;
	struct {
		git_oid oid;
		git_tree *tree;
	} head;
	struct commit *commits;
} squares_repo;

int register_commit(squares_repo *destination, git_oid *oid,
	git_time_t time);
int open_repository(squares_repo **destination, const char *path);
void close_repository(squares_repo *destination);

int import_repository(squares_repo *destination, const char *path);

typedef int (*walk_func)(squares_repo *destination, git_repository *repo,
	git_oid *oid, git_commit *commit);

int walk_repository(squares_repo *destination, git_repository *repo,
	walk_func f);

#endif
