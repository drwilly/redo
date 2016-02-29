#ifndef _TMPFILE_H
#define _TMPFILE_H

extern int      tmpfile_create();
extern int      tmpfile_link(const int fd, const char *dest);

#endif
