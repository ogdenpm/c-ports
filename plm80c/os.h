#pragma once
#include <stdint.h>
#include <stdio.h>
// linkage to application
void usage();
void Start();

// io.c routines
void setTrap(void (*trap)(void));
char const *basename(char const *path);
char *MapFile(char *osName, const char *isisPath);
FILE *Fopen(char const *pathP, char *access);
int Delete(char const *ipath);
int Access(char const *ipath, int mode);

_Noreturn void Exit(int retCode);

// error handlers
_Noreturn void FatalCmdLineErr(char const *errMsg);
_Noreturn void FatalError(char const *fmt, ...);
_Noreturn void IoError(char const *path, char const *fmt, ...);

// cmd line variables and support functions
extern char *cmdP;             // current location on command line
extern char const *invokeName; // sanitised invoking command

char *getCmdLine(int argc, char **argv);
// char *GetToken(void);
char *GetText(void);
char *GetNSToken(void);
    uint16_t GetNumber(void);
int printCmdLine(FILE *fp, int width);
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
