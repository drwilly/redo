#include <unistd.h>
#include <errno.h>

#include "reporting.h"
#include "path.h"

extern int rename(const char *, const char *);

int
path_exists(const char *path) {
	if(access(path, F_OK) == 0) {
		return 1;
	} else if(errno != ENOENT) {
		die_errno("Could not access file '%s'", path);
	}

	return 0;
}

int
try_unlink(const char *file) {
	return unlink(file) == -1 && errno != ENOENT;
}

int
try_link_tmpfile(const char *tmpfile, const char *dest) {
	if(rename(tmpfile, dest) == -1) {
		int err = errno;
		unlink(tmpfile);
		errno = err;
		return -1;
	}

	return 0;
}
