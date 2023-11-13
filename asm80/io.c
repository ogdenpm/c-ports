/****************************************************************************
 *  io.c: part of the C port of Intel's ISIS-II asm80             *
 *  The original ISIS-II application is Copyright Intel                     *
 *																			*
 *  Re-engineered to C by Mark Ogden <mark.pm.ogden@btinternet.com> 	    *
 *                                                                          *
 *  It is released for hobbyist use and for academic interest			    *
 *                                                                          *
 ****************************************************************************/

// vim:ts=4:shiftwidth=4:expandtab:
#include <ctype.h>
#include <fcntl.h>
#include <showVersion.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>

#ifdef _WIN32
#include <io.h>
#else
#include <errno.h>
#include <unistd.h>
#define _MAX_PATH 4096
#define O_BINARY  0
#define _stricmp  strcasecmp
#endif

#include "asm80.h"

int _argc;
char **_argv;
bool useLC       = true;
bool killObjFile = true;
char dateStr[22] = { 0 }; // [yyyy-mm-dd hh:mm]

char *deviceMap[10];
#ifdef _WIN32
#define DIRSEP "/\\"
#else
#define DIRSEP "/"
#endif

char *basename(char *path) {
    char *s;
#ifdef _WIN32
    if (path[0] && path[1] == ':')
        path += 2;
#endif
    while ((s = strpbrk(path, DIRSEP)))
        path = s + 1;
    return path;
}

static char *MapFile(char *osName, const char *isisPath) {
    char *s;
    char dev[] = { "ISIS_Fx" };

    if (isisPath[0] == ':' && toupper(isisPath[1]) == 'F' && isdigit(isisPath[2]) &&
        isisPath[3] == ':') {
        int i  = isisPath[2] - '0';
        dev[6] = isisPath[2];
        if (!deviceMap[i] && !(deviceMap[i] = getenv(dev)))
            deviceMap[i] = "";
        if (strlen(deviceMap[i]) + strlen(isisPath + 4) + 1 > _MAX_PATH)
            FatalError("Mapped path name too long:\n %s", isisPath);

        strcpy(osName, deviceMap[i]);
        s = strchr(osName, '\0');
        if (s != osName && !strchr(DIRSEP, s[-1]) && !strchr(DIRSEP, isisPath[3]))
            strcpy(s, "/");
        strcat(s, isisPath + 4);
    } else if (strlen(isisPath) > _MAX_PATH) {
        FatalError("Path name too long:\n %s", isisPath);
    } else
        strcpy(osName, isisPath);
    return osName;
}

void wrapUp(void) { // called on exit
    if (lstFp)      // close listing file
        fclose(lstFp);
    if (objFp) {
        fclose(objFp);
        if (errCnt) // remove object file unless successfully written
            unlink(objFile);
    }
}

int main(int argc, char **argv) {
    size_t len;
    char *s;

    CHK_SHOW_VERSION(argc, argv);

    _argc = argc; // used in reporting the command line
    _argv = argv;

#ifdef _WIN32
    if ((s = strrchr(argv[0], '.')) && strcasecmp(s, ".exe") == 0)
        *s = '\0';
#endif
    if (argc == 1) {
        fputs("No source file!!\n", stderr);
        exit(1);
    }
    /* check if source filename part is all upper case
       if it is assume extents will be upper case as well
    */
    for (s = argv[1]; *s && !islower(*s); s++)
        if (isupper(*s))
            useLC = false;
    if (*s)
        useLC = true;

    len = 0;
    for (int i = 2; i < argc; i++) // add args lengths with space between
        len += strlen(argv[i]) + 1;

    s = cmdchP = cmdLineBuf = xmalloc(len + 3);

    *s                      = 0;
    for (int i = 2; i < argc; i++) { // add args with space between
        strcat(s, " ");
        strcat(s, argv[i]);
    }
    strcat(s, "\n");
    atexit(wrapUp);

    time_t now;
    time(&now);
    strftime(dateStr, sizeof(dateStr), " [%F %R]", localtime(&now));

    Start(argv[1]);
}

FILE *Fopen(char const *pathP, char *access) {
    char name[_MAX_PATH + 1];

    MapFile(name, pathP);
#ifdef _WIN32
    /* if a file exists and is opened for write in windows
       the filename case remains as per the original. Although
       this doesn't impact execution on windows it looks odd.
       so make sure the file is removed before opening
    */
    if (*access == 'w' && unlink(name))
        errno = 0;
#endif
    return fopen(name, access);
}

_Noreturn void IoError(char const *path, char const *msg) {
    fprintf(stderr, "%s: %s: %s", path, msg, strerror(errno));
    exit(1);
}

_Noreturn void FatalError(char const *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    fputs("Fatal Error: ", stderr);
    vfprintf(stderr, fmt, args);
    va_end(args);
    fputc('\n', stderr);
    exit(1);
}

void Warn(char const *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    fputs("Warning: ", stderr);
    vfprintf(stderr, fmt, args);
    va_end(args);
    fputc('\n', stderr);
}