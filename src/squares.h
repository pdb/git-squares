#ifndef SQUARES_H_
#define SQUARES_H_

#include <git2.h>

struct commit {
	git_oid oid;
	git_time_t time;
	struct commit *next;
};

struct destination {
	git_repository *repo;
	git_signature *signature;
	struct {
		git_oid oid;
		git_tree *tree;
	} head;
	struct commit *commits;
};

int register_commit(struct destination *destination, git_oid *oid,
	git_time_t time);
int open_repository(struct destination **destination, const char *path);
void close_repository(struct destination *destination);

int import_repository(struct destination *destination, const char *path);

typedef int (*walk_func)(struct destination *destination, git_repository *repo,
	git_oid *oid, git_commit *commit);

int walk_repository(struct destination *destination, git_repository *repo,
	walk_func f);

#endif
