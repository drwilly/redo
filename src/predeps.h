#ifndef _PREDEPS_H
#define _PREDEPS_H

void predep_record(int db, const char type, const char *path);
int predeps_changed(const char *target);

#endif
