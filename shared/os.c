/****************************************************************************
 *  os.c: part of the C port of Intel's ISIS-II 8085 toolchain              *
 *  The original ISIS-II application is Copyright Intel                     *
 *                                                                          *
 *  Re-engineered to C by Mark Ogden <mark.pm.ogden@btinternet.com>         *
 *                                                                          *
 *  It is released for academic interest and personal use only              *
 ****************************************************************************/
#include "../shared/os.h"
#include "../shared/cmdline.h"
#include <ctype.h>
#include <errno.h>
#include <showVersion.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#ifdef _WIN32
#include <io.h>
#include <direct.h>
#define mkdir(dir, access) _mkdir(dir)
#else
#include <errno.h>
#include <unistd.h>
#include <sys/stat.h>
extern int fcloseall(void);
#endif

#define CLCHUNK 256 // size increase as command line grows

#define STDIN   0
#define STDOUT  1


char const *invokeName; // sanitised invoking command
bool ttyin;
bool ttyout;

char const *deviceMap[10]; // mappings of :Fx: to directories

// interactive trap handler
static void (*iTrap)(int retCode);
static char const *rmOnError;

void SetITrap(void (*f)(int retCode)) {
    iTrap = f;
}
// auto remove file on error
void RmOnError(char const *fname) {
    rmOnError = fname;
}

int main(int argc, char **argv) {
    CHK_SHOW_VERSION(argc, argv); // version info

    invokeName = basename(argv[0]); // remove the directory part
#ifdef _WIN32
    char *s;
    // remove .exe under windows
    if ((s = strrchr(invokeName, '.')) && stricmp(s, ".exe") == 0)
        *s = '\0';
#endif
    // check for help request
    if (argc == 2 && strcmp(argv[1], "-h") == 0) {
        usage();
        exit(0);
    }
    ttyin  = isatty(STDIN);
    ttyout = ttyin && isatty(STDOUT);
    cmdP   = getCmdLine(argc, argv);
    Start();
}

_Noreturn void Exit(int retCode) {
    if (fcloseall() < 0)
        fprintf(stderr, "Warning: Problem closing open files\n");
    // delete any flagged file on error
    if (retCode && rmOnError)
        unlink(rmOnError);
    if (iTrap) {
        rmOnError = NULL;
        iTrap(retCode);
    }
    exit(retCode);
}

bool hasIsisDrive(char const *path) {
    return path[0] == ':' && toupper(path[1]) == 'F' && isdigit(path[2]) && path[3] == ':';
}

char const *basename(char const *path) {
    char *s;
    if (hasIsisDrive(path))
        path += 4;
#ifdef _WIN32
    else if (path[0] && path[1] == ':') // skip leading device
        path += 2;
#endif
    while ((s = strpbrk(path, DIRSEP))) // skip all directory components
        path = s + 1;
    return path;
}

/*
    map a filename of the form [:Fx:]path to host OS format
*/
char *MapFile(char *osName, const char *isisPath) {
    char *s;
    static char dev[] = { "ISIS_Fx" };

    if (hasIsisDrive(isisPath)) {
        int i  = isisPath[2] - '0';
        dev[6] = isisPath[2];
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

void mkpath(char *file) {
    char dir[_MAX_PATH + 1];
    char *s = dir;
#ifdef _WIN32
    if (file[0] && file[1] == ':') { // skip drive
        *s++ = *file++;
        *s++ = *file++;
    }
#endif
    if (*file)
        *s++ = *file++; // avoid dir == ""
    while (1) {
        while (*file && !strchr(DIRSEP, *file))
            *s++ = *file++;
        if (*file == 0)
            return;
        *s = 0;
        if (access(dir, 0) < 0 && mkdir(dir, 0777) != 0 && errno != EEXIST)
            IoError(dir, "Cannot create directory", dir);
        *s++ = *file++;
    }
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
    return strcpy(xmalloc(strlen(str) + 1), str);
}