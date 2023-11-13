/****************************************************************************
 *  asm6m.c: part of the C port of Intel's ISIS-II asm80                    *
 *  The original ISIS-II application is Copyright Intel                     *
 *                                                                          *
 *  Re-engineered to C by Mark Ogden <mark.pm.ogden@btinternet.com>         *
 *                                                                          *
 *  It is released for academic interest and personal use only              *
 ****************************************************************************/
#include "asm80.h"
#include <ctype.h>
static int lineBufSize; // current size of lineBuf, auto expanded
#define LCHUNK 256      // growth step for lineBuf

void SyntaxError(void) {
    SourceError('Q');
}

void DoubleDefError(void) {
    SourceError('D');
}

void ExpressionError(void) {
    SourceError('E');
}

void CommandError(void) {
    SourceError('C');
}

void OpcodeOperandError(void) {
    SourceError('O');
}

void NameError(void) {
    SourceError('R');
}

void MultipleDefError(void) {
    SourceError('M');
}

void ValueError(void) {
    SourceError('V');
}

void NestingError(void) {
    SourceError('N');
}

void PhaseError(void) {
    SourceError('P');
}

void StackError(void) {
    RuntimeError(RTE_STACK);
}

void FileError(void) {
    RuntimeError(RTE_FILE);
}

void IllegalCharError(void) {
    SourceError('I');
}

void BalanceError(void) {
    SourceError('B');
}

void UndefinedSymbolError(void) {
    SourceError('U');
}

void LocationError(void) {
    SourceError('L');
}

void OperandError(void) {
    SourceError('X');
}

byte HaveTokens(void) {
    return tokenIdx != 0;
}

void PopToken(void) {
#ifdef TRACE
    DumpTokenStack(true);
#endif
    token = tokenStk[tokenIdx];
    if (tokenIdx)
        tokenIdx--;
}

/* nest - sw == 1 -> nest macro sw == 2 -> nest if */
void Nest(byte sw) {
    macroCondStk[++macroCondSP] = macroCondStk[0];
    /* record whether current nest is macro of if */
    if ((macroCondStk[0] = sw) == 1) {
        if (++macroDepth > 9) {
            StackError();
            macroDepth = 0;
        } else {
            macro[macroDepth] = curMacro;
            curMacro.condSP   = macroCondSP;
            curMacro.ifDepth  = ifDepth;
            nestMacro         = true;
        }
    } else {
        if (++ifDepth > 8) {
            StackError();
            ifDepth = 0;
        } else {
            skipIf[ifDepth] = skipIf[0];
            inElse[ifDepth] = inElse[0];
        }
    }
}

void UnNest(byte sw) {
    if (sw != macroCondStk[0]) { /* check for unbalanced unnest */
        NestingError();
        if (sw == 2) /* ! macro unnest */
            return;
        macroCondSP = curMacro.condSP;
        ifDepth     = curMacro.ifDepth;
    }

    macroCondStk[0] = macroCondStk[macroCondSP]; /* restore macro stack */
    macroCondSP--;
    if (sw == 1) { /* is unnest macro */
        curMacro = macro[macroDepth];
        ReadM(curMacro.blk);
        savedMtype = curMacro.mtype;
        if (--macroDepth == 0) { /* end of macro nest */
            expandingMacro = 0;  /* not expanding */
            topMacroArg    = macroArgs;
        }
    } else {
        skipIf[0] = skipIf[ifDepth]; /* pop skipIf and inElse status */
        inElse[0] = inElse[ifDepth];
        ifDepth--;
    }
}

void PushToken(byte type) {
    if (tokenIdx >= MAXTOKENS - 1)
        StackError();
    else {
        tokenStk[++tokenIdx] = token;
        token.start += token.size; /* advance for next token */
        token.type = type;
        token.attr = token.size = 0;
        token.symbol            = NULL;
        token.symId             = 0;
    }
#ifdef TRACE
    DumpTokenStack(false);
#endif
}

void CollectByte(byte c) {
    int n;
    n = token.start + token.size++;
    if (n >= lineBufSize - 1) // allow room for a possible '\0'
        lineBuf = xrealloc(lineBuf, lineBufSize += LCHUNK);
    lineBuf[n] = c;
}

void GetTokenText(byte type) {
    PushToken(type); /* save any previous token and initialise this one */
    reget = 1;       /* force re get of first character */
    bool isnum = type == O_NUMBER;
    while ((type = GetChClass()) == CC_DIG || type == CC_LET ||
           type == CC_DOLLAR || type == CC_BAR) { /* digit || letter */
        if (type != CC_DOLLAR && !(isnum && type == CC_BAR))
            CollectByte(toupper(curChar));
    }
    reget = 1; /* force re get of Exit() char */
}

void GetNum(void) {
    word num;
    byte radix, digit, i;
    //    byte chrs based tokenStart [1];

    GetTokenText(O_NUMBER);
    radix = tokenStart[--token.size];
    if (radix == 'H')
        radix = 16;

    if (radix == 'D')
        radix = 10;

    if (radix == 'O' || radix == 'Q')
        radix = 8;

    if (radix == 'B')
        radix = 2;

    if (radix > 16)
        radix = 10;
    else
        token.size--;

    num = 0;
    for (i = 0; i <= token.size; i++) {
        if (!isalnum(tokenStart[i])) {
            IllegalCharError();
            digit = 0;
        } else {
            if ((digit = tokenStart[i] - '0') > 9)
                digit -= 7;
            if (digit >= radix)
                if (tokenIdx < 2 || !(tokenStk[2].type == NULVAL)) { /* bug fix tokIdx may be < 2 */
                    IllegalCharError();
                    digit = 0;
                }
        }

        num = num * radix + digit;
    }
    /* replace with packed number */
    token.size = 0;
    CollectByte((num)&0xff);
    CollectByte((num) >> 8);
}

void GetStr(void) {
    PushToken(O_STRING);

    while (GetCh() != EOLCH) {
        if (curChar == '\'' && GetCh() != '\'') { // if not '' then all done
            reget = 1;
            return;
        } else
            CollectByte(curChar); // collect char - '' becomes '
    }
    BalanceError(); // EOL seen before closing '
    reget = 1;      // push back EOLCH
}