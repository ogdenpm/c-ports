/****************************************************************************
 *  fatal.c: part of the C port of Intel's ISIS-II plm80c             *
 *  The original ISIS-II application is Copyright Intel                     *
 *																			*
 *  Re-engineered to C by Mark Ogden <mark.pm.ogden@btinternet.com> 	    *
 *                                                                          *
 *  It is released for hobbyist use and for academic interest			    *
 *                                                                          *
 ****************************************************************************/

#include "os.h"
#include "plm.h"

void Fatal(char const *str, byte len) {
    printf("\n\nPL/M-80 FATAL ERROR --\n\n"
           "%.*s\n"
           "\nCOMPILATION TERMINATED\n\n",
           len, str);
    Exit(1);
} /* Fatal() */
