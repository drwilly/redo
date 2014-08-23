#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

#include "reporting.h"

void *
xcalloc(size_t nmemb, size_t size) {
	void *rv = calloc(nmemb, size);
	if(!rv) {
		die_errno("error during calloc(%lu, %lu)", nmemb, size);
	}
	return rv;
}

int
xopen(const char *pathname, int flags, ...) {
	int rv;

	if(flags & (O_CREAT|O_TMPFILE)) {
		va_list params;
		va_start(params, flags);
		int mode = va_arg(params, int);
		va_end(params);

		rv = open(pathname, flags, mode);
	} else {
		rv = open(pathname, flags);
	}

	if(rv == -1) {
		die_errno("error during open(%s, %d)", pathname, flags);
	}
	return rv;
}

int
xclose(int fd) {
	int rv = close(fd);
	if(rv == -1) {
		die_errno("error during close(%d)", fd);
	}
	return rv;
}

char *
xstrdup(const char *s) {
	char *rv = strdup(s);
	if(!rv) {
		die_errno("error during strdup(%s)", s);
	}
	return rv;
}

int
try_unlink(const char *pathname) {
	return unlink(pathname) == 0 || errno == ENOENT;
}
