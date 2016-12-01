#ifndef _STRALLOC_STRING_H
#define _STRALLOC_STRING_H

#include <skalibs/stralloc.h>

extern int      stralloc_string_cats1(stralloc *sa, const char *str1);
extern int      stralloc_string_cats2(stralloc *sa, const char *str1, const char *str2);
extern int      stralloc_string_cats3(stralloc *sa, const char *str1, const char *str2, const char *str3);
extern int      stralloc_string_cats4(stralloc *sa, const char *str1, const char *str2, const char *str3, const char *str4);
extern int      stralloc_string_cats5(stralloc *sa, const char *str1, const char *str2, const char *str3, const char *str4, const char *str5);

#endif
