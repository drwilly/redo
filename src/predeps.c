#include <skalibs/stddjb.h>
#include <skalibs/allreadwrite.h> // TODO figure out how to use bufalloc
#include <skalibs/stdcrypto.h>

#include <sys/uio.h>

#include "reporting.h"
#include "path.h"

#include "predeps.h"

// TODO remove
#define len(x) (sizeof(x) / sizeof(*x))

struct predep {
	char type;
	char path[PATH_MAX];
	char hash[20];
};

static
void
file_compute_hash(const char *file, char *digest) {
	int fd = open_read(file);
	size_t count;
#if 1
	struct stat sb;
	if(fstat(fd, &sb) != 0) {
		die_errno("stat of %s failed", file);
	}
	count = sb.st_blksize;
#else
	count = 4096;
#endif

	SHA1Schedule ctx;
	unsigned int messagelen;
	char message[count];

	sha1_init(&ctx);
	while((messagelen = fd_read(fd, message, count)) > 0) {
		sha1_update(&ctx, message, messagelen);
	}
	sha1_final(&ctx, digest);

	fd_close(fd);
}

static
ssize_t
predep_read(int db_fd, struct predep *dep) {
	struct iovec iov[] = {
		{
			.iov_base = &dep->type,
			.iov_len = sizeof(dep->type),
		}, {
			.iov_base = dep->path,
			.iov_len = len(dep->path),
		}, {
			.iov_base = dep->hash,
			.iov_len = len(dep->hash),
		}
	};

	ssize_t nbytes_expected = 0;
	for(int i = 0; i < len(iov); i++) {
		nbytes_expected += iov[i].iov_len;
	}

	ssize_t nbytes_read = readv(db_fd, iov, len(iov));
	if(nbytes_read < nbytes_expected && nbytes_read > 0) {
		die_errno("error during readv(%d, %p, %lu): expected=%d actual=%d",
				db_fd, iov, len(iov), nbytes_expected, nbytes_read
			);
	}

	return nbytes_read;
}

static
ssize_t
predep_write(int db_fd, struct predep *dep) {
	struct iovec iov[] = {
		{
			.iov_base = &dep->type,
			.iov_len = sizeof(dep->type),
		}, {
			.iov_base = dep->path,
			.iov_len = len(dep->path),
		}, {
			.iov_base = dep->hash,
			.iov_len = len(dep->hash),
		}
	};

	ssize_t nbytes_expected = 0;
	for(int i = 0; i < len(iov); i++) {
		nbytes_expected += iov[i].iov_len;
	}
	
	ssize_t nbytes_written = writev(db_fd, iov, len(iov));
	if(nbytes_written < nbytes_expected) {
		die_errno("error during writev(%d, %p, %lu): expected=%d actual=%d",
				db_fd, iov, len(iov), nbytes_expected, nbytes_written
			);
	}

	return nbytes_written;
}

void
predep_record(int db_fd, const char type, const char *path) {
	struct predep dep;

	dep.type = type;
	strncpy(dep.path, path, len(dep.path));
	if(type == 't' || type == 's') {
		file_compute_hash(path, dep.hash);
	}

	predep_write(db_fd, &dep);
}

static
int
predep_hash_changed(struct predep *dep) {
	char filehash[len(dep->hash)];
	file_compute_hash(dep->path, filehash);

	return memcmp(dep->hash, filehash, len(dep->hash));
}

static
int
predep_created(struct predep *dep) {
	return path_exists(dep->path);
}

static
int
predep_changed_source(struct predep *dep) {
	return !path_exists(dep->path) || predep_hash_changed(dep);
}

static
int
openchecktargetclose(const char *target) {
	static stralloc sa = STRALLOC_ZERO;
	sa.len = 0;
	predeps_sadbfile(&sa, target);
	return predeps_opencheckclose(sa.s);
}

static
int
predep_changed_target(struct predep *dep) {
	return !path_exists(dep->path) || predep_hash_changed(dep) || openchecktargetclose(dep->path);
}

static
int
predep_modified(struct predep *dep) {
	switch(dep->type) {
	case 't':
		return predep_changed_target(dep);
	case 's':
		return predep_changed_source(dep);
	case 'n':
		return predep_created(dep);
	default:
		die("invalid predep type '%c'", dep->type);
	}
}

int
predeps_changed(int db_fd) {
	int modified = 0;
	for(struct predep dep; predep_read(db_fd, &dep); ) {
		modified = predep_modified(&dep);
		if(modified) {
			break;
		}
	}

	return modified;
}

int
predeps_opencheckclose(const char *file) {
	int fd = open_read(file);
	if(fd == -1) {
		return 1;
	}
	int changed = predeps_changed(fd);
	fd_close(fd);

	return changed;
}

int
predeps_sadbfile(stralloc *sa, const char *target) {
	stralloc_cats(sa, target);
	stralloc_cats(sa, ".predeps");
	return 0;
}
