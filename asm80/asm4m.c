/****************************************************************************
 *  asm4m.c: part of the C port of Intel's ISIS-II asm80             *
 *  The original ISIS-II application is Copyright Intel                     *
 *																			*
 *  Re-engineered to C by Mark Ogden <mark.pm.ogden@btinternet.com> 	    *
 *                                                                          *
 *  It is released for hobbyist use and for academic interest			    *
 *                                                                          *
 ****************************************************************************/

#include "asm80.h"

bool StrUcEqu(char const *s, char const *t)
{
    while (*s) {
        if (*s++ != (*t++ & 0x5f))
            return false;
    }
    return true;
}


bool IsSkipping(void) {
    return (mSpoolMode & 1) || skipIf[0];
}

void Sub546F(void) {
    spIdx = NxtTokI();
    if (expectOperand)
        SyntaxError();
    if (HaveTokens())
        if (!(tokenStk[spIdx].type == O_DATA || lineNumberEmitted))
            SyntaxError();
    if (inDB || inDW) {
            if (tokenIdx == 1 && ! BlankAsmErrCode() && token.size != 1)
            token.size = 2;
    }
    else if (! BlankAsmErrCode() && HaveTokens())
        if (tokenStk[spIdx].size > 3)
            tokenStk[spIdx].size = 3;
}


void FinishLine(void) {

    Sub546F();
    if (IsPhase2Print()) {    /* update the line number */
        lineNo++;

        if (ShowLine() || ! BlankAsmErrCode())
            PrintLine();
    }

    if (skipRuntimeError) {
        exit(1);
    }

    if (!isControlLine)
    {
        int i = tokenIdx < 2 || inDB || inDW ? 0 : 2;


        int w6BCE = IsSkipping() || !isInstr ? 0 : tokenStk[i].start + tokenStk[i].size;

        if (ChkGenObj())
            Ovl8();
        b6B2C = true;
        segLocation[activeSeg] = effectiveAddr = (word)(segLocation[activeSeg] + w6BCE);
    }

    if (controls.xref && haveUserSymbol && phase == 1)
        EmitXref(XREF_REF, name);

    FlushM(false);

    while (tokenIdx > 0) {
        PopToken();
    }

    InitLine();
    if (b6B33)
    {
        finished = true;
        if (IsPhase2Print() && controls.symbols)
            Sub7041_8447();

        if (ChkGenObj())
            ReinitFixupRecs();
    }
}