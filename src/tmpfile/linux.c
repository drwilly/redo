#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

#include <skalibs/djbunix.h>

extern int sprintf(char *, const char *, ...);
extern int rename(char *, char *);

extern int sauniquename(stralloc *);

int
tmpfile_create() {
	return open3(".", O_WRONLY|O_TMPFILE, 0644);
}

int
tmpfile_link(const int fd, const char *target) {
	char file[64];
	if(sprintf(file, "/dev/fd/%d", fd) <= 0) return -1;
	int rv = linkat(AT_FDCWD, file, AT_FDCWD, target, AT_SYMLINK_FOLLOW);
	if(rv == -1) {
		stralloc tmptarget = STRALLOC_ZERO;
		if(!stralloc_copys(&tmptarget, target)) goto err;
		if(sauniquename(&tmptarget) == -1) goto err;
		if(!stralloc_0(&tmptarget)) goto err;
		if(linkat(AT_FDCWD, file, AT_FDCWD, tmptarget.s, AT_SYMLINK_FOLLOW) == -1) goto err;
		rv = rename(tmptarget.s, target);
err:
		stralloc_free(&tmptarget);
	}

	return rv;
}
