#ifndef _CHECKSUM_H
#define _CHECKSUM_H

extern void     (*file_checksum_compute)(const char *file, unsigned char *checksum);
extern int      file_checksum_changed(const char *file, const char *checksum_str);

extern void     hexstring_from_checksum(char *checksum_str, const unsigned char *checksum);

#endif
