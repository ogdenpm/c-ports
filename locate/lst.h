#pragma once
#include <stdio.h>
#include <stdarg.h>
extern FILE *lstFp;
extern char *lstName;

void openLst(char const *signon);	// opens lstName
void Putc(int c);
int Printf(char const *fmt, ...);
int vPrintf(char const *fmt, va_list args);
int PrintfAndLog(char const *fmt, ...);