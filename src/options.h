#ifndef _OPTIONS_H
#define _OPTIONS_H

#define REDO_ENV_DEPTH "REDO_DEPTH"
#define REDO_ENV_KEEPGOING "REDO_KEEPGOING"
#define REDO_ENV_JOBS "REDO_JOBS"
#define REDO_ENV_DEBUG "REDO_DEBUG"
#define REDO_ENV_NOCOLOR "REDO_NOCOLOR"
#define REDO_ENV_SHUFFLE "REDO_SHUFFLE"

extern int      args_filter_options(int argc, char *argv[]);
extern int      args_process_options(int argc, char *argv[]);

#endif
