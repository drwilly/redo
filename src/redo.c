#include <stdlib.h>
#include <unistd.h>
#include <limits.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <wait.h>
#include <sys/stat.h>

#include "util.h"
#include "libc_wrapper.h"

#include "reporting.h"
#include "environment.h"
#include "path.h"
#include "predeps.h"

#include "redo.h"

struct buildparams {
	char workdir[PATH_MAX];
	char *dofile;
	char targetfile[PATH_MAX];
	char basename[NAME_MAX+1];
};

char *
read_hashbang(const char *file) {
	static char line1[PATH_MAX];

	int fd = xopen(file, O_RDONLY);
	ssize_t nbytes_read = read(fd, line1, len(line1));
	xclose(fd);

	if(nbytes_read < 3 || line1[0] != '#' || line1[1] != '!') {
		// not a hashbang
		return NULL;
	}

	char *newline = memchr(line1, '\n', nbytes_read);
	if(!newline) {
		// interpreter exceeds PATH_MAX
		return NULL;
	}
	*newline = '\0';

	return &line1[2];

}

/*
 * in
 * 	target (relative to cwd)
 * out
 * 	workdir (dofile location)
 * 	dofile name
 * 	targetfile (relative to dofile location)
 * 	basename
 * 	dofile candidates (relative to target location)
 */
int
foo(struct buildparams *bp, const char *target) {
	// TODO dofile candidates
	// TODO explanation
	strcpy(bp->targetfile, path_absolute(target));
	strcpy(bp->workdir, bp->targetfile);

	bp->dofile = strrchr(bp->workdir, '/') + 1;
	strcpy(bp->basename, bp->dofile);

	// TODO
	// MAXDEPTH is the number of parent directories we visit
	for(int MAXDEPTH = 0; MAXDEPTH >= 0; MAXDEPTH--) {
		sprintf(bp->dofile, "%s.do", bp->basename);
		if(path_exists(bp->workdir)) {
			// split workdir / dofile
			*(bp->dofile - 1) = '\0';

			// strip ext
			char *ext = strrchr(bp->basename, '.');
			if(ext)
				*ext = '\0';

			return 0;
		}
		for(char *ext = strchr(bp->basename, '.'); ext; ext = strchr(ext, '.')) {
			ext++;
			static const char *wildcard_patterns[] = { "_.%s.do", "default.%s.do" };
			for(int i = 0; i < len(wildcard_patterns); i++) {
				sprintf(bp->dofile, wildcard_patterns[i], ext);
				if(path_exists(bp->workdir)) {
					// split workdir / dofile
					*(bp->dofile - 1) = '\0';

					// strip ext
					bp->basename[strlen(bp->basename) - strlen(ext) - 1] = '\0';

					return 0;
				}
			}
		}
		bp->dofile += sprintf(bp->dofile, "../");
	}

	return 1;
}

// TODO target != targetfile
int
build(const char *target, struct buildparams *bp) {
	char tmpfile[PATH_MAX];
	snprintf(tmpfile, len(tmpfile), "%s/%s", bp->workdir, "XXXXXXXXXX");
	int tmpfile_fd = mkstemp(tmpfile);
	if(tmpfile_fd == -1) {
		die_errno("Could not create tmpfile %s", tmpfile);
	}
	fcntl(tmpfile_fd, F_SETFD, FD_CLOEXEC);

	pid_t pid = fork();
	if(pid == -1) {
		return -1;
	} else if(pid) {
		xclose(tmpfile_fd);

		info("%s:%s", bp->dofile, target);

		int status;
		waitpid(pid, &status, 0);

		if(WEXITSTATUS(status) == 0) {
			struct stat sb;
			if(stat(tmpfile, &sb) != 0 || sb.st_size == 0) {
				try_unlink(tmpfile);
			} else if(rename(tmpfile, target) != 0) {
				try_unlink(tmpfile);
				return 1;
			}
		} else {
			try_unlink(tmpfile);
			error("%s:%s returned %d", bp->dofile, target, WEXITSTATUS(status));
		}

		return WEXITSTATUS(status);
	}

	if(chdir(bp->workdir) == -1) {
		die_errno("Could not chdir to %s", bp->workdir);
	}

/*
fprintf(stderr, "Build params:\n\tworkdir=%s\n\tdofile=%s\n\ttargetfile=%s\n\tbasename=%s\n",
	bp->workdir, bp->dofile, bp->targetfile, bp->basename
);
*/

	if(dup2(tmpfile_fd, STDOUT_FILENO) == -1) {
		die_errno("Could not redirect stdout to fd %d", tmpfile_fd);
	}

	char dbfile[PATH_MAX];
	snprintf(dbfile, PATH_MAX, "%s.predeps", bp->targetfile);
	int db_fd = xopen(dbfile, O_WRONLY|O_CREAT|O_TRUNC, 0644);

	predep_record(db_fd, 's', bp->dofile);

	redo_setenv_int(REDO_ENV_DEPTH, redo_getenv_int(REDO_ENV_DEPTH, 0) + 1);
	redo_setenv_int(REDO_ENV_DB, db_fd);

#define exec(exe, ...) execl(exe, exe, __VA_ARGS__, (char *)NULL)
	char *interpreter = read_hashbang(bp->dofile);
	if(interpreter) {
		return exec(interpreter, bp->dofile, target, bp->basename, "/dev/fd/1");
	} else {
		// FIXME "-e" goes somewhere else
		return exec(REDO_DEFAULT_INTERPRETER, "-e", bp->dofile, target, bp->basename, "/dev/fd/1");
	}
#undef exec
}

int
redo(const char *target) {
	int rv = 0;

	struct buildparams bp;
	if(foo(&bp, target) == 0) {
		rv = build(target, &bp);
	} else {
		error("No dofile for target '%s'. Stop.", target);
		if(0 /* db-exists */) {
			// no rule, but dbfile?
			warn("");
		}
		rv = 1;
	}

	return rv;
}

int
redo_ifchange(const char *target, int parent_db) {
	int rv = 0;

	struct buildparams bp;
	if(foo(&bp, target) == 0) {
		if(!path_exists(target)) {
			/* rebuild (virtual target or clean build) */
			rv = build(target, &bp);
		} else if(predeps_changed(target)) {
			/* rebuild (deps dirty) */
			rv = build(target, &bp);
		}
		if(rv == 0) {
			predep_record(parent_db, 't', target);
		}
	} else if(path_exists(target)) {
		predep_record(parent_db, 's', target);
	} else {
		error("No dofile for target '%s'. Stop.", target);
		if(0 /* db-exists */) {
			// no rule, but dbfile?
			warn("");
		}
		rv = 1;
	}

	return rv;
}

int
redo_ifcreate(const char *target, int parent_db) {
	predep_record(parent_db, 'n', target);
	return path_exists(target);
}
