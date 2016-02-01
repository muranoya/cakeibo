#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdint.h>
#include <sys/stat.h>
#include <locale.h>

//#include "hashmap.h"
//#include "string.h"
#include "util.h"
#include "date.h"

#define DATA_DIR (".cakeibo")

#define CMD_ADD  ("add")
#define CMD_SHOW ("show")
#define CMD_HELP ("help")

#define MAX_CAT_LEN (100)
#define MAX_LOC_LEN (100)
#define MAX_NOTE_LEN (500)

// MAX_LOC_LEN + MAX_CAT_LEN + MAX_NOTE_LEN + 日付 + 金額の文字列
// を格納できるくらい余裕のある大きさにする必要がある
#define BUF_LEN (1024)

#define DELIMITER ("; ")

struct record
{
    struct mydate date;
    char *cat;
    char *loc;
    int32_t money;
    char *note;
};

static void free_records(struct record *r, int len);
static void uses(char *argv[]);
static FILE *open(struct mydate *t, const char *mode, int ignore_error);
static struct record 
            *load(struct mydate *t, int *len, int ignore_open_error);
static void append(struct mydate *t, char *cat, char *loc, int32_t money, char *note);
static int  parsemoney(char *str, int32_t *m);
static int  recordcmp(const void *a, const void *b);
static void modeadd_opt(int *argc, char **argv[], char **loc, char **note, int *silent, int *force);
static void modeadd(int argc, char *argv[]);
static void modeshow_opt(int *argc, char **argv[], int *sort);
static void modeshow(int argc, char *argv[]);

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
            "%s %s [日付] [品名] [金額] [-m 備考] [-l 場所] [-s] [-f]\n"
            "\t家計簿に新しいデータを追加する\n"
            "\t[日付]: 省略可(その場合コマンドを実行した時点の時間になる)\n"
            "\t\t指定する場合，日にちまでは指定する必要がある\n"
            "\t[-s]: 結果を表示しない\n"
            "\t[-f]: 確認せずに追加する\n\n"

            "%s %s [範囲初めの日付] [範囲終わりの日付] [-s]\n"
            "\t家計簿のデータを表示する\n"
            "\t[範囲初めの日付]: 省略可\n"
            "\t[範囲終わりの日付]: 省略可\n"
            "\t範囲終わりの日付を省略した場合，範囲初めの日付に含まれるデータを表示する．\n"
            "\t2つとも省略した場合，今月のデータだけが表示される．\n"
            "\t[-s]: ソートして表示する\n\n"

            "%s %s\n"
            "\tこのヘルプを表示する\n\n"

            "DESCRIPTION\n"
            "\t[日付]: 年/月/日\n"
            "\t\t右から順に省略してよい\n"
            "\t[金額]: \",\"を含む記述が可能\n",
            argv[0], CMD_ADD,
            argv[0], CMD_SHOW,
            argv[0], CMD_HELP);

    exit(EXIT_FAILURE);
}

static FILE *
open(struct mydate *t, const char *mode, int ignore_error)
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
            t->year);
    mkdir(path,
            S_IRUSR | S_IWUSR | S_IXUSR |
            S_IRGRP | S_IWGRP | S_IXGRP |
            S_IROTH |           S_IXOTH);
    
    snprintf(path, len, "%s/%s/%d/%d",
            home, DATA_DIR,
            t->year, t->month);

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

static struct record *
load(struct mydate *t, int *len, int ignore_open_error)
{
    char buf[BUF_LEN];
    char buf2[BUF_LEN];
    struct record *recs;
    struct record *rec_p;
    int recs_num;
    FILE *fp = open(t, "r", ignore_open_error);
    int i, p = 0, line = 0;
    int delimiter_len = strlen(DELIMITER);
    char *errstr = NULL;
    int temp;

    if (fp == NULL) return NULL;

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
        rec_p->cat = NULL;
        rec_p->loc = NULL;
        rec_p->note = NULL;

        // 日付
        p = parsedate(buf, &(rec_p->date), MASK_YMD);
        if (strncmp(DELIMITER, buf+p, delimiter_len))
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
            //if (*(buf+p) == '\n') break;
            if (strncmp(DELIMITER, buf+p, delimiter_len) == 0) break;
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
            errstr, t->year, t->month, line+1);
    exit(EXIT_FAILURE);
    return NULL;
}

static void
append(struct mydate *t, char *cat, char *loc, int32_t money, char *note)
{
    FILE *fp = open(t, "a", 0);
    
    fprintf(fp,
            "%s%s"
            "%s%s"
            "%s%s"
            "%d%s"
            "%s%s\n",
            date2str(t), DELIMITER,
            cat, DELIMITER,
            loc == NULL ? "" : loc, DELIMITER,
            money, DELIMITER,
            note == NULL ? "" : note, DELIMITER);

    fclose(fp);
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
    int sign = 1, p = 0;

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
recordcmp(const void *a, const void *b)
{
    struct record *r1 = (struct record*)a;
    struct record *r2 = (struct record*)b;

    return date2int(&(r1->date)) - date2int(&(r2->date));
}

static void
recordsort(struct record *rec, int num)
{
    qsort(rec, num, sizeof(struct record), recordcmp);
}

static void
modeadd_opt(int *argc, char **argv[], char **loc, char **note, int *silent, int *force)
{
    int result;
    int note_len;
    int loc_len;

    *loc = NULL;
    *note = NULL;
    *silent = 0;
    *force = 0;
    while ((result = getopt(*argc, *argv, "l:m:sf")) != -1)
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
            case 'f':
                *force = 1;
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
    struct mydate time;
    int silent;
    int force;
    char *note, *cat, *loc;
    int cat_len;
    int32_t money;

    if (strcmp(CMD_ADD, argv_org[1])) uses(argv_org);
    modeadd_opt(&argc, &argv, &loc, &note, &silent, &force);
    if (argc != 3 && argc != 4) uses(argv_org);

    // 日付
    if (argc == 3)
    {
        getnowdate(&time);
    }
    else
    {
        parsedate(*argv, &time, MASK_YMD);
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

    printf("日付: %s\n品名: %s\n", date2str_w(&time, MASK_YMD), cat);
    if (loc != NULL) printf("場所: %s\n", loc);
    printf("金額: %'d\n", money);
    if (note != NULL) printf("備考: %s\n", note);

    if (!force)
    {
        char buf[32];
        printf("追加してよいですか？ [y/n] ");
        fgets(buf, sizeof(buf)/sizeof(buf[0]), stdin);
        
        if (strncmp("n", buf, 1) == 0)
        {
            printf("中断しました\n");
            exit(EXIT_FAILURE);
        }
    }

    // 追加
    append(&time, cat, loc, money, note);

    if (!silent)
    {
        int i, len, sum = 0;
        struct record *recs = load(&time, &len, 0);

        for (i = 0; i < len; ++i) sum += (recs+i)->money;

        printf("%d年%d月の合計使用金額: %'d\n", time.year, time.month, sum);
        free_records(recs, len);
    }

    free(note);
    free(loc);
    free(cat);
}

static void
modeshow_opt(int *argc, char **argv[], int *sort)
{
    int result;

    *sort = 0;
    while ((result = getopt(*argc, *argv, "s")) != -1)
    {
        switch (result)
        {
            case 's':
                *sort = 1;
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
modeshow(int argc, char *argv[])
{
    struct record *recs;
    struct record *rec_p;
    int recs_num;
    struct mydate stime;
    struct mydate mtime;
    struct mydate etime;
    int i;
    int si, mi, ei;
    int sum;
    int sort;

    if (strcmp(CMD_SHOW, argv[1])) uses(argv);
    modeshow_opt(&argc, &argv, &sort);

    if (argc == 1)
    {
        getnowdate(&stime);
        stime.mask = MASK_YEAR | MASK_MONTH;
        stime.day = 1;
        etime = stime;
        incdate(&etime, MASK_MONTH);
    }
    else
    {
        parsedate(argv[0], &stime, MASK_YEAR);
        if (argc == 2)
        {
            etime = stime;
            incdate(&etime, etime.mask);
        }
        else
        {
            parsedate(argv[1], &etime, MASK_YMD);
            if (date2int(&stime) > date2int(&etime))
            {
                fprintf(stderr, "Error: 日付の範囲指定が間違っています\n");
                exit(EXIT_FAILURE);
            }
            if (stime.mask != MASK_YMD || etime.mask != MASK_YMD)
            {
                fprintf(stderr, 
                        "Error: 日付範囲の初めと終わりを指定する場合，"
                        "日にちまで指定する必要があります\n");
                exit(EXIT_FAILURE);
            }
        }
    }

    si = date2int(&stime);
    ei = date2int(&etime);
    for (mtime = stime; date2int(&mtime) < ei; incdate(&mtime, MASK_MONTH))
    {
        recs = load(&mtime, &recs_num, 1);
        if (recs == NULL) break;
        if (sort) recordsort(recs, recs_num);
        printf("==== %sの家計簿 ====\n", date2str_w(&mtime, MASK_YEAR | MASK_MONTH));

        sum = 0;
        for (i = 0; i < recs_num; ++i)
        {
            rec_p = recs+i;
            mi = date2int(&(rec_p->date));
            if (si <= mi && mi <= ei)
            {
                sum += rec_p->money;
                printf("%s 品名: %s 場所: %s 金額: %'d 備考: %s\n",
                        date2str_w(&(rec_p->date), MASK_YMD),
                        rec_p->cat,
                        rec_p->loc,
                        rec_p->money,
                        rec_p->note);
            }
        }
        printf("\n合計使用金額: %'d\n1日の平均使用金額: %'d\n1回の平均使用金額: %'d\n\n",
                sum,
                sum / daysinmonth(mtime.year, mtime.month),
                sum / recs_num);
        free_records(recs, recs_num);
    }
}

int
main(int argc, char *argv[])
{
    setlocale(LC_NUMERIC, getenv("LANG"));
    if (argc <= 1)                           uses(argv);
    if      (strcmp(CMD_ADD,  argv[1]) == 0) modeadd(argc, argv);
    else if (strcmp(CMD_SHOW, argv[1]) == 0) modeshow(argc, argv);
    else if (strcmp(CMD_HELP, argv[1]) == 0) uses(argv);
    else                                     uses(argv);

    return EXIT_SUCCESS;
}

