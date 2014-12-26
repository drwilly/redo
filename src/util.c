#include "util.h"

size_t
str_split(char ***delimv, const char *str, const char delim) {
	size_t bufsize = 4;
	size_t delimc = 0;
	*delimv = realloc(*delimv, bufsize * sizeof(*delimv));

	(*delimv)[delimc++] = str;
	for((*delimv)[delimc] = strchrnul(str+1, delim); (*delimv)[delimc][0] != '\0';) {
		(*delimv)[delimc]++;
		if(++delimc >= bufsize) {
			bufsize <<= 1;
			*delimv = realloc(*delimv, bufsize * sizeof(*delimv));
		}
		(*delimv)[delimc] = strchrnul((*delimv)[delimc-1]+1, delim);
	}

	return delimc;
}
