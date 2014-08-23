#ifndef _LIBC_WRAPPER_H
#define _LIBC_WRAPPER_H

void *  xcalloc(size_t nmemb, size_t size);
int     xopen(const char *pathname, int flags, ...);
int     xclose(int fd);
char *  xstrdup(const char *s);
int     try_unlink(const char *pathname);

#endif
