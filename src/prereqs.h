#ifndef _PREREQS_H
#define _PREREQS_H

extern int      prereqs_existfor(const char *target);
extern int      prereqs_changedfor(const char *target);

extern int      prereqs_renamefor(const char *target, const char *tmpfile);

extern size_t   prereq_record_target(const char *file);
extern size_t   prereq_record_source(const char *file);
extern size_t   prereq_record_absent(const char *file);

#endif
