/****************************************************************************
 *  io.c: part of the C port of Intel's ISIS-II plm80c             *
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
#define DIRSEP  "/\\:"
#else
#include <unistd.h>
#include <errno.h>
#define _MAX_PATH 4096
#define O_BINARY    0
#define DIRSEP    "/"
#endif

#define CLCHUNK 256 // size increase as command line grows

#define STDIN   0
#define STDOUT  1

char *cmdP; // current location on command line

char *invokeName; // sanitised invoking command
bool prompt;


#include "plm.h"

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

#define MAXLL	122
static char _commandLine[MAXLL + 1];
static char *_commandLinePtr;



static const char *error_strings[] =
{
    "Success",
    "No memory available for buffer",
    "File is not open",
    "No more file handles available",
    "Invalid pathname",
    "Bad device name in filename",
    "Trying to write a file opened in read mode",
    "Disk is full",
    "Trying to read a file opened in write mode",
    "Cannot create file",
    "Cannot rename across devices",
    "Destination file exists",
    "File is already open",
    "File not found",
    "Permissions error",
    "Attempting to overwrite operating system",
    "Invalid executable image",
    "Attempt to rename or delete a device",
    "Invalid function number",
    "Can't seek on a device",
    "Can't seek to before start of file",
    "File is not open in line mode",
    "Bad access mode",
    "No filename specified",
    "Disk error",
    "Invalid echo file specified",
    "Bad attribute",
    "Bad seek mode",
    "Null file extension",
    "End of file on console input",
    "Drive not ready",
    "Can't seek in a write-only file",
    "Can't delete an open file",
    "Bad system call parameter",
    "Bad switch argument in load",
    "Cannot seek past EOF in file open for read",
};

#define MAXERROR (sizeof(error_strings) / sizeof(error_strings[0]))


void Error(word ErrorNum)
{
    if (ErrorNum >= 0 && ErrorNum < MAXERROR)
        fprintf(stderr, "%s.\n", error_strings[ErrorNum]);
    else
        fprintf(stderr, "Unknown error %d.\n", ErrorNum);
}




