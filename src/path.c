#include <unistd.h>
#include <errno.h>

#include "reporting.h"
#include "path.h"

int
path_exists(const char *path) {
	if(access(path, F_OK) == 0) {
		return 1;
	} else if(errno != ENOENT) {
		die_errno("access('%s', F_OK) failed", path);
	}

	return 0;
}
