#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <time.h>
#include "date.h"

static int date2str_(uint32_t mask, uint32_t a, const char *f, int v, char *str);
static int parsedate_(const char *str, uint32_t *mask, uint32_t mask2, int upper, int lower, int *t);

static int
date2str_(uint32_t mask, uint32_t a, const char *f, int v, char *str)
{
    char temp[32];

    if ((mask & a) == a)
    {
        snprintf(temp, sizeof(temp)/sizeof(temp[0]), f, v);
        strncat(str, temp, sizeof(temp)/sizeof(temp[0]));
        return 1;
    }
    return 0;
}

static int
parsedate_(const char *str, uint32_t *mask, uint32_t mask2, int upper, int lower, int *t)
{
    int p = 0;
    int temp = 0;

    for (;;)
    {
        if (!('0' <= str[p] && str[p] <= '9')) break;
        temp = temp * 10 + str[p++] - '0';
    }

    if (lower <= temp && temp <= upper)
    {
        *t = 0;
        return 0;
    }
    else
    {
        *t = temp;
        *mask |= mask2;
    }
    return p;
}

int
isleapyear(int year)
{
    return (year % 4) == 0 && ((year % 100) != 0 || (year % 400) == 0);
}

int
daysinmonth(int y, int m)
{
    int month[] = {
        0,
        31, 28, 31, 30,
        31, 30, 31, 31,
        30, 31, 30, 31
    };
    int leap = isleapyear(y) ? 1 : 0;

    if (1 <= m && m <= 12)
    {
        return month[m] + leap;
    }
    else
    {
        return -1;
    }
}

void
incdate(struct mydate *t, uint32_t mask)
{
    if ((mask & MASK_DAY) == MASK_DAY)
    {
        t->day++;
        if (t->day > daysinmonth(t->year, t->month))
        {
            t->day = 1;
            t->month++;
            if (t->month > 12)
            {
                t->month = 1;
                t->year++;
            }
        }
    }
    else if ((mask & MASK_MONTH) == MASK_MONTH)
    {
        t->month++;
        if (t->month > 12)
        {
            t->month = 1;
            t->year++;
        }
    }
    else if ((mask & MASK_YEAR) == MASK_YEAR)
    {
        t->year++;
    }
}

int
date2int(const struct mydate *t)
{
    int x = 0;

    if ((t->mask & MASK_YEAR) == MASK_YEAR)
    {
        x = t->year * 100 * 100;
    }
    if ((t->mask & MASK_MONTH) == MASK_MONTH)
    {
        x += t->month * 100;
    }
    if ((t->mask & MASK_DAY) == MASK_DAY)
    {
        x += t->day;
    }
    return x;
}

void
getnowdate(struct mydate *t)
{
    time_t now;
    struct tm *ts;

    now = time(NULL);
    ts = localtime(&now);
    t->year  = ts->tm_year + 1900;
    t->month = ts->tm_mon + 1;
    t->day   = ts->tm_mday;
    t->mask  = MASK_YMD;
}

char *
date2str(const struct mydate *t)
{
    static char str[256];
    memset(str, '\0', sizeof(str)/sizeof(str[0]));
    
    date2str_(t->mask, MASK_YEAR,  "%d",  t->year,  str) && 
    date2str_(t->mask, MASK_MONTH, "/%d", t->month, str) && 
    date2str_(t->mask, MASK_DAY,   "/%d", t->day,   str);
    return str;
}

char *
date2str_w(const struct mydate *t, uint32_t m)
{
    static char str[256];
    memset(str, '\0', sizeof(str)/sizeof(str[0]));
    
    date2str_(t->mask, MASK_YEAR,  "%4d年", t->year,  str) && 
    date2str_(t->mask, MASK_MONTH, "%2d月", t->month, str) && 
    date2str_(t->mask, MASK_DAY,   "%2d日", t->day,   str);
    return str;
}

int
parsedate(const char *str, struct mydate *t, uint32_t m)
{
    int ret;
    int p = 0;
    t->mask = MASK_EMPTY;
    t->year = 0;
    t->month = t->day = 1;

    ret = parsedate_(str+p, &(t->mask), MASK_YEAR,  2000, 2100, &(t->year));
    if (!ret) return 0; else p += ret;
    if (str[p] != '/') goto LEND; else p++;

    ret = parsedate_(str+p, &(t->mask), MASK_MONTH,    1,   12, &(t->month));
    if (!ret) goto LEND; else p += ret;
    if (str[p] != '/') goto LEND; else p++;

    ret = parsedate_(str+p, &(t->mask), MASK_DAY,      1,   31, &(t->day));
    if (!ret) goto LEND; else p += ret;

LEND:
    if ((m & t->mask) != m)
    {
        fprintf(stderr, "日付指定に必要な要素が足りません(%s)\n", str);
        exit(EXIT_FAILURE);
    }
    return p;
}

