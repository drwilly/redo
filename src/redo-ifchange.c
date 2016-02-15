#include <stdlib.h>

#include <skalibs/djbunix.h>
#include <skalibs/stralloc.h>

#include "reporting.h"
#include "environment.h"
#include "path.h"
#include "predeps.h"

#include "redo.h"

static
int
changed(int argc, char *argv[]) {
	int c = 1;
	stralloc dbfile = STRALLOC_ZERO;
	for(int i = 1; i < argc; i++) {
		dbfile.len = 0;
		predeps_sadbfile(&dbfile, argv[i]);
		stralloc_0(&dbfile);

		if(path_exists(argv[i]) && !path_exists(dbfile.s)) {
			predep_record(3, 's', argv[i]);
		} else if(predeps_opencheckclose(dbfile.s)) {
			argv[c++] = argv[i];
		}
	}
	stralloc_free(&dbfile);

	return c;
}

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
		}
	}

	return c;
}

int
main(int argc, char *argv[]) {
	fd_ensure_open(3, 1);
	coe(3);

	argc = options(argc, argv);
	argc = changed(argc, argv);

	if(argc <= 1)
		return 0;

	pid_t pid = fork();
	if(pid == -1) {
		die_errno("fork failed");
		return -1; // unreached
	} else if(!pid) {
		char *newargv[argc+1];
		newargv[0] = "redo";
		for(int i = 1; i < argc; i++) {
			newargv[i] = argv[i];
		}
		newargv[argc] = NULL;
		return execvp(newargv[0], newargv);
	} else {
		int status;
		waitpid_nointr(pid, &status, 0);
		if(WEXITSTATUS(status) == 0) {
			for(int i = 1; i < argc; i++) {
				predep_record(3, 't', argv[i]);
			}
		}
		fd_close(3);
		return WEXITSTATUS(status);
	}
}
