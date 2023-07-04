/****************************************************************************
 *  cntrlm.c: part of the C port of Intel's ISIS-II asm80             *
 *  The original ISIS-II application is Copyright Intel                     *
 *																			*
 *  Re-engineered to C by Mark Ogden <mark.pm.ogden@btinternet.com> 	    *
 *                                                                          *
 *  It is released for hobbyist use and for academic interest			    *
 *                                                                          *
 ****************************************************************************/

#include "asm80.h"
#include <ctype.h>

static byte controlTable[] = {
/* format of entries
    byte -> 00PNxxxx where
        P -> 0 general control 1 -> primary control
        N -> 0 NO prefix not supported 1-> NO prefix supported
        xxxx -> number of chars in name string
    name string
*/
            "\x35" "DEBUG"     "\x3A" "MACRODEBUG"
            "\x34" "XREF"      "\x37" "SYMBOLS"
            "\x36" "PAGING"    "\x33" "TTY"
            "\x35" "MOD85"     "\x35" "PRINT"
            "\x36" "OBJECT"    "\x39" "MACROFILE"
            "\x29" "PAGEWIDTH" "\x2A" "PAGELENGTH"
            "\x7"  "INCLUDE"   "\x5"  "TITLE"
            "\x4"  "SAVE"      "\x7"  "RESTORE"
            "\x5"  "EJECT"     "\x14" "LIST"
            "\x13" "GEN"       "\x14" "COND"};

byte tokVal;
bool savedCtlList, savedCtlGen;
bool controlError;


static bool ChkParen(byte parenType)
{
    SkipWhite();
    reget = 0;
    return parenType == curChar;
}



static byte GetTok(void) {
    tokBufLen = 0;
    tokType = TT_ID;
    if (IsEOL())
        return curChar;

    SkipNextWhite();
    if (isalpha(curChar)) {  /* letter */
        GetTokenText(O_ID);
        if (BlankAsmErrCode() && token.size <= 14) {
            memcpy(tokBuf, lineBuf, tokBufLen = token.size);
            tokBuf[tokBufLen] = '\0';
        }
    } else if (isdigit(curChar)) {   /* digit ? */
        GetNum();
        if (BlankAsmErrCode()) {
            tokNumVal = GetNumVal();
            tokType = TT_NUM;
            tokBuf[0] = ' ';
        }
    } else if (curChar == '\'') {   /* string ? */
        GetStr();
        if (BlankAsmErrCode()) {
            tokBufLen = 64;         /* cap at 64 chars */
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
    PopToken();      /* restore the stack from GetId/GetNum/GetStr() calls */
    return tokBuf[0];
}



static void GetFileNameOpt(void) {
    SkipNextWhite();
    int endCh = ')';
    int spcCnt  = 0;    // keep count of trailing spaces. Avoids adding to tokBuf

    if (curChar == '\'') {
        endCh = '\'';
        curChar = GetCh();
    }
    while (curChar != EOLCH && curChar != endCh && tokBufIdx <= MAXFILENAME) {
        if (endCh == ')' && curChar == ' ') // possibly trailing space
            spcCnt++;
        else {
            // any spaces pending, were actually embedded in the name, add them back
            while (spcCnt && tokBufIdx <= MAXFILENAME) {
                tokBuf[tokBufIdx++] = ' ';
                spcCnt--;
            }
            if (tokBufIdx <= MAXFILENAME)   // just in cases space addition cased overrun
                tokBuf[tokBufIdx++] = curChar;
        }

        curChar = GetCh();
    }
    tokBuf[tokBufIdx] = '\0';
    if (endCh == '\'' && curChar == '\'')
        SkipNextWhite();
    if (curChar != ')' || tokBufIdx == 0 || tokBufIdx > MAXFILENAME)
        FileError();
}


static void GetFileParam(void) {
    tokBufIdx = 0;
    if (!ChkParen('(')) /* ( */
        FileError();
    else {
        GetFileNameOpt();
        if (! ChkParen(')'))    /* ) */
            FileError();
    }
}

static bool GetControlNumArg(void) {
    if (ChkParen('(')) { /* ( */
        tokVal = GetTok();
        if (tokType == TT_NUM)
            return ChkParen(')');    /* ) */
    }
    return false;
}

static void SetControl(byte ctlVal, byte noInvalid)
{
    if (!noInvalid) {
        controls.all[controlId] = ctlVal;
        if (!ctlVal)
            controlId = 17;    /* no action needed */
    }
}

static byte LookupControl(void) {
    byte cmdIdx, cmdStartIdx;
    bool ctlVal;
    byte cmdLen, ctlFlags, noInvalid;
    pointer controlP, nextControlP;
    bool *ctlSeenP;

    cmdLen = (byte)tokBufLen;
    cmdStartIdx = 0;
    ctlVal = true;
    if (tokBuf[0] == 'N' && tokBuf[1] == 'O') {   /* check for NO prefix */
        cmdStartIdx = 2;    /* don't match the NO in the table */
        ctlVal = false;        /* control will be set as false */
        cmdLen = tokBufLen - 2;    /* length of string to match excludes the NO */
    }

    controlP = controlTable;
    controlId = 0;
    cmdIdx = cmdStartIdx;

    while (controlId < sizeof(controls)) {
        nextControlP = controlP + (*controlP & 0xF) + 1;
        if ((*controlP & 0xF) == cmdLen)
        {
            ctlFlags = *controlP;
            while (cmdIdx < tokBufLen) {
                controlP++;
                if (*controlP != tokBuf[cmdIdx])
                    cmdIdx = tokBufLen + 1;    /* cause early Exit() */
                else
                    cmdIdx = cmdIdx + 1;    /* check next character */
            }

            if (cmdIdx == tokBufLen)        /* found it */
                goto found;
        }
        controlP = nextControlP;
        controlId = controlId + 1;
        cmdIdx = cmdStartIdx;
    }
    return 255;                        /* not found */

found:
    if ((noInvalid = (ctlFlags & 0x10) != 0x10) && !ctlVal)
        return 255;    /* NO not supported */

    if ((ctlFlags & 0x20) != 0x20)        /* general control */
    {
        SetControl(ctlVal, noInvalid);
        return controlId;
    }

    if (! primaryValid)            /* is a PRIMARY control */
        return 255;

    ctlSeenP = &controlSeen[controlId];        /* make sure we only see a primary once */
    if (*ctlSeenP)
        return 255;
    *ctlSeenP = true;
    SetControl(ctlVal, noInvalid);
    return controlId;
}

static void ProcessControl(void) {
    /* simple controls already processed */
    if (controlId >= 17 || controlId < 5)
        return;

    switch (controlId - 5) {
    case 0:            /* TTY */
            controls.tty = true;
            return;
    case 1:            /* MOD85 */
            controls.mod85 = true;
            return;
    case 2:            /* PRINT */
            GetFileParam();
            if (lstFile)
                free(lstFile);
            lstFile = strdup(tokBuf);
            return;
    case 3:            /* OBJECT */
            GetFileParam();
            if (objFile)
                free(objFile);
            objFile = strdup(tokBuf);
            return;
    case 4:            /* MACROFILE */
            controls.macroFile = true;
            if (ChkParen('(')) {
                while ((curChar = GetCh()) != ')')
                    if (IsEOL())
                        FatalError("Missing ')' parsing MACROFILE (drive)");

            } else
                reget = 1;
            return;
    case 5:            /* PAGEWIDTH */
            if (GetControlNumArg()) {
                if (tokNumVal == 0)
                    tokNumVal = 0xffff;
                else if (tokNumVal < 72)
                    tokNumVal = 72;
                pageWidth          = tokNumVal;

                return;
            }
            break;
    case 6:            /* PAGELENGTH */
            if (GetControlNumArg()) {
                if (tokNumVal == 0)
                    tokNumVal = 0xffff;
                else if (tokNumVal < 15)
                    tokNumVal = 15;
                pageLength          = tokNumVal;
                return;
            }
            break;
    case 7:            /* INCLUDE */
            if (! pendingInclude) {		// multiple includes on control line not supported
                if (fileIdx == 5)
                    StackError();
                else
                {
                    fileIdx++;
                    GetFileParam();
                    files[fileIdx].name = strdup(tokBuf);
                    pendingInclude = true;
                    if (scanCmdLine)
                        includeOnCmdLine = true;
                    return;
                }
            }
            break;
    case 8:            /* TITLE */
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
    case 9:            /* SAVE */
            if (saveIdx > 7)
                StackError();
            else {
                memcpy(&saveStack[saveIdx++], &controls.list, 3);
                return;
            }
            break;
    case 10:            /* RESTORE */
            if (saveIdx > 0) {
                memcpy(&controls.list, &saveStack[--saveIdx], 3);
                return;
            }
            break;
    case 11:            /* EJECT */
        controls.eject++;
            return;
    }
    controlError = true;
}

void ParseControls(void) {
    isControlLine = true;
    ctlListChanged = savedCtlList = controls.list;
    savedCtlGen = controls.gen;
    controlError = false;

    while (GetTok() != EOLCH && ! controlError) {
        if (tokBuf[0] == ';')        /* skip comments */
            Skip2EOL();
        else if (LookupControl() == 255)    /* Error() ? */
            controlError = true;
        else
            ProcessControl();
    }

    if (controlError) {
        if (tokBuf[0] != EOLCH) {
            reget = 0;
            Skip2EOL();
        }

        if (scanCmdLine)
            RuntimeError(RTE_CMDLINE);    /* command line error */
        else
            CommandError();
    }

    if (controls.list != savedCtlList)
        ctlListChanged = true;
    else if (controls.gen != savedCtlGen && expandingMacro)
        ctlListChanged = false;

    reget = 0;
}

