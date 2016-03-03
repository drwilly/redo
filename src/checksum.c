#include <skalibs/stddjb.h>
#include <skalibs/stdcrypto.h>

#include "reporting.h"

static const char *hexdigit = "0123456789abcdef";

static
void
sha1_file(const char *file, char digest[20]) {
	int fd = open_read(file);
	if(fd == -1) {
		die_errno("open_read('%s') failed", file);
	}
	size_t count;
#if 1
	struct stat sb;
	if(fstat(fd, &sb) != 0) {
		die_errno("stat('%s', ...) failed", file);
	}
	count = sb.st_blksize;
#else
	count = 4096;
#endif

	SHA1Schedule ctx;
	size_t messagelen;
	char message[count];

	sha1_init(&ctx);
	while((messagelen = fd_read(fd, message, count)) > 0) {
		sha1_update(&ctx, message, messagelen);
	}
	sha1_final(&ctx, digest);

	fd_close(fd);
}

void (*file_checksum_compute)(const char *file, char *checksum) = &sha1_file;

int
file_checksum_changed(const char *file, const char checksum_str[20*2+1]) {
	char checksum[20];
	file_checksum_compute(file, checksum);
	for(int i = 0; i < 20; i++) {
		if(checksum_str[2*i+0] != hexdigit[(checksum[i] & 0xf0) >> 4]) return 1;
		if(checksum_str[2*i+1] != hexdigit[(checksum[i] & 0x0f) >> 0]) return 1;
	}

	return 0;
}

void
hexstring_from_checksum(char *checksum_str, const char *checksum) {
	for(int i = 0; i < 20; i++) {
		checksum_str[2*i+0] = hexdigit[(checksum[i] & 0xf0) >> 4];
		checksum_str[2*i+1] = hexdigit[(checksum[i] & 0x0f) >> 0];
	}
}
