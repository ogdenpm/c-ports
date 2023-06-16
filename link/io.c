/****************************************************************************
 *  io.c: part of the C port of Intel's ISIS-II link             *
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

#ifdef _WIN32
#include <io.h>
#define DIRSEP "/\\"
#else
#include <errno.h>
#include <unistd.h>
#define _MAX_PATH 4096
#define DIRSEP    "/"
#endif

#include "link.h"

#define STDIN  0
#define STDOUT 1
char *commandLine;      // users command line
char *tokenLine;        // copy of the line, modified to create C string tokens
int cmdLineLen;         // current command line length
int cmdLineSize;        // space allocate for the command line
char const *invokeName; // sanitised invoking command
bool warnOK = true;     // if false then warnings are teated as errors
#define CLCHUNK 256     // size increase as command line grows

static void getCmdLine(int argc, char **argv);

char *deviceMap[10];

char *basename(char *path) {
    char *s;
#ifdef _WIN32
    if (path[0] && path[1] == ':') // skip leading device
        path += 2;
#endif
    while ((s = strpbrk(path, DIRSEP))) // skip all directory components
        path = s + 1;
    return path;
}
/*
    map a filename of the form [:Fx:]path to host OS format
*/
static char *MapFile(char *osName, const char *isisPath) {
    char *s;
    static char dev[5] = { ":Fx:" };

    if (isisPath[0] == ':' && toupper(isisPath[1]) == 'F' && isdigit(isisPath[2]) &&
        isisPath[3] == ':') {
        int i  = isisPath[2] - '0';
        dev[2] = isisPath[2];
        if (!deviceMap[i] && !(deviceMap[i] = getenv(dev)))
            deviceMap[i] = ".";                                          // give a minimal default
        if (strlen(deviceMap[i]) + strlen(isisPath + 4) + 1 > _MAX_PATH) // will it fit
            FatalError("Mapped path name too long:\n %s", isisPath);

        strcpy(osName, deviceMap[i]);
        s = strchr(osName, '\0'); // append point
        if (s != osName && !strchr(DIRSEP, s[-1]) && !strchr(DIRSEP, isisPath[3]))
            strcpy(s, "/"); // make sure there is a directory separator
        strcat(s, isisPath + 4);
    } else if (strlen(isisPath) > _MAX_PATH) {
        FatalError("Path name too long:\n %s", isisPath);
    } else
        strcpy(osName, isisPath);
    return osName;
}

void printDriveMap() {  // show which :Fx: drive maps are  used
    bool showMsg = true;
    for (int i = 0; i < 10; i++) {
        if (deviceMap[i]) {
            if (showMsg) {
                fputs("\nISIS DRIVE MAPPING\n", lstFp);
                showMsg = false;
            }
            fprintf(lstFp, ":F%d: -> %s\n", i, deviceMap[i]);
        }
    }
}

int main(int argc, char **argv) {

    CHK_SHOW_VERSION(argc, argv);   // version info

    invokeName = basename(argv[0]); // remove the directory part
#ifdef _WIN32
    char *s;
    // remove .exe under windows
    if ((s = strrchr(invokeName, '.')) && strcasecmp(s, ".exe") == 0)
        *s = '\0';
#endif
    // check for help request
    if (argc == 2 && strcmp(argv[1], "-h") == 0)
        usage();
    getCmdLine(argc, argv);

    Start();
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

// support functions to read / write little eadian numbers
word putWord(pointer buf, word val) {
    buf[0] = val & 0xff;
    buf[1] = val / 256;
    return val;
}

word getWord(pointer buf) {
    return buf[0] + buf[1] * 256;
}

_Noreturn void FatalError(char const *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    fputs("Fatal Error: ", stderr);
    vfprintf(stderr, fmt, args);
    va_end(args);
    fputc('\n', stderr);
    Exit(1);
}

// returns true if more appends to the command line are needed
static bool appendCmdLine(char const *s) {
    int c;
    static bool contSeen = false;

    if (cmdLineLen + strlen(s) + 3 > cmdLineSize) // allow for #\n\0 for error reporting
        commandLine = xrealloc(commandLine, cmdLineSize += CLCHUNK);
    while ((c = *s++)) {
        if (c != '\n') {
            if (contSeen && c > ' ')
                FatalCmdLineErr("Non white space after &");
            else if (c == '&')
                contSeen = true;
            commandLine[cmdLineLen++] = c >= ' ' ? c : ' ';
        } else {
            while (commandLine[cmdLineLen - 1] == ' ') // trim
                cmdLineLen--;
            commandLine[cmdLineLen++] = '\n';
            if (!contSeen)
                return false;   // user didn't have & at end of line so all done
            contSeen = false;   // remove flag for next line
        }
    }
    return true;    // not reached end of line or &\n seen
}

static void getCmdLine(int argc, char **argv) {
    bool haveArg = argc > 1;   // allows for response file if no command line args
    appendCmdLine("-");
    appendCmdLine(invokeName);
    for (int i = 1; i < argc; i++) {
        appendCmdLine(" ");
        appendCmdLine(argv[i]);
    }
    char line[256];
    line[0] = '\n'; // terminate the invocation line
    line[1] = '\0';
    while (appendCmdLine(line) || !haveArg) {
        haveArg = true;
        /* issue the prompt if not redirected to a file */
        if (strchr(line, '\n') && isatty(STDIN) && isatty(STDOUT))
            fputs("**", stdout);
        appendCmdLine("**");
        if (!fgets(line, 256, stdin)) {
            fprintf(stderr, "Unexpected EOF on command line\n");
            warned = 1;
            appendCmdLine("\n\n"); // finish off line even if previous had &
            break;
        }
    }
    commandLine[cmdLineLen] = '\0';
    tokenLine               = xmalloc(cmdLineLen + 1);
    memcpy(tokenLine, commandLine, cmdLineLen + 1);
}

_Noreturn void IoError(char const *path, char const *msg) {
    fprintf(stderr, "%s: %s: %s", path, msg, strerror(errno));
    Exit(1);
}

/* safe malloc & realloc */
void *xmalloc(size_t size) {
    void *p = malloc(size);
    if (!p)
        FatalError("Out of memory");
    return p;
}

void *xrealloc(void *p, size_t size) {
    void *q = realloc(p, size);
    if (!q)
        FatalError("Out of memory");
    return q;
}
