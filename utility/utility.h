#pragma once
#include <stdio.h>
#include <stdbool.h>
// message.c

bool openLog(char const *name);
void closeLog();
void logMsg(char const *fmt, ...);


_Noreturn void fatal(char const *fmt, ...);
_Noreturn void Exit(int exitCode);   // allows user defined exit
void warn(char const *fmt, ...);

// file.c
char *basename(char *path);
char *makeFilename(char const *path, char const *ext, bool force);
// memory.c
void *safeMalloc(size_t size);
void *safeRealloc(void *old, size_t size);
char *safeStrdup(char const *s);

    // option.c
/*
    Help contains the usage syntax and the option descriptions.
    When shown  it is preceded by the simple version information and app desciption
    And followed by a note about the standard -v, -V and -h options.
    The string can contain one %s which is replaced by the program name used to
    run the application
*/
extern char const help[];
extern char const *optArg; // optional argument (or error message)
extern int optInd;         // current index into argv
extern int optOpt;         // option scanned
extern char *programName;

int getOpt(int argc, char **argv, const char *options);
void chkStdOptions(int argc, char **argv);
_Noreturn void usage(char *fmt, ...);
void showVersion(FILE *fp, bool full);

// getch.c
int _getch(void);
