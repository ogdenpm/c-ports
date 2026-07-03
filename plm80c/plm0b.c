/****************************************************************************
 *  plm0b.c: part of the C port of Intel's ISIS-II plm80                    *
 *  The original ISIS-II application is Copyright Intel                     *
 *                                                                          *
 *  Re-engineered to C by Mark Ogden <mark.pm.ogden@btinternet.com>         *
 *                                                                          *
 *  It is released for academic interest and personal use only              *
 ****************************************************************************/

/*
 * plm0b.c — Compiler control-line and command-tail parser
 *
 * Responsibilities
 * ----------------
 *  • Parse compiler control lines (lines beginning with '$') and the
 *    command-tail supplied when the compiler is invoked.
 *  • Maintain the active values of all compiler controls (LIST, CODE,
 *    COND, PRINT, OBJECT, IXREF, DEPEND, PAGELENGTH, PAGEWIDTH, DATE,
 *    TITLE, LEFTMARGIN, OPTIMIZE, INTVECTOR, WORKFILES, MAKEDEPEND, etc.).
 *  • Handle the SAVE/RESTORE stack that lets controls be pushed and
 *    popped up to five levels deep.
 *  • Implement conditional-compilation controls: IF, ELSEIF, ELSE,
 *    ENDIF, COND, NOCOND, SET and RESET.
 *  • Open INCLUDE files and push the source-file stack.
 *  • Drive the line-level input loop (GetCodeLine / GetLin) that feeds
 *    source characters to the lexer.
 *
 * Control-option dispatch
 * -----------------------
 *  optTable[]          – static descriptor table; one entry per recognised
 *                        control keyword.  Each entry records the keyword
 *                        length, name, optional lexical token id, whether
 *                        the control is primary (must appear before the
 *                        first non-control line), a controls[] slot index
 *                        (0xFF = no simple slot), a controlVal used by the
 *                        ParseControlExtra() switch, and a primaryId index
 *                        into primaryCtrlSeen[].
 *  FindOption()        – linear scan of optTable[] for the token just read.
 *  ParseControl()      – read one option name, locate it in optTable[], then
 *                        either update controls[] directly (simple controls)
 *                        or call ParseControlExtra() for those that need
 *                        additional argument parsing.
 *  ParseControlLine()  – split a '$'-prefixed source line into individual
 *                        controls and dispatch each one via ParseControl().
 *
 * Input helpers
 * -------------
 *  NxtCh()             – advance curChP by one, set chrClass; stops at CR.
 *  SkipWhite()         – skip spaces (and CR on the command line).
 *  ParseNum()          – read a PLM-style numeric literal (decimal, hex,
 *                        octal or binary suffix notation).
 *  ParseId()           – collect an identifier into tokenStr / optVal.pstr.
 *  GetOptStr()         – collect a bare option-name token (no parens).
 *  AcceptFileName()    – collect a filename enclosed in parentheses,
 *                        trimming surrounding whitespace; allocates a
 *                        NUL-terminated heap copy.
 *  AcceptRP()          – consume optional whitespace then a closing ')'.
 *  SkipToRPARorEOL()   – error-recovery: discard input up to ')' or EOL.
 *
 * Conditional-compilation expression evaluator
 * ---------------------------------------------
 *  GetVar()            – look up a CONDVAR symbol; sets info to BADINFO on
 *                        failure.
 *  GetIfVal()          – read one operand: a numeric constant (0-255) or a
 *                        CONDVAR reference.
 *  GetTest()           – read a relational operator (=, <, <=, >, >=, <>).
 *  ChkNot()            – consume zero or more NOT prefixes, returning a
 *                        bitmask to XOR into the operand.
 *  GetLogical()        – read AND / OR / XOR or end-of-expression.
 *  ParseIfCond()       – full expression evaluator; returns the boolean
 *                        result of the IF/ELSEIF condition.
 *
 * Source-line input
 * -----------------
 *  InGetC()            – read one raw character from srcFil, mapping EOF to
 *                        ISISEOF and discarding '\r'.
 *  RSrcLn()            – read the rest of a source line into lineBuf;
 *                        sets trunc if the line exceeds MAXLINE characters.
 *  GetSrcLine()        – write line-info record, fill lineBuf via RSrcLn()
 *                        and update linfo counters.
 *  GetCodeLine()       – main source-line driver: skips control lines
 *                        (dispatching each to ParseControlLine), handles
 *                        conditional-compilation skipping via
 *                        ChkEndSkipping(), manages INCLUDE-file nesting,
 *                        and synthesises a synthetic EOF token when the
 *                        outermost file is exhausted.
 *  GetLin()            – top-level "get next line" entry point called by the
 *                        lexer; returns the next expansion line from a
 *                        LITERAL macro if one is active, otherwise calls
 *                        GetCodeLine().
 *  GNxtCh()            – advance inChrP past '\n' line boundaries, calling
 *                        GetLin() as needed; exported to the lexer.
 */

#include "os.h"
#include "plm.h"
#include <ctype.h>
#include <stdlib.h>
#include <time.h>

#define BADINFO (infotab + MAXINFO + 100) // after table and keywords

// forward references
static uint8_t InGetC();
static void GetLin();

/*
 * optTable – compiler control descriptor table
 *
 * Each entry describes one recognised compiler control keyword:
 *
 *  optLen      length of the keyword string (no NUL).
 *  optName     keyword string (not NUL-terminated, compared via strnicmp).
 *  tokenId     if non-zero, the T1_xxx token written to the lexical stream
 *              when this control is processed; zero means the control only
 *              updates an internal variable or needs extra argument parsing.
 *  primary     true  → the control is primary and must appear before the
 *                       first non-control source line.
 *              false → the control may appear anywhere.
 *  controlId   index into the controls[] array for simple boolean/value
 *              controls; 0xFF means the control is not a simple array slot
 *              and ParseControlExtra() handles it instead.
 *  controlVal  argument passed to the ParseControlExtra() switch to select
 *              the handler (e.g. 0=PAGELENGTH, 1=PAGEWIDTH, …).
 *  primaryId   index into primaryCtrlSeen[]; tracks whether a primary
 *              control has already been seen so re-specification can be
 *              diagnosed.  Unused (0) for non-primary controls.
 */
static struct {
    uint8_t optLen;
    char *optName;
    uint8_t tokenId;    // if none zero then token id to put in lexical stream
    bool primary;       // true if primary control
    uint8_t controlId;  // index into controls[] if not 0xff
    uint8_t controlVal; // index used in switch in ParseControlExtra
    uint8_t primaryId;  // index into primaryCtrlSeen
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

/* primaryCtrlSeen[i] – set to true the first time primary control i is seen;
   used to diagnose illegal re-specification of a primary control. */
uint8_t primaryCtrlSeen[15]; // C defaults to all false

/* saveStack / saveDepth – SAVE/RESTORE control stack (max 5 levels). */
static struct {
    uint8_t code;
    uint8_t list;
    uint8_t cond;
    uint16_t leftMargin;
} saveStack[5];

static uint8_t saveDepth;

/* Current values of the most frequently tested controls. */
static bool CODE = false;
static bool LIST = true;
static bool COND = true;

/* curChP  – pointer to the character currently being examined.
   chrClass – character class of *curChP as returned by cClass[]. */
static char *curChP;
static uint8_t chrClass;

/*
 * optVal – holds the string value parsed by AcceptFileName() / GetOptStr() /
 *          ParseId().  The union allows it to be accessed either as a (len,
 *          str) pair suitable for long host filenames, or as a Pascal-style
 *          pstr_t for identifier tokens.
 */
static union {
    struct {
        uint16_t len; // allow long filenames on command line.
        char *str;
    };
    pstr_t *pstr;
} optVal;

static uint16_t optNumValue;     // numeric value parsed by GetOptNumVal() / ParseNum()
static bool lstGiven;            // true once a PRINT/NOPRINT control has been seen
static bool inIFpart;            // true while inside the true branch of a skipped IF
static uint16_t skippingIfDepth; // ifDepth value at which skipping began

/*
 * NxtCh – advance curChP to the next input character.
 *
 * On entry:  curChP points to the current character; chrClass reflects it.
 * On exit:   curChP is incremented, *curChP is normalised (non-printable
 *            characters are replaced with ' '), and chrClass is updated.
 * End-of-line is signalled by CR ('\r'); once chrClass == CC_NEWLINE the
 * function returns immediately without advancing further.
 * On command-line input CR also acts as a line-continuation marker and is
 * treated as end-of-line here.
 */
static void NxtCh() {
    if (chrClass == CC_NEWLINE)
        return;
    curChP++;
    if (*curChP == '\r')
        chrClass = CC_NEWLINE;
    else if ((chrClass = cClass[*(uint8_t *)curChP]) == CC_NONPRINT)
        *curChP = ' ';
} /* NxtCh() */

/*
 * BadCmdTail – report a syntax/value error on a control line or command tail.
 *
 * When processing the command tail (moreCmdLine is true) this is a fatal
 * error; otherwise error code err is written to the phase-1 output stream
 * via Wr1SyntaxError().
 */
static void BadCmdTail(uint8_t err) {
    if (moreCmdLine) // processing command line
        Fatal("ILLEGAL COMMAND TAIL SYNTAX OR VALUE");
    else
        Wr1SyntaxError(err);
}

/*
 * UnknownCtrl – report an unrecognised control keyword.
 *
 * Fatal when on the command tail; ERR9 (INVALID CONTROL) otherwise.
 */
static void UnknownCtrl() {
    if (moreCmdLine) // processing command line
        Fatal("UNRECOGNIZED CONTROL IN COMMAND TAIL");
    else
        Wr1SyntaxError(ERR9); /* INVALID CONTROL */
}

/* SkipWhite – skip space characters (and CR continuations on the command line). */
static void SkipWhite() // also skips non print characters
{
    while (*curChP == ' ' || (moreCmdLine && *curChP == '\r')) // space and cmdline continuation
        NxtCh();
}

/*
 * SkipToRPARorEOL – error-recovery helper.
 *
 * Discard input up to and including the next ')' or to end-of-line,
 * whichever comes first.  Used to resync after a malformed control parameter.
 */
static void SkipToRPARorEOL() {
    while (*curChP != '\r' && *curChP != ')' && *curChP != '\n')
        NxtCh();
    if (*curChP == ')')
        NxtCh();
}

/*
 * AcceptRP – consume optional whitespace then a mandatory closing ')'.
 *
 * If ')' is not found, BadCmdTail(ERR12) is raised and the rest of the
 * parameter list is discarded via SkipToRPARorEOL().
 */
static void AcceptRP() {
    SkipWhite();
    if (*curChP != ')') {
        BadCmdTail(ERR12); /* INVALID CONTROL PARAMETER */
        SkipToRPARorEOL();
    } else
        NxtCh();
}

/*
 * AcceptFileName – parse an optional filename argument enclosed in '(' ')'.
 *
 * If '(' is not present, optVal.len is set to 0 (no filename given).
 * Otherwise the text between the parentheses is trimmed of surrounding
 * whitespace, copied to a NUL-terminated heap buffer, and stored in
 * optVal.str / optVal.len.  Tabs are not permitted inside the filename.
 * The caller is responsible for freeing optVal.str when len > 0.
 */
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
        optVal.len = (uint16_t)(s - optVal.str);
        char *cstr = safeMalloc(optVal.len + 1);
        memcpy(cstr, optVal.str, optVal.len);
        cstr[optVal.len] = '\0';

        optVal.str       = cstr;
        AcceptRP();
    }
}

/*
 * Asc2Num – convert a substring to an unsigned 16-bit integer.
 *
 * Converts the characters in [firstChP, lastChP] using the given radix
 * (2, 8, 10 or 16).  Returns 0xFFFF if any character is invalid for the
 * radix or if the result exceeds 0xFFFF.
 */
static uint16_t Asc2Num(char *firstChP, char *lastChP, uint8_t radix) {

    if (lastChP < firstChP || radix == 0)
        return 0xffff;

    uint32_t num = 0; // 32 bit to simplify overflow test
    uint8_t digit;
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
    return (uint16_t)num;
}

/*
 * ChkRadix – determine the numeric radix from the suffix character.
 *
 * Examines the last character of the numeric token pointed to by *pLastCh:
 *   'B'       → binary  (2)
 *   'Q' / 'O' → octal   (8)
 *   'D'       → decimal (10)
 *   'H'       → hex     (16)
 *   digit     → decimal (10), suffix pointer not consumed
 *
 * *pLastCh is decremented past the suffix letter when one is found.
 * Returns 0 if the suffix is not a recognised radix specifier.
 */
static uint8_t ChkRadix(char **pLastCh) {
    uint8_t *p;

    p = (uint8_t *)*pLastCh;
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

/*
 * ParseNum – parse a PLM-style numeric literal from the current position.
 *
 * Advances past '(' and optional whitespace, collects alphanumeric
 * characters, determines the radix via ChkRadix(), then converts the
 * digit string with Asc2Num().  Returns the 16-bit value, or 0xFFFF on
 * any parse error.
 */
static uint16_t ParseNum() {
    char *firstCh, *lastCh;
    uint8_t radix;

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

/*
 * GetOptNumVal – parse a parenthesised numeric argument.
 *
 * Expects '(' followed by a numeric literal and ')'.  Stores the result in
 * optNumValue (0 on missing '(', which also triggers ERR11).
 */
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

/*
 * GetOptStr – collect a bare control-name token into optVal.
 *
 * Reads characters until a space, '(', end-of-line or CR (command-line
 * continuation) is encountered.  Sets optVal.str and optVal.len; does not
 * NUL-terminate.
 */
static void GetOptStr() {
    optVal.str = curChP;
    while (*curChP != ' ' && *curChP != '(' && chrClass != CC_NEWLINE &&
           *curChP != '\r') // \r is cmdline continuation
        NxtCh();
    optVal.len = (uint8_t)(curChP - optVal.str);
}

/*
 * ParseId – collect an identifier into tokenStr / optVal.pstr.
 *
 * Skips leading whitespace, then collects A-Z, 0-9, '$' and '_' characters
 * up to maxLen (dollar signs are silently dropped).  The result is stored
 * as a Pascal-style string in tokenStr via optVal.pstr.  If no identifier
 * characters are found, optVal.pstr->len is 0.
 */
static void ParseId(uint8_t maxLen) {
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

/*
 * GetVar – look up a conditional-compilation variable (CONDVAR).
 *
 * Parses an identifier of up to 31 characters via ParseId(), then looks it
 * up in the symbol table with Lookup() and FindScopedInfo().  On error
 * (empty name, name too long, or the symbol resolves to a special
 * post-table entry) info is set to BADINFO and the input position is
 * rewound to just before the token so the caller can re-parse if needed.
 * ERR184 is raised if the identifier exceeds 31 characters.
 */
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

/*
 * GetLogical – read a logical operator token (AND / OR / XOR) or end of expression.
 *
 * Returns:
 *   0  – end of expression (no operator or end-of-line)
 *   1  – OR
 *   2  – AND
 *   3  – XOR
 *   4  – unrecognised token (ERR185 raised, remainder of parameter skipped)
 */
static uint8_t GetLogical() {
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

/*
 * GetTest – read a relational operator.
 *
 * Returns:
 *   0  – no operator found
 *   1  – =
 *   2  – <
 *   3  – <=
 *   4  – >
 *   5  – >=
 *   6  – <>
 */
static uint8_t GetTest() {
    uint8_t test;

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

/*
 * ChkNot – consume zero or more consecutive NOT prefixes.
 *
 * Each "NOT" token found toggles the returned bitmask between 0x00 and
 * 0xFF so the caller can XOR it against the operand value.  If no NOT is
 * present the input position is restored and 0 is returned.
 */
static uint8_t ChkNot() // checks for potentially multiple NOT prefixes
{
    uint8_t notStatus = 0;

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

/*
 * GetIfVal – read one operand of a conditional-compilation expression.
 *
 * Returns a numeric constant in the range 0-255, or the current condFlag
 * value of a CONDVAR symbol (0 if undefined).  Returns 256 on a parse error
 * (ERR180 or ERR186 raised).
 */
static uint16_t GetIfVal() {
    uint16_t val;

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

/*
 * ParseIfCond – evaluate a full conditional-compilation expression.
 *
 * Processes a sequence of operands, optional NOT prefixes, relational
 * operators and AND/OR/XOR logical operators.  Converts the final result
 * to a C bool using PL/M truth semantics (LSB of the accumulated value).
 * Returns true if the condition holds, true also on any parse error
 * (conservative: include the block rather than silently discard it).
 */
static bool ParseIfCond() {
    uint8_t andFactor, orFactor, xorFactor;
    uint16_t val1, val2;
    uint8_t relOp, not1, not2;

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
            andFactor = (uint8_t)val1;
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

/*
 * OptPageLen – handle PAGELENGTH(n).
 *
 * Valid range: n >= 4.  PAGELEN is set to n-3 (top-of-page overhead lines
 * are pre-subtracted).  ERR91 is raised for out-of-range values.
 */
static void OptPageLen() {
    GetOptNumVal();
    if (optNumValue < 4 || optNumValue == 0xFFFF)
        BadCmdTail(ERR91); /* ILLEGAL PAGELENGTH CONTROL VALUE */
    else
        PAGELEN = optNumValue - 3;
}

/*
 * OptPageWidth – handle PAGEWIDTH(n).
 *
 * Valid range: 60-132 characters.  ERR92 is raised otherwise.
 */
static void OptPageWidth() {
    GetOptNumVal();
    if (optNumValue < 60 || optNumValue > 132)
        BadCmdTail(ERR92); /* ILLEGAL PAGEWIDTH CONTROL VALUE */
    else
        PWIDTH = (uint8_t)optNumValue;
}

/*
 * OptDate – handle DATE[(value)].
 *
 * Two forms:
 *   DATE          – no argument: fills DATE[] with the current local
 *                   date/time as "yyyy-mm-dd hh:mm:ss" (19 chars).
 *   DATE(string)  – explicit value: up to 10 characters enclosed in
 *                   parentheses; nested '(' ')' are permitted.  The value
 *                   is terminated by a newline, single quote, or unmatched
 *                   closing ')'.
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
        uint8_t nesting = 0;
        NxtCh();
        optVal.str = curChP;
        for (;;) {
            // new line or ' terminates
            if (chrClass == CC_NEWLINE || *curChP == '\r' || *curChP == '\'')
                break;
            if (*curChP == '(') // not sure why nesting is supported for what was 9 char limit
                nesting++;
            else if (*curChP == ')') {
                if (nesting-- == 0)
                    break;
            }
            NxtCh();
        }
        optVal.len = curChP - optVal.str > 10 ? 10 : (uint16_t)(curChP - optVal.str);
        AcceptRP();
        memcpy(DATE, optVal.str, optVal.len);
        DATELEN          = optVal.len;
        DATE[optVal.len] = '\0';
    }
}

/*
 * OptTitle – handle TITLE('string').
 *
 * The argument must be enclosed in '(' ')' and the string must be delimited
 * by single quotes.  Two adjacent single quotes within the string are
 * treated as one literal quote.  The value is truncated to 60 characters
 * and stored in TITLE[].  ERR11/ERR12 are raised for missing or malformed
 * arguments.
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

/*
 * OptLeftMargin – handle LEFTMARGIN(n).
 *
 * Sets LEFTMARGIN to n.  ERR12 is raised if n is 0 or unparseable; n is
 * clamped to 1 in the zero case so subsequent processing is safe.
 */
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

/*
 * OptIXRef – handle IXREF[(filename)].
 *
 * Enables cross-reference output (IXREF = true).  If a filename argument is
 * present it overrides the auto-generated ixiFileName.
 */
static void OptIXRef() {
    AcceptFileName();
    if (optVal.len) { // use supplied name rather than auto generated name
        free(ixiFileName);
        ixiFileName = optVal.str;
    }

    IXREF = true;
}

/*
 * OptMakeDepend – handle MAKEDEPEND[(filename)].
 *
 * Enables dependency-file generation (DEPEND = true).  If a filename
 * argument is present it overrides the auto-generated depFileName.
 */
static void OptMakeDepend() {
    AcceptFileName();
    if (optVal.len) { // use supplied name rather than auto generated name
        free(depFileName);
        depFileName = optVal.str;
    }
    DEPEND = true;
}

/*
 * OptObject – handle OBJECT[(filename)].
 *
 * Enables object-file output (OBJECT = true).  If a filename argument is
 * present it overrides the auto-generated objFileName.
 */
static void OptObject() {
    AcceptFileName();
    if (optVal.len) { // use supplied name rather than auto generated name
        free(objFileName);
        objFileName = optVal.str;
    }
    OBJECT = true;
}

/*
 * OptInclude – handle INCLUDE(filename).
 *
 * Opens the named file and pushes it onto the source-file stack
 * (srcFileTable[]), incrementing srcFileIdx.  The filename is registered in
 * the includes[] array (via newInclude()) so that makedepend dependencies
 * are tracked and compiler phases can refer to it by index.  The token
 * T1_INCLUDE followed by the file index is written to the phase-1 stream.
 *
 * Nesting is limited to 5 levels; ERR13 is raised if exceeded.
 * ERR14 is raised if INCLUDE is not the last control on its line.
 * ERR15 is raised if no filename is supplied.
 *
 * Note: no aliasing check is performed — paths that resolve to the same
 * file but are spelled differently (e.g. ":F0:file.plm" vs "file.plm")
 * will be treated as distinct entries in the includes[] array.
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

/*
 * OptPrint – handle PRINT[(filename)].
 *
 * Enables listing output (PRINT = true).  If a filename argument is present
 * it overrides lstFileName and sets isList.  ERR16 is raised if PRINT is
 * specified more than once.  lstGiven is set so re-specification is caught.
 */
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

/*
 * OptIntVector – handle INTVECTOR(interval, location).
 *
 * Parses two comma-separated numeric arguments inside parentheses.
 * interval must be 4 or 8; location must be aligned to interval*8 and
 * must not exceed 0xFFE0.  ERR176/ERR177 are raised for invalid values.
 * On success intVecNum and intVecLoc are updated.
 */
static void OptIntVector() {
    uint8_t vecNum;
    uint16_t vecLoc;

    SkipWhite();
    if (*curChP != '(')
        BadCmdTail(ERR11); /* MISSING CONTROL PARAMETER */
    else {
        vecNum = (uint8_t)ParseNum();
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

/*
 * AcceptDrive – accept an ISIS drive specifier ":Fn:" followed by a specific character.
 *
 * Checks that curChP begins with ":Fn:" (where n is a digit) and that the
 * character immediately after it matches follow.  Advances curChP past both
 * the drive spec and the follow character on success.
 * Returns true on success, false otherwise.
 */
static bool AcceptDrive(uint8_t follow) {
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

/*
 * OptWorkFiles – handle WORKFILES(:Fn:, :Fm:).
 *
 * Parses two ISIS drive specifiers enclosed in parentheses and separated by
 * a comma.  The drive specs are accepted but not acted upon (they were
 * relevant to the original ISIS-II environment).  ERR11/ERR12 are raised for
 * missing or malformed arguments.
 */
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

/*
 * OptSave – handle SAVE.
 *
 * Pushes the current values of CODE, LIST, COND and LEFTMARGIN onto
 * saveStack[].  Up to 5 levels are supported; ERR187 is raised on overflow.
 */
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

/*
 * OptRestore – handle RESTORE.
 *
 * Pops the top entry from saveStack[] and restores CODE, LIST, COND and
 * LEFTMARGIN to their saved values.  The restored CODE and LIST states are
 * also written to the phase-1 stream so later phases see the change.
 * ERR188 is raised if the stack is empty.
 */
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

/*
 * OptSetReset – handle SET(var[=val], …) and RESET(var, …).
 *
 * Parses a parenthesised, comma-separated list of CONDVAR names.
 *   SET   – creates the variable if absent, optionally assigns a numeric
 *            value (0-255) via "= n"; defaults to 0xFF (true).
 *   RESET – sets condFlag to 0 (false).
 *
 * ERR180 is raised for invalid variable names; ERR181 for an out-of-range
 * constant in a SET assignment.
 */
static void OptSetReset(bool isSet) {
    uint16_t val;

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
                info->condFlag = (uint8_t)val;
            } else
                info->condFlag = isSet;
            if (*curChP != ',') {
                AcceptRP();
                return;
            }
        }
}

/*
 * OptIf – handle IF(condition).
 *
 * Increments ifDepth, then evaluates the condition via ParseIfCond().  If
 * the condition is false, conditional skipping is enabled (skippingCOND =
 * true) and the current skip depth is recorded.  When COND is false and
 * listing is active, T1_NOLIST is written to suppress listing of the
 * skipped block.
 */
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

/*
 * OptElseElseIf – handle ELSE and ELSEIF.
 *
 * When the previous IF branch was taken, ELSE/ELSEIF must now start
 * skipping the remaining branches, so skippingCOND is set and the depth
 * recorded.  ERR182 is raised if there is no enclosing IF.  The rest of
 * the control line is discarded (the ELSEIF condition is evaluated by
 * ChkEndSkipping when scanning forward through skipped lines).
 */
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

/*
 * OptEndIf – handle ENDIF.
 *
 * Decrements ifDepth.  ERR183 is raised if there is no open IF.
 */
static void OptEndIf() {
    if (ifDepth == 0)       // no if
        BadCmdTail(ERR183); /* MISPLACED ENDIF OPTION */
    else
        ifDepth--;
}

/*
 * ParseControlExtra – dispatcher for controls that need argument parsing.
 *
 * Called when the matched optTable entry has controlId == 0xFF, meaning the
 * control is not a simple boolean toggle.  The controlVal field from the
 * matched entry selects the appropriate handler.
 *
 * controlVal  handler / action
 * ----------  ----------------
 *  0          OptPageLen()
 *  1          OptPageWidth()
 *  2          OptDate()
 *  3          OptTitle()
 *  4          OptLeftMargin()
 *  5          OptObject()        (OBJECT)
 *  6          OptInclude()
 *  7          OptPrint()
 *  8          PRINT = false      (NOPRINT)
 *  9          OBJECT = false     (NOOBJECT)
 * 10          OptWorkFiles()
 * 11          OptIntVector()
 * 12          intVecNum = 0      (NOINTVECTOR)
 * 13          OptIXRef()
 * 14          IXREF = false      (NOIXREF)
 * 15          OptSave()
 * 16          OptRestore()
 * 17          OptSetReset(true)  (SET)
 * 18          OptSetReset(false) (RESET)
 * 19          OptIf()
 * 20          OptElseElseIf()    (ELSE / ELSEIF)
 * 21          OptEndIf()
 * 22          COND = true
 * 23          COND = false       (NOCOND)
 * 24          OptMakeDepend()
 */
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

/*
 * FindOption – look up the current option token in optTable[].
 *
 * Performs a linear search for an entry whose optLen and optName match
 * optVal (case-insensitive).  Sets tknFlagsP to the matching entry, or to
 * NULL if no match is found.
 */
static void FindOption() {
    for (uint32_t i = 0; i < DIM(optTable); i++) {
        if (optTable[i].optLen == optVal.len &&
            strnicmp(optVal.str, optTable[i].optName, optVal.len) == 0) {
            tknFlagsP = &optTable[i];
            return;
        }
    }
    tknFlagsP = 0;
}

/*
 * SkipControlParam – discard an optional parenthesised parameter.
 *
 * Used for error recovery: if a '(' follows the current position, it and
 * everything up to the matching ')' or end-of-line are consumed.
 */
static void SkipControlParam() {
    SkipWhite();
    if (*curChP == '(') {
        NxtCh();
        SkipToRPARorEOL();
    }
}

/*
 * ParseControl – parse and dispatch a single compiler control.
 *
 * Reads one option name via GetOptStr() and looks it up in optTable[] with
 * FindOption().  Then:
 *   • Unknown name          → UnknownCtrl(), parameter skipped.
 *   • Primary control used after a non-control line
 *                           → ERR10, parameter skipped.
 *   • Primary control seen twice
 *                           → ERR95, parameter skipped.
 *   • Simple boolean/value control (controlId != 0xFF, tokenId == 0)
 *                           → controls[controlId] = controlVal.
 *   • Simple token control  (controlId != 0xFF, tokenId != 0)
 *                           → T1_xxx written to phase-1 stream; CODE/LIST
 *                              state variables updated.
 *   • Complex control       (controlId == 0xFF)
 *                           → ParseControlExtra() called.
 */
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

/*
 * ParseControlLine – parse all controls on a single '$'-prefixed source line.
 *
 * pch points to the first character after the '$'.  Initialises curChP and
 * chrClass, then repeatedly calls ParseControl() until end-of-line.
 */
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

/*
 * ChkEndSkipping – examine one control line while conditional skipping is active.
 *
 * Called from GetCodeLine() when skippingCOND is true and the current line
 * begins with '$'.  Looks for IF (increments ifDepth), ENDIF (decrements
 * ifDepth; ends skip when depth falls below skippingIfDepth), ELSE (ends
 * skip if at the original IF depth and inIFpart is true), or ELSEIF
 * (re-evaluates the condition at the original depth).
 * If skipping ends and COND is false, T1_LIST is written to resume listing.
 */
static void ChkEndSkipping(uint8_t *pch) {
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
bool trunc; // set true by RSrcLn() when a source line exceeds MAXLINE characters

/*
 * GNxtCh – advance inChrP to the next source character (exported to lexer).
 *
 * Increments inChrP, then calls GetLin() to fetch the next source line
 * whenever a '\n' is encountered.  Sets nextCh to the new character.
 * (The original code assumed \r\n line endings; '\r' is handled by InGetC.)
 */
void GNxtCh() {
    inChrP++;
    while (*inChrP == '\n')
        GetLin();
    // hack as original code assumed \r\n
    nextCh = *inChrP;
}

/*
 * InGetC – read one raw character from the current source file into *inChrP.
 *
 * Discards '\r' (CR) characters so the rest of the input machinery only
 * ever sees '\n' as a line terminator.  Maps EOF to the ISISEOF sentinel
 * byte (0x81).  Returns the character stored.
 */
static uint8_t InGetC() {
    int c;
    while ((c = fgetc(srcFil.fp)) == '\r')
        ;
    return *inChrP = c == EOF ? ISISEOF : c & 0x7f;
}

/*
 * RSrcLn – read the remainder of the current source line into lineBuf.
 *
 * Reads characters via InGetC() until '\n' or ISISEOF is encountered, or
 * until MAXLINE characters have been consumed.  If the line is too long,
 * trunc is set and the excess characters are discarded.
 */
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

/*
 * GetSrcLine – fetch one complete source line into lineBuf.
 *
 * Writes the pending line-info record, fills lineBuf via InGetC()/RSrcLn(),
 * appends "\r\n", resets inChrP to the start of lineBuf, and increments the
 * line counter.  Statement and block counts are zeroed for the new line and
 * lineInfoToWrite is set so the record will be emitted before the next line.
 * Returns without reading if ISISEOF is encountered immediately.
 */
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

/*
 * GetCodeLine – main source-line driver; called by GetLin().
 *
 * Loops calling GetSrcLine() until a line is ready for the lexer:
 *   • Control lines ('$' after LEFTMARGIN) are parsed via ParseControlLine()
 *     and the loop continues.
 *   • Lines within a skipped conditional block are examined by
 *     ChkEndSkipping() to detect IF/ENDIF/ELSE/ELSEIF nesting; the loop
 *     continues until skipping ends.
 *   • ERR86 is raised if a source line was truncated.
 *   • INCLUDE-file nesting: when EOF is reached on a nested file, that file
 *     is closed and srcFil is restored from srcFileTable[].
 *   • When the outermost file (srcFileIdx == 0) reaches EOF, a synthetic
 *     token sequence is substituted to close any open strings/comments and
 *     supply an EOF marker.  An unclosed IF nesting at this point raises
 *     ERR188.
 *   • isNonCtrlLine is set true on the first non-control source line,
 *     preventing further primary controls from being accepted.
 */
static void GetCodeLine() {
    uint8_t *startOfLine;

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
                Wr1Byte(T1_STMTCNT);   // note zero stmts on this line
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

/*
 * GetLin – top-level "get next line" entry point called by the lexer.
 *
 * If the compiler is currently expanding a LITERAL macro (macroDepth > 0),
 * the current macro's info entry is restored to LIT_T and inChrP is reset
 * to the saved expansion position, effectively returning to the enclosing
 * expansion level.  Otherwise GetCodeLine() is called to fetch the next
 * physical source line.
 */
static void GetLin() {
    if (macroDepth != 0) {       // last line was a nested lit expansion
        macroInfo->type = LIT_T; // reset info entry to LIT_T from MACRO_T
        macroInfo       = macroPtrs[macroDepth].macroInfo;
        inChrP          = macroPtrs[macroDepth--].text; // get the curent loc in the expansion
    } else
        GetCodeLine();
}