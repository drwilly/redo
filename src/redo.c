#include <stdlib.h>
#include <stdio.h>

#include <skalibs/djbunix.h>
#include <skalibs/stralloc.h>

#include "reporting.h"
#include "environment.h"
#include "path.h"
#include "predeps.h"

#include "redo.h"

static
int
run_dofile(const char *dofile, const char *targetfile, const char *basename) {
	char tmpfile[] = "./tmp-XXXXXX";
	int tmpfile_fd = mkstemp(tmpfile);
	if(tmpfile_fd == -1) {
		die_errno("Could not create temporary outfile '%s'", tmpfile);
	}

	pid_t pid = fork();
	if(pid == -1) {
		die_errno("fork failed");
	} else if(!pid) {
		if(fd_move(1, tmpfile_fd) == -1) {
			die_errno("Could not redirect stdout to '%s'", tmpfile);
		}

		redo_setenv_int(REDO_ENV_DEPTH, redo_getenv_int(REDO_ENV_DEPTH, 0) + 1);

		stralloc dotslashdofile = STRALLOC_ZERO;
		stralloc_cats(&dotslashdofile, "./");
		stralloc_cats(&dotslashdofile, dofile);
		stralloc_0(&dotslashdofile);

		if(execlp(dotslashdofile.s, dofile, targetfile, basename, "/dev/fd/1", (char *)NULL) == -1) {
			die_errno("execlp('%s', '%s', '%s', '%s', '%s') failed",
				dotslashdofile.s, dofile, targetfile, basename, "/dev/fd/1"
			);
		}
		return -1; // unreached
	} else {
		fd_close(tmpfile_fd);

		int status;
		waitpid_nointr(pid, &status, 0);

		if(WEXITSTATUS(status) == 0) {
			struct stat sb;
			if(stat(tmpfile, &sb) == 0 && sb.st_size > 0) {
				return try_link_tmpfile(tmpfile, targetfile);
			} else {
				try_unlink(tmpfile);
			}
		} else {
			try_unlink(tmpfile);
		}

		return WEXITSTATUS(status);
	}
}

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
		stralloc_cats(dofile, &targetfile->s[str_chr(targetfile->s, '.')]);
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

static
int
redo(const char *target) {
	stralloc workdir = STRALLOC_ZERO;
	sadirname(&workdir, target, str_len(target));
	stralloc_0(&workdir);
	if(chdir(workdir.s) == -1) {
		die_errno("Could not chdir to %s", workdir.s);
	}
	stralloc_free(&workdir);

	char tmpdbfile[] = "./tmp-XXXXXX";
	switch(mkstemp(tmpdbfile)) {
	case  3: break; // ok
	case -1: die_errno("Could not create temporary dbfile '%s'", tmpdbfile);
	default: die("Could not create temporary dbfile '%s' on fd %d", tmpdbfile, 3);
	}

	stralloc targetfile = STRALLOC_ZERO;
	stralloc dofile = STRALLOC_ZERO;
	stralloc basename = STRALLOC_ZERO;

	int rv = 0;
	if(lookup_params(&dofile, &targetfile, &basename, target)) {
		info("%s:%s", dofile.s, target);
		rv = run_dofile(dofile.s, targetfile.s, basename.s);
		if(rv == 0) {
			stralloc dbfile = STRALLOC_ZERO;
			predeps_sadbfile(&dbfile, targetfile.s);
			stralloc_0(&dbfile);
			if(try_link_tmpfile(tmpdbfile, dbfile.s) != 0) {
				die_errno("Could not store target dependencies in '%s'", dbfile.s);
			}
			stralloc_free(&dbfile);
		} else {
			error("%s:%s returned %d", dofile.s, target, rv);
			try_unlink(tmpdbfile);
		}
	} else {
		rv = 1;
		error("No dofile for target '%s'. Stop.", target);
		if(0 /* db-exists */) {
			// no rule, but dbfile?
			warning("");
		}
	}

	fd_close(3);

	stralloc_free(&targetfile);
	stralloc_free(&dofile);
	stralloc_free(&basename);

	return rv;
}

// TODO remove
#define len(x) (sizeof(x) / sizeof(*x))

static
int
options(int argc, char *argv[]) {
	int c = 1;
	for(int i = 1; i < argc; i++) {
		if(argv[i][0] != '-') {
			argv[c++] = argv[i];
		} else if(strcmp(argv[i], "--") == 0) {
			while(++i < argc) {
				argv[c++] = argv[i];
			}
		} else if(strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0) {
			// TODO
		} else if(strcmp(argv[i], "--version") == 0 || strcmp(argv[i], "-v") == 0) {
			// TODO
		} else if(strcmp(argv[i], "--jobs") == 0 || strcmp(argv[i], "-j") == 0) {
			int jobs = 0;
			if(i+1 < argc && strspn(argv[i+1], "0123456789") == strlen(argv[i+1])) {
				jobs = atoi(argv[++i]);
			}
			redo_setenv_int(REDO_ENV_JOBS, jobs);
		} else if(strcmp(argv[i], "--keep-going") == 0 || strcmp(argv[i], "-k") == 0) {
			redo_setenv_int(REDO_ENV_KEEPGOING, 1);
		} else if(strcmp(argv[i], "--debug") == 0 || strcmp(argv[i], "-d") == 0) {
			redo_setenv_int(REDO_ENV_DEBUG, redo_getenv_int(REDO_ENV_DEBUG, 0) + 1);
		} else if(strcmp(argv[i], "--no-color") == 0) {
			redo_setenv_int(REDO_ENV_NOCOLOR, 1);
		} else if(strcmp(argv[i], "--shuffle") == 0) {
			int seed = 1;
			if(i+1 < argc && strspn(argv[i+1], "0123456789") == strlen(argv[i+1])) {
				seed = atoi(argv[++i]);
			}
			redo_setenv_int(REDO_ENV_SHUFFLE, seed);
		}
	}

	return c;
}

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
	argc = options(argc, argv);

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
			int j = rand() % (i + 1);
			if(i == j)
				continue;
			char *tmp = argv[i];
			argv[i] = argv[j];
			argv[j] = tmp;
		}
	}

	if(argc <= 1)
		return redo(REDO_DEFAULT_TARGET);

	int rv = 0;
	stralloc cwd = STRALLOC_ZERO;
	sagetcwd(&cwd);
	stralloc_0(&cwd);
	for(int i = 1; i < argc; i++) {
		rv = redo(argv[i]);
		chdir(cwd.s);
		if(rv != 0) {
			break;
		}
	}
	stralloc_free(&cwd);

	return rv;
}
