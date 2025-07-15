/****************************************************************************
 *  fatal.c: part of the C port of Intel's ISIS-II plm80                    *
 *  The original ISIS-II application is Copyright Intel                     *
 *                                                                          *
 *  Re-engineered to C by Mark Ogden <mark.pm.ogden@btinternet.com>         *
 *                                                                          *
 *  It is released for academic interest and personal use only              *
 ****************************************************************************/
#include "os.h"
#include "plm.h"

void Fatal(char const *str) {
    printf("\n\nPL/M-80 FATAL ERROR --\n\n"
           "%s\n"
           "\nCOMPILATION TERMINATED\n\n",
           str);
    Exit(1);
} /* Fatal() */



