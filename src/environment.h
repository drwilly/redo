#ifndef _ENVIRONMENT_H
#define _ENVIRONMENT_H

int            redo_getenv_int(const char *k, int def);
void           redo_setenv_int(const char *k, int v); 
const char *   redo_getenv_str(const char *k, const char *def);
void           redo_setenv_str(const char *k, const char *v); 

#endif
