#ifndef _UTIL_H
#define _UTIL_H

#include <stddef.h>

#define	len(x) (sizeof(x) / sizeof(*x))

size_t	str_split(char ***delimv, const char *str, const char delim);

#endif
