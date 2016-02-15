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
redo_ifchange(const char *target) {
	int rv = 0;
	stralloc dbfile = STRALLOC_ZERO;
	predeps_sadbfile(&dbfile, target);
	stralloc_0(&dbfile);

	if(path_exists(target) && !path_exists(dbfile.s)) {
		predep_record(3, 's', target);
	} else if(predeps_opencheckclose(dbfile.s)) {
		pid_t pid = fork();
		if(pid == -1) {
			die_errno("fork failed");
		} else if(!pid) {
			execlp("redo", "redo", target, (char *)NULL);
		} else {
			int status;
			waitpid_nointr(pid, &status, 0);
			rv = WEXITSTATUS(status);
			if(rv == 0) {
				predep_record(3, 't', target);
			}
		}
	}
	stralloc_free(&dbfile);

	return rv;
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
	argc = options(argc, argv);

	fd_ensure_open(3, 1);
	coe(3);

	int rv = 0;
	for(int i = 1; i < argc; i++) {
		rv = redo_ifchange(argv[i]);
		if(rv != 0) {
			break;
		}
	}
	fd_close(3);

	return rv;
}
