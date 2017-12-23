#include <skalibs/bytestr.h>

#include "environment.h"

#include "options.h"

extern int atoi(const char *);

static
int
is_digitstr(const char *s) {
	while(*s) {
		switch(*s++) {
		case '0': case '1':
		case '2': case '3':
		case '4': case '5':
		case '6': case '7':
		case '8': case '9':
			break;
		default:
			return 0;
		}
	}
	return 1;
}

int
args_filter_options(int argc, char *argv[]) {
	int c = 1;
	for(int i = c; i < argc; i++) {
		if(argv[i][0] != '-') {
			argv[c++] = argv[i];
		} else if(str_equal(argv[i], "--")) {
			while(++i < argc) {
				argv[c++] = argv[i];
			}
		}
	}

	return c;
}

int
args_process_options(int argc, char *argv[]) {
	int c = 1;
	for(int i = c; i < argc; i++) {
		if(argv[i][0] != '-') {
			argv[c++] = argv[i];
		} else if(str_equal(argv[i], "--")) {
			while(++i < argc) {
				argv[c++] = argv[i];
			}
		} else if(str_equal(argv[i], "--help") || str_equal(argv[i], "-h")) {
			// TODO
		} else if(str_equal(argv[i], "--version") || str_equal(argv[i], "-v")) {
			// TODO
		} else if(str_equal(argv[i], "--jobs") || str_equal(argv[i], "-j")) {
			int jobs = 0;
			if(i+1 < argc && is_digitstr(argv[i+1])) {
				jobs = atoi(argv[++i]);
			}
			redo_setenv_int(REDO_ENV_JOBS, jobs);
		} else if(str_equal(argv[i], "--keep-going") || str_equal(argv[i], "-k")) {
			redo_setenv_int(REDO_ENV_KEEPGOING, 1);
		} else if(str_equal(argv[i], "--debug") || str_equal(argv[i], "-d")) {
			redo_setenv_int(REDO_ENV_DEBUG, redo_getenv_int(REDO_ENV_DEBUG, 0) + 1);
		} else if(str_equal(argv[i], "--no-color")) {
			redo_setenv_int(REDO_ENV_NOCOLOR, 1);
		} else if(str_equal(argv[i], "--shuffle")) {
			int seed = 1;
			if(i+1 < argc && is_digitstr(argv[i+1])) {
				seed = atoi(argv[++i]);
			}
			redo_setenv_int(REDO_ENV_SHUFFLE, seed);
		}
	}

	return c;
}
