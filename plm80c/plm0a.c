/****************************************************************************
 *  plm0a.c: part of the C port of Intel's ISIS-II plm80                    *
 *  The original ISIS-II application is Copyright Intel                     *
 *                                                                          *
 *  Re-engineered to C by Mark Ogden <mark.pm.ogden@btinternet.com>         *
 *                                                                          *
 *  It is released for academic interest and personal use only              *
 ****************************************************************************/
#include "plm.h"

byte cClass[] = {
    CC_NONPRINT, CC_NONPRINT, CC_NONPRINT, CC_NONPRINT,  CC_NONPRINT, CC_NONPRINT, CC_NONPRINT,
    CC_NONPRINT, CC_NONPRINT, CC_WSPACE,   CC_NEWLINE,   CC_NONPRINT, CC_NONPRINT, CC_WSPACE,
    CC_NONPRINT, CC_NONPRINT, CC_NONPRINT, CC_NONPRINT,  CC_NONPRINT, CC_NONPRINT, CC_NONPRINT,
    CC_NONPRINT, CC_NONPRINT, CC_NONPRINT, CC_NONPRINT,  CC_NONPRINT, CC_NONPRINT, CC_NONPRINT,
    CC_NONPRINT, CC_NONPRINT, CC_NONPRINT, CC_NONPRINT,  CC_WSPACE,   CC_INVALID,  CC_INVALID,
    CC_INVALID,  CC_DOLLAR,   CC_INVALID,  CC_INVALID,   CC_QUOTE,    CC_LPAREN,   CC_RPAREN,
    CC_STAR,     CC_PLUS,     CC_COMMA,    CC_MINUS,     CC_PERIOD,   CC_SLASH,    CC_DIGIT,
    CC_DIGIT,    CC_DIGIT,    CC_DIGIT,    CC_DIGIT,     CC_DIGIT,    CC_DIGIT,    CC_DIGIT,
    CC_DIGIT,    CC_DIGIT,    CC_COLON,    CC_SEMICOLON, CC_LESS,     CC_EQUALS,   CC_GREATER,
    CC_INVALID,  CC_INVALID,  CC_HEXCHAR,  CC_HEXCHAR,   CC_HEXCHAR,  CC_HEXCHAR,  CC_HEXCHAR,
    CC_HEXCHAR,  CC_ALPHA,    CC_ALPHA,    CC_ALPHA,     CC_ALPHA,    CC_ALPHA,    CC_ALPHA,
    CC_ALPHA,    CC_ALPHA,    CC_ALPHA,    CC_ALPHA,     CC_ALPHA,    CC_ALPHA,    CC_ALPHA,
    CC_ALPHA,    CC_ALPHA,    CC_ALPHA,    CC_ALPHA,     CC_ALPHA,    CC_ALPHA,    CC_ALPHA,
    CC_INVALID,  CC_INVALID,  CC_INVALID,  CC_INVALID,   CC_UNDERBAR, CC_INVALID,  CC_HEXCHAR,
    CC_HEXCHAR,  CC_HEXCHAR,  CC_HEXCHAR,  CC_HEXCHAR,   CC_HEXCHAR,  CC_ALPHA,    CC_ALPHA,
    CC_ALPHA,    CC_ALPHA,    CC_ALPHA,    CC_ALPHA,     CC_ALPHA,    CC_ALPHA,    CC_ALPHA,
    CC_ALPHA,    CC_ALPHA,    CC_ALPHA,    CC_ALPHA,     CC_ALPHA,    CC_ALPHA,    CC_ALPHA,
    CC_ALPHA,    CC_ALPHA,    CC_ALPHA,    CC_ALPHA,     CC_INVALID,  CC_INVALID,  CC_INVALID,
    CC_INVALID,  CC_NONPRINT
};

byte tToLMap[] = { T1_IDENTIFIER, T1_NUMBER, T1_STRING, T1_PLUSSIGN, T1_MINUSSIGN, T1_STAR, T1_SLASH,
                   T1_MOD, T1_PLUS, T1_MINUS, T1_AND, T1_OR, T1_XOR, T1_NOT, 0, T1_LT, T1_LE, T1_EQ, T1_NE, // T1_14
                   T1_GE, T1_GT, T1_COLONEQUALS, T1_INVALID, T1_INVALID,           // COLON, SEMICOLON
                   T1_PERIOD, T1_LPAREN, T1_RPAREN, T1_COMMA, T1_CALL, T1_INVALID,  // DECLATE
                   T1_DISABLE, T1_INVALID, T1_ENABLE, T1_END, T1_GO, T1_GOTO,       // DO
                   T1_HALT, T1_IF, T1_PROCEDURE, T1_RETURN, T1_INVALID, T1_INVALID, // ADDRESS, AT
                   T1_INVALID, T1_INVALID, T1_INVALID, T1_INVALID, T1_INVALID, T1_INVALID,
                   // BASED, BYTE, DATA, EXTERNAL, INITIAL, INTERRUPT
                   T1_INVALID, T1_INVALID, T1_INVALID, T1_INVALID, T1_INVALID, T1_BY, T1_INVALID,
                   // LABEL, LITERALLY, PUBLIC, REENTRANT, STRUCTURE, BY, CASE
                   T1_INVALID, T1_INVALID, T1_INVALID, T1_TO, T1_INVALID };
                   // ELSE, EOF, THEN, TO, WHILE

/* public variables */
macro_t macroPtrs[6];

word macroDepth = 0;
word tokenVal;
byte *inChrP;
word stateStack[100];
word stateSP;
sym_t *stmtLabels[10];
word stmtLabelCnt;

word curStmtCnt = 0;
word curBlkCnt  = 0;
info_t *macroInfo;
sym_t *markedSym;
byte lineBuf[MAXLINE + 1];
byte tokenType;
word wTokenLen; // used for strings and lit, len > 255
byte tokenStr[MAXSTRING + 2];   // pstr len, str, '\0'
byte nextCh;
byte stmtStartCode;
byte stmtStartToken;
byte startLexCode;
word doBlkCnt = 0;
sym_t *stmtStartSymbol;
bool lineInfoToWrite = false;
bool isNonCtrlLine   = false;
bool yyAgain         = false;
word curScope;
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
    Wr1LineInfo();
    vfWbyte(&utf1, v);
}

void Wr1Word(uint16_t v) {
    Wr1LineInfo();
    vfWword(&utf1, v);
}

void Wr1Info(info_t *inf) {
    Wr1LineInfo();
    vfWword(&utf1, ToIdx(inf));
}

void Rd1Buf(void *buf, uint16_t len) {
    vfRbuf(&utf1, buf, len);
}

uint8_t Rd1Byte() {
    return vfRbyte(&utf1);
}

uint16_t Rd1Word() {
    return vfRword(&utf1);
}

void Wr1SyntaxError(word err) {
    hasErrors = true;
    Wr1Byte(T1_SYNTAXERROR);
    Wr1Word(err);
} /* SyntaxError() */

void Wr1TokenError(word err, sym_t *sym) {
    hasErrors = true;
    Wr1Byte(T1_TOKENERROR);
    Wr1Word(err);
    Wr1Word((uint16_t)(sym ? sym - symtab : 0));
} /* TokenError() */

void Wr1TokenErrorAt(word err) {
    Wr1TokenError(err, markedSym);
} /* TokenErrorAt() */

_Noreturn void LexFatalError(word err) {
    Wr1TokenErrorAt(err);
    fatalErrorCode = err;
    longjmp(exception, -1);
}

void PushBlock(word idAndLevel) {
    if (scopeSP == 34)
        LexFatalError(ERR84); /* LIMIT EXCEEDED: BLOCK NESTING */
    else {
        scopeChains[++scopeSP] = idAndLevel;
        curBlkCnt++;
    }
} /* PushBlock() */

void PopBlock() {
    if (scopeSP == 0)
        LexFatalError(ERR96); /* COMPILER ERROR: SCOPE STACK UNDERFLOW */
    else {
        curBlkCnt--;
        curScope = scopeChains[--scopeSP];
    }
} /* PopBlock() */

void Wr1LexToken() {
    if (tToLMap[tokenType] == T1_INVALID)
        Wr1TokenErrorAt(ERR171); /* INVALID USE OF DELIMITER OR RESERVED WORD IN EXPRESSION */
    else {
        Wr1Byte(tToLMap[tokenType]);
        if (tokenType == L_IDENTIFIER)
            Wr1Word((uint16_t)(curSym ? curSym - symtab : 0));
        else if (tokenType == L_NUMBER)
            Wr1Word(tokenVal);
        else if (tokenType == L_STRING) {
            Wr1Word(wTokenLen);
            Wr1Buf(tokenStr + 1, wTokenLen);
        }
    }
}

void Wr1XrefUse() {
    if (XREF) {
        Wr1Byte(T1_XREFUSE);
        Wr1Info(info);
    }
}

void Wr1XrefDef() {
    if (XREF || IXREF || SYMBOLS) {
        Wr1Byte(T1_XREFDEF);
        Wr1Info(info);
    }
}
