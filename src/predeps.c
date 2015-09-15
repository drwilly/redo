#include <stdio.h>
#include <limits.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/uio.h>
#include <string.h>

#include "util.h"

#include "reporting.h"
#include "libc_wrapper.h"
#include "path.h"
#include "hash.h"

#include "predeps.h"

struct predep {
	char type;
	char path[PATH_MAX];
	unsigned char hash[REDO_HASH_LENGTH];
};

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
		die_errno("error during readv(%d, %p, %lu)", db_fd, iov, len(iov));
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
		die_errno("error during writev(%d, %p, %lu)", db_fd, iov, len(iov));
	}

	return nbytes_written;
}

void
predep_record(int db_fd, const char type, const char *path) {
//fprintf(stderr, "recording '%s'\n", path);
	struct predep dep;

	dep.type = type;
	strncpy(dep.path, path, len(dep.path));
	if(type == 's' || type == 't') {
		hash(path, dep.hash);
	}

	predep_write(db_fd, &dep);
}

static
int
predep_created(struct predep *dep) {
	return path_exists(dep->path);
}

static
int
predep_changed_source(struct predep *dep) {
	if(!path_exists(dep->path)) {
		return 1;
	}

	unsigned char filehash[len(dep->hash)];
	hash(dep->path, filehash);

	return memcmp(dep->hash, filehash, len(dep->hash));
}

static
int
predep_changed_target(struct predep *dep) {
	return predep_changed_source(dep) || predeps_changed(dep->path);
}

static
int
predep_modified(struct predep *dep) {
//fprintf(stderr, "checking db entry '%s'\n", dep->path);
	switch(dep->type) {
	case 'n':
		return predep_created(dep);
	case 's':
		return predep_changed_source(dep);
	case 't':
		return predep_changed_target(dep);
	default:
		die("invalid predep type '%c'", dep->type);
	}
}

int
predeps_changed(const char *target) {
	char dbfile[PATH_MAX];
	snprintf(dbfile, PATH_MAX, "%s.predeps", target);
	if(!path_exists(dbfile)) {
		return 1;
	}

	int modified = 0;
	int db_fd = xopen(dbfile, O_RDONLY);
//fprintf(stderr, "checking dbfile '%s'\n", dbfile);
	for(struct predep dep; predep_read(db_fd, &dep); ) {
		modified = predep_modified(&dep);
		if(modified) {
//fprintf(stderr, "%s [%d] '%c' %s\n", dbfile, modified, dep.type, dep.path);
			break;
		}
	}
	xclose(db_fd);

	return modified;
}
