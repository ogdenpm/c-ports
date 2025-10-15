/****************************************************************************
 *  os.h: part of the C port of Intel's ISIS-II 8085 toolchain              *
 *  The original ISIS-II application is Copyright Intel                     *
 *                                                                          *
 *  Re-engineered to C by Mark Ogden <mark.pm.ogden@btinternet.com>         *
 *                                                                          *
 *  It is released for academic interest and personal use only              *
 ****************************************************************************/
#pragma once
#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>


#ifdef _WIN32
#define DIRSEP "/\\"
#else
#define _MAX_PATH 4096
#define DIRSEP    "/"
#endif

#ifndef _MSC_VER
#ifndef stricmp
#define stricmp strcasecmp
#endif
#endif
// linkage to application
void Start();

// io.c routines
void SetITrap(void (*trap)(int retCode));
void RmOnError(char const *fname);

bool hasIsisDrive(char const *path);
char *MapFile(char *osName, const char *isisPath);
FILE *Fopen(char const *pathP, char *access);
int Delete(char const *ipath);
int Rename(char const *iold, char const *inew);
int Access(char const *ipath, int mode);
void mkpath(char *file);

_Noreturn void Exit(int retCode);

// error handlers
_Noreturn void FatalCmdLineErr(char const *errMsg);
_Noreturn void fatal(char const *fmt, ...);
_Noreturn void IoError(char const *path, char const *fmt, ...);

// cmd line variables and support functions

extern char const *invokeName; // sanitised invoking command
extern bool ttyin;
extern bool ttyout;


void printDriveMap(FILE *fp);

void *safeMalloc(size_t size);
void *safeRealloc(void *p, size_t size);
char *safeStrdup(char const *str);
