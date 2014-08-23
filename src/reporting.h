#ifndef _REPORTING_H
#define _REPORTING_H

#include <stdarg.h>

#define NORETURN __attribute__((__noreturn__))
#define NORETURN_PTR __attribute__((__noreturn__))

void vreportf(const char *prefix, const char *fmt, va_list params);

void set_die_routine(NORETURN_PTR void (*routine)(const char *fmt, va_list params));
void set_error_routine(void (*routine)(const char *fmt, va_list params));
void set_warning_routine(void (*routine)(const char *fmt, va_list params));
void set_info_routine(void (*routine)(const char *fmt, va_list params));
void set_die_is_recursing_routine(int (*routine)(void));

void NORETURN die(const char *fmt, ...);
void NORETURN die_errno(const char *fmt, ...);

void error(const char *fmt, ...);
void warning(const char *fmt, ...);
void info(const char *fmt, ...);

#endif
