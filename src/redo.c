#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

#include <skalibs/djbunix.h>
#include <skalibs/stralloc.h>

#include "reporting.h"
#include "environment.h"
#include "path.h"
#include "predeps.h"

#include "options.h"

#include "redo.h"

static
int
lookup_params(stralloc *dofile, stralloc *targetfile, stralloc *basename, const char *target) {
	sabasename(targetfile, target, str_len(target));

	stralloc_copy(dofile, targetfile);
	stralloc_cats(dofile, ".do");
	stralloc_0(dofile);

	stralloc_copy(basename, targetfile);
	stralloc_0(basename);

	stralloc_0(targetfile);

	if(path_exists(dofile->s)) {
		predep_record_source(dofile->s);
		return 1;
	} else {
		predep_record_absent(dofile->s);
	}

	static const char *const wildcards[] = { "_", "default", NULL };
	for(int i = 0; wildcards[i]; i++) {
		stralloc_copys(dofile, wildcards[i]);
		stralloc_cats(dofile, targetfile->s + str_chr(targetfile->s, '.'));
		stralloc_cats(dofile, ".do");
		stralloc_0(dofile);

		if(path_exists(dofile->s)) {
			basename->len = (basename->len-1) - ((dofile->len-1) - str_len(wildcards[i]) - str_len(".do"));
			stralloc_0(basename);
			predep_record_source(dofile->s);
			return 1;
		} else {
			predep_record_absent(dofile->s);
		}
	}

	return 0;
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
		for(int i = argc - 1; i >= 1; i--) {
			int j = rand() % i;
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
		char *target = argv[i];

		stralloc dbfile = STRALLOC_ZERO;
		stralloc_cats(&dbfile, target);
		stralloc_cats(&dbfile, ":redo.db");
		stralloc_0(&dbfile);

		int dbfd = open_excl(dbfile.s);
		if(dbfd == -1) {
//			die_errno("dbfile '%s'", dbfile.s);
			error("open_excl('%s') failed", dbfile.s);
			rv = 1;
			goto cleanup_dbfile;
		}

		stralloc outfile = STRALLOC_ZERO;
		stralloc_cats(&outfile, target);
		stralloc_cats(&outfile, ":redo.out");
		stralloc_0(&outfile);

		int outfd = open_excl(outfile.s);
		if(outfd == -1) {
//			die_errno("outfile '%s'", outfile.s);
			error("open_excl('%s') failed", outfile.s);
			rv = 1;
			goto cleanup_outfile;
		}

		pid_t pid = fork();
		if(pid == -1) {
			die_errno("fork() failed");
		} else if(!pid) {
			if(fd_move(3, dbfd) == -1) {
				die_errno("fd_move(%d, %d) failed", 3, dbfd);
			}
			if(fd_move(1, outfd) == -1) {
				die_errno("fd_move(%d, %d) failed", 1, outfd);
			}

			stralloc workdir = STRALLOC_ZERO;
			sadirname(&workdir, target, str_len(target));
			stralloc_0(&workdir);
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

			static const char *dotslash = "./";
			char dotslashdofile[str_len(dotslash) + str_len(dofile.s) + 1];
			str_copy(dotslashdofile, dotslash);
			str_copy(dotslashdofile + str_len(dotslash), dofile.s);

			execlp(dotslashdofile, dofile.s, targetfile.s, basename.s, "/dev/fd/1", (char *)NULL);
			die_errno("execlp('%s', '%s', '%s', '%s', '%s', NULL) failed",
				dotslashdofile, dofile.s, targetfile.s, basename.s, "/dev/fd/1"
			);
		} else {
			int status;
			waitpid_nointr(pid, &status, 0);

			if(WEXITSTATUS(status) == 0) {
				if(predeps_renamefor(target, dbfile.s) == -1) {
					error("predeps_renamefor('%s', '%s') failed", target, dbfile.s);
					rv = 1;
				}
				struct stat sb;
				if(stat(outfile.s, &sb) == 0 && sb.st_size > 0) {
					if(rename(outfile.s, target) == -1) {
						error("rename(%d, '%s') failed", outfile, target);
						rv = 1;
					}
				}
			} else {
				error("%s:%s returned %d", "TODO", target, WEXITSTATUS(status));
				rv = 1;
			}
		}
		unlink(outfile.s);
		fd_close(outfd);
cleanup_outfile:
		stralloc_free(&outfile);
		unlink(dbfile.s);
		fd_close(dbfd);
cleanup_dbfile:
		stralloc_free(&dbfile);
	}

	return rv;
}
