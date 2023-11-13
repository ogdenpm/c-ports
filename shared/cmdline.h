/****************************************************************************
 *  cmdline.h: part of the C port of Intel's ISIS-II 8085 toolchain         *
 *  The original ISIS-II application is Copyright Intel                     *
 *                                                                          *
 *  Re-engineered to C by Mark Ogden <mark.pm.ogden@btinternet.com>         *
 *                                                                          *
 *  It is released for academic interest and personal use only              *
 ****************************************************************************/
#pragma once
#include <stdio.h>
#include <stdint.h>
extern char *cmdP; // current location on command line

char *getInteractiveLine();
char *getCmdLine(int argc, char **argv);
char *GetToken(void);
char *PeekToken(void);
char *GetText(void);
char *GetNonSpaceToken(void);
uint16_t GetNumber(void);
int printCmdLine(FILE *fp, int width, int offset);
_Noreturn void FatalCmdLineErr(char const *errMsg);
void SkipWs();
void ExpectChar(uint8_t ch, char const *msg);
void SetEndToken(char *p);