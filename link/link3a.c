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

byte controls[] = "\x1\x3MAP" "\x2\x4NAME" "\x3\5PRINT";
static char cin[] = ":CI: ";
static byte cout[] = "\x5" ":CO: ";

static spath_t info;
static char login[] = "\fISIS-II OBJECT LINKER ";
static char msgInvoked[] = " INVOKED by:\r\n";
static char msgtailerror[] = "COMMAND TAIL ERROR NEAR #:";
static byte linkTmpTemplate[] = "\0LINK\0\0TMP";
static char *scmdP;
static char *cmdP;

void FatalCmdLineErr(word errCode)
{
    if (Delimit(cmdP) == cmdP)
    {				/* isn't a filename so a single char */
        if (*cmdP != '\r')       /* don't skip past the EOL */
            cmdP++;
    }
    else
        cmdP = (char *)Delimit(cmdP);	// remove const
    *cmdP = '#';	/* mark after Error() - temp remote const */
    ConOutStr(msgtailerror, 26);
    ReportError(errCode);
    ConOutStr(scmdP, (word)(cmdP - scmdP + 1));
    ConOutStr(CRLF, 2);
    Exit(1);
} /* FatalCmdLineErr() */


void SkipNonArgChars(char const *s)
{

    cmdP = (char *)Deblank(s);	// remove const
    while (*cmdP == '&') {		/* skip continuation lines */
        cmdP = (char *)Deblank(cmdP + 5);	// remove const
    }
} /* SkipNonArgChars() */


void ExpectChar(byte ch, byte errCode)
{
    SkipNonArgChars(cmdP);
    if (*cmdP == ch)
        SkipNonArgChars(cmdP+1);
    else
        FatalCmdLineErr(errCode);
} /* ExpectChar() */


void ChkLP()
{
    ExpectChar('(', ERR227);	/* left parenthesis expected */
} /* ChkLP() */


void ChkRP()
{
    ExpectChar(')', ERR228);	/* right parenthesis expected */
} /* ChkRP() */


void ExpectComma()
{
    ExpectChar(',', ERR203);	/* Invalid() syntax */
} /* ExpectComma() */


void CheckFile()
{
    Spath(cmdP, &info, &statusIO);
    if (statusIO > 0)
        FatalCmdLineErr(statusIO);
    SkipNonArgChars(Delimit(cmdP));
} /* CheckFile() */


void ErrNotDiscFile()
{
    MakeFullName(&info, inFileName.str);
    FileError(ERR17, inFileName.str, true); /* not a disk file */
} /* ErrNotDiscFile() */


void GetModuleName(psModName_t *pstr)
{

    if (*cmdP < '?' || *cmdP > 'Z')
        FatalCmdLineErr(ERR225);	/* invalid name */
    pstr->len = 0;

    while (('0' <= *cmdP && *cmdP <= '9') || ('?' <= *cmdP && *cmdP <= 'Z')) {
        if (pstr->len >= 31)
            FatalCmdLineErr(ERR226);	/* name too long */
        pstr->str[pstr->len++] = *cmdP;	
        cmdP++;
    }
} /* GetModuleName() */


void AddFileToInputList()
{
    MakeFullName(&info, inFileName.str);
    inFileName.len = (byte)(Delimit(inFileName.str) - inFileName.str);
    if (objFileHead == 0)
        objFileHead = curObjFile = (library_t *)GetLow(sizeof(library_t) + inFileName.len);
    else
    {
        curObjFile->link = (library_t *)GetLow(sizeof(library_t) + inFileName.len);
        curObjFile = curObjFile->link;	
    }
    curObjFile->link = 0;
    curObjFile->modList = 0;
    curObjFile->publicsMode = 0;
    curObjFile->hasModules = 0;
    Pstrcpy(&inFileName, &curObjFile->name);
} /* AddFileToInputList() */


void GetInputListItem()
{
    psModName_t curModuleName;

    if (Strequ(cmdP, "PUBLICS", 7))
    {
        cmdP = (char *)Delimit(cmdP);	/* past the PUBLICS - remove const */
        ChkLP();			/* (file */
        CheckFile();
        if (info.deviceType != 3)	/* must be disk file */
            ErrNotDiscFile();
        AddFileToInputList();
        curObjFile->publicsMode = true;	/* record PUBLICS */
        while (*cmdP == ',') {		/* process any more PUBLICS files */
            ExpectComma();
            CheckFile();
            if (info.deviceType != 3)
                ErrNotDiscFile();
            AddFileToInputList();
            curObjFile->publicsMode = true;
        }
        ChkRP();			/* Close() with) */
    }
    else
    {
        CheckFile();			/* check we have a disk file */
        if (info.deviceType != 3)
            ErrNotDiscFile();
        AddFileToInputList();
        if (*cmdP == '(')		/* check if we have a module list */
        {
            ChkLP();		/* gobble up the ( */
            curObjFile->hasModules = TRUE;	/* note have module list */
            GetModuleName(&curModuleName);
            curModule = (curObjFile->modList = (module_t *)GetLow(sizeof(module_t) + curModuleName.len));
            curModule->link = 0;
            curModule->symlist = 0;
            curModule->scode = 0;
            Pstrcpy(&curModuleName, &curModule->name);
            SkipNonArgChars(cmdP);
            while (*cmdP == ',') {		/* get more modules if (specified */
                ExpectComma();
                GetModuleName(&curModuleName);
                curModule->link = (module_t *)GetLow(sizeof(module_t) + curModuleName.len);
                curModule = curModule->link;
                curModule->link = 0;
                curModule->symlist = 0;
                curModule->scode = 0;
                Pstrcpy(&curModuleName, &curModule->name);
                SkipNonArgChars(cmdP);
            }
            ChkRP();
        }
    }
} /* GetInputListItem() */


void ParseControl()
{
    byte i, j, k;
    /* a controls item has format
        index, strlen, string
    */
    k = (byte)(Delimit(cmdP) - cmdP);	/* control string len */
    i= 0;
    j= 0;
    if (k > 0)		/* there is a control */
        while (i < 18) {	/* Size() of the controls table */
            if (k == controls[i + 1] && Strequ(cmdP, (char *)&controls[i + 2], k))
            {	/* found */
                j = controls[i];
                i = 18;	/* force Exit() */
            }
            else
                i = i + controls[i+1] + 2;
        }

    if (j == 0)		/* ! found */
        FatalCmdLineErr(ERR229);	/* Unrecognised() control */
    SkipNonArgChars(cmdP + k);	/* past the control */
    switch (j) {
    case 1:	mapWanted = true; break;	/* MAP */
    case 2: 		/* NAME(modulename) */
        ChkLP();
        GetModuleName(&outModuleName);
        ChkRP();
        break;
    case 3:		/* PRINT(file) */
        ChkLP();
        CheckFile();
        MakeFullName(&info, printFileName.str);
        printFileName.len = (byte)(Delimit(printFileName.str) - printFileName.str);
        ChkRP();
        break;
    }
} /* ParseControl() */


void ReadCmdLine()
{
    Read(CI_DEV, (pointer)cmdP, 128, &actRead, &statusIO);
    FileError(statusIO, cin, true);
    cmdP[actRead] = '\r';
    CrStrUpper(cmdP);
} /* ReadCmdLine() */


void ParseCmdLine()
{
    address p;
    char *q;

    membot = MEMORY;
    topHeap = MemCk();
    if (High(MemCk() - MEMORY) - 1 >= 0x40)
        npbuf = 3072;
    else
        npbuf = 1056;
    sbufP = GetLow(npbuf);
    ebufP = bufP = sbufP + npbuf;
    scmdP = (char *)GetHigh(0);	/* alloc 0 length() buffer below heap. Will() fix later */
    *scmdP = '-';		/* put - at start of command buf */
    Rescan(1, &statusIO);	/* rescan the command line */
    cmdP = scmdP + 1;	/* insert point */
    ReadCmdLine();	/* Read() the command line */
    cmdP = (char *)Deblank(cmdP);	/* skip space and any DEBUG prefix */
    if (Strequ(cmdP, "DEBUG ", 6))
        cmdP = (char *)Deblank(cmdP + 6);
    q = cmdP;
    CheckFile();		/* reads the invoking filename */
    info.ext[0] = 'O';	/* generate the overlay name */
    info.ext[1] = 'V';
    info.ext[2] = 'L';
    MakeFullName(&info, filePath.str); /* make into a normalised file name */
    filePath.len = (byte)(Delimit(filePath.str) - filePath.str); /* add the pascal style string len */
    cmdP = q;		/* back to the start of the invoking command to keep for listing */

    while (*cmdP != '\r') {		/* collect until end of a non continued line */
        if (*cmdP == '&')
        {
            p.cp = cmdP;	/* mark where the & is */
            cmdP = (char *)Deblank(cmdP + 1);
            if (*cmdP != '\r')
                FatalCmdLineErr(ERR203);	/* invalid syntax */
            cmdP = p.cp;	/* back to the & */
            ConOutStr("**", 2);	/* prompt user for more */
            cmdP[1] = '\r'; 	/* put the \r\n** in the buffer */
            cmdP[2] = '\n';
            cmdP[3] = '*';
            cmdP[4] = '*';
            cmdP = cmdP + 5;	/* point to insert next line */
            ReadCmdLine();	/* get the line */
        }
        else
            cmdP = cmdP + 1;	/* next char */
    }	/* do while */

    ConOutStr(&login[1], sizeof(login) - 2);	/* put login message minus form feed */
    ConOutStr(VERSION, 4);
    ConOutStr(CRLF, 2);
    p.bp =  GetHigh((word)(cmdP - scmdP + 2));		/* formally allocate the memory for the command line */
    SkipNonArgChars(Delimit(q));		/* skip the invoking app name */
    objFileHead = 0;				/* end of link file list marker */
    GetInputListItem();				/* get the first file to link */
    while (*cmdP == ',') {				/* get any more in the list */
        ExpectComma();
        GetInputListItem();
    }

    if (Strequ(cmdP, "TO ", 3) || Strequ(cmdP, "TO&", 3))	/* need TO or TO& */
    {
        SkipNonArgChars(cmdP + 2);	/* past the to */
        q = cmdP;				/* start of filename */
        CheckFile();				/* target must be a disk file or :BB: */
        if (info.deviceType != 3 && info.deviceId != 22) /* file || :BB: */
            ErrNotDiscFile();
        cmdP = q;				/* reset */
        MakeFullName(&info, toFileName.str);	/* get the full filename */
        toFileName.len = (byte)(Delimit(toFileName.str) - toFileName.str);
        curObjFile = objFileHead;		/* check target isn't a file we are linking from */
        while (curObjFile > 0) {
            if (Strequ((char *)&toFileName, (char *)&curObjFile->name, toFileName.len + 1) && ! curObjFile->publicsMode)
                FatalCmdLineErr(ERR234);	/* Duplicate() file name */
            curObjFile = curObjFile->link;
        }
        SkipNonArgChars(Delimit(cmdP));
    }
    else
        FatalCmdLineErr(ERR233);	/* 'to' expected */

    /* put the temp file on the same disk as the target (or :BB:) */
    linkTmpTemplate[0] = info.deviceId;
    MakeFullName((spath_t *)linkTmpTemplate, linkTmpFile.str); // MakeFullName only modifies the start of the spath info
    linkTmpFile.len = (byte)(Delimit(linkTmpFile.str) - linkTmpFile.str);
    /* at this point we have the Input() and output files so process the options */
    /* create a default module name from the target file name */
    outModuleName.len = 0;
    while (info.name[outModuleName.len] != 0 && outModuleName.len < 6) {
        outModuleName.str[outModuleName.len] = info.name[outModuleName.len];
        outModuleName.len++;
    }
    /* print to :CO: if (! specified */
    Pstrcpy(cout, &printFileName);
    mapWanted = 0;	/* default is no map file */
    while (*cmdP != '\r') {	/* while there are controls */
        ParseControl();
    }
    for (p.w = 0; p.w <= 127; p.w++) {
        hashTab[p.w] = 0;
    }
    for (p.w = SEG_ABS; p.w <= SEG_RESERVE; p.w++) {
        segLen[p.w] = 0;
        alignType[p.w] = 0;
    }
    for (p.w = 0; p.w <= 255; p.w++) {
        segmap[p.w] = 0;		/* mark seg as unused */
    }
    outTranId = 0;
    outTranVn = 0;
    modEndModTyp = 0;
    modEndSegId = 0;
    modEndOffset = 0;
    headSegOrderLink = 0;
    unresolved = maxExternCnt = 0;
    headUnresolved = 0;
    /* Open() print file (could be console) */
    Open(&printFileNo, printFileName.str, 2, 0, &statusIO);
    FileError(statusIO, printFileName.str, TRUE);
    /* if (printing to other than console, log the login & command line */
    if (printFileNo > 0)
    {
        WriteBytes(login, sizeof(login) - 1);
        WriteBytes(VERSION, 4);
        WriteBytes(msgInvoked, 14);
        WriteBytes(scmdP, (word)(cmdP - scmdP + 2));
    }
} /* ParseCmdLine() */

