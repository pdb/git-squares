#ifndef SQUARES_H_
#define SQUARES_H_

#include <git2.h>

typedef int (*walk_func)(git_repository *repo, git_oid *oid, git_commit *commit);

int walk_repository(git_repository *repo, walk_func f);

#endif
