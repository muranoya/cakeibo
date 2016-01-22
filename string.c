#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "string.h"
#include "util.h"

#define INIT_LEN (16)

static void realloc_string(String *string, int newsize);

String *
make_string()
{
    String *string = (String*)try_malloc(sizeof(String));
    string->str = (char*)try_malloc(sizeof(char)*INIT_LEN);
    string->len = 0;
    string->size = INIT_LEN;
    return string;
}

String *
new_string(const String *str)
{
    String *string = make_string();
    append_string(string, str);
    return string;
}

String *
new2_string(const char *str)
{
    String *string = make_string();
    append3_string(string, str);
    return string;
}

void
free_string(String **string)
{
    if (*string == NULL) return;
    free((*string)->str);
    free(*string);
    *string = NULL;
}

void
append_string(String *dst, const String *src)
{
    int i;
    char *s, *d;

    if (dst->size <= (dst->len + src->len + 1))
    {
        realloc_string(dst, dst->size + src->len + 1);
    }

    s = src->str;
    d = dst->str+dst->len;
    for (i = 0; i < src->len; ++i) *d++ = *s++;
    *d = '\0';
    dst->len += src->len;
}

void
append2_string(String *dst, char c)
{
    if (dst->size <= (dst->len + 2))
    {
        realloc_string(dst, dst->size + INIT_LEN);
    }

    *(dst->str+dst->len) = c;
    *(dst->str+dst->len+1) = '\0';
    dst->len++;
}

void
append3_string(String *dst, const char *src)
{
    int len;
    char *d;

    len = strlen(src);
    if (dst->size <= (dst->len + len + 1))
    {
        realloc_string(dst, dst->size + len + 1);
    }

    d = dst->str + dst->len;
    while (*src != '\0') *d++ = *src++;
    *d = '\0';
    dst->len += len;
}

int
cmp_string(const String *s1, const String *s2)
{
    return strcmp(s1->str, s2->str);
}

int
cmp2_string(const String *s1, const char *s2)
{
    return strcmp(s1->str, s2);
}

static void
realloc_string(String *string, int newsize)
{
    string->str = (char*)try_realloc(string->str, sizeof(char)*newsize);
    string->size = newsize;
}

#ifdef TEST_STRING
int
main(int argc, char *argv[])
{
    String *str, *str2;

    str = make_string();
    append2_string(str, 'a'); printf("%s\n", str->str);
    append2_string(str, 'b'); printf("%s\n", str->str);
    append2_string(str, 'c'); printf("%s\n", str->str);
    append3_string(str, "defghijklmnopqrstuvwxyz");
    printf("%s\n", str->str);

    str2 = make_string();
    append3_string(str2, "ABCDEFG"); printf("%s\n", str2->str);
    append_string(str, str2); printf("%s\n", str->str);

    free_string(&str);
    free_string(&str2);
    
    return EXIT_SUCCESS;
}
#endif

