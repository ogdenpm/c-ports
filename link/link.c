/****************************************************************************
 *  link.c: part of the C port of Intel's ISIS-II link             *
 *  The original ISIS-II application is Copyright Intel                     *
 *																			*
 *  Re-engineered to C by Mark Ogden <mark.pm.ogden@btinternet.com> 	    *
 *                                                                          *
 *  It is released for hobbyist use and for academic interest			    *
 *                                                                          *
 ****************************************************************************/

/*
 * vim:ts=4:shiftwidth=4:expandtab:
 */
#include "link.h"
#include <stdio.h>
word inFile;
word tofilefd;
word printFileNo;
word pad_4565;
word tmpfilefd;
word statusIO;
word actRead;
char *inFileName;
char *toFileName;
char *printFileName;
bool mapWanted;
psModName_t outModuleName;
byte modEndModTyp;
byte outTranId;
byte outTranVn;
byte modEndSegId;
word modEndOffset;
word segLen[6];
byte alignType[6];
byte segmap[256];
pointer membot;
pointer topHeap;
record_t *inRecordP;
pointer erecP;
pointer inP;
word recNum;
word recLen;
word npbuf;
pointer sbufP;
pointer bufP;
pointer ebufP;
pointer soutP;
pointer outP;
pointer eoutP;
library_t *objFileHead;
library_t *curObjFile;
module_t *curModule;
symbol_t *hashTab[128];
symbol_t *headSegOrderLink;
symbol_t *comdefInfoP;
symbol_t *symbolP;
word unresolved;
word maxExternCnt;
symbol_t *headUnresolved;

word inBlk;
word inByt;
pointer inbP;
byte inCRC;

byte COPYRIGHT[] = "[C] 1976, 1977, 1979 INTEL CORP'";
char VERSION[] = "V3.0";
byte DUMMYREC[] = {0,0,0};

/* EXTERNALS */
extern byte overlayVersion[4];

void ConOutStr(char const * pstr, word count)
{
    Write(CO_DEV, pstr, count, &statusIO);
} /* ConOutStr() */

_Noreturn void FatalErr(byte errCode)
{
    fprintf(stderr, " %s", inFileName);

    if (curModule)
        fprintf(stderr, "(%.*s)", curModule->name.len, curModule->name.str);
    fputc(',', stderr);

    ReportError(errCode);
    fprintf(stderr, " RECORD TYPE %02XH, RECORD NUMBER ", inRecordP->rectyp);
    if (recNum > 0)
        fprintf(stderr, "%5d\n", recNum);
    else
        fputs("*****\n", stderr);

    Exit(1);
} /* FatalErr() */

_Noreturn void IllFmt()
{
    FatalErr(ERR218);   /* Illegal() record format */
} /* IllFmt() */

_Noreturn void IllegalRelo()
{
    FatalErr(ERR212);   /* Illegal() relo record */
} /* IllegalRelo() */

_Noreturn void BadRecordSeq()
{
    FatalErr(ERR224);   /* Bad() record sequence */
} /* BadRecordSeq() */

void pstrcpy(pstr_t const *psrc, pstr_t *pdst)
{
    memcpy(pdst, psrc, psrc->len + 1);
} /* pstrcpy() */

byte HashF(pstr_t *pstr)
{
    byte i, j;

    j = pstr->len;
    for (i = 0; i < pstr->len; i++)
        j = RorB(j, 1) ^ pstr->str[i];
    return j & 0x7F;
} /* HashF() */

bool Lookup(pstr_t *pstr, symbol_t **pitemRef, byte mask)
{
    symbol_t *p;
    byte i;

    i = pstr->len + 1;     /* Size() of string including length() byte */
    *pitemRef = (p = (symbol_t *)&hashTab[HashF(pstr)]);
    for (p = p->hashLink; p; p = p->hashLink) {
        *pitemRef = p;
        if ((p->flags & mask) != AUNKNOWN ) /* ignore undef entries */
            if (Strequ((char *)pstr, (char *)&p->name, i) )
                return true;
    }
    return false;
} /* Lookup() */

void WriteBytes(void const *bufP, word count)
{    
    Write(printFileNo, bufP, count, &statusIO);
    FileError(statusIO, printFileName, TRUE);
} /* WriteBytes() */

void WriteCRLF()
{
    WriteBytes("\n", 1);
} /* WriteCRLF() */

void WriteAndEcho(void const *buffP, word count)
{
    
    WriteBytes(buffP, count);
    if (printFileNo > 0 )
        ConOutStr(buffP, count);
} /* WriteAndEcho() */

void WAEFnAndMod(char *buffP, word count)
{
    WriteAndEcho(buffP, count);
    WriteAndEcho(inFileName, strlen(inFileName));
    WriteAndEcho("(", 1);
    WriteAndEcho(curModule->name.str, curModule->name.len);
    WriteAndEcho(")\r\n", 3);
} /* WAEFnAndMod */

void Start()
{
    ParseCmdLine();
    Phase1();
    Phase2();
    Close(printFileNo, &statusIO);
    Exit(0);

} /* Start */

