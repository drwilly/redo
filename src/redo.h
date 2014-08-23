#ifndef _REDO_H
#define _REDO_H

#include <stdio.h>

#define REDO_ENV_DEPTH "REDO_DEPTH"
#define REDO_ENV_DB "REDO_DB"
#define REDO_ENV_KEEPGOING "REDO_KEEPGOING"
#define REDO_ENV_JOBS "REDO_JOBS"
#define REDO_ENV_DEBUG "REDO_DEBUG"
#define REDO_ENV_NOCOLOR "REDO_NOCOLOR"
#define REDO_ENV_SHUFFLE "REDO_SHUFFLE"

#define REDO_DEFAULT_TARGET "all"
#define REDO_DEFAULT_INTERPRETER "/bin/sh"

int     redo(const char *target);
int     redo_ifchange(const char *target, int parent_db);
int     redo_ifcreate(const char *target, int parent_db);

#endif
