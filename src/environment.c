#include <stdlib.h>
#include <stdio.h>

#include "util.h"

#include "reporting.h"
#include "environment.h"

int
redo_getenv_int(const char *k, int def) {
	const char *v =	getenv(k);
	return v ? atoi(v) : def;
}

void
redo_setenv_int(const char *k, int v) {
	char buf[16];
	snprintf(buf, len(buf), "%d", v);
	if(setenv(k, buf, 1) != 0) {
		die_errno("setenv failed");
	}
}

const char *
redo_getenv_str(const char *k, const char *def) {
	const char *v = getenv(k);
	return v ? v : def;
}

void
redo_setenv_str(const char *k, const char *v) {
	if(setenv(k, v, 1) != 0) {
		die_errno("setenv failed");
	}
}
