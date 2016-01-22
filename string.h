#ifndef STRING_H
#define STRING_H

typedef struct
{
    char *str;
    int len;
    int size;
} String;

String *make_string();
String *new_string(const String *str);
String *new2_string(const char *str);
void   free_string(String **string);
void   append_string(String *dst, const String *src);
void   append2_string(String *dst, char c);
void   append3_string(String *dst, const char *src);
int    cmp_string(const String *s1, const String *s2);
int    cmp2_string(const String *s1, const char *s2);

#endif
