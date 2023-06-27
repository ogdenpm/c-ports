/****************************************************************************
 *  io.c: part of the C port of Intel's ISIS-II locate             *
 *  The original ISIS-II application is Copyright Intel                     *
 *																			*
 *  Re-engineered to C by Mark Ogden <mark.pm.ogden@btinternet.com> 	    *
 *                                                                          *
 *  It is released for hobbyist use and for academic interest			    *
 *                                                                          *
 ****************************************************************************/

// vim:ts=4:shiftwidth=4:expandtab:
// #include "loc.h"
#include <ctype.h>
#include <showVersion.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "omf.h"
#include "os.h"
#include "lst.h"

#ifdef _WIN32
#include <io.h>
#define DIRSEP "/\\"
#else
#include <errno.h>
#include <unistd.h>
#define _MAX_PATH 4096
#define DIRSEP    "/"
#endif

#define CLCHUNK 256 // size increase as command line grows

#define STDIN   0
#define STDOUT  1

char *cmdP; // current location on command line

char *invokeName; // sanitised invoking command

char const *deviceMap[10]; // mappings of :Fx: to directories

// forward ref
static char *getCmdLine(int argc, char **argv);

int main(int argc, char **argv) {
    CHK_SHOW_VERSION(argc, argv); // version info

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
    cmdP = getCmdLine(argc, argv);
    Start();
}

_Noreturn void Exit(int retCode) {
    if (lstFp)
        fclose(lstFp);

    if (retCode) {

        if (omfOutFp) {
            fclose(omfOutFp);
            unlink(omfOutName);
        }
    }
    exit(retCode);
}

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

// variant of fopen that maps :Fx: prefixes if needed
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

// print the ISIS drive mapping if any
void printDriveMap() { // show which :Fx: drive maps are  used
    bool showMsg = true;
    for (int i = 0; i < 10; i++) {
        if (deviceMap[i]) {
            if (showMsg) {
                Printf("\nISIS DRIVE MAPPING\n");
                showMsg = false;
            }
            Printf(":F%d: -> %s\n", i, deviceMap[i]);
        }
    }
}

// command line routines
static char *commandLine; // users command line
static int cmdLineLen;    // current command line length
static int cmdLineSize;   // space allocate for the command line
static char *tokenLine;   // copy of the line, modified to create C string tokens
// returns true if more appends to the command line are needed
static bool appendCmdLine(char const *s) {
    int c;
    static bool contSeen = false;
    int len              = (int)strlen(s);

    if (cmdLineLen + len + 3 > cmdLineSize) { // allow for #\n\0 for error reporting
        while (cmdLineLen + len + 3 > cmdLineSize)
            cmdLineSize += CLCHUNK;
        commandLine = xrealloc(commandLine, cmdLineSize);
    }
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
                return false; // user didn't have & at end of line so all done
            contSeen = false; // remove flag for next line
        }
    }
    return true; // not reached end of line or &\n seen
}

static char *getCmdLine(int argc, char **argv) {
    bool haveArg = argc > 1; // allows for response file if no command line args
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
            appendCmdLine("\n\n"); // finish off line even if previous had &
            break;
        }
    }
    commandLine[cmdLineLen] = '\0';
    tokenLine               = xmalloc(cmdLineLen + 1);
    memcpy(tokenLine, commandLine, cmdLineLen + 1);
    return commandLine;
}

static char *SkipSpc(char *pch) {
    while (*pch == ' ')
        pch++;
    return pch;
}

static char *skipToDelim(char *pch) {
    char *p = strpbrk(pch, " ,()&\n");
    return p ? p : strchr(pch, '\0');
}

// public command line handling functions
void SkipNonArgChars(char *pch) {
    cmdP = SkipSpc(pch);
    /* skip any continuation bits and leading space */
    while (*cmdP == '&') {
        cmdP = SkipSpc(cmdP + 5);
    }
}

void ExpectChar(uint8_t ch, char const *msg) {
    SkipNonArgChars(cmdP);
    if (ch == *cmdP)
        SkipNonArgChars(cmdP + 1);
    else
        FatalCmdLineErr(msg);
}

/* use a shadow copy of commandLine to create '\0' terminated tokens */
char *GetToken(void) {
    SkipNonArgChars(cmdP);
    char *token                   = tokenLine + (cmdP - commandLine);
    cmdP                          = skipToDelim(cmdP);
    tokenLine[cmdP - commandLine] = '\0';
    return token;
}

uint16_t ParseNumber(void) {
    char const *pch;
    uint8_t radix, digit;
    uint32_t num = 0;
    char *token  = GetToken();

    for (pch = token; isxdigit(*pch); pch++)
        ;
    char suffix = toupper(*pch);
    if (suffix == 'H')
        radix = 16;
    else if (suffix == 'O' || suffix == 'Q')
        radix = 8;
    else if (*pch != '\0')
        num = 0x10000;
    else if (toupper(pch[-1]) == 'B' || toupper(pch[-1]) == 'D') {
        if (--pch == token)
            num = 0x10000;
        else
            radix = toupper(pch[-1]) == 'B' ? 2 : 10;
    } else
        radix = 10;

    for (; num < 0x10000 && token < pch; token++) {
        digit = isdigit(*token) ? *token - '0' : toupper(*token) - 'A' + 10;
        if (digit >= radix)
            num = 0x10000;
        else
            num = num * radix + digit;
    }
    if (num >= 0x10000)
        FatalCmdLineErr("Invalid number");
    return (uint16_t)num;
}


// print the command line, splitting long lines
void printCmdLine(FILE *fp) {
    int col = 0;
    char *s = commandLine;
    char *brk;
    while ((brk = strpbrk(s, ", &\n"))) {
        brk++; // include the break char
        if (col + (brk - s) >= 120) {
            fputs("\\\n    ", fp);
            col = 4;
        }
        fprintf(fp, "%.*s", (int)(brk - s), s);
        if (brk[-1] != '\n')
            col += (int)(brk - s);
        else
            col = 0;
        s = brk;
    }
}

// common error handlers
_Noreturn void FatalCmdLineErr(char const *errMsg) {
    if (*cmdP != '\n') /* don't skip past the EOL */
        cmdP++;
    strcpy((char *)cmdP, "#\n"); // mark problem (remove 'const' from cmdP)
    fprintf(stderr, "Command line error near #: %s\n", errMsg);
    printCmdLine(stderr);
    Exit(1);
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

_Noreturn void IoError(char const *path, char const *msg) {
    fprintf(stderr, "%s: %s: %s\n", path, msg, strerror(errno));

    char name[_MAX_PATH + 1];
    MapFile(name, path);
    if (strcmp(name, path))
        fprintf(stderr, "Mapped file is: %s\n", name);

    Exit(1);
}


// safe memory allocation
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

char *xstrdup(char const *str) {
    char *s = strdup(str);
    if (!s)
        FatalError("Out of memory");
    return s;
}