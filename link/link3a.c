/****************************************************************************
 *  link3a.c: part of the C port of Intel's ISIS-II link                    *
 *  The original ISIS-II application is Copyright Intel                     *
 *                                                                          *
 *  Re-engineered to C by Mark Ogden <mark.pm.ogden@btinternet.com>         *
 *                                                                          *
 *  It is released for academic interest and personal use only              *
 ****************************************************************************/
#include "link.h"
#include <ctype.h>
#include <stdio.h>
#include "cmdline.h"
#include "utility.h"

void ChkLP() {
    ExpectChar('(', "Left parenthesis expected");
} /* ChkLP() */

void ChkRP() {
    ExpectChar(')', "Right parenthesis expected");
} /* ChkRP() */

void ExpectComma() {
    ExpectChar(',', "Comma expected");
} /* ExpectComma() */

void AddFileToInputList(char *token) {
    if (objFileList == 0)
        objFileList = objFile = safeMalloc(sizeof(objFile_t));
    else {
        objFile->next = safeMalloc(sizeof(objFile_t));
        objFile       = objFile->next;
    }
    objFile->next    = 0;
    objFile->modules = 0;
    objFile->publics = false;
    objFile->isLib   = false;
    objFile->name    = token;
}

/* ISIS only allowed filenames in 6.3 format so PUBLICS could never start
   a filename, code modified to check for publics string only
*/

void GetInputListItem() {
    pstr_t const *curModuleName;
    char *token = GetToken();
    if (!*token)
        FatalCmdLineErr("Expected file name");
    if (stricmp(token, "publics") == 0) {
        ChkLP(); // publics ( file [,file]* )
        token = GetToken();
        if (!*token)
            FatalCmdLineErr("Expected module name\n");
        AddFileToInputList(token);
        objFile->publics = true;                         /* record PUBLICS */
        while (*cmdP && *cmdP != '\n' && *cmdP != ')') { /* process any more PUBLICS files */
            if (*cmdP == ',')
                cmdP++;
            AddFileToInputList(GetToken());
            objFile->publics = true;
        }
        ChkRP(); /* Close() with) */
    } else {
        AddFileToInputList(token);
        if (*cmdP == '(') /* check if we have a module list */
        {
            ChkLP();                /* gobble up the ( */
            objFile->isLib  = true; /* note have module list */
            curModuleName   = GetModuleName(GetToken());
            module          = (objFile->modules = safeMalloc(sizeof(module_t)));
            module->next    = 0;
            module->symbols = 0;
            module->cbias   = 0;
            module->name    = curModuleName;
            while (*cmdP && *cmdP != ')') { /* get more modules if (specified */
                if (*cmdP == ',')
                    cmdP++;
                token = GetToken();
                if (!*token)
                    FatalCmdLineErr("Expected module name\n");
                curModuleName   = GetModuleName(token);
                module->next    = safeMalloc(sizeof(module_t));
                module          = module->next;
                module->next    = 0;
                module->symbols = 0;
                module->cbias   = 0;
                module->name    = curModuleName;
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
        moduleName = GetModuleName(GetToken());
        ChkRP();
    } else if (stricmp(token, "PRINT") == 0) {
        ChkLP();
        lstName = GetText();
        if (!*lstName)
            FatalCmdLineErr("Missing listing file for PRINT control");
        ChkRP();
    } else if (stricmp(token, "EXTERNOK") == 0)
        externOk = true;
    else
        FatalCmdLineErr("Unknown control");
} /* ParseControl() */

void ParseCmdLine() {
    puts("OMF85 OBJECT LINKER " VERSION); /* put login message */

    GetNonSpaceToken();                      // skip the invoke name
    objFileList = 0;                 /* end of link file list marker */
    GetInputListItem();              /* get the first file to link */
    while (*cmdP && *cmdP != '\n') { /* get any more in the list */
        if (*cmdP == ',')
            cmdP++;
        else {
            char *peekToken = PeekToken();
            if (stricmp(peekToken, "TO") == 0)
                break;
        }
        GetInputListItem();
    }

    char *token = GetToken();

    if (stricmp(token, "to") == 0)
        omfOutName = GetToken();
    else
        FatalCmdLineErr("'TO' expected");

    while (*cmdP != '\n') /* while there are controls */
        ParseControl();

    /* check target isn't a file we are linking from */
    for (objFile = objFileList; objFile; objFile = objFile->next) {
        if (namecmp(omfOutName, objFile->name) == 0 && !objFile->publics)
            FatalCmdLineErr("Target file is duplicate of input file");
    }
    // if no name, generate one from the output file
    if (!moduleName || moduleName->len == 0) {
        char const *fname = basename(omfOutName);
        // use local non const var to setup the name
        pstr_t *mname = (pstr_t *)c2pstrdup(*fname ? fname : "NONAME");
        if (mname->len > 31)
            mname->str[31] = '\0';
        char *s = mname->str;
        if (isdigit(*s))
            *s++ = '@';
        for (; *s && (*s != '.' || s == mname->str); s++)
            *s = isalnum(*s) || *s == '@' || *s == '_' ? toupper(*s) : '@';
        *s         = '\0';
        mname->len = (uint8_t)(s - mname->str);
        moduleName = mname;
    }
    openLst("OMF85 OBJECT LINKER " VERSION);
    /* if (printing to other than console, log the login & command line */
}