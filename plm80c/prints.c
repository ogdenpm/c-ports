/****************************************************************************
 *  prints.c: part of the C port of Intel's ISIS-II plm80                   *
 *  The original ISIS-II application is Copyright Intel                     *
 *                                                                          *
 *  Re-engineered to C by Mark Ogden <mark.pm.ogden@btinternet.com>         *
 *                                                                          *
 *  It is released for academic interest and personal use only              *
 ****************************************************************************/
#include "plm.h"

void PrintStr(char const *str, byte len) {
    printf("%.*s", len, str);
} /* PrintStr() */


void lstc(byte ch) {
    if (col == 0) {
        if (linLft == 0 || linLft <= skipCnt)
            NewPgl();
        else
            while (skipCnt != 0) {
                WrLstC('\n');
                linLft--;
                skipCnt--;
            }
        col = 0;
    } else if (col >= PWIDTH)
        NlLead();

    if (ch == '\t') {
        if (col < margin)
            return;
        byte i = tWidth - (col - margin) % tWidth;
        if (col + i >= PWIDTH) {
            NlLead();
            return;
        }
        while (i-- != 0) {
            WrLstC(' ');
            col++;
        }
    } else {
        WrLstC(ch);
        if (ch == '\n') {
            linLft--;
            col = 0;
        } else
            col++;
    }
}


void WrLstC(char ch) {
    fputc(ch, lstFile.fp);
}
