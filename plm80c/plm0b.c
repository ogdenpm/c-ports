/****************************************************************************
 *  plm0b.c: part of the C port of Intel's ISIS-II plm80c             *
 *  The original ISIS-II application is Copyright Intel                     *
 *																			*
 *  Re-engineered to C by Mark Ogden <mark.pm.ogden@btinternet.com> 	    *
 *                                                                          *
 *  It is released for hobbyist use and for academic interest			    *
 *                                                                          *
 ****************************************************************************/

#include "os.h"
#include "plm.h"
#include <ctype.h>
#include <stdlib.h>

#define BADINFO 0xffff

// forward references
static byte InGetC();
static void GetLin();

// optTable is encoded with entries as follows
// name as pstr, control bytes as per structure below
static struct {
    byte optLen;
    char *optName;
    byte tokenId;    // if none zero then token id to put in lexical stream
    bool primary;    // true if primary control
    byte controlId;  // index into controls if not 0xff
    byte controlVal; // index used in switch in PraseExtControl
    byte primaryId;  // index into primaryCtrlSeen
} *tknFlagsP, optTable[] = {
    { 5, "PRINT", 0, 0, 0xFF, 7, 0 },       { 7, "NOPRINT", 0, 0, 0xFF, 8, 0 },
    { 4, "LIST", L_LIST, 1, 0, 0, 0 },      { 6, "NOLIST", L_NOLIST, 1, 0, 0, 0 },
    { 4, "CODE", L_CODE, 1, 0, 0, 0 },      { 6, "NOCODE", L_NOCODE, 1, 0, 0, 0 },
    { 4, "XREF", 0, 0, 1, 1, 1 },           { 6, "NOXREF", 0, 0, 1, 0, 1 },
    { 7, "SYMBOLS", 0, 0, 2, 1, 2 },        { 9, "NOSYMBOLS", 0, 0, 2, 0, 2 },
    { 5, "DEBUG", 0, 0, 3, 1, 3 },          { 7, "NODEBUG", 0, 0, 3, 0, 3 },
    { 6, "PAGING", 0, 0, 4, 1, 4 },         { 8, "NOPAGING", 0, 0, 4, 0, 4 },
    { 10, "PAGELENGTH", 0, 0, 0xFF, 0, 5 }, { 9, "PAGEWIDTH", 0, 0, 0xFF, 1, 6 },
    { 4, "DATE", 0, 0, 0xFF, 2, 7 },        { 5, "TITLE", 0, 0, 0xFF, 3, 8 },
    { 5, "EJECT", L_EJECT, 1, 0, 0, 0 },    { 10, "LEFTMARGIN", 0, 1, 0xFF, 4, 0 },
    { 6, "OBJECT", 0, 0, 0xFF, 5, 9 },      { 8, "NOOBJECT", 0, 0, 0xFF, 9, 9 },
    { 8, "OPTIMIZE", 0, 0, 6, 1, 10 },      { 10, "NOOPTIMIZE", 0, 0, 6, 0, 10 },
    { 7, "INCLUDE", 0, 1, 0xFF, 6, 0 },     { 9, "WORKFILES", 0, 0, 0xFF, 10, 11 },
    { 9, "INTVECTOR", 0, 0, 0xFF, 11, 12 }, { 11, "NOINTVECTOR", 0, 0, 0xFF, 12, 12 },
    { 5, "IXREF", 0, 0, 0xFF, 13, 13 },     { 7, "NOIXREF", 0, 0, 0xFF, 14, 13 },
    { 4, "SAVE", 0, 1, 0xFF, 15, 0 },       { 7, "RESTORE", 0, 1, 0xFF, 16, 0 },
    { 3, "SET", 0, 1, 0xFF, 17, 0 },        { 5, "RESET", 0, 1, 0xFF, 18, 0 },
    { 2, "IF", 0, 1, 0xFF, 19, 0 },         { 6, "ELSEIF", 0, 1, 0xFF, 20, 0 },
    { 4, "ELSE", 0, 1, 0xFF, 20, 0 },       { 5, "ENDIF", 0, 1, 0xFF, 21, 0 },
    { 4, "COND", 0, 1, 0xFF, 22, 0 },       { 6, "NOCOND", 0, 1, 0xFF, 23, 0 },
    { 10, "MAKEDEPEND", 0, 0, 0xFF, 24, 0 } };


byte primaryCtrlSeen[14]; // C defaults to all false
static struct {
    byte code;
    byte list;
    byte cond;
    word leftMargin;
} saveStack[5];

static byte saveDepth;
static bool CODE = false;
static bool LIST = true;
static bool COND = true;
static char *curChP;
static byte chrClass;
static struct {
    word len; // allow long filenames on command line.
    char *str;
} optVal;

static word optNumValue;
static bool lstGiven;
static bool inIFpart;
static word skippingIfDepth;

// NxtCh()
// on input curChP points to current char and chrClass is set to the correspondingly
//
// if more chars on line returns next char - converting to uppercase
// and converting none printable chars to space
// end of line is at CR and if on command line also & the continuation marker
// also sets chrClass

static void NxtCh() {
    if (chrClass == CC_NEWLINE)
        return;
    curChP++;
    if (*curChP == '\r')
        chrClass = CC_NEWLINE;
    else if ((chrClass = cClass[*(uint8_t *)curChP]) == CC_NONPRINT)
        *curChP = ' ';
    //    if (*curChP >= 'a')
    //        *curChP &= 0x5f;
} /* NxtCh() */

static void BadCmdTail(byte err) {
    if (moreCmdLine != 0) // processing command line
        Fatal("ILLEGAL COMMAND TAIL SYNTAX OR VALUE");
    else
        Wr1SyntaxError(err);
}

static void UnknownCtrl() {
    if (moreCmdLine != 0) // processing command line
        Fatal("UNRECOGNIZED CONTROL IN COMMAND TAIL");
    else
        Wr1SyntaxError(ERR9); /* INVALID CONTROL */
}

static void SkipWhite() // also skips non print characters
{
    while (*curChP == ' ' || (moreCmdLine && *curChP == '\r')) // space and cmdline continuation
        NxtCh();
}

static void SkipToRPARorEOL() {
    while (*curChP != '\r' && *curChP != ')' && *curChP != '\n')
        NxtCh();
    if (*curChP == ')')
        NxtCh();
}

static void AcceptRP() {
    SkipWhite();
    if (*curChP != ')') {
        BadCmdTail(ERR12); /* INVALID CONTROL PARAMETER */
        SkipToRPARorEOL();
    } else
        NxtCh();
}

static void AcceptOptStrVal() {
    byte nesting = 0;

    SkipWhite();
    if (*curChP != '(')
        optVal.len = 0;
    else {
        NxtCh();
        optVal.str = curChP;
        for (;;) {
            if (chrClass == CC_NEWLINE || *curChP == '\'')
                break;
            if (*curChP == '(')
                nesting++;
            else if (*curChP == ')') {
                if (nesting-- == 0)
                    break;
            }
            NxtCh();
        }
        optVal.len = (word)(curChP - optVal.str);
        AcceptRP();
    }
} /* AcceptOptStrVal() */

static void AcceptFileName() {
    SkipWhite();
    if (*curChP != '(')
        optVal.len = 0;
    else {
        NxtCh();
        SkipWhite();
        optVal.str = curChP;
        while (*curChP != ')' && chrClass != CC_NEWLINE) // doesn't allow tab
            NxtCh();
        char *s = curChP;
        while (s > optVal.str && s[-1] == ' ') // trim trailing space
            s--;
        optVal.len = (word)(s - optVal.str);
        char *cstr = xmalloc(optVal.len + 1);
        memcpy(cstr, optVal.str, optVal.len);
        cstr[optVal.len] = '\0';

        optVal.str       = cstr;
        AcceptRP();
    }
}

static word Asc2Num(char *firstChP, char *lastChP, byte radix) {
    word num;
    unsigned trial; // use unsigned (>= 32 bit) to smiplify test for overflow
    byte digit;

    if (lastChP < firstChP || radix == 0)
        return 0xffff;

    num = 0;
    while (firstChP <= lastChP) {
        if (cClass[*(pointer)firstChP] <= CC_DECDIGIT)
            digit = *firstChP - '0';
        else if (cClass[*(pointer)firstChP] < CC_ALPHA)
            digit = *firstChP - '7';
        else
            return 0xffff;
        if (digit >= radix)
            return 0xffff;
        //        if ((trial = num * radix + digit) < digit || ((word)(num * radix + digit) - digit)
        //        / radix != num)
        if ((trial = num * radix + digit) > 0xffff)
            return 0xffff;
        num = (word)trial;
        firstChP++;
    }
    return num;
}

static byte ChkRadix(char **pLastCh) {
    byte *p;

    p = (byte *)*pLastCh;
    if (cClass[*p] <= CC_DECDIGIT)
        return 10;
    --*pLastCh;
    if (*p == 'B')
        return 2;
    if (*p == 'Q' || *p == 'O')
        return 8;
    if (*p == 'H')
        return 16;
    if (*p == 'D')
        return 10;
    else
        return 0;
}

static word ParseNum() {
    char *firstCh, *lastCh;
    byte radix;

    NxtCh();
    SkipWhite();
    firstCh = curChP;
    while (chrClass <= CC_ALPHA)
        NxtCh();
    lastCh = curChP - 1;
    SkipWhite();
    radix = ChkRadix(&lastCh);
    return Asc2Num(firstCh, lastCh, radix);
}

static void GetOptNumVal() {
    SkipWhite();
    optNumValue = 0;
    if (*curChP != '(')
        BadCmdTail(ERR11);
    else {
        optNumValue = ParseNum();
        AcceptRP();
    }
}

static void GetOptStr() {
    optVal.str = curChP;
    while (*curChP != ' ' && *curChP != '(' && chrClass != CC_NEWLINE &&
           *curChP != '\r') // \r is cmdline continuation
        NxtCh();
    optVal.len = (byte)(curChP - optVal.str);
}

static void ParseId(byte maxLen) {
    static char pstr[33];

    optVal.str = pstr + 1;
    optVal.len = 0;
    SkipWhite();
    if (chrClass == CC_HEXCHAR || chrClass == CC_ALPHA)         // A-Z
        while (chrClass <= CC_ALPHA || chrClass == CC_DOLLAR) { // 0-9 A-Z $
            if (chrClass != CC_DOLLAR && optVal.len <= maxLen) {
                pstr[optVal.len + 1] = toupper(*(uint8_t *)curChP);
                optVal.len++;
            }
            NxtCh();
        }
    pstr[0] = optVal.len > maxLen ? maxLen : optVal.len;
}

static void GetVar() {
    char *tmp;
    tmp = curChP - 1;
    ParseId(31);
    if (optVal.len == 0) {
        infoIdx  = BADINFO;
        info     = NULL;
        curChP   = tmp;
        chrClass = 0;
        NxtCh();
        return;
    }
    if (optVal.len > 31) {
        optVal.len--;
        BadCmdTail(ERR184); /* CONDITIONAL COMPILATION PARAMETER NAME TOO LONG */
    }
    Lookup((pstr_t *)(optVal.str - 1));
    if (High(symtab[curSym].infoIdx) == 0xFF) { /* special */
        infoIdx  = BADINFO;
        info     = NULL;
        curChP   = tmp;
        chrClass = 0;
        NxtCh();
        return;
    }

    FindScopedInfo(1);
}

// return logical operator
// none (0), OR (1), AND (2), XOR (3), bad (4)
static byte GetLogical() {
    ParseId(3);
    if (optVal.len == 0 && chrClass == CC_NEWLINE)
        return 0;
    if (optVal.len == 2) {
        if (strncmp((char *)optVal.str, "OR", 2) == 0)
            return 1;
    } else if (optVal.len == 3) {
        if (strncmp((char *)optVal.str, "AND", 3) == 0)
            return 2;
        else if (strncmp((char *)optVal.str, "XOR", 3) == 0)
            return 3;
    }
    BadCmdTail(ERR185); /* MISSING OPERATOR IN CONDITIONAL COMPILATION Expression */
    SkipToRPARorEOL();
    return 4;
}

// returns test operation
// = (1), < (2), <= (3), > (4), >= (5), <> (6)
static byte GetTest() {
    byte test;

    test = 0;
    SkipWhite();
    if (*curChP == '<') {
        NxtCh();
        if (*curChP == '>') {
            NxtCh();
            return 6;
        }
        test = 2;
    } else if (*curChP == '>') {
        NxtCh();
        test = 4;
    }
    if (*curChP == '=') {
        NxtCh();
        test++;
    }
    return test;
}

static byte ChkNot() // checks for potentially multiple NOT prefixes
{
    byte notStatus;
    char *tmp;

    notStatus = 0;

    while ((1)) {
        tmp = curChP - 1;
        ParseId(3);
        if (optVal.len != 3 || strncmp((char *)optVal.str, "NOT", 3) != 0) {
            curChP = tmp;
            return notStatus;
        }
        notStatus = ~notStatus;
    }
}

static word GetIfVal() {
    word val;

    chrClass = 0;
    NxtCh();
    SkipWhite();
    if (chrClass < CC_HEXCHAR) { // starts with a digit
        curChP--;
        val = ParseNum();
        if (val > 255)
            BadCmdTail(ERR186); /* INVALID CONDITIONAL COMPILATION CONSTANT, TOO LARGE */
        return Low(val);
    } else {
        GetVar(); // variable
        if (infoIdx == BADINFO) {
            BadCmdTail(ERR180); /* MISSING OF INVALID CONDITIONAL COMPILATION PARAMETER */
            SkipToRPARorEOL();
            return 256; // error value
        } else if (infoIdx == 0)
            return 0; // default to false if name not defined
        else
            return info->condFlag; // else return current value
    }
}

static bool ParseIfCond() {
    byte andFactor, orFactor, xorFactor;
    word val1, val2;
    byte relOp, not1, not2;

    andFactor = 0xff;
    orFactor  = 0;
    xorFactor = 0;

    while ((1)) {
        not1 = ChkNot();
        val1 = GetIfVal();
        if (val1 > 255) // illegal name
            return true;

        relOp = GetTest();
        if (relOp > 0) {     // we have a rel op
            not2 = ChkNot(); // check for not prefixes
            val2 = GetIfVal();
            if (val2 > 255)
                return true; // illegal name
            val2 = not2 ^ val2;
            switch (relOp) {
            case 1:
                val1 = val1 == val2 ? 0xff : 0;
                break;
            case 2:
                val1 = val1 < val2 ? 0xff : 0;
                break;
            case 3:
                val1 = val1 <= val2 ? 0xff : 0;
                break;
            case 4:
                val1 = val1 > val2 ? 0xff : 0;
                break;
            case 5:
                val1 = val1 >= val2 ? 0xff : 0;
                break;
            case 6:
                val1 = val1 != val2 ? 0xff : 0;
                break;
            }
        }
        val1      = (not1 ^ val1) & andFactor;
        andFactor = 0xff;
        switch (GetLogical()) {
        case 0: // apply final OR/XOR and convert to bool using PLM rule
            return ((val1 | orFactor) ^ xorFactor) & 1;
        case 1:
            orFactor  = (val1 | orFactor) ^ xorFactor;
            xorFactor = 0;
            break;
        case 2:
            andFactor = (byte)val1;
            break;
        case 3:
            xorFactor = (val1 | orFactor) ^ xorFactor;
            orFactor  = 0;
            break;
        case 4:
            return true; // bad so assume true
        }
    }
}

static void OptPageLen() {
    GetOptNumVal();
    if (optNumValue < 4 || optNumValue == 0xFFFF)
        BadCmdTail(ERR91); /* ILLEGAL PAGELENGTH CONTROL VALUE */
    else
        SetPageLen(optNumValue - 3);
}

static void OptPageWidth() {
    GetOptNumVal();
    if (optNumValue < 60 || optNumValue > 132)
        BadCmdTail(ERR92); /* ILLEGAL PAGEWIDTH CONTROL VALUE */
    else
        SetPageWidth(optNumValue);
}

static void OptDate() {
    AcceptOptStrVal();
    SetDate(optVal.str, (byte)optVal.len);
}

// promoted to file level
static bool LocalSetTitle() {
    byte len;
    NxtCh();
    if (*curChP != '\'')
        return false;
    len = 0;
    while (1) {
        NxtCh();
        if (*curChP == '\r' || *curChP == '\n')
            break;
        if (*curChP == '\'') {
            NxtCh();
            if (*curChP != '\'')
                break;
        }
        if (len <= 59) {
            TITLE[len++] = *curChP;
        }
    }
    if (len != 0)
        TITLELEN = len;
    else {
        TITLELEN = 1;
        TITLE[0] = ' ';
    }
    if (*curChP != ')')
        return false;
    else {
        NxtCh();
        return true;
    }
}

static void OptTitle() {
    SkipWhite();
    if (*curChP != '(')
        BadCmdTail(ERR11); /* MISSING CONTROL PARAMETER */
    else if (!LocalSetTitle()) {
        BadCmdTail(ERR12); /* INVALID CONTROL PARAMETER */
        SkipToRPARorEOL();
    }
}

static void OptLeftMargin() {
    GetOptNumVal();
    if (optNumValue == 0) {
        BadCmdTail(ERR12); /* INVALID CONTROL PARAMETER */
        optNumValue = 1;
    } else if (optNumValue == 0xFFFF) {
        BadCmdTail(ERR12); /* INVALID CONTROL PARAMETER */
        return;
    }
    LEFTMARGIN = optNumValue;
}

static void OptIXRef() {
    AcceptFileName();
    if (optVal.len) {
        free(ixiFileName);
        ixiFileName = optVal.str;
    }

    IXREF = true;
}

static void OptMakeDepend() {
    AcceptFileName();
    if (optVal.len) {
        free(depFileName);
        depFileName = optVal.str;
    }
    DEPEND = true;
}

static void OptObject() {
    AcceptFileName();
    if (optVal.len) {
        free(objFileName);
        objFileName = optVal.str;
    }
    OBJECT = true;
}

static void OptInclude() {
    AcceptFileName();
    if (optVal.len == 0) {
        BadCmdTail(ERR15); /* MISSING INCLUDE CONTROL PARAMETER */
        return;
    }

    if (srcFileIdx >= 5)
        Wr1SyntaxError(ERR13); /* LIMIT EXCEEDED: INCLUDE NESTING */
    else {
        int includeIdx = newInclude(optVal.str);
        if (includes[includeIdx] != optVal.str)
            free(optVal.str);
        srcFileTable[srcFileIdx++] = srcFil;
        OpenF(&srcFil, "SOURCE", includes[includeIdx], "rt");
        Wr1Byte(L_INCLUDE);
        Wr1Word(includeIdx);
    }
    SkipWhite();
    if (*curChP != '\r' && *curChP != '\n')
        BadCmdTail(ERR14); /* INVALID CONTROL FORMAT, INCLUDE not last() CONTROL */
}

static void OptPrint() {
    AcceptFileName();
    if (lstGiven)
        BadCmdTail(ERR16); /* ILLEGAL PRINT CONTROL */
    else if (optVal.len) {
        free(lstFileName);
        lstFileName = optVal.str;
        isList      = true;
    }
    PRINT    = true;
    lstGiven = true;
}

static void OptIntVector() {
    byte vecNum;
    word vecLoc;

    SkipWhite();
    if (*curChP != '(')
        BadCmdTail(ERR11); /* MISSING CONTROL PARAMETER */
    else {
        vecNum = (byte)ParseNum();
        if (vecNum != 4 && vecNum != 8) {
            BadCmdTail(ERR176); /* INVALID INTVECTOR INTERVAL VALUE */
            SkipToRPARorEOL();
        } else if (*curChP != ',') {
            BadCmdTail(ERR177); /* INVALID INTVECTOR LOCATION VALUE */
            SkipToRPARorEOL();
        } else {
            vecLoc = ParseNum();
            if (vecLoc > 0xFFE0 || vecLoc % (vecNum * 8) != 0 || *curChP != ')') {
                BadCmdTail(ERR177); /* INVALID INTVECTOR LOCATION VALUE */
                SkipToRPARorEOL();
            } else {
                NxtCh();
                intVecNum = vecNum;
                intVecLoc = vecLoc;
            }
        }
    }
}

// lifted to filescope

static bool isOK;
static pointer lFname;

static void AcceptRangeChrs(byte lch, byte hch) {
    if (isOK) {
        if (*curChP < lch || hch < *curChP) {
            BadCmdTail(ERR12); /* INVALID CONTROL PARAMETER */
            SkipToRPARorEOL();
            isOK = false;
        } else {
            *lFname++ = *curChP;
            NxtCh();
        }
    }
}

static bool AcceptDrive(pointer fname, byte follow) {
    lFname = fname;

    isOK   = true;
    SkipWhite();
    AcceptRangeChrs(':', ':');
    AcceptRangeChrs('F', 'F');
    AcceptRangeChrs('0', '9');
    AcceptRangeChrs(':', ':');
    SkipWhite();
    AcceptRangeChrs(follow, follow);
    return isOK;
}

static void OptWorkFiles() {
    byte wrkFiles1[5], wrkFiles2[5];

    SkipWhite();
    if (*curChP != '(')
        BadCmdTail(ERR11); /* MISSING CONTROL PARAMETER */
    else {
        NxtCh();
        if (!AcceptDrive(wrkFiles1, ','))
            return;
        if (!AcceptDrive(wrkFiles2, ')'))
            return;
        BadCmdTail(ERR8); // INVAILD CONTROL FORMAT
    }
}

static void OptSave() {
    if (saveDepth >= 5)
        BadCmdTail(ERR187); /* LIMIT EXCEEDED: NUMBER OF SAVE LEVELS > 5 */
    else {
        saveStack[saveDepth].code         = CODE;
        saveStack[saveDepth].list         = LIST;
        saveStack[saveDepth].cond         = COND;
        saveStack[saveDepth++].leftMargin = LEFTMARGIN;
    }
}

static void OptRestore() {
    if (saveDepth == 0)
        BadCmdTail(ERR188); /* MISPLACED RESTORE OPTION */
    else {
        saveDepth  = saveDepth - 1;
        CODE       = saveStack[saveDepth].code;
        LIST       = saveStack[saveDepth].list;
        COND       = saveStack[saveDepth].cond;
        LEFTMARGIN = saveStack[saveDepth].leftMargin;
        if (CODE)
            Wr1Byte(L_CODE);
        else
            Wr1Byte(L_NOCODE);
        if (LIST)
            Wr1Byte(L_LIST);
        else
            Wr1Byte(L_NOLIST);
    }
}

static void OptSetReset(bool isSet) {
    word val;

    SkipWhite();
    if (*curChP != '(')
        BadCmdTail(ERR11); /* MISSING CONTROL PARAMETER */
    else
        while ((1)) {
            NxtCh();
            GetVar();
            if (infoIdx == BADINFO) {
                BadCmdTail(ERR180); /* MISSING or INVALID CONDITIONAL COMPILATION PARAMETER */
                SkipToRPARorEOL();
                return;
            }
            if (infoIdx == 0)
                CreateInfo(1, CONDVAR_T, curSym);
            SkipWhite();
            if (*curChP == '=' && isSet) {
                val = ParseNum();
                if (val > 255) {
                    BadCmdTail(ERR181); /* MISSING or INVALID CONDITIONAL COMPILATION CONSTANT */
                    SkipToRPARorEOL();
                    return;
                }
                info->condFlag = (byte)val;
            } else
                info->condFlag = isSet;
            if (*curChP != ',') {
                AcceptRP();
                return;
            }
        }
}

static void OptIf() {
    ifDepth++;
    if (!ParseIfCond()) { // if not true so skip
        skippingCOND    = true;
        inIFpart        = true;
        skippingIfDepth = ifDepth; // record depth for unwind
        if (!COND && LIST)         // if COND false and currently listing - surpress listing
            Wr1Byte(L_NOLIST);
    }
}

static void OptElseElseIf() {
    if (ifDepth == 0)       // no corresponding if!!
        BadCmdTail(ERR182); /* MISPLACED else or ELSEIF OPTION */
    else {
        skippingCOND    = true;
        inIFpart        = false;
        skippingIfDepth = ifDepth;
        if (!COND)
            if (LIST)
                Wr1Byte(L_NOLIST);
    }
    while (*curChP != '\r') {
        NxtCh();
    }
}

static void OptEndIf() {
    if (ifDepth == 0)       // no if
        BadCmdTail(ERR183); /* MISPLACED ENDIF OPTION */
    else
        ifDepth--;
}

static void ParseControlExtra() {
    switch (tknFlagsP->controlVal) {
    case 0:
        OptPageLen();
        break;
    case 1:
        OptPageWidth();
        break;
    case 2:
        OptDate();
        break;
    case 3:
        OptTitle();
        break;
    case 4:
        OptLeftMargin();
        break;
    case 5:
        OptObject();
        break;
    case 6:
        OptInclude();
        break;
    case 7:
        OptPrint();
        break;
    case 8:
        PRINT = false;
        break;
    case 9:
        OBJECT = false;
        break;
    case 10:
        OptWorkFiles();
        break;
    case 11:
        OptIntVector();
        break;
    case 12:
        intVecNum = 0;
        break;
    case 13:
        OptIXRef();
        break;
    case 14:
        IXREF = false;
        break;
    case 15:
        OptSave();
        break;
    case 16:
        OptRestore();
        break;
    case 17:
        OptSetReset(true);
        break;
    case 18:
        OptSetReset(false);
        break;
    case 19:
        OptIf();
        break;
    case 20:
        OptElseElseIf();
        break;
    case 21:
        OptEndIf();
        break;
    case 22:
        COND = true;
        break;
    case 23:
        COND = false;
        break;
    case 24:    // MAKEDEPEND
        OptMakeDepend();
        break;

    }
}

static void FindOption() {
    for (int i = 0; i < sizeof(optTable) / sizeof(optTable[0]); i++) {
        if (optTable[i].optLen == optVal.len &&
            strnicmp(optVal.str, optTable[i].optName, optVal.len) == 0) {
            tknFlagsP = &optTable[i];
            return;
        }
    }
    tknFlagsP = 0;
}

static void SkipControlParam() {
    SkipWhite();
    if (*curChP == '(') {
        NxtCh();
        SkipToRPARorEOL();
    }
}

static void ParseControl() {
    GetOptStr();
    if (optVal.len == 0) {
        BadCmdTail(ERR8); /* INVALID CONTROL FORMAT */
        SkipControlParam();
    } else {
        FindOption();
        if (tknFlagsP == 0) {
            UnknownCtrl();
            SkipControlParam();
        } else {
            if (!tknFlagsP->primary) {
                if (isNonCtrlLine) {
                    Wr1SyntaxError(
                        ERR10); /* ILLEGAL USE OF PRIMARY CONTROL AFTER NON-CONTROL LINE */
                    SkipControlParam();
                    return;
                } else if (primaryCtrlSeen[tknFlagsP->primaryId]) {
                    BadCmdTail(ERR95); /* ILLEGAL RESPECIFICATION OF PRIMARY CONTROL IGNORED */
                    SkipControlParam();
                    return;
                } else
                    primaryCtrlSeen[tknFlagsP->primaryId] = true;
            }

            if (tknFlagsP->controlId != 0xFF) // simple control
            {
                if (tknFlagsP->tokenId == 0) // simple update to control value only
                    controls[tknFlagsP->controlId] = tknFlagsP->controlVal;
                else { // write control to lexical stream
                    Wr1Byte(tknFlagsP->tokenId);
                    if (tknFlagsP->tokenId == L_CODE) // update values for CODE and LIST
                        CODE = true;
                    else if (tknFlagsP->tokenId == L_NOCODE)
                        CODE = false;
                    else if (tknFlagsP->tokenId == L_LIST)
                        LIST = true;
                    else if (tknFlagsP->tokenId == L_NOLIST)
                        LIST = false;
                }
            } else
                ParseControlExtra(); // not a simple control do further processing
        }
    }
}

void ParseControlLine(char *pch) {
    curChP   = pch;
    chrClass = 0;

    while (1) {
        SkipWhite();
        if (chrClass == CC_NEWLINE)
            return;
        ParseControl();
    }
}

static void ChkEndSkipping(byte *pch) {
    curChP = (char *)pch;
    if (*curChP == '$') {
        chrClass = 0;
        NxtCh();
        SkipWhite();
        GetOptStr();
        if (optVal.len == 2 && strnicmp(optVal.str, "IF", 2) == 0) // nested IF
            ifDepth++;
        else if (optVal.len == 5 && strnicmp(optVal.str, "ENDIF", 5) == 0) {
            if (--ifDepth < skippingIfDepth) // end of skipping
                skippingCOND = false;
        } else if (skippingIfDepth == ifDepth &&
                   inIFpart) { // else/elseif at same level as initial IF
            if (optVal.len == 4 && strnicmp(optVal.str, "ELSE", 4) == 0)
                skippingCOND = false; // else so now not skipping
            else if (optVal.len == 6 && strnicmp(optVal.str, "ELSEIF", 6) == 0)
                skippingCOND = !ParseIfCond(); // still skipping if condition false
        }
        if (!skippingCOND) // no longer skipping so record any change of listing status
            if (!COND)
                if (LIST)
                    Wr1Byte(L_LIST);
    }
}

// converted from plm0c.asm
bool trunc;

void GNxtCh() {
    inChrP++;
    while (*inChrP == '\n')
        GetLin();
    // hack as original code assumed \r\n
    nextCh = *inChrP;
}

// get next char into *inChrP
// char is 0x81 on EOF, cr is discarded

static byte InGetC() {
    int c;
    while ((c = fgetc(srcFil.fp)) == '\r')
        ;
    return *inChrP = c == EOF ? ISISEOF : c & 0x7f;
}

static void RSrcLn() {
    int i = MAXLINE; // max chars;

    for (;;) {
        if (*inChrP == '\n' || *inChrP == ISISEOF)
            return;   // got a line (or eof)
        if (--i == 0) // not reached line limit
            break;
        inChrP++;
        InGetC();
    }
    trunc = true;                                  // oops too long
    while (InGetC() != '\n' && *inChrP != ISISEOF) // junk rest of long line
        ;
}

static void GetSrcLine() {
    Wr1LineInfo(); // make sure linfo is written
    inChrP = lineBuf;
    InGetC();
    if (*inChrP == ISISEOF) /* eof */
        return;
    trunc = false; // reset truncation flag
    RSrcLn();      // get line
    *inChrP++ = '\r';
    *inChrP   = '\n';
    inChrP    = lineBuf;                 // point to start
    linfo.lineCnt++;                     // account for new line
    linfo.stmtCnt = linfo.blkCnt = 0;    // no stmt or blkcnt for this line yet
    lineInfoToWrite              = true; // note linfo needs to be written
}

static void GetCodeLine() {
    byte *startOfLine;

    while (1) {
        GetSrcLine();
        if (*inChrP != ISISEOF) {
            startOfLine = inChrP + LEFTMARGIN - 1; // skip over leftmargin (tabs are 1 char !!)
            while (inChrP < startOfLine) {
                if (*inChrP == '\r')
                    return; // effectively a blank line
                inChrP++;
            }
            if (skippingCOND) // is skipping check to see if finished
                ChkEndSkipping(inChrP);
            else if (*inChrP == '$') { // control line
                Wr1Byte(L_STMTCNT);    // note zero stmts on this line
                Wr1Word(0);
                if (trunc) {
                    Wr1SyntaxError(ERR86); /* LIMIT EXCEEDED: SOURCE LINE LENGTH */
                    trunc = false;
                }
                ParseControlLine((char *)inChrP + 1); // process controls
            } else {
                // first none control line (even a comment) stops primary controls
                isNonCtrlLine = true;
                return; // we have a line
            }
        } else if (srcFileIdx == 0) {   // EOF at end of main file
            if (ifDepth)                // oops we are nested (error code seems to be incorrect)
                Wr1SyntaxError(ERR188); /* MISPLACED RESTORE OPTION */
            inChrP = (pointer) "/*'/**/EOF   "; // string to make sure any comments, strings are
                                                // closed and EOF
            return;
        } else {
            CloseF(&srcFil); // close nested file
            srcFil = srcFileTable[--srcFileIdx];
        }
    }
}

static void GetLin() {
    if (macroDepth != 0) {              // last line was a nested lit expansion
        infotab[macroIdx].type = LIT_T; // reset info entry to LIT_T from MACRO_T
        macroIdx               = macroPtrs[macroDepth].macroIdx;
        inChrP = macroPtrs[macroDepth--].text; // get the curent loc in the expansion
    } else
        GetCodeLine();
}