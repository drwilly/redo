#ifndef _PREDEPS_H
#define _PREDEPS_H

void predep_record(int db_fd, const char type, const char *file);
int predeps_changed(int db_fd);
int predeps_opencheckclose(const char *file);

int predeps_sadbfile(stralloc *sa, const char *target);

#endif
