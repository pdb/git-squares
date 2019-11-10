# git-squares

It may not always be possible to publish Git repositories on GitHub, though it
may still be desirable to see personal work on these repositories reflected in
a GitHub contributions graph.

**git-squares** enables this by mirroring minimal commit data from any number
of local repositories to one (or more) repositories pushed to GitHub for the
sole purpose of providing input for GitHub contribution graphs.

The only data exported from local repositories are:
* commit hashes; and
* commit times.

No other potentially sensitive data is exported - no repository names or paths,
no branch or tag names, no commit messages or content, no author or committer
details, etc.

The only dependency for this project is
[libgit2](https://github.com/libgit2/libgit2).

## Installation

**git-squares** can be built and installed from a fresh clone of this
repository using the following commands:

```
$ autoreconf -i
$ ./configure
$ make
$ make install
```

This will install the `git-squares` binary which can be invoked directly, or -
if installed to a location specified in `$PATH` - indirectly via `git` itself:

<pre><code>$ git squares <i>command</i> [<i>args</i>]
</code></pre>

## Usage

### Setup

**git-squares** can manage its data in any repository, though a dedicated
repository - e.g., <code><i>user</i>/squares</code> - is strongly recommended.
This repository, in Github, may either be public or private (if the "Include
private contributions on my profile" option is selected).

The only requirement is that this _squares repository_ have at least one
commit. Commits created by **git-squares** may be intermingled with _any_ other
commits and so this repository can simply start with an empty initial commit.

### Importing Commits

Commits from any number of local repositories may be imported by running:

<pre><code>$ git squares <b>import</b> <i>REPO</i> ...
</code></pre>

from _within_ a squares repository.

This will scan all branches in all specified repositories for unknown commits
and create a new commit, on the HEAD of the squares repository, for each one
found.

As commit hashes are recorded in the squares repository this command can be run
against the same local repositories multiple times without creating duplicate
commits in the squares repository.
