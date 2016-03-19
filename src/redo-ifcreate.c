#include <skalibs/djbunix.h>

#include "reporting.h"
#include "path.h"
#include "predeps.h"

#include "options.h"

static
int
redo_ifcreate(const char *target) {
	predep_record_absent(target);
	return path_exists(target);
}

int
main(int argc, char *argv[]) {
	argc = args_filter_options(argc, argv);

	int rv = 0;
	for(int i = 1; !rv && i < argc; i++) {
		rv = redo_ifcreate(argv[i]);
	}

	return rv;
}
