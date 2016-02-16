#include <stdlib.h>

#include <skalibs/djbunix.h>

#include "reporting.h"
#include "path.h"
#include "predeps.h"

#include "redo.h"

static
int
redo_ifcreate(const char *target) {
	predep_record_absent(target);
	return path_exists(target);
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

	int rv = 0;
	for(int i = 1; i < argc; i++) {
		rv = redo_ifcreate(argv[i]);
		if(rv != 0) {
			break;
		}
	}
	fd_close(3);

	return rv;
}
