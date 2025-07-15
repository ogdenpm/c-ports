/****************************************************************************
 *  plm0b.c: part of the C port of Intel's ISIS-II plm80                    *
 *  The original ISIS-II application is Copyright Intel                     *
 *                                                                          *
 *  Re-engineered to C by Mark Ogden <mark.pm.ogden@btinternet.com>         *
 *                                                                          *
 *  It is released for academic interest and personal use only              *
 ****************************************************************************/
#include "os.h"
#include "plm.h"
#include <ctype.h>
#include <stdlib.h>
#include <time.h>

#define BADINFO (infotab + MAXINFO + 100)   // after table and keywords

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
} *tknFlagsP, optTable[] = { { 5, "PRINT", 0, 0, 0xFF, 7, 0 },
                             { 7, "NOPRINT", 0, 0, 0xFF, 8, 0 },
                             { 4, "LIST", T1_LIST, 1, 0, 0, 0 },
                             { 6, "NOLIST", T1_NOLIST, 1, 0, 0, 0 },
                             { 4, "CODE", T1_CODE, 1, 0, 0, 0 },
                             { 6, "NOCODE", T1_NOCODE, 1, 0, 0, 0 },
                             { 4, "XREF", 0, 0, 1, 1, 1 },
                             { 6, "NOXREF", 0, 0, 1, 0, 1 },
                             { 7, "SYMBOLS", 0, 0, 2, 1, 2 },
                             { 9, "NOSYMBOLS", 0, 0, 2, 0, 2 },
                             { 5, "DEBUG", 0, 0, 3, 1, 3 },
                             { 7, "NODEBUG", 0, 0, 3, 0, 3 },
                             { 6, "PAGING", 0, 0, 4, 1, 4 },
                             { 8, "NOPAGING", 0, 0, 4, 0, 4 },
                             { 10, "PAGELENGTH", 0, 0, 0xFF, 0, 5 },
                             { 9, "PAGEWIDTH", 0, 0, 0xFF, 1, 6 },
                             { 4, "DATE", 0, 0, 0xFF, 2, 7 },
                             { 5, "TITLE", 0, 0, 0xFF, 3, 8 },
                             { 5, "EJECT", T1_EJECT, 1, 0, 0, 0 },
                             { 10, "LEFTMARGIN", 0, 1, 0xFF, 4, 0 },
                             { 6, "OBJECT", 0, 0, 0xFF, 5, 9 },
                             { 8, "NOOBJECT", 0, 0, 0xFF, 9, 9 },
                             { 8, "OPTIMIZE", 0, 0, 6, 1, 10 },
                             { 10, "NOOPTIMIZE", 0, 0, 6, 0, 10 },
                             { 7, "INCLUDE", 0, 1, 0xFF, 6, 0 },
                             { 9, "WORKFILES", 0, 0, 0xFF, 10, 11 },
                             { 9, "INTVECTOR", 0, 0, 0xFF, 11, 12 },
                             { 11, "NOINTVECTOR", 0, 0, 0xFF, 12, 12 },
                             { 5, "IXREF", 0, 0, 0xFF, 13, 13 },
                             { 7, "NOIXREF", 0, 0, 0xFF, 14, 13 },
                             { 4, "SAVE", 0, 1, 0xFF, 15, 0 },
                             { 7, "RESTORE", 0, 1, 0xFF, 16, 0 },
                             { 3, "SET", 0, 1, 0xFF, 17, 0 },
                             { 5, "RESET", 0, 1, 0xFF, 18, 0 },
                             { 2, "IF", 0, 1, 0xFF, 19, 0 },
                             { 6, "ELSEIF", 0, 1, 0xFF, 20, 0 },
                             { 4, "ELSE", 0, 1, 0xFF, 20, 0 },
                             { 5, "ENDIF", 0, 1, 0xFF, 21, 0 },
                             { 4, "COND", 0, 1, 0xFF, 22, 0 },
                             { 6, "NOCOND", 0, 1, 0xFF, 23, 0 },
                             { 10, "MAKEDEPEND", 0, 0, 0xFF, 24, 15 } };

byte primaryCtrlSeen[15]; // C defaults to all false
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
static union {
    struct {
        word len; // allow long filenames on command line.
        char *str;
    };
    pstr_t *pstr;
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
} /* NxtCh() */

static void BadCmdTail(byte err) {
    if (moreCmdLine) // processing command line
        Fatal("ILLEGAL COMMAND TAIL SYNTAX OR VALUE");
    else
        Wr1SyntaxError(err);
}

static void UnknownCtrl() {
    if (moreCmdLine) // processing command line
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
        char *cstr = safeMalloc(optVal.len + 1);
        memcpy(cstr, optVal.str, optVal.len);
        cstr[optVal.len] = '\0';

        optVal.str       = cstr;
        AcceptRP();
    }
}

static word Asc2Num(char *firstChP, char *lastChP, byte radix) {

    if (lastChP < firstChP || radix == 0)
        return 0xffff;

    uint32_t num = 0; // 32 bit to simplify overflow test
    byte digit;
    for (; firstChP <= lastChP; firstChP++) {
        if (cClass[*(pointer)firstChP] == CC_DIGIT)
            digit = *firstChP - '0';
        else if (cClass[*(pointer)firstChP] == CC_HEXCHAR)
            digit = *firstChP - 'A' + 10;
        else
            return 0xffff;
        if (digit >= radix || (num = num * radix + digit) > 0xffff)
            return 0xffff;
    }
    return (word)num;
}

static byte ChkRadix(char **pLastCh) {
    byte *p;

    p = (byte *)*pLastCh;
    if (cClass[*p] == CC_DIGIT)
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
    optVal.pstr      = (pstr_t *)tokenStr;
    optVal.pstr->len = 0;
    SkipWhite();
    if (chrClass == CC_HEXCHAR || chrClass == CC_ALPHA || chrClass == CC_UNDERBAR) // A-Z or _
        while (chrClass <= CC_ALPHA || chrClass == CC_DOLLAR ||
               chrClass == CC_UNDERBAR) { // 0-9 A-Z $
            if (chrClass != CC_DOLLAR && optVal.pstr->len < maxLen)
                optVal.pstr->str[optVal.pstr->len++] = toupper(*(uint8_t *)curChP);
            NxtCh();
        }
}

static void GetVar() {
    char *tmp = curChP - 1;
    ParseId(32);
    if (optVal.pstr->len == 0) {
        info     = BADINFO;
        curChP   = tmp;
        chrClass = 0;
        NxtCh();
        return;
    }
    if (optVal.pstr->len > 31) {
        optVal.pstr->len = 31;
        BadCmdTail(ERR184); /* CONDITIONAL COMPILATION PARAMETER NAME TOO LONG */
    }
    Lookup(optVal.pstr);
    if (curSym->infoChain >= infotab + MAXINFO) { /* special */
        info     = BADINFO;
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
    ParseId(4);
    if (optVal.pstr->len == 0 && chrClass == CC_NEWLINE)
        return 0;
    if (optVal.pstr->len == 2) {
        if (strncmp(optVal.pstr->str, "OR", 2) == 0)
            return 1;
    } else if (optVal.pstr->len == 3) {
        if (strncmp(optVal.pstr->str, "AND", 3) == 0)
            return 2;
        else if (strncmp(optVal.pstr->str, "XOR", 3) == 0)
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
    byte notStatus = 0;

    while ((1)) {
        char *tmp = curChP - 1;
        ParseId(4);
        if (optVal.pstr->len != 3 || strncmp(optVal.pstr->str, "NOT", 3) != 0) {
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
        if (info >= BADINFO) {
            BadCmdTail(ERR180); /* MISSING OF INVALID CONDITIONAL COMPILATION PARAMETER */
            SkipToRPARorEOL();
            return 256; // error value
        } else
            return info ? info->condFlag : 0; // return current value or false if not defined
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
            case 1: // =
                val1 = val1 == val2;
                break;
            case 2: // <
                val1 = val1 < val2;
                break;
            case 3: // <=
                val1 = val1 <= val2;
                break;
            case 4: // >
                val1 = val1 > val2;
                break;
            case 5: // >=
                val1 = val1 >= val2;
                break;
            case 6: // <>
                val1 = val1 != val2;
                break;
            }
            val1 = val1 ? 0xff : 0; // convert to PL/M true/false
        }
        val1      = (not1 ^ val1) & andFactor;
        andFactor = 0xff;
        switch (GetLogical()) {
        case 0: // apply final OR/XOR and convert to bool using PLM rule
            return ((val1 | orFactor) ^ xorFactor) & 1;
        case 1: // OR
            orFactor  = (val1 | orFactor) ^ xorFactor;
            xorFactor = 0;
            break;
        case 2: // AND
            andFactor = (byte)val1;
            break;
        case 3: // XOR
            xorFactor = (val1 | orFactor) ^ xorFactor;
            orFactor  = 0;
            break;
        case 4:          // Bad
            return true; // assume true
        }
    }
}

static void OptPageLen() {
    GetOptNumVal();
    if (optNumValue < 4 || optNumValue == 0xFFFF)
        BadCmdTail(ERR91); /* ILLEGAL PAGELENGTH CONTROL VALUE */
    else
        PAGELEN = optNumValue - 3;
}

static void OptPageWidth() {
    GetOptNumVal();
    if (optNumValue < 60 || optNumValue > 132)
        BadCmdTail(ERR92); /* ILLEGAL PAGEWIDTH CONTROL VALUE */
    else
        PWIDTH = (byte)optNumValue;
}

/*
    Parse DATE value
    Original variant is enclosed in ( ) and nesting of () is supported. Truncated to 10 chars (was 9)
    New variant has no explicit value and the current date/time in yyyy-mm-dd hh:mm:ss format is
    used as the value. In listings the time element is removed if there isn't sufficient room
*/

static void OptDate() {
    SkipWhite();
    if (*curChP != '(') {
        time_t nowTime;
        time(&nowTime);
        struct tm *now = localtime(&nowTime);
        DATELEN        = sprintf(DATE, "%04d-%02d-%02d %02d:%02d:%02d", now->tm_year + 1900,
                                 now->tm_mon + 1, now->tm_mday, now->tm_hour, now->tm_min, now->tm_sec);
    } else {
        byte nesting = 0;
        NxtCh();
        optVal.str = curChP;
        for (;;) {
            // new line or ' terminates
            if (chrClass == CC_NEWLINE || *curChP == '\r' || *curChP == '\'')
                break;
            if (*curChP == '(')     // not sure why nesting is supported for what was 9 char limit
                nesting++;
            else if (*curChP == ')') {
                if (nesting-- == 0)
                    break;
            }
            NxtCh();
        }
        optVal.len = curChP - optVal.str > 10 ? 10 : (word)(curChP - optVal.str);
        AcceptRP();
        memcpy(DATE, optVal.str, optVal.len);
        DATELEN          = optVal.len;
        DATE[optVal.len] = '\0';
    }
}

/*
    parse TITLE value
    value is enclosed in ( ) and has opening and closing single quotes
    two adjacent single quotes are treated as a single quote as part of the value
    the value is truncated to the first 60 characters
*/
static void OptTitle() {
    TITLELEN = 0;
    SkipWhite();
    if (*curChP != '(') {
        BadCmdTail(ERR11); /* MISSING CONTROL PARAMETER */
        return;
    }
    NxtCh();
    if (*curChP == '\'') {
        while (1) {
            NxtCh();
            if (*curChP == '\r' || *curChP == '\n') // end of line ('\r' for continuation line)
                break;
            if (*curChP == '\'') { // double '
                NxtCh();
                if (*curChP != '\'')
                    break;
            }
            if (TITLELEN < 60) {
                TITLE[TITLELEN++] = *curChP;
            }
        }
        if (*curChP == ')') {
            TITLE[TITLELEN] = '\0';
            NxtCh();
            return;
        }
    }
    TITLE[TITLELEN = 0] = '\0';
    BadCmdTail(ERR12); /* INVALID CONTROL PARAMETER */
    SkipToRPARorEOL();
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
    if (optVal.len) {   // use supplied name rather than auto generated name
        free(ixiFileName);
        ixiFileName = optVal.str;
    }

    IXREF = true;
}

static void OptMakeDepend() {
    AcceptFileName();
    if (optVal.len) {   // use supplied name rather than auto generated name
        free(depFileName);
        depFileName = optVal.str;
    }
    DEPEND = true;
}

static void OptObject() {
    AcceptFileName();
    if (optVal.len) {   // use supplied name rather than auto generated name
        free(objFileName);
        objFileName = optVal.str;
    }
    OBJECT = true;
}


/*
    parse INCLUDE  filename
    As filenames can be longer than the ISIS :Fx:nnnnnn.eee format
    The names are saved in an array of unique filenames and the index is passed
    between the various compiler phases. This array is also used to determine
    the makedepend dependencies
    However, no check is made on whether names alias each other e.g.
    :F0:file.plm, file.plm and FILE.PLM will be assumed to be different even if :F0: is
    defined to be the current directory
*/
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
        Wr1Byte(T1_INCLUDE);
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
            NxtCh();
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

static bool AcceptDrive(byte follow) {
    SkipWhite();
    if (curChP[0] == ':' && toupper(curChP[1]) == 'F' && isdigit(curChP[2]) && curChP[3] == ':') {
        curChP += 4;
        SkipWhite();
        if (*curChP == follow) {
            NxtCh();
            return true;
        }
    }

    return false;
}

static void OptWorkFiles() {

    SkipWhite();
    if (*curChP != '(')
        BadCmdTail(ERR11); /* MISSING CONTROL PARAMETER */
    else {
        NxtCh();
        if (!AcceptDrive(',') || !AcceptDrive(')')) {
            BadCmdTail(ERR12); /* INVALID CONTROL PARAMETER */
            SkipToRPARorEOL();
        }
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
        CODE       = saveStack[--saveDepth].code;
        LIST       = saveStack[saveDepth].list;
        COND       = saveStack[saveDepth].cond;
        LEFTMARGIN = saveStack[saveDepth].leftMargin;
        if (CODE)
            Wr1Byte(T1_CODE);
        else
            Wr1Byte(T1_NOCODE);
        if (LIST)
            Wr1Byte(T1_LIST);
        else
            Wr1Byte(T1_NOLIST);
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
            if (info >= BADINFO) {
                BadCmdTail(ERR180); /* MISSING or INVALID CONDITIONAL COMPILATION PARAMETER */
                SkipToRPARorEOL();
                return;
            }
            if (!info)
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
            Wr1Byte(T1_NOLIST);
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
                Wr1Byte(T1_NOLIST);
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
    case 24: // MAKEDEPEND
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
                    if (tknFlagsP->tokenId == T1_CODE) // update values for CODE and LIST
                        CODE = true;
                    else if (tknFlagsP->tokenId == T1_NOCODE)
                        CODE = false;
                    else if (tknFlagsP->tokenId == T1_LIST)
                        LIST = true;
                    else if (tknFlagsP->tokenId == T1_NOLIST)
                        LIST = false;
                }
            } else
                ParseControlExtra(); // not a simple control do further processing
        }
    }
}

void ParseControlLine(char *pch) {
    curChP   = pch - 1;
    chrClass = 0;

    NxtCh();
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
                    Wr1Byte(T1_LIST);
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
                Wr1Byte(T1_STMTCNT);    // note zero stmts on this line
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
    if (macroDepth != 0) {       // last line was a nested lit expansion
        macroInfo->type = LIT_T; // reset info entry to LIT_T from MACRO_T
        macroInfo       = macroPtrs[macroDepth].macroInfo;
        inChrP          = macroPtrs[macroDepth--].text; // get the curent loc in the expansion
    } else
        GetCodeLine();
}
