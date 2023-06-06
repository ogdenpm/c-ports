/****************************************************************************
 *  link3a.c: part of the C port of Intel's ISIS-II link             *
 *  The original ISIS-II application is Copyright Intel                     *
 *																			*
 *  Re-engineered to C by Mark Ogden <mark.pm.ogden@btinternet.com> 	    *
 *                                                                          *
 *  It is released for hobbyist use and for academic interest			    *
 *                                                                          *
 ****************************************************************************/

#include "link.h"
#include <stdio.h>
#include <ctype.h>

static char *cin  = ":CI:";
static char *cout = ":CO:";


static spath_t info;
static char login[]           = "\fISIS-II OBJECT LINKER ";
static char msgInvoked[]      = " INVOKED by:\r\n";
char *cmdP;

_Noreturn void FatalCmdLineErr(word errCode) {
    if (Delimit(cmdP) == cmdP) { /* isn't a filename so a single char */
        if (*cmdP != '\n')       /* don't skip past the EOL */
            cmdP++;
    } else
        cmdP = (char *)Delimit(cmdP); // remove const
    fputs("COMMAND TAIL ERROR NEAR #:", stderr);
    ReportError(errCode);
    fprintf(stderr, "%.*s#\n", (int)(cmdP - commandLine), commandLine);
    Exit(1);
} /* FatalCmdLineErr() */

void SkipNonArgChars(char const *s) {

    cmdP = (char *)Deblank(s);            // remove const
    while (*cmdP == '&') {                /* skip continuation lines */
        cmdP = (char *)Deblank(cmdP + 4); // remove const
    }
} /* SkipNonArgChars() */

void ExpectChar(byte ch, byte errCode) {
    SkipNonArgChars(cmdP);
    if (*cmdP == ch)
        SkipNonArgChars(cmdP + 1);
    else
        FatalCmdLineErr(errCode);
} /* ExpectChar() */

void ChkLP() {
    ExpectChar('(', ERR227); /* left parenthesis expected */
} /* ChkLP() */

void ChkRP() {
    ExpectChar(')', ERR228); /* right parenthesis expected */
} /* ChkRP() */

void ExpectComma() {
    ExpectChar(',', ERR203); /* Invalid() syntax */
} /* ExpectComma() */



void ErrNotDiscFile(char *token) {
    inFileName = token;
    FileError(ERR17, inFileName, true); /* not a disk file */
} /* ErrNotDiscFile() */

void GetModuleName(char * token, psModName_t *pstr) {
    pstr->len = 0;
    if (isdigit(token[0]))
        FatalCmdLineErr(ERR225); /* invalid name */
    for (char *s = token; *s; s++) {
        if (isalnum(*s) || *s == '?' || *s == '@' || *s == '_') {
            if (pstr->len >= 31)
                FatalCmdLineErr(ERR226); /* name too long */
            else
                pstr->str[pstr->len++] = toupper(*s);
        } else
            FatalCmdLineErr(ERR225); /* invalid name */
    }
} /* GetModuleName() */





void AddFileToInputList(char *token) {
    inFileName = token;
    if (objFileHead == 0)
        objFileHead = curObjFile = (library_t *)GetLow(sizeof(library_t));
    else {
        curObjFile->link = (library_t *)GetLow(sizeof(library_t));
        curObjFile       = curObjFile->link;
    }
    curObjFile->link        = 0;
    curObjFile->modList     = 0;
    curObjFile->publicsMode = 0;
    curObjFile->hasModules  = 0;
    curObjFile->name        = inFileName;
} /* AddFileToInputList() */


/* use a shadow copy of commandLine to create '\0' terminated tokens */
char *GetToken() {
    SkipNonArgChars(cmdP);
    char *token = tokenLine + (cmdP - commandLine);
    while (*cmdP && !strchr(" \t,()&\n", *cmdP))
        cmdP++;
    tokenLine[cmdP - commandLine] = '\0';
    return token;
}

/* ISIS only allowed filenames in 6.3 format so PUBLICS could never be
   a filename. With other OS publics could by the start of a filename
   code modified to check for publics string only
*/

void GetInputListItem() {
    psModName_t curModuleName;
    char *token = GetToken();
    if (!*token)
        FatalCmdLineErr(ERR23);
    if (_stricmp(token, "publics") == 0) {
        ChkLP(); /* (file */
        CheckFile(token);
        if (info.deviceType != 3) /* must be disk file */
            ErrNotDiscFile(token);
        AddFileToInputList(token);
        curObjFile->publicsMode = true; /* record PUBLICS */
        while (*cmdP == ',') {          /* process any more PUBLICS files */
            ExpectComma();
            token = GetToken();
            CheckFile(token);
            if (info.deviceType != 3)
                ErrNotDiscFile(token);
            AddFileToInputList(token);
            curObjFile->publicsMode = true;
        }
        ChkRP(); /* Close() with) */
    } else {
        CheckFile(token); /* check we have a disk file */
        if (info.deviceType != 3)
            ErrNotDiscFile(token);
        AddFileToInputList(token);
        if (*cmdP == '(') /* check if we have a module list */
        {
            ChkLP();                       /* gobble up the ( */
            curObjFile->hasModules = TRUE; /* note have module list */
            token                  = GetToken();
            GetModuleName(token, &curModuleName);
            curModule =
                (curObjFile->modList = (module_t *)GetLow(sizeof(module_t) + curModuleName.len));
            curModule->link    = 0;
            curModule->symlist = 0;
            curModule->scode   = 0;
            Pstrcpy(&curModuleName, &curModule->name);
            SkipNonArgChars(cmdP);
            while (*cmdP == ',') { /* get more modules if (specified */
                ExpectComma();
                token = GetToken();
                GetModuleName(token, &curModuleName);
                curModule->link    = (module_t *)GetLow(sizeof(module_t) + curModuleName.len);
                curModule          = curModule->link;
                curModule->link    = 0;
                curModule->symlist = 0;
                curModule->scode   = 0;
                Pstrcpy(&curModuleName, &curModule->name);
                SkipNonArgChars(cmdP);
            }
            ChkRP();
        }
    }
} /* GetInputListItem() */

void ParseControl() {
    char *token = GetToken();

    if (_stricmp(token, "MAP") == 0)
        mapWanted = true;
    else if (_stricmp(token, "NAME") == 0) {
        ChkLP();
        token = GetToken();
        GetModuleName(token, &outModuleName);
        ChkRP(); 
    } else if (_stricmp(token, "PRINT") == 0) {
        ChkLP();
        token = GetToken();
        CheckFile(token);
        printFileName = token;
        ChkRP();
    } else
        FatalCmdLineErr(ERR229); /* Unrecognised() control */
} /* ParseControl() */

void ParseCmdLine() {

    membot  = MEMORY;
    topHeap = MemCk();
    npbuf   = 3072;

    sbufP   = GetLow(npbuf);
    ebufP = bufP = sbufP + npbuf;
    cmdP         = commandLine + 1;

    printf("OBJECT LINKER %s\n", VERSION); /* put login message */

    GetToken();             // skip the invoke name
    objFileHead = 0;       /* end of link file list marker */
    GetInputListItem();    /* get the first file to link */
    while (*cmdP == ',') { /* get any more in the list */
        ExpectComma();
        GetInputListItem();
    }

    char *token = GetToken();

    if (_stricmp(token, "to") == 0)
    {
        token = GetToken();
        
        /* start of filename */
        CheckFile(token);                                     /* target must be a disk file or :BB: */
        if (info.deviceType != 3 && info.deviceId != 22) /* file || :BB: */
            ErrNotDiscFile(token);
        toFileName = token;
        /* check target isn't a file we are linking from */
        for (curObjFile     = objFileHead; curObjFile; curObjFile = curObjFile->link) {
            if (strcmp(toFileName, curObjFile->name) == 0 &&
                !curObjFile->publicsMode)
                FatalCmdLineErr(ERR234); /* Duplicate() file name */
        }
        SkipNonArgChars(Delimit(cmdP));
    } else
        FatalCmdLineErr(ERR233); /* 'to' expected */

    
    /* at this point we have the Input() and output files so process the options */
    /* create a default module name from the target file name */
    outModuleName.len = 0;
    while (info.name[outModuleName.len] != 0 && outModuleName.len < 6) {
        outModuleName.str[outModuleName.len] = info.name[outModuleName.len];
        outModuleName.len++;
    }
    /* print to :CO: if (! specified */
    printFileName = cout;
    mapWanted = 0;          /* default is no map file */
    while (*cmdP != '\n')  /* while there are controls */
        ParseControl();

    /* Open() print file (could be console) */
    Open(&printFileNo, printFileName, 2, 0, &statusIO);
    FileError(statusIO, printFileName, TRUE);
    /* if (printing to other than console, log the login & command line */
    if (printFileNo > 0) {
        WriteBytes(login, sizeof(login) - 1);
        WriteBytes(VERSION, 4);
        WriteBytes(msgInvoked, 14);
        WriteBytes(commandLine, (word)(cmdP - commandLine + 2));
    }
} /* ParseCmdLine() */


void CheckFile(char *token) {
    Spath(token, &info, &statusIO);
    if (statusIO > 0)
        FatalCmdLineErr(statusIO);
} /* CheckFile() */