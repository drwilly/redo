#ifndef _PREDEPS_H
#define _PREDEPS_H

extern int      predeps_existfor(const char *target);
extern int      predeps_changedfor(const char *target);

extern int      predeps_open();
extern int      predeps_linkfor(const int fd, const char *target);

extern size_t   predep_record_target(const char *file);
extern size_t   predep_record_source(const char *file);
extern size_t   predep_record_absent(const char *file);

#endif
