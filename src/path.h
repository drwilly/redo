#ifndef _PATH_H
#define _PATH_H

int           is_dir_sep(char c);

int           path_is_absolute(const char *path);
int           path_exists(const char *path);

int           try_unlink(const char *file);

#endif
