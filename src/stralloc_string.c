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
		{ .s = (char *)str1, .len = str_len(str1) },
		{ .s = "", .len = 1 },
	};
	return stralloc_string_catv_internal(sa, v, 1+1);
}

int
stralloc_string_cats2(stralloc *sa, const char *str1, const char *str2) {
	siovec_t v[2+1] = {
		{ .s = (char *)str1, .len = str_len(str1) },
		{ .s = (char *)str2, .len = str_len(str2) },
		{ .s = "", .len = 1 },
	};
	return stralloc_string_catv_internal(sa, v, 2+1);
}

int
stralloc_string_cats3(stralloc *sa, const char *str1, const char *str2, const char *str3) {
	siovec_t v[3+1] = {
		{ .s = (char *)str1, .len = str_len(str1) },
		{ .s = (char *)str2, .len = str_len(str2) },
		{ .s = (char *)str3, .len = str_len(str3) },
		{ .s = "", .len = 1 },
	};
	return stralloc_string_catv_internal(sa, v, 3+1);
}

int
stralloc_string_cats4(stralloc *sa, const char *str1, const char *str2, const char *str3, const char *str4) {
	siovec_t v[4+1] = {
		{ .s = (char *)str1, .len = str_len(str1) },
		{ .s = (char *)str2, .len = str_len(str2) },
		{ .s = (char *)str3, .len = str_len(str3) },
		{ .s = (char *)str4, .len = str_len(str4) },
		{ .s = "", .len = 1 },
	};
	return stralloc_string_catv_internal(sa, v, 4+1);
}

int
stralloc_string_cats5(stralloc *sa, const char *str1, const char *str2, const char *str3, const char *str4, const char *str5) {
	siovec_t v[5+1] = {
		{ .s = (char *)str1, .len = str_len(str1) },
		{ .s = (char *)str2, .len = str_len(str2) },
		{ .s = (char *)str3, .len = str_len(str3) },
		{ .s = (char *)str4, .len = str_len(str4) },
		{ .s = (char *)str5, .len = str_len(str5) },
		{ .s = "", .len = 1 },
	};
	return stralloc_string_catv_internal(sa, v, 5+1);
}
