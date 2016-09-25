#ifndef _CHECKSUM_H
#define _CHECKSUM_H

extern void     file_checksum_str_compute(const char *file, char *checksum_str);
extern int      file_checksum_str_changed(const char *file, const char *checksum_str);

#endif
