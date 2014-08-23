#include <fcntl.h>
#include <stdlib.h>
#include <string.h>

#include "libc_wrapper.h"

#include "reporting.h"
#include "environment.h"

#include "redo.h"

#define len(x) (sizeof(x) / sizeof(*x))

int
options(char *targetv[], int argc, char *argv[]) {
	int option_end = 0;
	int targetc = 0;
	for(int i = 1; i < argc; i++) {
		if(option_end || argv[i][0] != '-') {
			targetv[targetc++] = argv[i];
		} else if(strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0) {
			// TODO
		} else if(strcmp(argv[i], "--version") == 0 || strcmp(argv[i], "-v") == 0) {
			// TODO
		} else if(strcmp(argv[i], "--jobs") == 0 || strcmp(argv[i], "-j") == 0) {
			int jobs = atoi(argv[++i]);
			if(jobs >= 0) {
				redo_setenv_int(REDO_ENV_JOBS, jobs);
			} else {
			}
		} else if(strcmp(argv[i], "--keep-going") == 0 || strcmp(argv[i], "-k") == 0) {
			redo_setenv_int(REDO_ENV_KEEPGOING, 1);
		} else if(strcmp(argv[i], "--debug") == 0 || strcmp(argv[i], "-d") == 0) {
			redo_setenv_int(REDO_ENV_DEBUG, redo_getenv_int(REDO_ENV_DEBUG, 0) + 1);
		} else if(strcmp(argv[i], "--no-color") == 0) {
			redo_setenv_int(REDO_ENV_NOCOLOR, 1);
		} else if(strcmp(argv[i], "--shuffle") == 0) {
			redo_setenv_int(REDO_ENV_SHUFFLE, 1);
		} else if(strcmp(argv[i], "--") == 0) {
			option_end = 1;
		}
	}

	return targetc;
}

void
redo_err(const char *fmt, va_list params) {
	char prefixfmt[16];
	snprintf(prefixfmt, len(prefixfmt), "%%-%ds", 4 + (2 * (1 + redo_getenv_int(REDO_ENV_DEPTH, 0))));

	char prefix[32];
	snprintf(prefix, len(prefix), prefixfmt, "redo");
	
	char msg[4096];
	vsnprintf(msg, len(msg), fmt, params);

	fprintf(stderr, "\033[31m%s\033[1m%s\033[m\n", prefix, msg);
}

void
redo_info(const char *fmt, va_list params) {
	char prefixfmt[16];
	snprintf(prefixfmt, len(prefixfmt), "%%-%ds", 4 + (2 * (1 + redo_getenv_int(REDO_ENV_DEPTH, 0))));

	char prefix[32];
	snprintf(prefix, len(prefix), prefixfmt, "redo");
	
	char msg[4096];
	vsnprintf(msg, len(msg), fmt, params);

	fprintf(stderr, "\033[32m%s\033[1m%s\033[m\n", prefix, msg);
}

int
main(int argc, char *argv[]) {
	char *targetv[argc];
	int targetc;
	
	targetc = options(targetv, argc, argv);

	if(redo_getenv_int(REDO_ENV_DEPTH, 0)) {
	}

	// TODO "nocolor" is slightly misleading because it changes the output format
	if(redo_getenv_int(REDO_ENV_NOCOLOR, 1) != 0) {
		//set_die_routine(&redo_err);
		set_error_routine(&redo_err);
		//set_warning_routine(&redo_);
		set_info_routine(&redo_info);
	}

	char *exe = strrchr(argv[0], '/');
	if(exe) {
		exe++;
	} else {
		exe = argv[0];
	}

	int rv = 0;
	if(strcmp(exe, "redo") == 0) {
		if(targetc == 0) {
			return redo(REDO_DEFAULT_TARGET);
		}
		for(int i = 0; i < targetc; i++) {
			rv |= redo(targetv[i]);
		}
	} else if(strcmp(exe, "redo-ifchange") == 0) {
		int parent_db = redo_getenv_int(REDO_ENV_DB, -1);
		for(int i = 0; i < targetc; i++) {
			rv |= redo_ifchange(targetv[i], parent_db);
		}
		xclose(parent_db);
	} else if(strcmp(exe, "redo-ifcreate") == 0) {
		if(redo_getenv_int(REDO_ENV_DEPTH, 0) == 0) {
			die("redo-ifcreate without parent makes no sense");
		}

		int parent_db = redo_getenv_int(REDO_ENV_DB, -1);
		for(int i = 0; i < targetc; i++) {
			rv |= redo_ifcreate(targetv[i], parent_db);
		}
		xclose(parent_db);
	} else {
		// FIXME
		die("redo does not recognize program-name '%s' (yes, this is a bug)", exe);
	}

	return rv;
}
