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

/*
 * Use this to get an absolute path from a relative one. If you want
 * to resolve links, you should use real_path.
 */
const char *
path_absolute(const char *path) {
	static char abspath[PATH_MAX+1];

	if(path[0] == '\0') {
		die("The empty string is not a valid path");
	}
	
	/* If the path is already absolute, then return path. As the user is
	 * never meant to free the return value, we're safe.
	 */
	if(path_is_absolute(path)) {
		return path;
	}

	if(!getcwd(abspath, PATH_MAX)) {
		die_errno("Could not determine current working directory");
	}
	size_t cwd_len = strlen(abspath);
	if(!is_dir_sep(abspath[cwd_len-1])) {
		abspath[cwd_len++] = '/';
	}
	if(strlen(path) + cwd_len >= PATH_MAX) {
		die("Too long path: %.*s", 60, path);
	}
	strcpy(&abspath[cwd_len], path);

	return abspath;
}

/*
const char *
path_normalize(const char *path) {
	static char normpath[PATH_MAX+1];

	char *p = path;
	char *n = normpath;
	for(;;) {
		if(p[0] == '.') {
			if(p[1] == '/') {
				p += 2;
			} else if(p[1] == '.') {

			}
		}
		*n++ = *p++;
	}

	return normpath;
}
*/
