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
#include <sys/stat.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <showVersion.h>


#ifdef _WIN32
#include <io.h>
#define close   _close
#define open    _open
#define read    _read
#define write   _write
#define unlink  _unlink
#define lseek   _lseek
#define tell    _tell
#else
#include <unistd.h>
#include <errno.h>
#define _MAX_PATH 4096
#define O_BINARY    0
#define _stricmp    strcasecmp
#endif

#include "asm80.h"

#define RANDOM_ACCESS	4	// additional access mode

typedef struct {
    byte	deviceId; // isis device Id
    byte	modes;	  // supported modes READ_MODE, WRITE_MODE, UPDATE_MODE and RANDOM_ACCESS
    char	name[_MAX_PATH];
} osfile_t;

#define STDIN	0
#define STDOUT	1

#define CO_DEV	0
#define CI_DEV	1
#define BB_DEV	2		// always provide BB it is harmless and simplifies things

#define AFTSIZE	20		// more files than usually available to ISIS

static struct {
    int	fd;
    int mode;
} aft[AFTSIZE] = { { 1 , WRITE_MODE }, {0, READ_MODE}, {0, UPDATE_MODE} };	// CO, CI, BB

pointer MEMORY;
#define AVAILMEM	0x9000

char *deviceMap[10];
#ifdef _WIN32
#define DIRSEP "/\\"
#else
#define DIRSEP "/"
#endif

static char *baseName(char *path) {
    char *s;
#ifdef _WIN32
    if (path[0] && path[1] == ':')
        path += 2;
#endif
    while (s = strpbrk(path, DIRSEP))
        path = s + 1;
    return path;
}

static char *MapFile(char *osName, const char *isisPath) {
    char *s;
    char dev[5] = { ":Fx:" };

    if (isisPath[0] == ':' && toupper(isisPath[1]) == 'F' && isdigit(isisPath[2]) &&
        isisPath[3] == ':') {
        int i  = isisPath[2] - '0';
        dev[2] = isisPath[2];
        if (!deviceMap[i] && !(deviceMap[i] = getenv(dev)))
            deviceMap[i] = "";
        strcpy(osName, deviceMap[i]);
        s = strchr(osName, '\0');
        if (s != osName && !strchr(DIRSEP, s[-1]) && !strchr(DIRSEP, isisPath[3]))
            strcpy(s, "/");
        strcat(s, isisPath + 4);
    } else
        strcpy(osName, isisPath);
    // path is unchanged  but file name mapped to lowercase
    for (s = baseName(osName); *s; s++)
        *s = tolower(*s);
    return osName;
}

void wrapUp(void) { // called on exit
    if (lstFp)      // close listing file
        fclose(lstFp);
    if (objFp) {
        fclose(objFp);
        if (errCnt) // remove object file if errors
            unlink(objFile);
    }
}



int main(int argc, char **argv)
{
    size_t len;
    char *s, *progname;

    CHK_SHOW_VERSION(argc, argv);

    /* find program name */
    progname = baseName(argv[0]);

    /* only allow alpha numeric char names otherwise it may confuse isis program */
    /* .exe is also excluded*/
    for (len = 0; isalnum(progname[len]) && len < 6; len++)
        ;
       
    for (int i = 1; i < argc; i++)	// add args lengths with space between
            len += strlen(argv[i]) + 1;

    s = cmdchP = cmdLineBuf = malloc(len + 3);
    
    for (int i = 0; isalnum(progname[i]) && i < 6; i++)
            *s++ = progname[i];
    *s = 0;
    for (int i = 1; i < argc; i++)  { // add args with space between
        strcat(s, " ");
        strcat(s, argv[i]);
    }
    strcat(s, "\r\n");

    MEMORY = (pointer)malloc(AVAILMEM);
    atexit(wrapUp);
    Start();
}

pointer MemCk(void) {
    return MEMORY + AVAILMEM - 1;	// address of last isis user memory
}



FILE *Fopen(char const *pathP, char *access)
{
    char name[_MAX_PATH];

    return fopen(MapFile(name, pathP), access);
}


_Noreturn void IoError(char const *path, char const *msg) {
    fprintf(stderr, "%s: %s: %s\n", path, msg, _sys_errlist[errno]);
    errCnt++;
    exit(1);
}

_Noreturn void FatalError(char const *msg) {
    fputs(msg, stderr);
    fputc('\n', stderr);
    errCnt++;
    exit(1);
}