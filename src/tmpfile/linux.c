#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

#include <skalibs/djbunix.h>

extern int sprintf(char *, const char *, ...);

int
tmpfile_create() {
	return open3(".", O_WRONLY|O_TMPFILE, 0644);
}

int
tmpfile_link(const int fd, const char *target) {
	char file[64];
	sprintf(file, "/dev/fd/%d", fd);
	int rv = linkat(AT_FDCWD, file, AT_FDCWD, target, AT_SYMLINK_FOLLOW);
	if(rv == -1 && errno == EEXIST && unlink(target) == 0) {
		rv = linkat(AT_FDCWD, file, AT_FDCWD, target, AT_SYMLINK_FOLLOW);
	}

	return rv;
}
