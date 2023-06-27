#pragma once
#include <stdint.h>
// linkage to application
_Noreturn void usage();
void Start();


// io.c routines
char *basename(char *path);
FILE *Fopen(char const *pathP, char *access);
_Noreturn void Exit(int retCode);

// error handlers
_Noreturn void FatalCmdLineErr(char const *errMsg);
_Noreturn void FatalError(char const *fmt, ...);
_Noreturn void IoError(char const *path, char const *msg);

// cmd line variables and support functions
extern char *cmdP;        // current location on command line
extern char *invokeName;  // sanitised invoking command

char *GetToken(void);
uint16_t ParseNumber(void);
void printCmdLine(FILE *fp);
void printDriveMap();

void *xmalloc(size_t size);
void *xrealloc(void *p, size_t size);
char *xstrdup(char const *str);
_Noreturn void FatalCmdLineErr(char const *errMsg);
void *xmalloc(size_t size);
void *xrealloc(void *p, size_t size);
char *xstrdup(char const *str);
void SkipNonArgChars(char *pch);
void ExpectChar(uint8_t ch, char const *msg);
