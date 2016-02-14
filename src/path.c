#include <unistd.h>
#include <limits.h>
#include <string.h>
#include <errno.h>

#include "reporting.h"
#include "path.h"

inline
int
is_dir_sep(char c) {
	return c == '/';
}

inline
int
path_is_absolute(const char *path) {
	return path && is_dir_sep(path[0]);
}


int
path_exists(const char *path) {
	if(access(path, R_OK) == 0) {
		return 1;
	} else if(errno != ENOENT) {
		die_errno("Could not access file '%s'", path);
	}

	return 0;
}

int
try_unlink(const char *file) {
	return unlink(file) == 0 || errno == ENOENT;
}
