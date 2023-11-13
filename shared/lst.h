/****************************************************************************
 *  lst.h: part of the C port of Intel's ISIS-II 8085 toolchain             *
 *  The original ISIS-II application is Copyright Intel                     *
 *                                                                          *
 *  Re-engineered to C by Mark Ogden <mark.pm.ogden@btinternet.com>         *
 *                                                                          *
 *  It is released for academic interest and personal use only              *
 ****************************************************************************/
#pragma once
#include <stdarg.h>
#include <stdio.h>
extern FILE *lstFp;
extern char *lstName;

void openLst(char const *signon); // opens lstName
void closeLst(void);
void Putc(int c);
int Printf(char const *fmt, ...);
int vPrintf(char const *fmt, va_list args);
int PrintfAndLog(char const *fmt, ...);