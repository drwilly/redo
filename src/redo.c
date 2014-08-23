#include <stdlib.h>
#include <unistd.h>
#include <limits.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <wait.h>
#include <sys/stat.h>

#include "libc_wrapper.h"

#include "reporting.h"
#include "environment.h"
#include "path.h"
#include "predeps.h"

#include "redo.h"

#define len(x) (sizeof(x) / sizeof(*x))

char *
read_hashbang(char *file) {
	static char line1[PATH_MAX];

	int fd = xopen(file, O_RDONLY);
	ssize_t nbytes_read = read(fd, line1, len(line1));
	xclose(fd);

	if(nbytes_read < 3 || line1[0] != '#' | line1[1] != '!') {
		// not a hashbang
		return NULL;
	}

	char *newline = memchr(line1, '\n', nbytes_read);
	if(!newline) {
		// interpreter exceeds PATH_MAX
		// FIXME does not check for a '\0'-byte occurring before the newline
		return NULL;
	}
	*newline = '\0';

	return &line1[2];

}

int
run_dofile(char *workdir, char *dofile, char *targetfile, char *basename, int db_fd, int output_fd) {
	pid_t pid = fork();
	if(pid == -1) {
		die_errno("fork failed");
	} else if(pid) {
		size_t wd_len = strlen(workdir);
		info("%s:%s", &dofile[wd_len], &targetfile[wd_len]);

		int status;
		waitpid(pid, &status, 0);

		if(WEXITSTATUS(status) != 0) {
			error("%s:%s returned %d", &dofile[wd_len], &targetfile[wd_len], WEXITSTATUS(status));
		}
	
		return WEXITSTATUS(status);
	}

	if(chdir(workdir) != 0) {
		die_errno("Could not chdir to %s", workdir);
	}

	redo_setenv_int(REDO_ENV_DEPTH, redo_getenv_int(REDO_ENV_DEPTH, 0) + 1);
	redo_setenv_int(REDO_ENV_DB, db_fd);

	// redirect stdout to provided fd
	if(dup2(output_fd, STDOUT_FILENO) == -1) {
		die_errno("redirect stdout failed");
	}

#define exec(exe, ...) execl(exe, exe, __VA_ARGS__, (char *)NULL)
	char *interpreter = read_hashbang(dofile);
	if(interpreter) {
		exec(interpreter, dofile, targetfile, basename, "/dev/fd/1");
	} else {
		// FIXME "-e" goes somewhere else
		exec(REDO_DEFAULT_INTERPRETER, "-e", dofile, targetfile, basename, "/dev/fd/1");
	}
#undef exec

	return 0; // unreached
}

int
gen_dirs(char ***dirv, char *target) {
	size_t dirc = 0;
	char *end = target;
	while(*end) {
		if(*end == '/') {
			dirc++;
		}
		end++;
	}
	*dirv = xcalloc(dirc, sizeof(*dirv));

	for(int i = 0; i < dirc; i++) {
		end = memrchr(target, '/', (end - target));
		(*dirv)[i] = xcalloc(end - target + 1, sizeof(*(*dirv)[i]));
		strncpy((*dirv)[i], target, end - target + 1);
	}
	return dirc;
}

int
gen_exts(char ***extv, char *target) {
	size_t extc = 0;
	char *end = target;
	while(*end) {
		if(*end == '.') {
			extc++;
		}
		end++;
	}
	*extv = xcalloc(extc, sizeof(**extv));

	for(int i = 0; i < extc; i++) {
		target = strchr(target, '.') + 1;
		(*extv)[i] = xcalloc(end - target, sizeof(*(*extv)[i]));
		strncpy((*extv)[i], target, end - target);
	}

	return extc;
}

int
redo(const char *target) {
	char *targetfile,
	     *workdir,
	     *dofile = NULL,
	     *basename,
	     *dbfile;

	targetfile = xstrdup(path_absolute(target));
	basename = xstrdup(strrchr(targetfile, '/') + 1);
	// FIXME
	dbfile = strcat(strcat(xcalloc(strlen(targetfile) + 9, sizeof(*dbfile)), targetfile), ".predeps");

	char **dirv;
	int dirc = gen_dirs(&dirv, targetfile);
	char **extv;
	int extc = gen_exts(&extv, basename);

	/* Given an absolute path /path/to/file.tar.gz the algorithm
	 * looks for the following files:
	 * 	/path/to/file.tar.gz.do
	 * 	/path/to/default.tar.gz.do
	 * 	/path/to/default.gz.do
	 * 	/path/file.tar.gz.do
	 * 	/path/default.tar.gz.do
	 * 	/path/default.gz.do
	 * 	...
	 * 	/default.gz.do
	 */
	int db_fd = xopen(dbfile, O_WRONLY|O_CREAT|O_TRUNC, 0644);
	for(int didx = 0; !dofile && didx < dirc; didx++) {
		char buf[PATH_MAX+1];
		size_t dir_len = strlen(dirv[didx]);
		memcpy(buf, dirv[didx], dir_len);

		snprintf(&buf[dir_len], PATH_MAX - dir_len, "%s.do", basename);
		if(path_exists(buf)) {
			workdir = xstrdup(dirv[didx]);
			dofile = xstrdup(buf);
			char *ext = strrchr(basename, '.');
			if(ext)
				*ext = '\0';
			break;
		} else {
			predep_record(db_fd, 'n', buf);
		}
		for(int eidx = 0; !dofile && eidx < extc; eidx++) {
			snprintf(&buf[dir_len], PATH_MAX - dir_len, "_.%s.do", extv[eidx]);
			if(path_exists(buf)) {
				workdir = xstrdup(dirv[didx]);
				dofile = xstrdup(buf);
				basename[strlen(basename) - strlen(extv[eidx]) - 1] = '\0';
				break;
			} else {
				predep_record(db_fd, 'n', buf);
			}

			snprintf(&buf[dir_len], PATH_MAX - dir_len, "default.%s.do", extv[eidx]);
			if(path_exists(buf)) {
				workdir = xstrdup(dirv[didx]);
				dofile = xstrdup(buf);
				basename[strlen(basename) - strlen(extv[eidx]) - 1] = '\0';
				break;
			} else {
				predep_record(db_fd, 'n', buf);
			}
		}
	}
	if(dofile) {
		predep_record(db_fd, 's', dofile);
	} else {
		die("No rule to make target '%s'. Stop.", target);
	}

	char tmpfile[PATH_MAX];
	snprintf(tmpfile, len(tmpfile), "%s/%s", workdir, "XXXXXXXXXX");
	int tmpfile_fd = mkstemp(tmpfile);
//	int tmpfile_fd = xopen(tmpfile, O_WRONLY|O_EXCL|O_CLOEXEC, 0644);

	int rc = run_dofile(workdir, dofile, targetfile, basename, db_fd, tmpfile_fd);

	xclose(db_fd);

	for(int didx = 0; didx < dirc; didx++) {
		free(dirv[didx]);
	}
	free(dirv);
	for(int eidx = 0; eidx < extc; eidx++) {
		free(extv[eidx]);
	}
	free(extv);

	struct stat sb;
	if(rc != 0 || fstat(tmpfile_fd, &sb) != 0 || sb.st_size == 0) {
		try_unlink(tmpfile);
	} else {
		if(rename(tmpfile, targetfile) == -1) {
			try_unlink(tmpfile);
			die_errno("Could not replace targetfile");
		}
	}
	xclose(tmpfile_fd);

	if(rc == 0 || redo_getenv_int(REDO_ENV_KEEPGOING, 0)) {
		return rc;
	} else {
		exit(rc);
	}
}

int
redo_ifchange(const char *target, int parent_db) {
	int rv = 0;
	if(predeps_changed(target)) {
//fprintf(stderr, "%s:%d\n", __FILE__, __LINE__);
		//FIXME this is wrong
		if(path_exists(target)) {
			predep_record(parent_db, 's', target);
		} else {
			rv |= redo(target);
			predep_record(parent_db, 't', target);
		}
	}

	return rv;
}

int
redo_ifcreate(const char *target, int parent_db) {
	predep_record(parent_db, 'n', target);

	return 0;
}
