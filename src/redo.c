#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <sys/stat.h>

#include <skalibs/djbunix.h>
#include <skalibs/bytestr.h>

#include "reporting.h"
#include "environment.h"
#include "path.h"
#include "prereqs.h"
#include "stralloc_string.h"

#include "options.h"

#include "redo.h"

extern int ftruncate(int, off_t);

static
int
lookup_params(stralloc *dofile, stralloc *basename, const char *targetfile) {
	stralloc_copyb(dofile, targetfile, str_len(targetfile) + 1);
	stralloc_string_cats1(dofile, ".do");

	stralloc_copyb(basename, targetfile, str_len(targetfile) + 1);

	if(path_exists(dofile->s)) {
		prereq_record_source(dofile->s);
		return 1;
	} else {
		prereq_record_absent(dofile->s);
	}

	for(const char *s = targetfile + str_chr(targetfile, '.'); *s; s += str_chr(s + 1, '.') + 1) {
		static const char *const wildcards[] = { "_", "default", NULL };
		for(const char *const *w = wildcards; *w; w++) {
			dofile->len = 0;
			stralloc_string_cats3(dofile, *w, s, ".do");

			if(path_exists(dofile->s)) {
				basename->len = (basename->len-1) - ((dofile->len-1) - str_len(*w) - str_len(".do"));
				basename->s[basename->len++] = '\0';
				prereq_record_source(dofile->s);
				return 1;
			} else {
				prereq_record_absent(dofile->s);
			}
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

		unsigned int sep_offset = str_rchr(target, '/');
		const char *targetfile = target;
		if(target[sep_offset] != '\0') {
			char workdir[sep_offset + 1];
			byte_copy(workdir, sep_offset, target);
			workdir[sep_offset] = '\0';
			if(chdir(workdir) == -1) {
				die_errno("chdir('%s') failed", workdir);
			}
			targetfile += sep_offset + 1;
		}

		stralloc dofile = STRALLOC_ZERO;
		stralloc basename = STRALLOC_ZERO;

		if(!lookup_params(&dofile, &basename, targetfile)) {
			die("No dofile for target '%s'. Stop.", target);
		}

		stralloc infostr = STRALLOC_ZERO;
		stralloc_string_cats5(&infostr,
			redo_getenv_str(REDO_ENV_PARENTS, ""), "\t", dofile.s, ":", target
		);
		redo_setenv_str(REDO_ENV_PARENTS, infostr.s);
		stralloc_free(&infostr);
		info("%s", "...");

		stralloc_insertb(&dofile, 0, "./", 2);

		execlp(dofile.s, dofile.s + 2, targetfile, basename.s, "/dev/fd/1", (char *)NULL);
		die_errno("execlp('%s', '%s', '%s', '%s', '%s', NULL) failed",
			dofile.s, dofile.s + 2, targetfile, basename.s, "/dev/fd/1"
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
	int err;
	stralloc dbfile = STRALLOC_ZERO;
	stralloc_string_cats2(&dbfile, target, ":redo.db");

	int outfile_isempty = 0;
	int dbfile_needstruncate = 0;
	// TODO don't discard errno info
	int dbfd = open_excl(dbfile.s);
	if(dbfd == -1) {
		if(errno != EEXIST) {
			error("open_excl('%s') failed", dbfile.s);
			err = 1;
			goto cleanup_dbfile;
		}
		dbfd = open_write(dbfile.s);
		if(dbfd == -1) {
			error("open_write('%s') failed", dbfile.s);
			err = 1;
			goto cleanup_dbfile;
		}
		dbfile_needstruncate = 1;
	}
	if(lock_exnb(dbfd) == -1) {
		if(errno != EWOULDBLOCK) {
			error("lock_exnb(%d) failed", dbfd);
			err = 1;
			goto cleanup_dbfd;
		}
		if(lock_ex(dbfd) == -1) {
			error("lock_ex(%d) failed", dbfd);
			err = 1;
			goto cleanup_dbfd;
		}
		err = 0;
		goto cleanup_dbfd;
	}

	if(dbfile_needstruncate && ftruncate(dbfd, 0) == -1) {
		error("ftruncate(%d, %d) failed", dbfd, 0);
		err = 1;
		goto cleanup_dbfd;
	}

	stralloc outfile = STRALLOC_ZERO;
	stralloc_string_cats2(&outfile, target, ":redo.out");

	int outfd = open_trunc(outfile.s);
	if(outfd == -1) {
		error("open_trunc('%s') failed", outfile.s);
		err = 1;
		goto cleanup_outfile;
	}

	err = build(target, dbfd, outfd);
	if(err) {
		error("build '%s' returned %d", target, err);
		goto cleanup_outfd;
	}

	struct stat sb;
	if(stat(outfile.s, &sb) == -1) {
		error("stat('%s', ...) failed", outfile.s);
		err = 1;
		goto cleanup_outfd;
	} else if(sb.st_size == 0) {
		outfile_isempty = 1;
	} else if(rename(outfile.s, target) == -1) {
		error("rename('%s', '%s') failed", outfile.s, target);
		err = 1;
		goto cleanup_outfd;
	}

	if(prereqs_renamefor(target, dbfile.s) == -1) {
		error("prereqs_renamefor('%s', '%s') failed", target, dbfile.s);
		err = 1;
		goto cleanup_outfile;
	}

cleanup_outfd:
	if((outfile_isempty || err) && unlink(outfile.s) == -1) {
		error("unlink('%s') failed", outfile.s);
	}
	fd_close(outfd);
cleanup_outfile:
	stralloc_free(&outfile);
	// lock on dbfd is released when fd is closed
cleanup_dbfd:
	if(err && unlink(dbfile.s) == -1) {
		error("unlink('%s') failed", dbfile.s);
	}
	fd_close(dbfd);
cleanup_dbfile:
	stralloc_free(&dbfile);

	return err;
}

void
shuffle_array(unsigned int seed, char *arr[], size_t len) {
	srand(seed);
	for(int i = len - 1; i >= 1; i--) {
		int j = (rand() % i);
		char *tmp = arr[i];
		arr[i] = arr[j];
		arr[j] = tmp;
	}
}

#define GREEN   "\033[32m"
#define YELLOW  "\033[33m"
#define RED     "\033[31m"
#define BOLD    "\033[1m"
#define PLAIN   "\033[m"

void
redo_err(const char *fmt, va_list params) {
	char prefix[4096];
	snprintf(prefix, sizeof(prefix), "%s%s\t", RED"redo"BOLD, redo_getenv_str(REDO_ENV_PARENTS, ""));
	vreportf(prefix, PLAIN, fmt, params);
}

void
redo_warn(const char *fmt, va_list params) {
	char prefix[4096];
	snprintf(prefix, sizeof(prefix), "%s%s\t", YELLOW"redo"BOLD, redo_getenv_str(REDO_ENV_PARENTS, ""));
	vreportf(prefix, PLAIN, fmt, params);
}

void
redo_info(const char *fmt, va_list params) {
	char prefix[4096];
	snprintf(prefix, sizeof(prefix), "%s%s\t", GREEN"redo"BOLD, redo_getenv_str(REDO_ENV_PARENTS, ""));
	vreportf(prefix, PLAIN, fmt, params);
}

#undef GREEN
#undef YELLOW
#undef RED
#undef BOLD
#undef PLAIN

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

	if(argc == 1) {
		char *newargv[] = { argv[0], REDO_DEFAULT_TARGET, (char *)NULL };
		argc = 2;
		argv = newargv;
	} else if(argc > 2) {
		unsigned int seed = redo_getenv_int(REDO_ENV_SHUFFLE, 0);
		if(seed) {
			shuffle_array(seed, argv + 1, argc - 1);
		}
	}

	int rv = 0;
	for(int i = 1; !rv && i < argc; i++) {
		rv = redo(argv[i]);
	}

	return rv;
}
