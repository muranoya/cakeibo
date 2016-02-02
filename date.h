#ifndef DATE_H
#define DATE_H

#define MASK_YEAR  (0x20U)
#define MASK_MONTH (0x10U)
#define MASK_DAY   (0x08U)
#define MASK_EMPTY (0x00U)
#define MASK_YMD (MASK_YEAR | MASK_MONTH | MASK_DAY)
#define MASK_YM  (MASK_YEAR | MASK_MONTH)

struct mydate
{
    uint32_t mask;
    int year;
    int month;
    int day;
};

/*
 * その年が閏年か判定する
 */
int  isleapyear(int year);
/*
 * その月が何日あるか返す
 * 月は1以上12以下の範囲で指定する
 * 計算できない場合，-1を返す
 */
int  daysinmonth(int y, int m);
/*
 * 指定した年月日を，指定した部分でインクリメントする
 */
void incdate(struct mydate *t, uint32_t mask);
/*
 * 年月日を整数値に変換する
 * 変換後の整数値は，単純に不等号で比較できる
 */
int  date2int(const struct mydate *t);
/*
 * 現在の年月日を取得する
 */
void getnowdate(struct mydate *t);
/*
 * 年月日を文字列に変換する
 */
char *date2str(const struct mydate *t);
/*
 * 年月日を人間が読みやすい形式に変換する
 */
char *date2str_w(const struct mydate *t, uint32_t m);
/*
 * 年月日の文字列を解釈してmydate構造体に変換する
 */
int  parsedate(const char *str, struct mydate *t, uint32_t m);

#endif
