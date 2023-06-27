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
#include <ctype.h>
#include <stdio.h>

char const *cmdP;
bool echoToStderr;
char *toName;
_Noreturn void FatalCmdLineErr(char const *errMsg) {
    if (*cmdP != '\n') /* don't skip past the EOL */
        cmdP++;
    strcpy((char *)cmdP, "#\n"); // mark problem (remove 'const' from cmdP)
    fprintf(stderr, "Command line error near #: %s\n", errMsg);
    printCmdLine(stderr);
    Exit(1);
} /* FatalCmdLineErr() */

void SkipNonArgChars(char const *s) {

    cmdP = Deblank(s);            // remove const
    while (*cmdP == '&') {        /* skip continuation lines */
        cmdP = Deblank(cmdP + 4); // remove const
    }
} /* SkipNonArgChars() */

void ExpectChar(byte ch, char const *errMsg) {
    SkipNonArgChars(cmdP);
    if (*cmdP == ch)
        SkipNonArgChars(cmdP + 1);
    else
        FatalCmdLineErr(errMsg);
} /* ExpectChar() */

void ChkLP() {
    ExpectChar('(', "Left parenthesis expected");
} /* ChkLP() */

void ChkRP() {
    ExpectChar(')', "Right parenthesis expected");
} /* ChkRP() */

void ExpectComma() {
    ExpectChar(',', "Comma expected");
} /* ExpectComma() */

void GetModuleName(char *token, psModName_t *pstr) {
    pstr->len = 0;
    if (!token[0])
        FatalCmdLineErr("Missing module name");
    if (isdigit(token[0]))
        FatalCmdLineErr("Invalid module name, cannot start with a digit");
    for (char *s = token; *s; s++) {
        if (isalnum(*s) || *s == '?' || *s == '@' || *s == '_') {
            if (pstr->len >= 31)
                FatalCmdLineErr("Module name too long");
            else
                pstr->str[pstr->len++] = toupper(*s);
        } else
            FatalCmdLineErr("Invalid module name, illegal character");
    }
} /* GetModuleName() */

void AddFileToInputList(char *token) {
    if (objFileList == 0)
        objFileList = objFile = xmalloc(sizeof(objFile_t));
    else {
        objFile->next = xmalloc(sizeof(objFile_t));
        objFile       = objFile->next;
    }
    objFile->next    = 0;
    objFile->modules = 0;
    objFile->publics = false;
    objFile->isLib   = false;
    objFile->name    = token;
}

/* use a shadow copy of commandLine to create '\0' terminated tokens */
char *GetToken() {
    SkipNonArgChars(cmdP);
    char *token                   = tokenLine + (cmdP - commandLine);
    cmdP                          = Delimit(cmdP);
    tokenLine[cmdP - commandLine] = '\0';
    return token;
}

/* ISIS only allowed filenames in 6.3 format so PUBLICS could never start
   a filename, code modified to check for publics string only
*/

void GetInputListItem() {
    psModName_t curModuleName;
    char *token = GetToken();
    if (!*token)
        FatalCmdLineErr("Expected name");
    if (stricmp(token, "publics") == 0) {
        ChkLP(); // publics ( file [,file]* )
        AddFileToInputList(GetToken());
        objFile->publics = true; /* record PUBLICS */
        while (*cmdP == ',') {   /* process any more PUBLICS files */
            ExpectComma();
            AddFileToInputList(GetToken());
            objFile->publics = true;
        }
        ChkRP(); /* Close() with) */
    } else {
        AddFileToInputList(token);
        if (*cmdP == '(') /* check if we have a module list */
        {
            ChkLP();               /* gobble up the ( */
            objFile->isLib = true; /* note have module list */
            GetModuleName(GetToken(), &curModuleName);
            module          = (objFile->modules = xmalloc(sizeof(module_t) + curModuleName.len + 1));
            module->next    = 0;
            module->symbols = 0;
            module->cbias   = 0;
            freezePstr((pstr_t *)&curModuleName, &module->name);
            SkipNonArgChars(cmdP);
            while (*cmdP == ',') { /* get more modules if (specified */
                ExpectComma();
                GetModuleName(GetToken(), &curModuleName);
                module->next    = xmalloc(sizeof(module_t) + curModuleName.len + 1);
                module          = module->next;
                module->next    = 0;
                module->symbols = 0;
                module->cbias   = 0;
                freezePstr((pstr_t *)&curModuleName, &module->name);
                SkipNonArgChars(cmdP);
            }
            ChkRP();
        }
    }
} /* GetInputListItem() */

void ParseControl() {
    char *token = GetToken();

    if (stricmp(token, "MAP") == 0 || strcmp(token, "-m") == 0)
        mapWanted = true;
    else if (stricmp(token, "NAME") == 0) {
        ChkLP();
        GetModuleName(GetToken(), &moduleName);
        ChkRP();
    } else if (strcmp(token, "-n") == 0)
        GetModuleName(GetToken(), &moduleName);
    else if (stricmp(token, "PRINT") == 0) {
        ChkLP();
        lstName = GetToken();
        if (!*lstName)
            FatalCmdLineErr("Missing listing file for PRINT control");
        ChkRP();
    } else if (strcmp(token, "-p") == 0) {
        lstName = GetToken();
        if (!*lstName)
            FatalCmdLineErr("Missing listing file for -p control");
    } else if (stricmp(token, "NOWARN") == 0 || strcmp(token, "-w") == 0)
        warnOK = false;
    else
        FatalCmdLineErr("Unknown control");
} /* ParseControl() */

void ParseCmdLine() {
    cmdP = commandLine + 1;

    puts("OMF85 Object Linker " VERSION); /* put login message */

    GetToken();            // skip the invoke name
    objFileList = 0;       /* end of link file list marker */
    GetInputListItem();    /* get the first file to link */
    while (*cmdP == ',') { /* get any more in the list */
        ExpectComma();
        GetInputListItem();
    }

    char *token = GetToken();

    if (stricmp(token, "to") == 0 || strcmp(token, "-o") == 0)
        toName = GetToken();
    else
        FatalCmdLineErr("'TO' or '-o' expected");

    SkipNonArgChars(Delimit(cmdP));

    while (*cmdP != '\n') /* while there are controls */
        ParseControl();

    /* check target isn't a file we are linking from */
    for (objFile = objFileList; objFile; objFile = objFile->next) {
        if (namecmp(toName, objFile->name) == 0 && !objFile->publics)
            FatalCmdLineErr("Target file is duplicate of input file");
    }
    // if no name, generate one from the output file
    if (moduleName.len == 0) {
        char *s = basename(toName);
        for (moduleName.len = 0; moduleName.len < 31 && *s && *s != '.'; s++, moduleName.len++)
            moduleName.str[moduleName.len] =
                isalnum(*s) || *s == '@' || *s == '_' ? toupper(*s) : '@';
        if (moduleName.len == 0)
            strcpy((char *)&moduleName, "\x6NONAME");
        else if (isdigit(moduleName.str[0]))
            moduleName.str[0] = '@';
    }

    if (!lstName || !*lstName) {
        lstFp   = stdout;
        lstName = "stdout";
    } else if (!(lstFp = Fopen(lstName, "wt")))
        IoError(lstName, "Open error");
    echoToStderr = !isatty(fileno(lstFp));
    /* if (printing to other than console, log the login & command line */
    if (lstFp != stdout) {
        Printf("OMF85 OBJECT LINKER " VERSION " INVOKED BY:\n");
        printCmdLine(lstFp);
    }
}

// print the command line, splitting long lines
void printCmdLine(FILE *fp) {
    int col = 0;
    char *s = commandLine;
    char *brk;
    while ((brk = strpbrk(s, ", &\n"))) {
        brk++; // include the break char
        if (col + (brk - s) >= 120) {
            fputs("\\\n    ", fp);
            col = 4;
        }
        fprintf(fp, "%.*s", (int)(brk - s), s);
        if (brk[-1] != '\n')
            col += (int)(brk - s);
        else
            col = 0;
        s = brk;
    }
}