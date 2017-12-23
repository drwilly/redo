#ifndef _ENVIRONMENT_H
#define _ENVIRONMENT_H

extern int            redo_getenv_int(const char *k, int def);
extern void           redo_setenv_int(const char *k, int v);
extern const char *   redo_getenv_str(const char *k, const char *def);
extern void           redo_setenv_str(const char *k, const char *v);

#endif
