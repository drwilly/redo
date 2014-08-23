#ifndef _PATH_H
#define _PATH_H

int           is_dir_sep(char c);
int           path_is_absolute(const char *path);
int           path_exists(const char *path);
const char *  path_absolute(const char *path);
//const char *  path_normalize(const char *path);

#endif
