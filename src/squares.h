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

extern struct destination destination;

int register_commit(git_oid *oid, git_time_t time);
int open_repository(const char *path);
void close_repository();

int import_repository(const char *path);

typedef int (*walk_func)(git_repository *repo, git_oid *oid, git_commit *commit);

int walk_repository(git_repository *repo, walk_func f);

#endif
