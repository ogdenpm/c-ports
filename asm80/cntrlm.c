/****************************************************************************
 *  cntrlm.c: part of the C port of Intel's ISIS-II asm80                   *
 *  The original ISIS-II application is Copyright Intel                     *
 *                                                                          *
 *  Re-engineered to C by Mark Ogden <mark.pm.ogden@btinternet.com>         *
 *                                                                          *
 *  It is released for academic interest and personal use only              *
 ****************************************************************************/
#include "../shared/os.h"
#include "asm80.h"
#include <ctype.h>
#include <stdint.h>

static struct {
    byte flags; /* format of entries
                00PNxxxx where
                P -> 0 general control 1 -> primary control
                N -> 0 NO prefix not supported 1-> NO prefix supported
                xxxx -> number of chars in name string
        */
    char *name;
} controlTable[] = { { 0x35, "DEBUG" },     { 0x3A, "MACRODEBUG" }, { 0x34, "XREF" },
                     { 0x37, "SYMBOLS" },   { 0x36, "PAGING" },     { 0x33, "TTY" },
                     { 0x35, "MOD85" },     { 0x35, "PRINT" },      { 0x36, "OBJECT" },
                     { 0x39, "MACROFILE" }, { 0x29, "PAGEWIDTH" },  { 0x2A, "PAGELENGTH" },
                     { 0x7, "INCLUDE" },    { 0x5, "TITLE" },       { 0x4, "SAVE" },
                     { 0x7, "RESTORE" },    { 0x5, "EJECT" },       { 0x14, "LIST" },
                     { 0x13, "GEN" },       { 0x14, "COND" },       { 0x2A, "MAKEDEPEND" },
                     { 0x28, "ISISNAME" } };

byte tokVal;
bool savedCtlList, savedCtlGen;
bool controlError;

#define MAXINCLUDES 40

char const *includes[MAXINCLUDES];
uint16_t includeCnt;
int maxSymbolSize = MAXSYMSIZE;

static bool ChkParen(byte parenType) {
    SkipWhite();
    reget = 0;
    return parenType == curChar;
}

static byte GetTok(void) {
    tokBufLen = 0;
    tokType   = TT_ID;
    if (IsEOL())
        return curChar;

    SkipNextWhite();
    if (isalpha(curChar)) { /* letter */
        GetTokenText(O_ID);
        if (BlankAsmErrCode() && token.size <= 14) {
            memcpy(tokBuf, lineBuf, tokBufLen = token.size);
            tokBuf[tokBufLen] = '\0';
        }
    } else if (isdigit(curChar)) { /* digit ? */
        GetNum();
        if (BlankAsmErrCode()) {
            tokNumVal = GetNumVal();
            tokType   = TT_NUM;
            tokBuf[0] = ' ';
        }
    } else if (curChar == '\'') { /* string ? */
        GetStr();
        if (BlankAsmErrCode()) {
            tokBufLen = 64; /* cap at 64 chars */
            if (token.size < 64)
                tokBufLen = token.size;
            tokType = TT_STR;
            if (tokBufLen > 0) {
                memcpy(tokBuf, lineBuf, tokBufLen);
                tokBuf[tokBufLen] = '\0';
            }
        }
    } else {
        tokBufLen = 1;
        tokBuf[0] = curChar;
        return curChar;
    }
    PopToken(); /* restore the stack from GetId/GetNum/GetStr() calls */
    return tokBuf[0];
}

static void GetFileNameOpt(void) {
    SkipNextWhite();
    int endCh = ')';

    if (curChar == '\'') {
        endCh   = '\'';
        curChar = GetCh();
    }
    while (curChar != EOLCH && curChar != endCh && tokBufIdx <= MAXFILENAME) {
        tokBuf[tokBufIdx++] = curChar < ' ' ? ' ' : curChar;
        curChar             = GetCh();
    }
    if (curChar == endCh && curChar == ')')
        while (tokBufIdx && tokBuf[tokBufIdx - 1] == ' ')
            tokBufIdx--;
    tokBuf[tokBufIdx] = '\0';
    if (endCh == '\'' && curChar == '\'')
        curChar = GetCh();
}

static void GetFileParam(void) {
    tokBufIdx = 0;
    if (!ChkParen('(')) /* ( */
        FileError();
    else {
        GetFileNameOpt();
        if (!ChkParen(')') || tokBufIdx == 0 || tokBufIdx > MAXFILENAME) /* ) */
            FileError();
    }
}

static bool GetControlNumArg(void) {
    if (ChkParen('(')) { /* ( */
        tokVal = GetTok();
        if (tokType == TT_NUM)
            return ChkParen(')'); /* ) */
    }
    return false;
}

static byte SetControl(byte id, byte val, byte noInvalid) {
    if (!noInvalid)
        controls.all[id] = val;
    return val ? id : 17; // 17 is no action needed
}

static byte LookupControl(void) {

    byte cmdLen = (byte)tokBufLen;
    bool ctlVal = true;
    char *name  = tokBuf;
    if (tokBufLen >= 2 && name[0] == 'N' && name[1] == 'O') { /* check for NO prefix */
        ctlVal = false;                                       /* control will be set as false */
        cmdLen -= 2; /* length of string to match excludes the NO */
        name += 2;
    }

    for (byte id = 0; id < sizeof(controlTable) / sizeof(controlTable[0]); id++) {
        byte ctlFlags = controlTable[id].flags;
        if (cmdLen == (ctlFlags & 0xf) && strncmp(name, controlTable[id].name, cmdLen) == 0) {
            bool noInvalid = !(ctlFlags & 0x10);
            if (noInvalid && !ctlVal)
                return 255; /* NO not supported */

            if ((ctlFlags & 0x20) != 0x20) /* general control */
                return SetControl(id, ctlVal, noInvalid);

            if (!primaryValid) /* is a PRIMARY control */
                return 255;

            if (controlSeen[id]) /* make sure we only see a primary once */
                return 255;
            controlSeen[id] = true;
            return SetControl(id, ctlVal, noInvalid);
        }
    }
    return 255; // not found
}

static void ProcessControl(byte id) {
    if (id == 255)
        controlError = true;

    switch (id) {
    case 5: /* TTY */
        controls.tty = true;
        return;
    case 6: /* MOD85 */
        controls.mod85 = true;
        return;
    case 7: /* PRINT */
        GetFileParam();
        if (lstFile)
            free(lstFile);
        lstFile = strdup(tokBuf);
        return;
    case 8: /* OBJECT */
        GetFileParam();
        if (objFile)
            free(objFile);
        objFile = strdup(tokBuf);
        return;
    case 9: /* MACROFILE */
        controls.macroFile = true;
        if (ChkParen('(')) {
            while ((curChar = GetCh()) != ')')
                if (IsEOL())
                    FatalError("Missing ')' parsing MACROFILE (drive)");
        } else
            reget = 1;
        return;
    case 10: /* PAGEWIDTH */
        if (GetControlNumArg()) {
            if (tokNumVal == 0)
                tokNumVal = 0xffff;
            else if (tokNumVal < 72)
                tokNumVal = 72;
            pageWidth = tokNumVal;

            return;
        }
        break;
    case 11: /* PAGELENGTH */
        if (GetControlNumArg()) {
            if (tokNumVal == 0)
                tokNumVal = 0xffff;
            else if (tokNumVal < 15)
                tokNumVal = 15;
            pageLength = tokNumVal;
            return;
        }
        break;
    case 12:                   /* INCLUDE */
        if (!pendingInclude) { // multiple includes on control line not supported
            if (fileIdx == 5)
                StackError();
            else {
                fileIdx++;
                GetFileParam();
                files[fileIdx].name = newInclude(tokBuf);
                pendingInclude      = true;
                if (scanCmdLine)
                    includeOnCmdLine = true;
                return;
            }
        }
        break;
    case 13: /* TITLE */
        if (ChkParen('(')) {
            tokVal = GetTok();
            if (tokType == TT_STR && tokBufLen != 0) {
                if (phase != 1 || (IsPhase1() && primaryValid)) {
                    strcpy(titleStr, tokBuf);
                    if (ChkParen(')')) {
                        controls.title = true;
                        return;
                    }
                }
            }
        }
        break;
    case 14: /* SAVE */
        if (saveIdx > 7)
            StackError();
        else {
            memcpy(&saveStack[saveIdx++], &controls.list, 3);
            return;
        }
        break;
    case 15: /* RESTORE */
        if (saveIdx > 0) {
            memcpy(&controls.list, &saveStack[--saveIdx], 3);
            return;
        }
        break;
    case 16: /* EJECT */
        controls.eject++;
        return;
    case 20: /* MAKEDEPEND */
        if (ChkParen('(')) {
            GetFileParam();
            if (!depFile)
                depFile = xstrdup(tokBuf);
        } else if (!depFile) {
            char const *s = basename((char *)includes[0]);
            int len       = (int)(strrchr(s, '.') ? strrchr(s, '.') - s : strlen(s));
            depFile       = xmalloc(len + 9); //  .deps/{src}.d;
            sprintf(depFile, ".deps/%.*s.d", len, s);
        }
        controls.makedepend = true;
        return;
    case 21: /* ISISNAME */
        controls.isisName = true;
        maxSymbolSize   = 6;
        return;
    default:
        return;
    }
    controlError = true;
}

void ParseControls(void) {
    isControlLine  = true;
    ctlListChanged = savedCtlList = controls.list;
    savedCtlGen                   = controls.gen;
    controlError                  = false;

    while (GetTok() != EOLCH && !controlError) {
        if (tokBuf[0] == ';') /* skip comments */
            Skip2EOL();
        else
            ProcessControl(LookupControl());
    }

    if (controlError) {
        if (tokBuf[0] != EOLCH) {
            reget = 0;
            Skip2EOL();
        }

        if (scanCmdLine)
            RuntimeError(RTE_CMDLINE); /* command line error */
        else
            CommandError();
    }

    if (controls.list != savedCtlList)
        ctlListChanged = true;
    else if (controls.gen != savedCtlGen && expandingMacro)
        ctlListChanged = false;

    reget = 0;
}

char const *newInclude(char const *fname) {
    for (uint16_t i = 0; i < includeCnt; i++)
        if (strcmp(includes[i], fname) == 0) // already known
            return includes[i];
    if (includeCnt == MAXINCLUDES)
        FatalError("Too many include files");
    return includes[includeCnt++] = xstrdup(fname);
}

void WriteDepend(char *depFileName) {
    mkpath(depFileName);
    FILE *fp = fopen(depFileName, "wt");
    if (!fp)
        IoError(depFileName, "Can't create dependency file");

    char oname[_MAX_PATH + 1];
    int col = 0;

    // the OS filename is shown e.g. :Fx: is mapped if used
    col += fprintf(fp, "%s:", MapFile(oname, objFile));
    for (int i = 0; i < includeCnt; i++) {
        if (col + strlen(MapFile(oname, includes[i])) > 120) {
            fputs(" \\\n    ", fp);
            col = 4;
        }
        col += fprintf(fp, " %s", oname);
    }
    putc('\n', fp);
    for (int i = 1; i < includeCnt; i++) // skip main source file
        fprintf(fp, "%s:\n", MapFile(oname, includes[i]));
    fclose(fp);
}