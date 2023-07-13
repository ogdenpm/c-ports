/****************************************************************************
 *  io.c: part of the C port of Intel's ISIS-II locate             *
 *  The original ISIS-II application is Copyright Intel                     *
 *																			*
 *  Re-engineered to C by Mark Ogden <mark.pm.ogden@btinternet.com> 	    *
 *                                                                          *
 *  It is released for hobbyist use and for academic interest			    *
 *                                                                          *
 ****************************************************************************/

#include "os.h"
#include <ctype.h>
#include <errno.h>
#include <showVersion.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <io.h>
#define DIRSEP "/\\"
#else
#include <errno.h>
#include <unistd.h>
extern int fcloseall(void);
#define _MAX_PATH 4096
#define DIRSEP    "/"
#endif

#define CLCHUNK 256 // size increase as command line grows

#define STDIN   0
#define STDOUT  1

char *cmdP; // current location on command line

char *invokeName; // sanitised invoking command
bool prompt;

char const *deviceMap[10]; // mappings of :Fx: to directories
char *endToken;            // used in error reporting


// interactive trap handler
static void (*itrap)(void);

void SetITrap(void (*f)(void)) {
    itrap = f;
}


// cleanup registration
#define MAXCLEAN 5
void (*cleanup[MAXCLEAN])(void);

void RegCleanup(void (*f)(void)) {
    for (int i = 0; i < MAXCLEAN; i++)
        if (cleanup[i] == f)    // already registered
            return;
    for (int i = 0; i < MAXCLEAN; i++)
        if (cleanup[i] == NULL) {
            cleanup[i] = f;
            return;
        }
    fprintf(stderr, "System error: Too many cleanups registered\n");
    exit(1);
}

void DeregCleanup(void (*f)(void)) {
    for (int i = 0; i < MAXCLEAN; i++)
        if (cleanup[i] == f) {
            cleanup[i] = NULL;
            return;
        }
}


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
    if (argc == 2 && strcmp(argv[1], "-h") == 0) {
        usage();
        exit(0);
    }
    prompt = isatty(STDIN) && isatty(STDOUT);
    cmdP   = getCmdLine(argc, argv);
    Start();
}

_Noreturn void Exit(int retCode) {
    if (fcloseall() < 0)
        fprintf(stderr, "Warning: Problem closing open files\n");
    if (retCode) {
        for (int i = 0; i < MAXCLEAN; i++) {
            if (cleanup[i]) {
                void (*tmp)(void) = cleanup[i];
                cleanup[i]        = NULL; // cleanups are single shot to avoid recursion
                tmp();
            }
        }
        if (itrap)
            itrap();
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

int Delete(char const *ipath) {
    char name[_MAX_PATH + 1];
    return unlink(MapFile(name, ipath));
}

int Rename(char const *iold, char const *inew) {
    char oname[_MAX_PATH + 1];
    char nname[_MAX_PATH + 1];
    return rename(MapFile(oname, iold), MapFile(nname, inew));
}

int Access(char const *ipath, int mode) {
    char name[_MAX_PATH + 1];
    return access(MapFile(name, ipath), mode);
}
// print the ISIS drive mapping if any
void printDriveMap(FILE *fp) { // show which :Fx: drive maps are  used
    bool showMsg = true;
    for (int i = 0; i < 10; i++) {
        if (deviceMap[i]) {
            if (showMsg) {
                fputs("\nISIS DRIVE MAPPING\n", fp);
                showMsg = false;
            }
            fprintf(fp, ":F%d: -> %s\n", i, deviceMap[i]);
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
    static bool ampersandSeen = false;
    int len                   = (int)strlen(s);

    if (cmdLineLen == 0)
        ampersandSeen = false;
    if (cmdLineLen + len + 3 > cmdLineSize) { // allow for #\n\0 for error reporting
        while (cmdLineLen + len + 3 > cmdLineSize)
            cmdLineSize += CLCHUNK;
        commandLine = xrealloc(commandLine, cmdLineSize);
    }
    while ((c = *s++)) {
        if (c != '\n') {
            if (c > ' ')
                ampersandSeen = false;
            else if (c == '&')
                ampersandSeen = true;
            commandLine[cmdLineLen++] = c >= ' ' ? c : ' ';
        } else {
            while (commandLine[cmdLineLen - 1] == ' ') // trim
                cmdLineLen--;

            if (!ampersandSeen) {
                commandLine[cmdLineLen++] = '\n';
                return false; // user didn't have & at end of line so all done
            }
            commandLine[cmdLineLen++] = '\r';
            ampersandSeen             = false; // remove flag for next line
        }
    }
    return true; // not reached end of line or &\n seen
}
// get the command line and a shadow copy for token constructio
//    if argc == 0 get line in interactive mode, argv ignored
char *getCmdLine(int argc, char **argv) {
    char line[256];
    cmdLineLen = 0;
    free(tokenLine);
    if (argc == 0) {
        if (prompt)
            putchar('*');
        strcpy(line, "*");
    } else {
        appendCmdLine(argc > 1 ? "-" : "*"); // flag whether actual cmd line params
        appendCmdLine(invokeName);
        bool useComma = false;
        char *sep     = " ";
        for (int i = 1; i < argc; i++) {
            if (strcmp(argv[i], "!") == 0) {
                if (!(useComma = !useComma))
                    sep = " ";
            } else {
                appendCmdLine(sep);
                appendCmdLine(argv[i]);
                if (useComma)
                    sep = ",";
            }
        }
        strcpy(line, argc > 0 ? "\n" : "&\n");
    }
    while (appendCmdLine(line)) {
        /* new a new line */
        if (strchr(line, '\n') && prompt)
            fputs("**", stdout);
        if (!fgets(line, 256, stdin)) {
            if (cmdLineLen > 1)
                appendCmdLine("\n\n"); // finish off line even if previous had &
            break;
        }
    }
    commandLine[cmdLineLen] = '\0';
    tokenLine               = xmalloc(cmdLineLen + 1);
    memcpy(tokenLine, commandLine, cmdLineLen + 1);
    return commandLine;
}

// public command line handling functions
void SkipWs() {
    while (*cmdP == ' ' || *cmdP == '\r')
        cmdP++;
}

void ExpectChar(uint8_t ch, char const *msg) {
    SkipWs();
    if (ch == *cmdP) {
        endToken = ++cmdP;
        SkipWs();
    } else
        FatalCmdLineErr(msg);
}

static char *ScanToken(char const *delims) {
    SkipWs();
    if (*cmdP == '\'') { // quoted token
        cmdP++;
        delims = "'\n\r";
    }
    char *token = tokenLine + (cmdP - commandLine);
    char *p     = strpbrk(cmdP, delims);
    if (!p)
        p = strchr(cmdP, '\0');
    if (*p != *delims) {
        if (*delims == '\'')
            FatalCmdLineErr("Missing terminating quote");
        else if (*delims == ')')
            FatalCmdLineErr("Missing terminating )");
    }
    if (*p == '\'')
        cmdP = p + 1;
    else if (*delims != '\'') { // trim unless started with '
        while (p > cmdP && p[-1] == ' ')
            p--;
        cmdP = p;
    }
    tokenLine[p - commandLine] = '\0';
    endToken                   = p;
    SkipWs();
    return token;
}

void SetEndToken(char *p) {
    endToken = p;
}

/* use a shadow copy of commandLine to create '\0' terminated tokens */
char *GetToken(void) {
    return ScanToken(" ,()\n\r");
}

char *GetText(void) {
    return ScanToken(")\n\r");
}

uint16_t GetNumber(void) {
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
    while (*s) {
        char *brk = strpbrk(s, ", '\n\r");
        if (*brk == '\'')
            brk = strpbrk(brk + 1, "'\n\r"); // for quoted token include it
        if (!brk)
            brk = strchr(s, '\0'); // if no break do whole string
        if (col + (brk + 1 - s) >= 120) {
            fputs("\\\n    ", fp);
            col = 4;
        }
        col += fprintf(fp, "%.*s", (int)(brk - s), s);
        if (!*brk || *brk == '\n')
            break;
        if (*brk == '\r') {
            fputs("&\n**", fp);
            col = 2;
        } else
            fputc(*brk, fp);
        s = brk + 1;
    }
    if (col)
        fputc('\n', fp);
}

// common error handlers
_Noreturn void FatalCmdLineErr(char const *errMsg) {
    strcpy(endToken, "#\n");
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

_Noreturn void IoError(char const *path, char const *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    fprintf(stderr, "%s: ", path);
    vfprintf(stderr, fmt, args);
    va_end(args);
    if (errno)
        fprintf(stderr, ": %s", errno == EINVAL ? "Invalid file name" : strerror(errno));
    fputc('\n', stderr);
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