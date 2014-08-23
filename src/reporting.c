/*
 * GIT - The information manager from hell
 *
 * Copyright (C) Linus Torvalds, 2005
 */

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <errno.h>
#include <string.h>

#include "reporting.h"

void
vreportf(const char *prefix, const char *fmt, va_list params) {
	char msg[4096];
	vsnprintf(msg, sizeof(msg), fmt, params);
	fprintf(stderr, "%s%s\n", prefix, msg);
}

static
NORETURN void
die_builtin(const char *fmt, va_list params) {
	vreportf("[X] ", fmt, params);
	exit(128);
}

static
void
error_builtin(const char *fmt, va_list params) {
	vreportf("[E]", fmt, params);
}

static
void
warn_builtin(const char *fmt, va_list params) {
	vreportf("[W] ", fmt, params);
}

static
void
info_builtin(const char *fmt, va_list params) {
	vreportf("[I] ", fmt, params);
}

static
int
die_is_recursing_builtin(void) {
	static int dying;
	return dying++;
}

/* If we are in a dlopen()ed .so write to a global variable would segfault
 * (ugh), so keep things static. */
static NORETURN_PTR void (*die_routine)(const char *err, va_list params) = die_builtin;
static void (*error_routine)(const char *fmt, va_list params) = error_builtin;
static void (*warn_routine)(const char *fmt, va_list params) = warn_builtin;
static void (*info_routine)(const char *fmt, va_list params) = info_builtin;
static int (*die_is_recursing)(void) = die_is_recursing_builtin;

void
set_die_routine(NORETURN_PTR void (*routine)(const char *fmt, va_list params)) {
	die_routine = routine;
}

void
set_error_routine(void (*routine)(const char *fmt, va_list params)) {
	error_routine = routine;
}

void
set_warning_routine(void (*routine)(const char *fmt, va_list params)) {
	warn_routine = routine;
}

void
set_info_routine(void (*routine)(const char *fmt, va_list params)) {
	info_routine = routine;
}

void
set_die_is_recursing_routine(int (*routine)(void)) {
	die_is_recursing = routine;
}

NORETURN void
die(const char *fmt, ...) {
	va_list params;

	if(die_is_recursing()) {
		fputs("fatal: recursion detected in die handler\n", stderr);
		exit(128);
	}

	va_start(params, fmt);
	die_routine(fmt, params);
	va_end(params);
}

NORETURN void
die_errno(const char *fmt, ...) {
	va_list params;
	char fmt_with_err[1024];
	char str_error[256], *err;
	int i, j;

	if(die_is_recursing()) {
		fputs("fatal: recursion detected in die_errno handler\n",
			stderr);
		exit(128);
	}

	err = strerror(errno);
	for(i = j = 0; err[i] && j < sizeof(str_error) - 1; ) {
		if((str_error[j++] = err[i++]) != '%')
			continue;
		if(j < sizeof(str_error) - 1) {
			str_error[j++] = '%';
		} else {
			/* No room to double the '%', so we overwrite it with
			 * '\0' below */
			j--;
			break;
		}
	}
	str_error[j] = 0;
	snprintf(fmt_with_err, sizeof(fmt_with_err), "%s: %s", fmt, str_error);

	va_start(params, fmt);
	die_routine(fmt_with_err, params);
	va_end(params);
}

#undef error
void
error(const char *fmt, ...) {
	va_list params;

	va_start(params, fmt);
	error_routine(fmt, params);
	va_end(params);
}

void
warning(const char *fmt, ...) {
	va_list params;

	va_start(params, fmt);
	warn_routine(fmt, params);
	va_end(params);
}

void
info(const char *fmt, ...) {
	va_list params;

	va_start(params, fmt);
	info_routine(fmt, params);
	va_end(params);
}
