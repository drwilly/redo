#ifndef _PATH_H
#define _PATH_H

int           path_exists(const char *path);

int           try_unlink(const char *file);
int           try_link_tmpfile(const char *tmpfile, const char *dest);

#endif
