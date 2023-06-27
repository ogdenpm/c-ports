#include <stdarg.h>
#include <stdio.h>
#include <stdbool.h>
#ifdef _MSC_VER
#include <io.h>
#else
#include <unistd.h>
#endif

#include "lst.h"
#include "os.h"

FILE *lstFp;
char *lstName;
static bool logToStderr;
/* and the print file (or console) */

void openLst(char const *signon) {
    if (lstName && *lstName) {
        if (!(lstFp = Fopen(lstName, "wt")))
            IoError(lstName, "Create error");
        Printf("%s INVOKED BY:\n", signon);
        printCmdLine(lstFp);
    } else {
        lstName = "stdout";
        lstFp   = stdout;
    }
    logToStderr = !isatty(fileno(lstFp));
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
