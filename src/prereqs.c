#include <skalibs/djbunix.h>
#include <skalibs/bytestr.h>
#include <skalibs/buffer.h>
#include <skalibs/skamisc.h>

#include <unistd.h>
#include <fcntl.h>

#include "reporting.h"
#include "path.h"

#include "checksum.h"

#include "prereqs.h"

extern int rename(const char *, const char *);

static const char *dbfile_suffix = ".prereqs";

int
prereqs_existfor(const char *target) {
	char dbfile[str_len(target) + str_len(dbfile_suffix) + 1];
	str_copy(dbfile, target);
	str_copy(dbfile + str_len(target), dbfile_suffix);
	return path_exists(dbfile);
}

int
prereqs_renamefor(const char *target, const char *tmpfile) {
	char dbfile[str_len(target) + str_len(dbfile_suffix) + 1];
	str_copy(dbfile, target);
	str_copy(dbfile + str_len(target), dbfile_suffix);
	return rename(tmpfile, dbfile);
}

static
int
prereq_created(const char *file) {
	return path_exists(file);
}

static
int
prereq_changed_source(const char *file, const char *checksum_str) {
	return !path_exists(file) || file_checksum_str_changed(file, checksum_str);
}

static
int
prereq_changed_target(const char *file, const char *checksum_str) {
	return !path_exists(file) || file_checksum_str_changed(file, checksum_str) || prereqs_changedfor(file);
}

static
int
prereq_changed_virtual(const char *file) {
	return prereqs_changedfor(file);
}

static
int
prereqs_changed(int dbfd) {
	char buf[BUFFER_INSIZE];
	buffer b = BUFFER_INIT(&buffer_read, dbfd, buf, BUFFER_INSIZE);

	stralloc ln = STRALLOC_ZERO;
	int changed = 0;
	ssize_t r;
	while(!changed && (r = skagetln(&b, &ln, '\n')) > 0) {
		char *dep[3]; // FIXME
		int i;
		dep[0] = ln.s;
		i = str_chr(dep[0], '\t');
		dep[0][i] = '\0';
		dep[1] = dep[0] + i + 1;
		i = str_chr(dep[1], '\t');
		dep[1][i] = '\0';
		dep[2] = dep[1] + i + 1;
		ln.s[ln.len-1] = '\0';
		switch(ln.s[0]) {
		case 'v':
			if(str_diff(dep[0], "virtual")) die("" /* TODO */);
			changed = prereq_changed_virtual(dep[1]);
			break;
		case 't':
			if(str_diff(dep[0], "target")) die("" /* TODO */);
			changed = prereq_changed_target(dep[1], dep[2]);
			break;
		case 's':
			if(str_diff(dep[0], "source")) die("" /* TODO */);
			changed = prereq_changed_source(dep[1], dep[2]);
			break;
		case 'a':
			if(str_diff(dep[0], "absent")) die("" /* TODO */);
			changed = prereq_created(dep[1]);
			break;
		default:
			die("" /* TODO */);
		}
		ln.len = 0;
	}
	stralloc_free(&ln);

	if(r == -1) die("");

	return changed;
}

static
int
prereqs_opencheckclose(const char *file) {
	int fd = open_read(file);
	if(fd == -1) {
		die_errno("open_read('%s') failed", file);
	}
	int changed = prereqs_changed(fd);
	fd_close(fd);

	return changed;
}

int
prereqs_changedfor(const char *target) {
	int pwdfd = AT_FDCWD;
	unsigned int sep_offset = str_rchr(target, '/');
	const char *targetfile = target;
	if(target[sep_offset] != '\0') {
		pwdfd = open2(".", O_CLOEXEC|O_DIRECTORY);
		if(pwdfd == -1) {
			die_errno("open2('.', O_CLOEXEC|O_DIRECTORY) failed");
		}
		char workdir[sep_offset + 1];
		byte_copy(workdir, sep_offset, target);
		workdir[sep_offset] = '\0';
		if(chdir(workdir) == -1) {
			die_errno("chdir('%s') failed", workdir);
		}
		targetfile += sep_offset + 1;
	}
	char dbfile[str_len(targetfile) + str_len(dbfile_suffix) + 1];
	str_copy(dbfile, targetfile);
	str_copy(dbfile + str_len(targetfile), dbfile_suffix);
	int changed = prereqs_opencheckclose(dbfile);
	if(pwdfd != AT_FDCWD && fchdir(pwdfd) == -1) {
		die_errno("fchdir('%d') failed", pwdfd);
	}

	return changed;
}

static
ssize_t
prereq_record_writev(struct iovec *iov, size_t n) {
	size_t total = 0;
	for(int i = 0; i < n; i++) {
		total += iov[i].iov_len;
	}
	ssize_t w = fd_writev(3, iov, n);
	return (w == -1) ? w : total - w;
}

static
ssize_t
prereq_record_virtual(const char *file) {
	struct iovec iov[] = {
		{
			.iov_base = "virtual",
			.iov_len = 7,
		}, {
			.iov_base = "\t",
			.iov_len = 1,
		}, {
			.iov_base = (void *)file,
			.iov_len = str_len(file),
		}, {
			.iov_base = "\n",
			.iov_len = 1,
		}
	};

	return prereq_record_writev(iov, 4);
}

ssize_t
prereq_record_target(const char *file) {
	if(!path_exists(file)) { // virtual target
		return prereq_record_virtual(file);
	}
	char checksum_str[20*2+1];
	file_checksum_str_compute(file, checksum_str);

	struct iovec iov[] = {
		{
			.iov_base = "target",
			.iov_len = 6,
		}, {
			.iov_base = "\t",
			.iov_len = 1,
		}, {
			.iov_base = (void *)file,
			.iov_len = str_len(file),
		}, {
			.iov_base = "\t",
			.iov_len = 1,
		}, {
			.iov_base = checksum_str,
			.iov_len = 20*2,
		}, {
			.iov_base = "\n",
			.iov_len = 1,
		}
	};

	return prereq_record_writev(iov, 6);
}

ssize_t
prereq_record_source(const char *file) {
	char checksum_str[20*2+1];
	file_checksum_str_compute(file, checksum_str);

	struct iovec iov[] = {
		{
			.iov_base = "source",
			.iov_len = 6,
		}, {
			.iov_base = "\t",
			.iov_len = 1,
		}, {
			.iov_base = (void *)file,
			.iov_len = str_len(file),
		}, {
			.iov_base = "\t",
			.iov_len = 1,
		}, {
			.iov_base = checksum_str,
			.iov_len = 20*2,
		}, {
			.iov_base = "\n",
			.iov_len = 1,
		}
	};

	return prereq_record_writev(iov, 6);
}

ssize_t
prereq_record_absent(const char *file) {
	struct iovec iov[] = {
		{
			.iov_base = "absent",
			.iov_len = 6,
		}, {
			.iov_base = "\t",
			.iov_len = 1,
		}, {
			.iov_base = (void *)file,
			.iov_len = str_len(file),
		}, {
			.iov_base = "\n",
			.iov_len = 1,
		}
	};

	return prereq_record_writev(iov, 4);
}
