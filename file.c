#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include "file.h"
#include "util.h"

FILE *
open(int y, int m, const char *mode, int ignore_error)
{
    FILE *fp = NULL;
    char *home;
    char *path;
    int len;

    home = getenv("HOME");

    len = sizeof(char)*(strlen(DATA_DIR)+strlen(home)+128);
    path = (char*)try_malloc(len);

    snprintf(path, len, "%s/%s",
            home, DATA_DIR);
    mkdir(path,
            S_IRUSR | S_IWUSR | S_IXUSR |
            S_IRGRP | S_IWGRP | S_IXGRP |
            S_IROTH |           S_IXOTH);
    
    snprintf(path, len, "%s/%s/%d",
            home, DATA_DIR,
            y);
    mkdir(path,
            S_IRUSR | S_IWUSR | S_IXUSR |
            S_IRGRP | S_IWGRP | S_IXGRP |
            S_IROTH |           S_IXOTH);
    
    snprintf(path, len, "%s/%s/%d/%d",
            home, DATA_DIR,
            y, m);

    if ((fp = fopen(path, mode)) == NULL)
    {
        if (!ignore_error)
        {
            perror("");
            exit(EXIT_FAILURE);
        }
    }

    free(path);
    return fp;
}

void
append(int y, int m, const char *del, const char *date,
        const char *cat, const char *loc, const char *note, int32_t money)
{
    FILE *fp = open(y, m, "a", 0);
    
    fprintf(fp,
            "%s%s"
            "%s%s"
            "%s%s"
            "%d%s"
            "%s%s\n",
            date, del,
            cat, del,
            loc == NULL ? "" : loc, del,
            money, del,
            note == NULL ? "" : note, del);

    fclose(fp);
}

