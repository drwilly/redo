#include <stdlib.h>

#include <skalibs/djbunix.h>

#include "reporting.h"
#include "path.h"
#include "predeps.h"

#include "redo.h"

static
int
redo_ifcreate(const char *target) {
	predep_record(3, 'n', target);
	return path_exists(target);
}

int
main(int argc, char *argv[]) {
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
