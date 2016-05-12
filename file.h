#ifndef FILE_H
#define FILE_H

#include <stdio.h>

#define DATA_DIR (".cakeibo")

FILE *open(int y, int m, const char *mode, int ignore_error);
void append(int y, int m, const char *del, const char *date,
        const char *cat, const char *loc, const char *note, int32_t money);

#endif
