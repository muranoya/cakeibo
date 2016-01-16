#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdint.h>
#include <time.h>
#include <sys/stat.h>
#include <locale.h>

#define DATA_DIR (".cakeibo")

#define CMD_ADD  ("add")
#define CMD_STAT ("stat")
#define CMD_SHOW ("show")
#define CMD_HELP ("help")

#define MASK_YEAR  (0x20U)
#define MASK_MONTH (0x10U)
#define MASK_DAY   (0x08U)
#define MASK_HOUR  (0x04U)
#define MASK_MIN   (0x02U)
#define MASK_SEC   (0x01U)
#define MASK_EMPTY (0x00U)
#define MASK_YMD (MASK_YEAR | MASK_MONTH | MASK_DAY)
#define MASK_ALL (MASK_YMD | MASK_HOUR | MASK_MIN | MASK_SEC)

#define MAX_CAT_LEN (100)
#define MAX_LOC_LEN (100)
#define MAX_NOTE_LEN (600)

// MAX_CAT_LEN+MAX_NOTE_LEN+日付+金額の文字列を格納できるくらい
// 余裕のある大きさにする必要がある
#define BUF_LEN (1024)

#define DELIMITER ("; ")

struct mytime
{
    uint32_t mask;
    struct tm time;
};

struct record
{
    struct mytime time;
    char *cat;
    char *loc;
    int32_t money;
    char *note;
};

static void *try_realloc(void *pointer, size_t size);
static void *try_malloc(size_t size);
static void free_records(struct record *r, int len);
static void uses(char *argv[]);
static FILE *open(struct mytime *t, const char *mode);
static struct record *load(struct mytime *t, int *len);
static void append(struct mytime *t, char *cat, char *loc, int32_t money, char *note);
static int datetime2str_(uint32_t mask, uint32_t a, const char *f, int v, char *str);
static char *datetime2str(struct mytime *t);
static char *datetime2str_w(struct mytime *t, uint32_t m);
static int parsemoney(char *str, int32_t *m);
static int parsedate_(char *str, uint32_t *mask, uint32_t mask2,
        int upper, int lower, int *t, int offset);
static int parsedate(char *str, struct mytime *t);
static void getnowdate(struct mytime *t);
static void modeadd_opt(int *argc, char **argv[], char **loc, char **note, int *silent);
static void modeadd(int argc, char *argv[]);
static void modestat(int argc, char *argv[]);
static void modeshow(int argc, char *argv[]);

static void *
try_realloc(void *pointer, size_t size)
{
    void *p;

    p = realloc(pointer, size);
    if (p == NULL)
    {
        perror("");
        exit(EXIT_FAILURE);
    }
    return p;
}

static void *
try_malloc(size_t size)
{
    void *p;

    p = malloc(size);
    if (p == NULL)
    {
        perror("");
        exit(EXIT_FAILURE);
    }
    return p;
}

static void
free_records(struct record *r, int len)
{
    struct record *recp;
    int i;
    for (i = 0; i < len; i++)
    {
        recp = r+i;
        free(recp->cat);
        free(recp->loc);
        free(recp->note);
    }
    free(r);
}

static void
uses(char *argv[])
{
    fprintf(stderr,
            "%s %s [日付] [品名] [金額] [-m 備考] [-l 場所] [-s]\n"
            "\t家計簿に新しいデータを追加する\n"
            "\t[日付]: 省略可(その場合コマンドを実行した時点の時間になる)\n"
            "\t\t指定する場合，日にちまでは指定する必要がある\n"
            "\t[-s]: 結果を表示しない\n\n"

            "%s %s [日付]\n"
            "\t家計簿の統計情報を表示する\n"
            "\t[日付]: 省略可(省略した場合，今月のデータが表示される)\n\n"
            
            "%s %s [日付]\n"
            "\t家計簿のデータを表示する\n"
            "\t[日付]: 省略可(省略した場合，今月のデータが表示される)\n\n"
            
            "%s %s\n"
            "\tこのヘルプを表示する\n\n"

            "DESCRIPTION\n"
            "\t[日付]: 年/月/日:時:分:秒\n"
            "\t\t右から順に省略してよい\n"
            "\t\t例: '2015/3/8 13:20'はよいが，日にちを省略した'2015/3 13:20'は不可\n"
            "\t[金額]: 単位は円，\",\"を含む記述が可能\n",
            argv[0], CMD_ADD,
            argv[0], CMD_STAT,
            argv[0], CMD_SHOW,
            argv[0], CMD_HELP);

    exit(EXIT_FAILURE);
}

static FILE *
open(struct mytime *t, const char *mode)
{
    FILE *fp;
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
            home, DATA_DIR, t->time.tm_year+1900);
    mkdir(path,
            S_IRUSR | S_IWUSR | S_IXUSR |
            S_IRGRP | S_IWGRP | S_IXGRP |
            S_IROTH |           S_IXOTH);

    snprintf(path, len, "%s/%s/%d/%d",
            home, DATA_DIR,
            t->time.tm_year+1900, t->time.tm_mon+1);

    if ((fp = fopen(path, mode)) == NULL)
    {
        perror("");
        exit(EXIT_FAILURE);
    }

    free(path);
    return fp;
}

static struct record *
load(struct mytime *t, int *len)
{
    char buf[BUF_LEN];
    char buf2[BUF_LEN];
    struct record *recs;
    struct record *rec_p;
    int recs_num;
    FILE *fp = open(t, "r");
    int i, p = 0, line = 0;
    int delimiter_len = strlen(DELIMITER);
    char *errstr = NULL;
    int temp;

    recs_num = 100;
    recs = (struct record*)try_malloc(sizeof(struct record)*recs_num);

    for (;; p = 0, line++)
    {
        if (fgets(buf, sizeof(buf)/sizeof(buf[0]), fp) == NULL) break;

        if (recs_num == line)
        {
            recs_num += 100;
            recs = (struct record*)try_realloc(recs, sizeof(struct record)*recs_num);
        }

        rec_p = recs+line;

        // 日付
        p = parsedate(buf, &(rec_p->time));
        if (p == 0 || strncmp(DELIMITER, buf+p, delimiter_len))
        {
            errstr = "ログファイルの日付フォーマットが正しくありません";
            goto LERROR;
        }
        p += delimiter_len;

        // 品名
        for (i = 0; ; i++, p++)
        {
            if (strncmp(DELIMITER, buf+p, delimiter_len) == 0) break;
            if (i > MAX_CAT_LEN)
            {
                errstr = "ログファイルの品名が長過ぎます";
                goto LERROR;
            }
            buf2[i] = *(buf+p);
        }
        buf2[i] = '\0';
        rec_p->cat = (char*)try_malloc((strlen(buf2)+1)*sizeof(char));
        strcpy(rec_p->cat, buf2);
        p += delimiter_len;

        // 場所
        for (i = 0; ; i++, p++)
        {
            if (strncmp(DELIMITER, buf+p, delimiter_len) == 0) break;
            if (i > MAX_LOC_LEN)
            {
                errstr = "ログファイルの場所名が長すぎます";
                goto LERROR;
            }
            buf2[i] = *(buf+p);
        }
        buf2[i] = '\0';
        rec_p->loc = (char*)try_malloc((strlen(buf2)+1)*sizeof(char));
        strcpy(rec_p->loc, buf2);
        p += delimiter_len;        

        // 金額
        if (!(temp = parsemoney(buf+p, &((recs+line)->money))))
        {
            errstr = "ログファイルの金額フォーマットが正しくありません";
            goto LERROR;
        }
        p += delimiter_len + temp;

        // 備考
        for (i = 0; ; i++, p++)
        {
            if (*(buf+p) == '\n') break;
            if (i > MAX_NOTE_LEN)
            {
                errstr = "ログファイルの備考が長過ぎます";
                goto LERROR;
            }
            buf2[i] = *(buf+p);
        }
        buf2[i] = '\0';
        rec_p->note = (char*)try_malloc((strlen(buf2)+1)*sizeof(char));
        strcpy(rec_p->note, buf2);
    }

    fclose(fp);
    *len = line;
    return recs;

LERROR:
    fprintf(stderr, "Error: %s(%d年%d月:%d行目)\n",
            errstr, t->time.tm_year+1900, t->time.tm_mon+1, line+1);
    exit(EXIT_FAILURE);
    return NULL;
}

static void
append(struct mytime *t, char *cat, char *loc, int32_t money, char *note)
{
    FILE *fp = open(t, "a");
    
    fprintf(fp,
            "%s%s"
            "%s%s"
            "%s%s"
            "%d%s"
            "%s\n",
            datetime2str(t), DELIMITER,
            cat, DELIMITER,
            loc == NULL ? "" : loc, DELIMITER,
            money, DELIMITER,
            note == NULL ? "" : note);

    fclose(fp);
}

static int
datetime2str_(uint32_t mask, uint32_t a, const char *f, int v, char *str)
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

static char *
datetime2str(struct mytime *t)
{
    static char str[256];
    memset(str, '\0', sizeof(str)/sizeof(str[0]));
    
    if (!datetime2str_(t->mask, MASK_YEAR,   "%d", t->time.tm_year+1900, str)) return str;
    if (!datetime2str_(t->mask, MASK_MONTH, "/%d", t->time.tm_mon+1,     str)) return str;
    if (!datetime2str_(t->mask, MASK_DAY,   "/%d", t->time.tm_mday,      str)) return str;
    if (!datetime2str_(t->mask, MASK_HOUR,  ":%d", t->time.tm_hour,      str)) return str;
    if (!datetime2str_(t->mask, MASK_MIN,   ":%d", t->time.tm_min,       str)) return str;
    if (!datetime2str_(t->mask, MASK_SEC,   ":%d", t->time.tm_sec,       str)) return str;
    return str;
}

static char *
datetime2str_w(struct mytime *t, uint32_t m)
{
    static char str[256];
    memset(str, '\0', sizeof(str)/sizeof(str[0]));
    
    if (((m & MASK_YEAR) == MASK_YEAR)
            && (!datetime2str_(t->mask, MASK_YEAR,  "%4d年", t->time.tm_year+1900, str))) return str;
    if (((m & MASK_MONTH) == MASK_MONTH)
            && (!datetime2str_(t->mask, MASK_MONTH, "%2d月", t->time.tm_mon+1,     str))) return str;
    if (((m & MASK_DAY) == MASK_DAY)
            && (!datetime2str_(t->mask, MASK_DAY,   "%2d日", t->time.tm_mday,      str))) return str;
    if (((m & MASK_HOUR) == MASK_HOUR)
            && (!datetime2str_(t->mask, MASK_HOUR, " %2d時", t->time.tm_hour,      str))) return str;
    if (((m & MASK_MIN) == MASK_MIN)
            && (!datetime2str_(t->mask, MASK_MIN,   "%2d分", t->time.tm_min,       str))) return str;
    if (((m & MASK_SEC) == MASK_SEC)
            && (!datetime2str_(t->mask, MASK_SEC,   "%2d秒", t->time.tm_sec,       str))) return str;
    return str;
}

// interger:
//     '-' decimal-constant
//     decimal-constant
// decimal-constant:
//     [0-9]
//     decimal-constant [0-9]
//     decimal-constant ',' [0-9]
static int
parsemoney(char *str, int32_t *m)
{
    int32_t ans = 0;
    int sign = 1;
    int p = 0;

    if (str[p] == '-') { sign = -1; p++; }
    if (!('0' <= str[p] && str[p] <= '9')) return 0;
    for (;;)
    {
        if (',' == str[p]) { p++; continue; }
        if (!('0' <= str[p] && str[p] <= '9')) break;
        ans = ans * 10 + str[p++] - '0';
    }
    *m = ans * sign;
    return p;
}

static int
parsedate_(char *str, uint32_t *mask, uint32_t mask2, int upper, int lower, int *t, int offset)
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
        *t = temp - offset;
        *mask |= mask2;
    }
    return p;
}

static int
parsedate(char *str, struct mytime *t)
{
    int ret;
    int p = 0;
    t->mask = MASK_EMPTY;

    ret = parsedate_(str+p, &t->mask, MASK_YEAR,  1900, 2100, &(t->time.tm_year), 1900);
    if (!ret) return 0;
    p += ret;
    if (str[p] != '/') goto LEND;
    p++;

    ret = parsedate_(str+p, &t->mask, MASK_MONTH,    1,   12, &(t->time.tm_mon),  1);
    if (!ret) goto LEND;
    p += ret;
    if (str[p] != '/') goto LEND;
    p++;

    ret = parsedate_(str+p, &t->mask, MASK_DAY,      1,   31, &(t->time.tm_mday), 0);
    if (!ret) goto LEND;
    p += ret;
    if (str[p] != ':') goto LEND;
    p++;

    ret = parsedate_(str+p, &t->mask, MASK_HOUR,     0,   23, &(t->time.tm_hour), 0);
    if (!ret) goto LEND;
    p += ret;
    if (str[p] != ':') goto LEND;
    p++;

    ret = parsedate_(str+p, &t->mask, MASK_MIN,      0,   59, &(t->time.tm_min),  0);
    if (!ret) goto LEND;
    p += ret;
    if (str[p] != ':') goto LEND;
    p++;

    ret = parsedate_(str+p, &t->mask, MASK_SEC,      0,   60, &(t->time.tm_sec),  0);
    if (!ret) goto LEND;
    p += ret;

LEND:
    return p;
}

static void
getnowdate(struct mytime *t)
{
    time_t now;
    struct tm *ts;

    now = time(NULL);
    ts = localtime(&now);
    t->time = *ts;
    t->mask = MASK_ALL;
}

static void
modeadd_opt(int *argc, char **argv[], char **loc, char **note, int *silent)
{
    int result;
    int note_len;
    int loc_len;

    *loc = NULL;
    *note = NULL;
    *silent = 0;
    while ((result = getopt(*argc, *argv, "l:m:s")) != -1)
    {
        switch (result)
        {
            case 'm':
                note_len = (strlen(optarg)+1)*sizeof(char);
                if (note_len > MAX_NOTE_LEN)
                {
                    fprintf(stderr, "Error: 備考が長過ぎます(最大%d文字，%ld文字)\n",
                            MAX_NOTE_LEN, strlen(optarg));
                    exit(EXIT_FAILURE);
                }
                *note = (char *)try_malloc(note_len);
                strcpy(*note, optarg);
                break;
            case 'l':
                loc_len = (strlen(optarg)+1)*sizeof(char);
                if (loc_len > MAX_LOC_LEN)
                {
                    fprintf(stderr, "Error: 場所が長過ぎます(最大%d文字，%ld文字)\n",
                            MAX_LOC_LEN, strlen(optarg));
                    exit(EXIT_FAILURE);
                }
                *loc = (char*)try_malloc(loc_len);
                strcpy(*loc, optarg);
                break;
            case 's':
                *silent = 1;
                break;
            default:
                uses(*argv);
                break;
        }
    }

    *argc -= optind;
    *argv += optind + 1;
}

static void
modeadd(int argc_org, char *argv_org[])
{
    int argc = argc_org;
    char **argv = argv_org;
    struct mytime time;
    int silent;
    char *note, *cat, *loc;
    int cat_len;
    int32_t money;

    if (strcmp(CMD_ADD, argv_org[1])) uses(argv_org);
    modeadd_opt(&argc, &argv, &loc, &note, &silent);
    if (argc != 3 && argc != 4) uses(argv_org);

    // 日付
    if (argc == 3)
    {
        getnowdate(&time);
    }
    else
    {
        if (!parsedate(*argv, &time))
        {
            fprintf(stderr, "Error: 日付のフォーマットが正しくありません(%s)\n",
                    *argv);
            exit(EXIT_FAILURE);
        }
        if ((time.mask & MASK_YMD) != MASK_YMD)
        {
            fprintf(stderr, "Error: 日付指定には年月日が最低でも必要です(%s)\n",
                    *argv);
            exit(EXIT_FAILURE);
        }
        argv++;
    }

    // 品名
    cat_len = (strlen(*argv)+1)*sizeof(char);
    if (cat_len > MAX_CAT_LEN)
    {
        fprintf(stderr, "Error: 品名が長過ぎます(最大%d文字，%ld文字)\n",
                MAX_CAT_LEN, strlen(*argv));
        exit(EXIT_FAILURE);
    }
    cat = (char*)try_malloc(cat_len);
    strcpy(cat, *argv++);

    // 金額
    if (strlen(*argv) != parsemoney(*argv, &money))
    {
        fprintf(stderr, "Error: 金額のフォーマットが正しくありません(%s)\n",
                *argv);
        exit(EXIT_FAILURE);
    }

    // 追加
    append(&time, cat, loc, money, note);

    if (!silent)
    {
        int i, len, sum = 0;
        struct record *recs = load(&time, &len);

        printf("日付: %s\n品名: %s\n",
                datetime2str_w(&time, MASK_ALL), cat);
        if (loc != NULL) printf("場所: %s\n", loc);
        printf("金額: %'d\n", money);
        if (note != NULL) printf("備考: %s\n", note);

        for (i = 0; i < len; ++i) sum += (recs+i)->money;
        printf("%d年%d月の合計使用金額: %'d円\n",
                time.time.tm_year+1900, time.time.tm_mon+1, sum);
        free_records(recs, len);
    }

    free(note);
    free(loc);
    free(cat);
}

static void
modestat(int argc, char *argv[])
{
}

static void
modeshow(int argc, char *argv[])
{
    struct record *recs;
    struct record *rec_p;
    int recs_num;
    struct mytime time;
    int i;

    if (strcmp(CMD_SHOW, argv[1])) uses(argv);

    if (argc == 2)
    {
        getnowdate(&time);
    }
    else
    {
        if (!parsedate(argv[2], &time))
        {
            fprintf(stderr, "Error: 日付のフォーマットが正しくありません(%s)\n",
                    argv[0]);
            exit(EXIT_FAILURE);
        }
        if ((time.mask & MASK_YEAR) != MASK_YEAR)
        {
            fprintf(stderr, "Error: 日付指定には年が最低でも必要です(%s)\n",
                    argv[0]);
            exit(EXIT_FAILURE);
        }
    }

    recs = load(&time, &recs_num);
    for (i = 0; i < recs_num; ++i)
    {
        rec_p = recs+i;
        printf("%s 品名:%s 場所:%s 金額:%'d\n",
                datetime2str_w(&(rec_p->time), MASK_YMD), rec_p->cat, rec_p->loc, rec_p->money);
        if (strlen(rec_p->note) > 0)
        {
            printf("備考:%s\n", rec_p->note);
        }
    }
}

int
main(int argc, char *argv[])
{
    setlocale(LC_NUMERIC, getenv("LANG"));
    if (argc <= 1)                           uses(argv);
    if      (strcmp(CMD_ADD,  argv[1]) == 0) modeadd(argc, argv);
    else if (strcmp(CMD_STAT, argv[1]) == 0) modestat(argc, argv);
    else if (strcmp(CMD_SHOW, argv[1]) == 0) modeshow(argc, argv);
    else if (strcmp(CMD_HELP, argv[1]) == 0) uses(argv);
    else                                     uses(argv);

    return EXIT_SUCCESS;
}

