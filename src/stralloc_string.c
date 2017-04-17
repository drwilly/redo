#include <skalibs/bytestr.h>

#include "stralloc_string.h"

static
int
stralloc_string_catv_internal(stralloc *sa, struct iovec const *iov, size_t n) {
	if(sa->s && sa->len > 0) sa->len--;
	if(!stralloc_catv(sa, iov, n)) goto err;
	return 1;
err:
	sa->len++;
	return 0;
}

int
stralloc_string_cats1(stralloc *sa, const char *str1) {
	struct iovec v[1+1] = {
		{ .iov_base = (char *)str1, .iov_len = str_len(str1) },
		{ .iov_base = "", .iov_len = 1 },
	};
	return stralloc_string_catv_internal(sa, v, 1+1);
}

int
stralloc_string_cats2(stralloc *sa, const char *str1, const char *str2) {
	struct iovec v[2+1] = {
		{ .iov_base = (char *)str1, .iov_len = str_len(str1) },
		{ .iov_base = (char *)str2, .iov_len = str_len(str2) },
		{ .iov_base = "", .iov_len = 1 },
	};
	return stralloc_string_catv_internal(sa, v, 2+1);
}

int
stralloc_string_cats3(stralloc *sa, const char *str1, const char *str2, const char *str3) {
	struct iovec v[3+1] = {
		{ .iov_base = (char *)str1, .iov_len = str_len(str1) },
		{ .iov_base = (char *)str2, .iov_len = str_len(str2) },
		{ .iov_base = (char *)str3, .iov_len = str_len(str3) },
		{ .iov_base = "", .iov_len = 1 },
	};
	return stralloc_string_catv_internal(sa, v, 3+1);
}

int
stralloc_string_cats4(stralloc *sa, const char *str1, const char *str2, const char *str3, const char *str4) {
	struct iovec v[4+1] = {
		{ .iov_base = (char *)str1, .iov_len = str_len(str1) },
		{ .iov_base = (char *)str2, .iov_len = str_len(str2) },
		{ .iov_base = (char *)str3, .iov_len = str_len(str3) },
		{ .iov_base = (char *)str4, .iov_len = str_len(str4) },
		{ .iov_base = "", .iov_len = 1 },
	};
	return stralloc_string_catv_internal(sa, v, 4+1);
}

int
stralloc_string_cats5(stralloc *sa, const char *str1, const char *str2, const char *str3, const char *str4, const char *str5) {
	struct iovec v[5+1] = {
		{ .iov_base = (char *)str1, .iov_len = str_len(str1) },
		{ .iov_base = (char *)str2, .iov_len = str_len(str2) },
		{ .iov_base = (char *)str3, .iov_len = str_len(str3) },
		{ .iov_base = (char *)str4, .iov_len = str_len(str4) },
		{ .iov_base = (char *)str5, .iov_len = str_len(str5) },
		{ .iov_base = "", .iov_len = 1 },
	};
	return stralloc_string_catv_internal(sa, v, 5+1);
}
