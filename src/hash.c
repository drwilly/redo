#include "reporting.h"

#include "hash.h"

#ifndef POLARSSL_ONLY_SSL

#include <polarssl/sha1.h>
static int (*hash_routine)(const char *path, unsigned char output[REDO_HASH_LENGTH]) = &sha1_file;

#endif

void
hash(const char *path, unsigned char output[REDO_HASH_LENGTH]) {
	if(hash_routine(path, output) != 0) {
		die("Could not compute hash of file '%s'", path);
	}
}
