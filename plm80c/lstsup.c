/****************************************************************************
 *  lstsup.c: part of the C port of Intel's ISIS-II plm80c             *
 *  The original ISIS-II application is Copyright Intel                     *
 *																			*
 *  Re-engineered to C by Mark Ogden <mark.pm.ogden@btinternet.com> 	    *
 *                                                                          *
 *  It is released for hobbyist use and for academic interest			    *
 *                                                                          *
 ****************************************************************************/

#include "plm.h"
#include <stdarg.h>
/* common source for lstsp[456].plm */


void NewLineLst() {
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
        tabTo -= col + 1;
    }
    while (tabTo-- != 0)
        lstc(' ');
}

void EjectNext() {
    linLft = 0;
}

void SetMarkerInfo(byte markerCol, byte marker, byte textCol) {
    wrapMarkerCol = markerCol;
    wrapMarker    = marker;
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

pstr_t const *hexfmt(byte digits, word val) {
    static char str[8];
    str[0] = sprintf(str + 1, "0%0*XH", digits, val);
    if (str[2] > '9')
        return (pstr_t *)str;
    str[1] = str[0] - 1;
    return (pstr_t *)(str + 1);

}