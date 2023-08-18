/****************************************************************************
 *  plm0a.c: part of the C port of Intel's ISIS-II plm80c             *
 *  The original ISIS-II application is Copyright Intel                     *
 *																			*
 *  Re-engineered to C by Mark Ogden <mark.pm.ogden@btinternet.com> 	    *
 *                                                                          *
 *  It is released for hobbyist use and for academic interest			    *
 *                                                                          *
 ****************************************************************************/

#include "plm.h"

byte cClass[] = {
    CC_NONPRINT, CC_NONPRINT, CC_NONPRINT, CC_NONPRINT,  CC_NONPRINT, CC_NONPRINT, CC_NONPRINT,
    CC_NONPRINT, CC_NONPRINT, CC_WSPACE,   CC_NEWLINE,   CC_NONPRINT, CC_NONPRINT, CC_WSPACE,
    CC_NONPRINT, CC_NONPRINT, CC_NONPRINT, CC_NONPRINT,  CC_NONPRINT, CC_NONPRINT, CC_NONPRINT,
    CC_NONPRINT, CC_NONPRINT, CC_NONPRINT, CC_NONPRINT,  CC_NONPRINT, CC_NONPRINT, CC_NONPRINT,
    CC_NONPRINT, CC_NONPRINT, CC_NONPRINT, CC_NONPRINT,  CC_WSPACE,   CC_INVALID,  CC_INVALID,
    CC_INVALID,  CC_DOLLAR,   CC_INVALID,  CC_INVALID,   CC_QUOTE,    CC_LPAREN,   CC_RPAREN,
    CC_STAR,     CC_PLUS,     CC_COMMA,    CC_MINUS,     CC_PERIOD,   CC_SLASH,    CC_BINDIGIT,
    CC_BINDIGIT, CC_OCTDIGIT, CC_OCTDIGIT, CC_OCTDIGIT,  CC_OCTDIGIT, CC_OCTDIGIT, CC_OCTDIGIT,
    CC_DECDIGIT, CC_DECDIGIT, CC_COLON,    CC_SEMICOLON, CC_LESS,     CC_EQUALS,   CC_GREATER,
    CC_INVALID,  CC_INVALID,  CC_HEXCHAR,  CC_HEXCHAR,   CC_HEXCHAR,  CC_HEXCHAR,  CC_HEXCHAR,
    CC_HEXCHAR,  CC_ALPHA,    CC_ALPHA,    CC_ALPHA,     CC_ALPHA,    CC_ALPHA,    CC_ALPHA,
    CC_ALPHA,    CC_ALPHA,    CC_ALPHA,    CC_ALPHA,     CC_ALPHA,    CC_ALPHA,    CC_ALPHA,
    CC_ALPHA,    CC_ALPHA,    CC_ALPHA,    CC_ALPHA,     CC_ALPHA,    CC_ALPHA,    CC_ALPHA,
    CC_INVALID,  CC_INVALID,  CC_INVALID,  CC_INVALID,   CC_INVALID,  CC_INVALID,  CC_HEXCHAR,
    CC_HEXCHAR,  CC_HEXCHAR,  CC_HEXCHAR,  CC_HEXCHAR,   CC_HEXCHAR,  CC_ALPHA,    CC_ALPHA,
    CC_ALPHA,    CC_ALPHA,    CC_ALPHA,    CC_ALPHA,     CC_ALPHA,    CC_ALPHA,    CC_ALPHA,
    CC_ALPHA,    CC_ALPHA,    CC_ALPHA,    CC_ALPHA,     CC_ALPHA,    CC_ALPHA,    CC_ALPHA,
    CC_ALPHA,    CC_ALPHA,    CC_ALPHA,    CC_ALPHA,     CC_INVALID,  CC_INVALID,  CC_INVALID,
    CC_INVALID,  CC_NONPRINT
};

byte tok2oprMap[] = { L_IDENTIFIER, L_NUMBER,  L_STRING,    L_PLUSSIGN,    L_MINUSSIGN, L_STAR,
                      L_SLASH,      L_MOD,     L_PLUS,      L_MINUS,       L_AND,       L_OR,
                      L_XOR,        L_NOT,     0,           L_LT,          L_LE,        L_EQ,
                      L_NE,         L_GE,      L_GT,        L_COLONEQUALS, L_INVALID,   L_INVALID,
                      L_PERIOD,     L_LPAREN,  L_RPAREN,    L_COMMA,       L_CALL,      L_INVALID,
                      L_DISABLE,    L_INVALID, L_ENABLE,    L_END,         L_GO,        L_GOTO,
                      L_HALT,       L_IF,      L_PROCEDURE, L_RETURN,      L_INVALID,   L_INVALID,
                      L_INVALID,    L_INVALID, L_INVALID,   L_INVALID,     L_INVALID,   L_INVALID,
                      L_INVALID,    L_INVALID, L_INVALID,   L_INVALID,     L_INVALID,   L_BY,
                      L_INVALID,    L_INVALID, L_INVALID,   L_INVALID,     L_TO,        L_INVALID };

/* public variables */
macro_t macroPtrs[6];

word macroDepth = 0;
word tokenVal;
byte *inChrP; /* has to be pointer as it accesses data outside info/symbol space */
word stateStack[100];
word stateIdx;
offset_t stmtLabels[10];
word stmtLabelCnt;

word curStmtCnt        = 0;
word curBlkCnt         = 0;
offset_t macroIdx      = 0;
offset_t markedSymbolP = 0;
byte lineBuf[MAXLINE + 1];
char inbuf[1280];
byte tokenType;
byte tokenStr[256];
byte nextCh;
byte stmtStartCode;
byte stmtStartToken;
byte startLexCode;
word doBlkCnt = 0;
byte tx1Buf[1280];
offset_t stmtStartSymbol;
bool lineInfoToWrite = false;
bool isNonCtrlLine   = false;
bool yyAgain         = false;
word curScope;
byte state;
bool skippingCOND = false;
word ifDepth      = 0;

void Wr1LineInfo() {
    if (lineInfoToWrite) {
        vfWbyte(&utf1, linfo.type);
        vfWbuf(&utf1, &linfo.lineCnt, sizeof(struct _linfo));
        lineInfoToWrite = false;
    }
} /* WriteLineInfo() */

void Wr1Buf(void const *buf, word len) {
    Wr1LineInfo();
    vfWbuf(&utf1, buf, len);
}

void Wr1Byte(uint8_t v) {
    Wr1Buf(&v, sizeof(v));
}

void Wr1Word(uint16_t v) {
    Wr1Buf(&v, sizeof(v));
}

void Rd1Buf(void *buf, uint16_t len) {
    vfRbuf(&utf1, buf, len);
}

uint8_t Rd1Byte() {
    uint8_t v;
    Rd1Buf(&v, sizeof(v));
    return v;
}

uint16_t Rd1Word() {
    uint16_t v;
    Rd1Buf(&v, sizeof(v));
    return v;
}

void Wr1InfoOffset(offset_t addr) {
    Wr1Word(addr);
} /* WrInfoOffset() */

void Wr1SyntaxError(byte err) {
    hasErrors = true;
    Wr1Byte(L_SYNTAXERROR);
    Wr1Word(err);
} /* SyntaxError() */

void Wr1TokenErrorAt(byte err) {
    hasErrors = true;
    Wr1Byte(L_TOKENERROR);
    Wr1Word(err);
    Wr1Word(markedSymbolP);
} /* TokenErrorAt() */

void Wr1TokenError(byte err, offset_t symP) {
    hasErrors = true;
    Wr1Byte(L_TOKENERROR);
    Wr1Word(err);
    Wr1Word(symP);
} /* TokenError() */

_Noreturn void LexFatalError(byte err) {
    hasErrors = true;
    if (state != 20) {
        if (err == ERR83) /* LIMIT EXCEEDED: DYNAMIC STORAGE */
            Fatal("DYNAMIC STORAGE OVERFLOW");
        Fatal("UNKNOWN FATAL ERROR");
    }
    Wr1TokenErrorAt(err);
    fatalErrorCode = err;
    longjmp(exception, -1);
}

void PushBlock(word idAndLevel) {
    if (blockDepth == 34)
        LexFatalError(ERR84); /* LIMIT EXCEEDED: BLOCK NESTING */
    else {
        procChains[++blockDepth] = idAndLevel;
        curBlkCnt++;
    }
} /* PushBlock() */

void PopBlock() {
    if (blockDepth == 0)
        LexFatalError(ERR96); /* COMPILER ERROR: SCOPE STACK UNDERFLOW */
    else {
        curBlkCnt--;
        curScope = procChains[--blockDepth];
    }
} /* PopBlock() */

void Wr1LexToken() {
    if (tok2oprMap[tokenType] == L_INVALID) {
        Wr1TokenErrorAt(ERR171); /* INVALID USE OF DELIMITER OR RESERVED WORD IN Expression */
        return;
    }
    Wr1Byte(tok2oprMap[tokenType]);
    if (tokenType == T_IDENTIFIER)
        Wr1Word(curSym);
    else if (tokenType == T_NUMBER)
        Wr1Word(tokenVal);
    else if (tokenType == T_STRING) {
        Wr1Word(tokenStr[0]);
        Wr1Buf(tokenStr + 1, tokenStr[0]);
    }
} /* WrLexToken() */

void Wr1XrefUse() {
    if (XREF) {
        Wr1Byte(L_XREFUSE);
        Wr1InfoOffset(infoIdx);
    }
}

void Wr1XrefDef() {
    if (XREF || IXREF || SYMBOLS) {
        Wr1Byte(L_XREFDEF);
        Wr1InfoOffset(infoIdx);
    }
}