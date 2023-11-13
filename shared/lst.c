/****************************************************************************
 *  lst.c: part of the C port of Intel's ISIS-II 8085 toolchain             *
 *  The original ISIS-II application is Copyright Intel                     *
 *                                                                          *
 *  Re-engineered to C by Mark Ogden <mark.pm.ogden@btinternet.com>         *
 *                                                                          *
 *  It is released for academic interest and personal use only              *
 ****************************************************************************/
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#ifdef _MSC_VER
#include <io.h>
#else
#include <unistd.h>
#endif

#include "../shared/lst.h"
#include "../shared/os.h"
#include "../shared/cmdline.h"

FILE *lstFp;
char *lstName;
static bool logToStderr;
/* and the print file (or console) */

void openLst(char const *signon) {
    if (lstName && *lstName) {
        if (!(lstFp = Fopen(lstName, "wt")))
            IoError(lstName, "Create error");
        Printf("%s INVOKED BY:\n", signon);
        printCmdLine(lstFp, 120, 0);
    } else {
        lstName = "stdout";
        lstFp   = stdout;
    }
    logToStderr = !isatty(fileno(lstFp));
}

void closeLst(void) {
    if (lstFp && lstFp != stdout && fclose(lstFp))
        IoError(lstName, "Close error");
}

void Putc(int c) {
    if (fputc(c, lstFp) < 0)
        IoError(lstName, "Write error");
}

int vPrintf(char const *fmt, va_list args) {
    int cnt = vfprintf(lstFp, fmt, args);
    if (cnt < 0)
        IoError(lstName, "Write error");
    return cnt;
}

int Printf(char const *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    int cnt = vPrintf(fmt, args);
    va_end(args);
    return cnt;
}

int PrintfAndLog(char const *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    int cnt = vPrintf(fmt, args);
    va_end(args);

    if (logToStderr) {
        va_start(args, fmt);
        vfprintf(stderr, fmt, args);
        va_end(args);
    }
    return cnt;
}