#include <skalibs/djbunix.h>

#include "predeps.h"

int
main(int argc, char *argv[]) {
	if(argc < 1) {
		return 1;
	} else if(str_equal(argv[1], "changed")) {
		predeps_changed(0);
	} else if(str_equal(argv[1], "record")) {
		predeps_record(1, ...);
	} else {
	}

	return 0;
}
