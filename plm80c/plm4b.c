/****************************************************************************
 *  plm4b.c: part of the C port of Intel's ISIS-II plm80c             *
 *  The original ISIS-II application is Copyright Intel                     *
 *																			*
 *  Re-engineered to C by Mark Ogden <mark.pm.ogden@btinternet.com> 	    *
 *                                                                          *
 *  It is released for hobbyist use and for academic interest			    *
 *                                                                          *
 ****************************************************************************/

#include "plm.h"
#include "os.h"
#include <stdlib.h>

/* plm4a.plm */
static struct {
    uint16_t errCode;
    char const *errStr;
} errStrings[] = { // merged plm4b & plm6b messages
    { 0x1, "INVALID PL/M-80 CHARACTER" },
    { 0x2, "UNPRINTABLE ASCII CHARACTER" },
    { 0x3, "IDENTIFIER, STRING, OR NUMBER TOO LONG, TRUNCATED" },
    { 0x4, "ILLEGAL NUMERIC CONSTANT TYPE" },
    { 0x5, "INVALID CHARACTER IN NUMERIC CONSTANT" },
    { 0x6, "ILLEGAL MACRO REFERENCE, RECURSIVE EXPANSION" },
    { 0x7, "LIMIT EXCEEDED: MACROS NESTED TOO DEEPLY" },
    { 0x8, "INVALID CONTROL FORMAT" },
    { 0x9, "INVALID CONTROL" },
    { 0xA, "ILLEGAL USE OF PRIMARY CONTROL AFTER NON-CONTROL LINE" },
    { 0xB, "MISSING CONTROL PARAMETER" },
    { 0xC, "INVALID CONTROL PARAMETER" },
    { 0xD, "LIMIT EXCEEDED: INCLUDE NESTING" },
    { 0xE, "INVALID CONTROL FORMAT, INCLUDE NOT LAST CONTROL" },
    { 0xF, "MISSING INCLUDE CONTROL PARAMETER" },
    { 0x10, "ILLEGAL PRINT CONTROL" },
    { 0x11, "INVALID PATH-NAME" },
    { 0x12, "INVALID MULTIPLE LABELS AS MODULE NAMES" },
    { 0x13, "INVALID LABEL IN MODULE WITHOUT MAIN PROGRAM" },
    { 0x14, "MISMATCHED IDENTIFIER AT END OF BLOCK" },
    { 0x15, "MISSING PROCEDURE NAME" },
    { 0x16, "INVALID MULTIPLE LABELS AS PROCEDURE NAMES" },
    { 0x17, "INVALID LABELLED END IN EXTERNAL PROCEDURE" },
    { 0x18, "INVALID STATEMENT IN EXTERNAL PROCEDURE" },
    { 0x19, "UNDECLARED PARAMETER" },
    { 0x1A, "INVALID DECLARATION, STATEMENT OUT OF PLACE" },
    { 0x1B, "LIMIT EXCEEDED: NUMBER OF DO BLOCKS" },
    { 0x1C, "MISSING 'THEN'" },
    { 0x1D, "ILLEGAL STATEMENT" },
    { 0x1E, "LIMIT EXCEEDED: NUMBER OF LABELS ON STATEMENT" },
    { 0x1F, "LIMIT EXCEEDED: PROGRAM TOO COMPLEX" },
    { 0x20, "INVALID SYNTAX, TEXT IGNORED UNTIL ';'" },
    { 0x21, "DUPLICATE LABEL DECLARATION" },
    { 0x22, "DUPLICATE PROCEDURE DECLARATION" },
    { 0x23, "LIMIT EXCEEDED: NUMBER OF PROCEDURES" },
    { 0x24, "MISSING PARAMETER" },
    { 0x25, "MISSING ')' AT END OF PARAMETER LIST" },
    { 0x26, "DUPLICATE PARAMETER NAME" },
    { 0x27, "INVALID ATTRIBUTE OR INITIALIZATION, NOT AT MODULE LEVEL" },
    { 0x28, "DUPLICATE ATTRIBUTE" },
    { 0x29, "CONFLICTING ATTRIBUTE" },
    { 0x2A, "INVALID INTERRUPT VALUE" },
    { 0x2B, "MISSING INTERRUPT VALUE" },
    { 0x2C, "ILLEGAL ATTRIBUTE, 'INTERRUPT' WITH PARAMETERS" },
    { 0x2D, "ILLEGAL ATTRIBUTE, 'INTERRUPT' WITH TYPED PROCEDURE" },
    { 0x2E, "ILLEGAL USE OF LABEL" },
    { 0x2F, "MISSING ')' AT END OF FACTORED DECLARATION" },
    { 0x30, "ILLEGAL DECLARATION STATEMENT SYNTAX" },
    { 0x31, "LIMIT EXCEEDED: NUMBER OF ITEMS IN FACTORED DECLARE" },
    { 0x32, "INVALID ATTRIBUTES FOR BASE" },
    { 0x33, "INVALID BASE, SUBSCRIPTING ILLEGAL" },
    { 0x34, "INVALID BASE, MEMBER OF BASED STRUCTURE OR ARRAY OF STRUCTURES" },
    { 0x35, "INVALID STRUCTURE MEMBER IN BASE" },
    { 0x36, "UNDECLARED BASE" },
    { 0x37, "UNDECLARED STRUCTURE MEMBER IN BASE" },
    { 0x38, "INVALID MACRO TEXT, NOT A STRING CONSTANT" },
    { 0x39, "INVALID DIMENSION, ZERO ILLEGAL" },
    { 0x3A, "INVALID STAR DIMENSION IN FACTORED DECLARATION" },
    { 0x3B, "ILLEGAL DIMENSION ATTRIBUTE" },
    { 0x3C, "MISSING ')' AT END OF DIMENSION" },
    { 0x3D, "MISSING TYPE" },
    { 0x3E, "INVALID STAR DIMENSION WITH 'STRUCTURE' OR 'EXTERNAL'" },
    { 0x3F, "INVALID DIMENSION WITH THIS ATTRIBUTE" },
    { 0x40, "MISSING STRUCTURE MEMBERS" },
    { 0x41, "MISSING ')' AT END OF STRUCTURE MEMBER LIST" },
    { 0x42, "INVALID STRUCTURE MEMBER, NOT AN IDENTIFIER" },
    { 0x43, "DUPLICATE STRUCTURE MEMBER NAME" },
    { 0x44, "LIMIT EXCEEDED: NUMBER OF STRUCTURE MEMBERS" },
    { 0x45, "INVALID STAR DIMENSION WITH STRUCTURE MEMBER" },
    { 0x46, "INVALID MEMBER TYPE, 'STRUCTURE' ILLEGAL" },
    { 0x47, "INVALID MEMBER TYPE, 'LABEL' ILLEGAL" },
    { 0x48, "MISSING TYPE FOR STRUCTURE MEMBER" },
    { 0x49, "INVALID ATTRIBUTE OR INITIALIZATION, NOT AT MODULE LEVEL" },
    { 0x4A, "INVALID STAR DIMENSION, NOT WITH 'DATA' OR 'INITIAL'" },
    { 0x4B, "MISSING ARGUMENT OF 'AT', 'DATA', OR 'INITIAL'" },
    { 0x4C, "CONFLICTING ATTRIBUTE WITH PARAMETER" },
    { 0x4D, "INVALID PARAMETER DECLARATION, BASE ILLEGAL" },
    { 0x4E, "DUPLICATE DECLARATION" },
    { 0x4F, "ILLEGAL PARAMETER TYPE, NOT BYTE OR ADDRESS" },
    { 0x50, "INVALID DECLARATION, LABEL MAY NOT BE BASED" },
    { 0x51, "CONFLICTING ATTRIBUTE WITH 'BASE'" },
    { 0x52, "INVALID SYNTAX, MISMATCHED '('" },
    { 0x53, "LIMIT EXCEEDED: DYNAMIC STORAGE" },
    { 0x54, "LIMIT EXCEEDED: BLOCK NESTING" },
    { 0x55, "LONG STRING ASSUMED CLOSED AT NEXT SEMICOLON OR QUOTE" },
    { 0x56, "LIMIT EXCEEDED: SOURCE LINE LENGTH" },
    { 0x57, "MISSING 'END', END-OF-FILE ENCOUNTERED" },
    { 0x58, "INVALID PROCEDURE NESTING, ILLEGAL IN REENTRANT PROCEDURE" },
    { 0x59, "MISSING 'DO' FOR MODULE" },
    { 0x5A, "MISSING NAME FOR MODULE" },
    { 0x5B, "ILLEGAL PAGELENGTH CONTROL VALUE" },
    { 0x5C, "ILLEGAL PAGEWIDTH CONTROL VALUE" },
    { 0x5D, "MISSING 'DO' FOR 'END', 'END' IGNORED" },
    { 0x5E, "ILLEGAL CONSTANT, VALUE > 65535" },
    { 0x5F, "ILLEGAL RESPECIFICATION OF PRIMARY CONTROL IGNORED" },
    { 0x60, "COMPILER ERROR: SCOPE STACK UNDERFLOW" },
    { 0x61, "COMPILER ERROR: PARSE STACK UNDERFLOW" },
    { 0x62, "INCLUDE FILE IS NOT A DISKETTE FILE" },
    /* 0X63 */
    { 0x64, "INVALID STRING CONSTANT IN Expression" },
    { 0x65, "INVALID ITEM FOLLOWS DOT OPERATOR" },
    { 0x66, "MISSING PRIMARY OPERAND" },
    { 0x67, "MISSING ')' AT END OF SUBEXPRESSION" },
    { 0x68, "ILLEGAL PROCEDURE INVOCATION WITH DOT OPERATOR" },
    { 0x69, "UNDECLARED IDENTIFIER" },
    { 0x6A, "INVALID INPUT/OUTPUT PORT NUMBER" },
    { 0x6B, "ILLEGAL INPUT/OUTPUT PORT NUMBER, NOT NUMERIC CONSTANT" },
    { 0x6C, "MISSING ')' AFTER INPUT/OUTPUT PORT NUMBER" },
    { 0x6D, "MISSING INPUT/OUTPUT PORT NUMBER" },
    { 0x6E, "INVALID LEFT OPERAND OF QUALIFICATION, NOT A STRUCTURE" },
    { 0x6F, "INVALID RIGHT OPERAND OF QUALIFICATION, NOT IDENTIFIER" },
    { 0x70, "UNDECLARED STRUCTURE MEMBER" },
    { 0x71, "MISSING ')' AT END OF ARGUMENT LIST" },
    { 0x72, "INVALID SUBSCRIPT, MULTIPLE SUBSCRIPTS ILLEGAL" },
    { 0x73, "MISSING ')' AT END OF SUBSCRIPT" },
    { 0x74, "MISSING '=' IN ASSIGNMENT STATEMENT" },
    { 0x75, "MISSING PROCEDURE NAME IN CALL STATEMENT" },
    { 0x76, "INVALID INDIRECT CALL, IDENTIFIER NOT AN ADDRESS SCALAR" },
    { 0x77, "LIMIT EXCEEDED: PROGRAM TOO COMPLEX" },
    { 0x78, "LIMIT EXCEEDED: Expression TOO COMPLEX" },
    { 0x79, "LIMIT EXCEEDED: Expression TOO COMPLEX" },
    { 0x7A, "LIMIT EXCEEDED: PROGRAM TOO COMPLEX" },
    { 0x7B, "INVALID DOT OPERAND, BUILT-IN PROCEDURE ILLEGAL" },
    { 0x7C, "MISSING ARGUMENTS FOR BUILT-IN PROCEDURE" },
    { 0x7D, "ILLEGAL ARGUMENT FOR BUILT-IN PROCEDURE" },
    { 0x7E, "MISSING ')' AFTER BUILT-IN PROCEDURE ARGUMENT LIST" },
    { 0x7F, "INVALID SUBSCRIPT ON NON-ARRAY" },
    { 0x80, "INVALID LEFT-HAND OPERAND OF ASSIGNMENT" },
    { 0x81, "ILLEGAL 'CALL' WITH TYPED PROCEDURE" },
    { 0x82, "ILLEGAL REFERENCE TO OUTPUT FUNCTION" },
    { 0x83, "ILLEGAL REFERENCE TO UNTYPED PROCEDURE" },
    { 0x84, "ILLEGAL USE OF LABEL" },
    { 0x85, "ILLEGAL REFERENCE TO UNSUBSCRIPTED ARRAY" },
    { 0x86, "ILLEGAL REFERENCE TO UNSUBSCRIPTED MEMBER ARRAY" },
    { 0x87, "ILLEGAL REFERENCE TO AN UNQUALIFIED STRUCTURE" },
    { 0x88, "INVALID RETURN FOR UNTYPED PROCEDURE, VALUE ILLEGAL" },
    { 0x89, "MISSING VALUE IN RETURN FOR TYPED PROCEDURE" },
    { 0x8A, "MISSING INDEX VARIABLE" },
    { 0x8B, "INVALID INDEX VARIABLE TYPE, NOT BYTE OR ADDRESS" },
    { 0x8C, "MISSING '=' FOLLOWING INDEX VARIABLE" },
    { 0x8D, "MISSING 'TO' CLAUSE" },
    { 0x8E, "MISSING IDENTIFIER FOLLOWING GOTO" },
    { 0x8F, "INVALID REFERENCE FOLLOWING GOTO, NOT A LABEL" },
    { 0x90, "INVALID GOTO LABEL, NOT AT LOCAL OR MODULE LEVEL" },
    { 0x91, "MISSING 'TO' FOLLOWING 'GO'" },
    { 0x92, "MISSING ')' AFTER 'AT' RESTRICTED Expression" },
    { 0x93, "MISSING IDENTIFIER FOLLOWING DOT OPERATOR" },
    { 0x94, "INVALID QUALIFICATION IN RESTRICTED REFERENCE" },
    { 0x95, "INVALID SUBSCRIPTING IN RESTRICTED REFERENCE" },
    { 0x96, "MISSING ')' AT END OF RESTRICTED SUBSCRIPT" },
    { 0x97, "INVALID OPERAND IN RESTRICTED Expression" },
    { 0x98, "MISSING ')' AFTER CONSTANT LIST" },
    { 0x99, "INVALID NUMBER OF ARGUMENTS IN CALL, TOO MANY" },
    { 0x9A, "INVALID NUMBER OF ARGUMENTS IN CALL, TOO FEW" },
    { 0x9B, "INVALID RETURN IN MAIN PROGRAM" },
    { 0x9C, "MISSING RETURN STATEMENT IN TYPED PROCEDURE" },
    { 0x9D, "INVALID ARGUMENT, ARRAY REQUIRED FOR LENGTH OR LAST" },
    { 0x9E, "INVALID DOT OPERAND, LABEL ILLEGAL" },
    { 0x9F, "COMPILER ERROR: PARSE STACK UNDERFLOW" },
    { 0xA0, "COMPILER ERROR: OPERAND STACK UNDERFLOW" },
    { 0xA1, "COMPILER ERROR: ILLEGAL OPERAND STACK EXCHANGE" },
    { 0xA2, "COMPILER ERROR: OPERATOR STACK UNDERFLOW" },
    { 0xA3, "COMPILER ERROR: GENERATION FAILURE" },
    { 0xA4, "COMPILER ERROR: SCOPE STACK OVERFLOW" },
    { 0xA5, "COMPILER ERROR: SCOPE STACK UNDERFLOW" },
    { 0xA6, "COMPILER ERROR: CONTROL STACK OVERFLOW" },
    { 0xA7, "COMPILER ERROR: CONTROL STACK UNDERFLOW" },
    { 0xA8, "COMPILER ERROR: BRANCH MISSING IN 'IF' STATEMENT" },
    { 0xA9, "ILLEGAL FORWARD CALL" },
    { 0xAA, "ILLEGAL RECURSIVE CALL" },
    { 0xAB, "INVALID USE OF DELIMITER OR RESERVED WORD IN Expression" },
    { 0xAC, "INVALID LABEL: UNDEFINED" },
    { 0xAD, "INVALID LEFT SIDE OF ASSIGNMENT: VARIABLE DECLARED WITH DATA ATTRIBUTE" },
    { 0xAE, "INVALID NULL PROCEDURE" },
    /* 0xAF */
    { 0xB0, "INVALID INTVECTOR INTERVAL VALUE" },
    { 0xB1, "INVALID INTVECTOR LOCATION VALUE" },
    { 0xB2,
      "INVALID 'AT' RESTRICTED REFERENCE, EXTERNAL ATTRIBUTE CONFLICTS WITH PUBLIC ATTRIBUTE" },
    { 0xB3, "OUTER 'IF' MAY NOT HAVE AN 'ELSE' PART" },
    { 0xB4, "MISSING OR INVALID CONDITIONAL COMPILATION PARAMETER" },
    { 0xB5, "MISSING OR INVALID CONDITIONAL COMPILATION CONSTANT" },
    { 0xB6, "MISPLACED ELSE OR ELSEIF OPTION" },
    { 0xB7, "MISPLACED ENDIF OPTION" },
    { 0xB8, "CONDITIONAL COMPILATION PARAMETER NAME TOO LONG" },
    { 0xB9, "MISSING OPERATOR IN CONDITIONAL COMPILATION Expression" },
    { 0xBA, "INVALID CONDITIONAL COMPILATION CONSTANT, TOO LARGE" },
    { 0xBB, "LIMIT EXCEEDED: NUMBER OF SAVE LEVELS > 5" },
    { 0xBC, "MISPLACED RESTORE OPTION" },
    { 0xBD, "NULL STRING NOT ALLOWED" },
    /* 0XBE, 0XBF, 0XC0, 0XC1, 0XC2, 0XC3, 0XC4, 0XC5, 0XC7 */
    { 0xC8, "LIMIT EXCEEDED: STATEMENT SIZE" },
    { 0xC9, "INVALID DO CASE BLOCK, AT LEAST ONE CASE REQUIRED" },
    { 0xCA, "LIMIT EXCEEDED: NUMBER OF ACTIVE CASES" },
    { 0xCB, "LIMIT EXCEEDED: NESTING OF TYPED PROCEDURE CALLS" },
    { 0xCC, "LIMIT EXCEEDED: NUMBER OF ACTIVE PROCEDURES AND DO CASE GROUPS" },
    { 0xCD, "ILLEGAL NESTING OF BLOCKS, ENDS NOT BALANCED" },
    { 0xCE, "LIMIT EXCEEDED: CODE SEGMENT SIZE" },
    { 0xCF, "LIMIT EXCEEDED: SEGMENT SIZE" },
    { 0xD0, "LIMIT EXCEEDED: STRUCTURE SIZE" },
    { 0xD1, "ILLEGAL INITIALIZATION OF MORE SPACE THAN DECLARED" },
    { 0xD2, "ILLEGAL INITIALIZATION OF A BYTE TO A VALUE > 255" },
    { 0xD3, "INVALID IDENTIFIER IN 'AT' RESTRICTED REFERENCE" },
    { 0xD4, "INVALID RESTRICTED REFERENCE IN 'AT', BASE ILLEGAL" },
    { 0xD5, "UNDEFINED RESTRICTED REFERENCE IN 'AT'" },
    { 0xD6, "COMPILER ERROR: OPERAND CANNOT BE TRANSFORMED" },
    { 0xD7, "COMPILER ERROR: EOF READ IN FINAL ASSEMBLY" },
    { 0xD8, "COMPILER ERROR: BAD LABEL ADDRESS" },
    { 0xD9, "ILLEGAL INITIALIZATION OF AN EXTERNAL VARIABLE" },
    { 0xDA, "ILLEGAL SUCCESSIVE USES OF RELATIONAL OPERATORS" },
    { 0xDB, "LIMIT EXCEEDED: NUMBER OF EXTERNALS > 255" }
};

void FlushRecs() {
    WriteRec(rec8, 1);
    WriteRec(rec6_4, 3);
    WriteRec(rec22, 1);
    WriteRec(rec24_1, 2);
    WriteRec(rec24_2, 2);
    WriteRec(rec24_3, 2);
    WriteRec(rec20, 1);
    putWord(&rec6_4[CONTENT_OFF], baseAddr);
}

void AddWrdDisp(pstr_t *pstr, word arg2w) {

    if (arg2w != 0) {
        if (arg2w > 0x8000) {
            pstr->str[pstr->len] = '-';
            arg2w                = -arg2w;
        } else
            pstr->str[pstr->len] = '+';
        pstr->len++;
        pstrcat(pstr, hexfmt(0, arg2w));
    }
}

void EmitLinePrefix() {
    if (!linePrefixChecked && listing) {
        SetStartAndTabW(15, 4);
        if (stmtCnt)
            lprintf("%4d", stmtCnt);
        TabLst(-7);
        if (blkCnt)
            lprintf("%2d", blkCnt);
        else
            lstStr("  ");
        if (srcFileIdx) {
            TabLst(-11);
            lstc('=');
            if (srcFileIdx != 1)
                lprintf("%d", srcFileIdx - 1);
        }
        if (lstLineLen) {
            TabLst(-15);
            lstStr(lstLine);
        } else
            lstc('\n');

        linePrefixEmitted = true;
    }
    linePrefixChecked = true;
    listing           = !listOff && PRINT;
}

void EmitStatementNo() {
    EmitLinePrefix();
    TabLst(-50);
    lprintf("; STATEMENT # %d\n", stmtNo);
}

void EmitLabel(char const *label) {
    if (codeOn) {
        EmitLinePrefix();
        TabLst(-26);
        lprintf("%s:\n", label);
    }
}

char const *FindErrStr() {
    for (int i = 0;
         i < sizeof(errStrings) / sizeof(errStrings[0]) && errStrings[i].errCode <= errData.num;
         i++)
        if (errStrings[i].errCode == errData.num)
            return errStrings[i].errStr;
    return "UNKNOWN ERROR";
}

void EmitError() {

    programErrCnt++;
    if (PRINT) {
        linePrefixChecked = linePrefixEmitted;
        listing           = true;
        EmitLinePrefix();
        lprintf("*** ERROR #%d, ", errData.num);
        if (errData.stmt)
            lprintf("STATEMENT #%d, ", errData.stmt);
        if (errData.info) {
            lstStr("NEAR '");
            infoIdx   = errData.info;
            curSym = GetSymbol();
            if (curSym != 0)
                lstStr(symtab[curSym].name->str);
            else
                lstStr("<LONG CONSTANT>");
            lstStr("', ");
        }
        lprintf("  %s\n", FindErrStr());
    }
}

void FatalError_ov46(byte arg1b) {
    errData.num = fatalErrorCode = arg1b;
    errData.info                 = 0;
    errData.stmt                 = stmtNo;
    EmitError();
    longjmp(exception, -1);
}

void ListCodeBytes() {
    if (codeOn) {
        if (opByteCnt > 0) {
            TabLst(-12);
            lprintf("%04X", baseAddr);
            TabLst(-18);
            for (int i = 0; i < opByteCnt; i++)
                lprintf("%02X", opBytes[i]);
        }
        TabLst(-26);
        SetStartAndTabW(26, 8);
        lstPstr((pstr_t *)&line);
        NewLineLst();
    }
}

static byte GetSourceCh() {
    int c;
    for (;;) {
        if ((c = fgetc(srcFil.fp)) == '\r')
            continue;
        if (c != EOF)
            return c & 0x7f;
        if (lstLineLen != 0)
            return '\n';
        if (srcFileIdx == 0) /* top level file */
            FatalError_ov46(ERR215);
        CloseF(&srcFil);
        srcFil = srcFileTable[--srcFileIdx];
    }
}

void GetSourceLine() {
    lstLineLen = 0;
    while (1) {
        lstLine[lstLineLen] = GetSourceCh();
        if (lstLine[lstLineLen] == '\n') {
            lstLine[lstLineLen + 1] = '\0'; // make a C string
            linePrefixChecked       = false;
            linePrefixEmitted       = false;
            return;
        } else if (lstLineLen < MAXLINE)
            lstLineLen++;
    }
}
