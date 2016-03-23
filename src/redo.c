#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

#include <skalibs/djbunix.h>

#include "reporting.h"
#include "environment.h"
#include "path.h"
#include "prereqs.h"
#include "stralloc_string.h"

#include "options.h"

#include "redo.h"

static
int
lookup_params(stralloc *dofile, stralloc *targetfile, stralloc *basename, const char *target) {
	stralloc_string_basename(targetfile, target, str_len(target));

	stralloc_copy(dofile, targetfile);
	stralloc_string_cats1(dofile, ".do");

	stralloc_copy(basename, targetfile);

	if(path_exists(dofile->s)) {
		prereq_record_source(dofile->s);
		return 1;
	} else {
		prereq_record_absent(dofile->s);
	}

	static const char *const wildcards[] = { "_", "default", NULL };
	for(int i = 0; wildcards[i]; i++) {
		dofile->len = 0;
		stralloc_string_cats3(dofile, wildcards[i], targetfile->s + str_chr(targetfile->s, '.'), ".do");

		if(path_exists(dofile->s)) {
			basename->len = (basename->len-1) - ((dofile->len-1) - str_len(wildcards[i]) - str_len(".do"));
			basename->s[basename->len++] = '\0';
			prereq_record_source(dofile->s);
			return 1;
		} else {
			prereq_record_absent(dofile->s);
		}
	}

	return 0;
}

static
int
build(const char *target, int dbfd, int outfd) {
	pid_t pid = fork();
	if(pid == -1) {
		die_errno("fork() failed");
	} else if(!pid) {
		if(fd_move2(3, dbfd, 1, outfd) == -1) {
			die_errno("fd_move2(%d, %d, %d, %d) failed", 3, dbfd, 1, outfd);
		}

		stralloc workdir = STRALLOC_ZERO;
		stralloc_string_dirname(&workdir, target, str_len(target));
		if(chdir(workdir.s) == -1) {
			die_errno("chdir('%s') failed", workdir.s);
		}
		stralloc_free(&workdir);

		stralloc targetfile = STRALLOC_ZERO;
		stralloc dofile = STRALLOC_ZERO;
		stralloc basename = STRALLOC_ZERO;

		if(!lookup_params(&dofile, &targetfile, &basename, target)) {
			die("No dofile for target '%s'. Stop.", target);
		}

		info("%s:%s", dofile.s, target);

		redo_setenv_int(REDO_ENV_DEPTH, redo_getenv_int(REDO_ENV_DEPTH, 0) + 1);

		stralloc dotslashdofile = STRALLOC_ZERO;
		stralloc_string_cats2(&dotslashdofile, "./", dofile.s);

		execlp(dotslashdofile.s, dofile.s, targetfile.s, basename.s, "/dev/fd/1", (char *)NULL);
		die_errno("execlp('%s', '%s', '%s', '%s', '%s', NULL) failed",
			dotslashdofile.s, dofile.s, targetfile.s, basename.s, "/dev/fd/1"
		);
	} else {
		int status;
		waitpid_nointr(pid, &status, 0);

		return WEXITSTATUS(status);
	}
}

static
int
redo(const char *target) {
	int rv;
	stralloc dbfile = STRALLOC_ZERO;
	stralloc_string_cats2(&dbfile, target, ":redo.db");

	int dbfd = open_excl(dbfile.s);
	if(dbfd == -1) {
//		die_errno("dbfile '%s'", dbfile.s);
		error("open_excl('%s') failed", dbfile.s);
		rv = 1;
		goto cleanup_dbfile;
	}

	stralloc outfile = STRALLOC_ZERO;
	stralloc_string_cats2(&outfile, target, ":redo.out");

	int outfd = open_excl(outfile.s);
	if(outfd == -1) {
//		die_errno("outfile '%s'", outfile.s);
		error("open_excl('%s') failed", outfile.s);
		rv = 1;
		goto cleanup_outfile;
	}
	rv = build(target, dbfd, outfd);
	if(rv == 0) {
		if(prereqs_renamefor(target, dbfile.s) == -1) {
			error("prereqs_renamefor('%s', '%s') failed", target, dbfile.s);
			rv = 1;
			goto cleanup_all;
		}
		struct stat sb;
		if(stat(outfile.s, &sb) == 0 && sb.st_size > 0) {
			if(rename(outfile.s, target) == -1) {
				error("rename('%s', '%s') failed", outfile.s, target);
				rv = 1;
				goto cleanup_all;
			}
		}
	} else {
		error("build '%s' returned %d", target, rv);
	}

cleanup_all:
	unlink(outfile.s);
	fd_close(outfd);
cleanup_outfile:
	stralloc_free(&outfile);
	unlink(dbfile.s);
	fd_close(dbfd);
cleanup_dbfile:
	stralloc_free(&dbfile);

	return rv;
}

// TODO remove
#define len(x) (sizeof(x) / sizeof(*x))

void
redo_err(const char *fmt, va_list params) {
	char msg[4096];
	vsnprintf(msg, len(msg), fmt, params);
	fprintf(stderr, "\033[31m%-*s\033[1m%s\033[m\n", 4 + (2 * (1 + redo_getenv_int(REDO_ENV_DEPTH, 0))), "redo", msg);
}

void
redo_warn(const char *fmt, va_list params) {
	char msg[4096];
	vsnprintf(msg, len(msg), fmt, params);
	fprintf(stderr, "\033[33m%-*s\033[1m%s\033[m\n", 4 + (2 * (1 + redo_getenv_int(REDO_ENV_DEPTH, 0))), "redo", msg);
}

void
redo_info(const char *fmt, va_list params) {
	char msg[4096];
	vsnprintf(msg, len(msg), fmt, params);
	fprintf(stderr, "\033[32m%-*s\033[1m%s\033[m\n", 4 + (2 * (1 + redo_getenv_int(REDO_ENV_DEPTH, 0))), "redo", msg);
}

int
main(int argc, char *argv[]) {
	argc = args_process_options(argc, argv);

	// TODO "nocolor" is slightly misleading because it changes the output format
	if(redo_getenv_int(REDO_ENV_NOCOLOR, 1) != 0) {
		//set_die_routine(&redo_err);
		set_error_routine(&redo_err);
		set_warning_routine(&redo_warn);
		set_info_routine(&redo_info);
	}

	unsigned int seed = redo_getenv_int(REDO_ENV_SHUFFLE, 0);
	if(seed) {
		srand(seed);
		for(int i = argc - 1; i >= 2; i--) {
			int j = (rand() % i) + 1;
			char *tmp = argv[i];
			argv[i] = argv[j];
			argv[j] = tmp;
		}
	}

	if(argc <= 1) {
		char *newargv[] = { "redo", REDO_DEFAULT_TARGET, (char *)NULL };
		argc = 2;
		argv = newargv;
	}

	int rv = 0;
	for(int i = 1; !rv && i < argc; i++) {
		rv = redo(argv[i]);
	}

	return rv;
}
