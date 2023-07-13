/****************************************************************************
 *  main0.c: part of the C port of Intel's ISIS-II plm80c             *
 *  The original ISIS-II application is Copyright Intel                     *
 *																			*
 *  Re-engineered to C by Mark Ogden <mark.pm.ogden@btinternet.com> 	    *
 *                                                                          *
 *  It is released for hobbyist use and for academic interest			    *
 *                                                                          *
 ****************************************************************************/

#include "plm.h"
//static byte copyright[] = "[C] 1976, 1977, 1982 INTEL CORP";

jmp_buf exception;

static void FinishLexPass()
{
    if (afterEOF)
        Wr1SyntaxError(ERR87);	/* MISSING 'end' , end-OF-FILE ENCOUNTERED */
    Wr1LineInfo();
    Wr1Byte(L_EOF);
    vfRewind(&utf1);
    CloseF(&srcFil);
} /* FinishLexPass() */


static void ParseCommandLine()
{
    if (moreCmdLine)
        ParseControlLine(cmdTextP);   // ParseControlLine will move to first char
    moreCmdLine = false;       // 0 if no more cmd line
    curScopeP = (wpointer)curScope;
    inChrP = " \n"; // GNxtCh will see \n and get a non blank line
    blockDepth = 1;
    procChains[1] = 0;

} /* ParseCommandLine() */

static void LexPass()
{
    ParseCommandLine();
    InitF(&srcFil, "SOURCE", srcFileTable[0].fNam);
    OpenF(&srcFil, "rt");
    GNxtCh(); // get the first char
    ParseProgram();
} /* LexPass() */


word Start0()
{
    if (setjmp(exception) == 0) {
        state = 20;	/* 9B46 */
        LexPass();
    }
    FinishLexPass();		/* exception goes to here */
    return 1; // Chain(overlay[1]);
}
