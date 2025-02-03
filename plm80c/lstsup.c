/****************************************************************************
 *  lstsup.c: part of the C port of Intel's ISIS-II plm80                   *
 *  The original ISIS-II application is Copyright Intel                     *
 *                                                                          *
 *  Re-engineered to C by Mark Ogden <mark.pm.ogden@btinternet.com>         *
 *                                                                          *
 *  It is released for academic interest and personal use only              *
 ****************************************************************************/
#include "plm.h"
#include <stdarg.h>
/* common source for lstsp[456].plm */

void NewLineLst(void) {
    if (col == 0 && linLft == 0)
        NewPgl();
    WrLstC('\n');
    linLft--;
    col = 0;
}

void TabLst(byte tabTo) {
    if (tabTo > 127) {
        tabTo = -tabTo;
        if (col >= tabTo)
            NewLineLst();
        tabTo = tabTo - col - 1;
    }
    while (tabTo-- != 0)
        lstc(' ');
}

void DotLst(byte tabTo) {
    while (col < tabTo)
        lstc(col & 1 ? '.' : ' ');
}

void EjectNext() {
    linLft = 0;
}

void SetMarkerInfo(byte markerCol, byte textCol) {
    wrapMarkerCol = markerCol;
    wrapTextCol   = textCol;
}

void SetStartAndTabW(byte startCol, byte width) {
    margin = startCol - 1;
    tWidth = width;
}

void SetSkipLst(byte cnt) {
    skipCnt = cnt;
}

void lstStr(char const *str) {
    while (*str)
        lstc(*str++);
}

void lstStrCh(char const *str, int endch) {
    while (*str && *str != endch)
        lstc(*str++);
}

void lprintf(char const *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    char tmp[1024];
    vsprintf(tmp, fmt, args);
    lstStr(tmp);
    va_end(args);
}

void lstPstr(pstr_t *ps) {
    lprintf("%.*s", ps->len, ps->str);
}

char const *hexfmt(byte digits, word val) {
    static char str[7] = "0";
    if (digits > 4)
        digits = 4;
    sprintf(str + 1, "%0*XH", digits, val);
    return str[1] <= '9' ? str + 1 : str;
}
