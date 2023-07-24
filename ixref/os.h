#pragma once
#include <stdint.h>
#include <stdio.h>
// linkage to application
void usage();
void Start();


// io.c routines
void SetITrap(void (*f)(void));
void RegCleanup(void (*f)(void));
void DeregCleanup(void (*f)(void));

char *basename(char *path);
FILE *Fopen(char const *pathP, char *access);
int Delete(char const *ipath);
int Rename(char const *iold, char const *inew);
int Access(char const *ipath, int mode);

_Noreturn void Exit(int retCode);

// error handlers
_Noreturn void FatalCmdLineErr(char const *errMsg);
_Noreturn void FatalError(char const *fmt, ...);
_Noreturn void IoError(char const *path, char const *fmt, ...);

// cmd line variables and support functions
extern char *cmdP;        // current location on command line
extern char *invokeName;  // sanitised invoking command

char *getCmdLine(int argc, char **argv);
char *GetToken(void);
char *GetText(void);
uint16_t GetNumber(void);
void printCmdLine(FILE *fp);
void printDriveMap(FILE *fp);

void *xmalloc(size_t size);
void *xrealloc(void *p, size_t size);
char *xstrdup(char const *str);
_Noreturn void FatalCmdLineErr(char const *errMsg);
void *xmalloc(size_t size);
void *xrealloc(void *p, size_t size);
char *xstrdup(char const *str);
void SkipWs();
void ExpectChar(uint8_t ch, char const *msg);
void SetEndToken(char *p);
