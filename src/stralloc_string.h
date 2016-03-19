#ifndef _STRALLOC_STRING_H
#define _STRALLOC_STRING_H

#include <skalibs/stralloc.h>

extern int      stralloc_string_cats1(stralloc *sa, const char *str1) ;
extern int      stralloc_string_cats2(stralloc *sa, const char *str1, const char *str2) ;
extern int      stralloc_string_cats3(stralloc *sa, const char *str1, const char *str2, const char *str3) ;

extern int      stralloc_string_dirname(stralloc *sa, const char *path, size_t pathlen);
extern int      stralloc_string_basename(stralloc *sa, const char *path, size_t pathlen);

#endif
