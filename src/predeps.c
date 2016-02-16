#include <skalibs/stddjb.h>
#include <skalibs/buffer.h>
#include <skalibs/stdcrypto.h>

#include "reporting.h"
#include "path.h"

#include "predeps.h"

static void sha1_file(const char *, char *);

static void (*file_hash_compute)(const char *file, char *digest) = &sha1_file;

static const char *hexdigit = "0123456789abcdef";

static
void
sha1_file(const char *file, char *digest) {
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

size_t
predep_record_target(const char *file) {
	unsigned char digest[20];
	file_hash_compute(file, digest);
	char digest_str[2*20];
	for(int i = 0; i < 20; i++) {
		digest_str[2*i+0] = hexdigit[(digest[i] & 0xf0) >> 4];
		digest_str[2*i+1] = hexdigit[(digest[i] & 0x0f) >> 0];
	}
	struct iovec iov[] = {
		{
			.iov_base = "target",
			.iov_len = 6,
		}, {
			.iov_base = "\t",
			.iov_len = 1,
		}, {
			.iov_base = file,
			.iov_len = str_len(file),
		}, {
			.iov_base = "\t",
			.iov_len = 1,
		}, {
			.iov_base = digest_str,
			.iov_len = 2*20,
		}, {
			.iov_base = "\n",
			.iov_len = 1,
		}
	};

	return fd_writev(3, iov, 6);
}

size_t
predep_record_source(const char *file) {
	unsigned char digest[20];
	file_hash_compute(file, digest);
	char digest_str[2*20];
	for(int i = 0; i < 20; i++) {
		digest_str[2*i+0] = hexdigit[(digest[i] & 0xf0) >> 4];
		digest_str[2*i+1] = hexdigit[(digest[i] & 0x0f) >> 0];
	}
	struct iovec iov[] = {
		{
			.iov_base = "source",
			.iov_len = 6,
		}, {
			.iov_base = "\t",
			.iov_len = 1,
		}, {
			.iov_base = file,
			.iov_len = str_len(file),
		}, {
			.iov_base = "\t",
			.iov_len = 1,
		}, {
			.iov_base = digest_str,
			.iov_len = 2*20,
		}, {
			.iov_base = "\n",
			.iov_len = 1,
		}
	};

	return fd_writev(3, iov, 6);
}

size_t
predep_record_absent(const char *file) {
	struct iovec iov[] = {
		{
			.iov_base = "absent",
			.iov_len = 6,
		}, {
			.iov_base = "\t",
			.iov_len = 1,
		}, {
			.iov_base = file,
			.iov_len = str_len(file),
		}, {
			.iov_base = "\n",
			.iov_len = 1,
		}
	};

	return fd_writev(3, iov, 4);
}

static
int
predep_hash_changed(const char *file, const char *digest_str) {
	char digest[20];
	file_hash_compute(file, digest);
	for(int i = 0; i < 20; i++) {
		if(digest_str[2*i+0] != hexdigit[(digest[i] & 0xf0) >> 4]) return 1;
		if(digest_str[2*i+1] != hexdigit[(digest[i] & 0x0f) >> 0]) return 1;
	}

	return 0;
}

static
int
predep_created(const char *file) {
	return path_exists(file);
}

static
int
predep_changed_source(const char *file, const char *digest_str) {
	return !path_exists(file) || predep_hash_changed(file, digest_str);
}

static
int
openchecktargetclose(const char *target) {
	stralloc sa = STRALLOC_ZERO;
	predeps_sadbfile(&sa, target);
	stralloc_0(&sa);
	int changed = predeps_opencheckclose(sa.s);
	stralloc_free(&sa);

	return changed;
}

static
int
predep_changed_target(const char *file, const char *digest_str) {
	return !path_exists(file) || predep_hash_changed(file, digest_str) || openchecktargetclose(file);
}

int
predeps_changed(int db_fd) {
	char buf[BUFFER_INSIZE];
	buffer b = BUFFER_INIT(&buffer_read, db_fd, buf, BUFFER_INSIZE);

	stralloc ln = STRALLOC_ZERO;
	int changed = 0;
	int r;
	while(!changed && (r = skagetln(&b, &ln, '\n')) > 0) {
		ln.s[ln.len-1] = '\0';
		char *dep[3]; // FIXME
		int i = 0;
		dep[0] = ln.s;
		i = str_chr(dep[0], '\t');
		dep[0][i] = '\0';
		i++;
		dep[1] = &dep[0][i];
		i = str_chr(&ln.s[i], '\t');
		dep[1][i] = '\0';
		i++;
		dep[2] = &dep[1][i];
		switch(ln.s[0]) {
		case 't':
			str_diff(dep[0], "target");
			changed = predep_changed_target(dep[1], dep[2]);
			break;
		case 's':
			str_diff(dep[0], "source");
			changed = predep_changed_source(dep[1], dep[2]);
			break;
		case 'a':
			str_diff(dep[0], "absent");
			changed = predep_created(dep[1]);
			break;
		default:
			break;
		}
		ln.len = 0;
	}
	stralloc_free(&ln);

	if(r == -1) die("");

	return changed;
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
