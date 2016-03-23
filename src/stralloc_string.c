#include <skalibs/djbunix.h>

#include "stralloc_string.h"

static
int
stralloc_string_catv_internal(stralloc *sa, siovec_t const *v, size_t n) {
	if(sa->s && sa->len > 0) sa->len--;
	if(!stralloc_catv(sa, v, n)) goto err;
	return 1;
err:
	sa->len++;
	return 0;
}

int
stralloc_string_cats1(stralloc *sa, const char *str1) {
	siovec_t v[1+1] = {
		{ .s = str1, .len = str_len(str1) },
		{ .s = "", .len = 1 },
	};
	return stralloc_string_catv_internal(sa, v, 1+1);
}

int
stralloc_string_cats2(stralloc *sa, const char *str1, const char *str2) {
	siovec_t v[2+1] = {
		{ .s = str1, .len = str_len(str1) },
		{ .s = str2, .len = str_len(str2) },
		{ .s = "", .len = 1 },
	};
	return stralloc_string_catv_internal(sa, v, 2+1);
}

int
stralloc_string_cats3(stralloc *sa, const char *str1, const char *str2, const char *str3) {
	siovec_t v[3+1] = {
		{ .s = str1, .len = str_len(str1) },
		{ .s = str2, .len = str_len(str2) },
		{ .s = str3, .len = str_len(str3) },
		{ .s = "", .len = 1 },
	};
	return stralloc_string_catv_internal(sa, v, 3+1);
}

static
int
stralloc_string_xname_internal(stralloc *sa, const char *path, size_t pathlen, int (*fn)(stralloc *, const char *, unsigned int)) {
	size_t oldlen = sa->len;
	if(sa->s && sa->len > 0) sa->len--;
	if(!fn(sa, path, pathlen)) goto err;
	if(!stralloc_0(sa)) goto err;
	return 1;
err:
	sa->len = oldlen;
	sa->s[sa->len-1] = '\0';
	return 0;
}

int
stralloc_string_dirname(stralloc *sa, const char *path, size_t pathlen) {
	return stralloc_string_xname_internal(sa, path, pathlen, &sadirname);
}

int
stralloc_string_basename(stralloc *sa, const char *path, size_t pathlen) {
	return stralloc_string_xname_internal(sa, path, pathlen, &sabasename);
}
