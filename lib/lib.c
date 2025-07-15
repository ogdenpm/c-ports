/****************************************************************************
 *  lib.c: part of the C port of Intel's ISIS-II lib             *
 *  The original ISIS-II application is Copyright Intel                     *
 *                                                                          *
 *  Re-engineered to C by Mark Ogden <mark.pm.ogden@btinternet.com>         *
 *                                                                          *
 *  It is released for academic interest and personal use only              *
 ****************************************************************************/

/*
    vim:ts=4:shiftwidth=4:expandtab:
*/

#include <setjmp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef _MSC_VER
#include <io.h>
#include <process.h>
#else
#include <unistd.h>
#define stricmp strcasecmp
#endif
#include "_version.h"
#include "cmdline.h"
#include "lib.h"
#include "omf.h"
#include "os.h"
#include "utility.h"
static file_t *fileHead;
static file_t *fileChain;

char const help[] =
    "Command line usage: %s [Command]\n"
    "Interactive usage:  %s\n"
    "                    Command\n"
    "                    ...\n"
    "Where Command is one of:\n"
    "  ADD fileModuleList TO libname\n"
    "  CREATE libname\n"
    "  DELETE libname(module[,module]*)\n"
    "  EXIT\n"
    "  HELP\n"
    "  INIT fileModuleList TO libname\n"
    "  LIST file fileModuleList [PUBLICS]\n\n"
    "The first character of each command can be used as a shorthand\n"
    "The INIT option is like ADD but ignores any existing library content\n"
    "\n"
    "fileModuleList syntax is: file[(module[,module]*)][,file[(module[,module]*)]]*\n"
    "Spaces can be used instead of commas to separate module and file names\n"
    "File names are of the format [:Fx:]path, where x is a digit and path\n"
    "The :Fx: maps to a directory prefix from the ISIS_Fx environment variable\n";

char *breakList[] = { "PUBLICS", "TO", NULL };

jmp_buf reset;

static void GetFileAndModuleNames(bool oneOnly, char *const *breakWords) {
    /* add another arg entry to the chain */
    bool needName = false;
    for (;;) {
        char *savePos = cmdP;
        char *token   = GetToken();
        if (!*token) {
            if (!fileHead || !needName)
                return;
            FatalCmdLineErr("Expected file name");
        }
        if (!needName && breakWords) {
            for (char *const *s = breakWords; *s; s++)
                if (stricmp(token, *s) == 0) {
                    cmdP = savePos;
                    return;
                }
        }
        fileChain->next    = safeMalloc(sizeof(file_t));
        fileChain          = fileChain->next;
        fileChain->name    = token;
        fileChain->next    = NULL;
        fileChain->modules = NULL;
        if (*cmdP == '(') { /* add the list of modules if present */
            cmdP++;         // past (
            namelist_t *p = (namelist_t *)&fileChain->modules;
            do {
                token = GetToken();
                if (!*token)
                    FatalCmdLineErr("Expected module name");
                namelist_t *q;
                for (q = fileChain->modules; q; q = q->next)
                    if (stricmp(q->name->str, token) == 0)
                        break;
                if (q) {
                    fprintf(stderr, "Skipping duplicate module request %s(...,%s%s)\n",
                            fileChain->name, q->name->str, *cmdP == ',' ? ",..." : "");
                    continue;
                }
                p->next = safeMalloc(sizeof(namelist_t));
                p       = p->next;
                p->next = NULL;
                p->seen = false;
                p->name = NULL; // in case GetModule errors
                p->name = GetModuleName(token);
                if ((needName = *cmdP == ','))
                    cmdP++;

            } while (*cmdP && *cmdP != ')');
            if (needName)
                FatalCmdLineErr("Expected module name");
            ExpectChar(')', "Missing )");
        }
        if (oneOnly)
            return;
        if ((needName = *cmdP == ','))
            cmdP++;
    }
}

static void AddCmd(bool ignoreExisting) {
    GetFileAndModuleNames(false, &breakList[1]);
    if (!fileHead) {
        if (*cmdP == '\n')
            fprintf(stderr, "Nothing to do\n");
        else {
            SetEndToken(cmdP + 1);
            FatalCmdLineErr("Unexpected text");
        }
        return;
    }
    /* need a TO */
    char *token = GetToken();
    if (stricmp(token, "TO"))
        FatalCmdLineErr("Expected 'TO'");

    char const *libFileName = GetToken();

    if (*cmdP != '\n') {
        SetEndToken(cmdP + 1);
        FatalCmdLineErr("Text after library name");
    }

    InitLib();
    if (!ignoreExisting)
        ProcessFile(libFileName, true, NULL); // copy the current library
    for (file_t *p = fileHead; p; p = p->next)
        ProcessFile(p->name, true, p->modules);

    FinaliseLib(libFileName);
}

static void DeleteCmd() {
    GetFileAndModuleNames(true, NULL);

    if (!fileHead)
        FatalCmdLineErr("Missing library name");
    if (*cmdP != '\n') {
        SetEndToken(cmdP + 1);
        FatalCmdLineErr("Text after file module list");
    }
    if (!fileHead->modules) {
        fprintf(stderr, "Nothing to delete\n");
        return;
    }

    InitLib();
    ProcessFile(fileHead->name, false, fileHead->modules);
    FinaliseLib(fileHead->name);
}

static void CreateCmd() {
    char *libName = GetToken();
    if (!*libName)
        FatalCmdLineErr("Expected library name");
    if (*cmdP != '\n') {
        SetEndToken(cmdP + 1);
        FatalCmdLineErr("Unexpected text");
    }
    if (Access(libName, 0) == 0)
        IoError(libName, "Already exists");
    InitLib();
    FinaliseLib(libName);
}

FILE *lstFp;
char *lstName;

void listReset(int retCode) {
    SetITrap(0);
    longjmp(reset, retCode ? retCode : -1);
}

static void ListCmd() {
    /* collect the users list of libraries */
    GetFileAndModuleNames(false, breakList);

    char *lstName = NULL; /* assume console output */
    char *token   = GetToken();
    if (stricmp(token, "TO") == 0) {
        lstName = GetToken();
        token   = GetToken();
    }

    bool listPublics = stricmp(token, "PUBLICS") == 0;
    if (*token && !listPublics)
        FatalCmdLineErr("Unknown control");
    else if (*cmdP != '\n') {
        SetEndToken(cmdP + 1);
        FatalCmdLineErr("Unexpected text");
    }

    SetITrap(listReset);

    if (!lstName || !(lstFp = Fopen(lstName, "wt"))) {
        lstFp   = stdout;
        lstName = "stdout";
    }
    for (file_t *p = fileHead; p; p = p->next) {
        openOMFIn(p->name);
        fprintf(lstFp, "  %s\n", p->name);

        GetRecord();
        if (inType != R_LIBHDR)
            IoError(p->name, "Not a library");

        ReadWord();                // mod count
        SeekOMFIn(ReadLocation()); // names location
        GetRecord();
        if (inType != R_LIBNAM)
            IoError(p->name, "Library 'Names' record not found");
        char *nameList = safeMalloc(recLen); // save a copy
        char *nameIn   = nameList;
        char *nameEnd  = nameIn + recLen - 1;
        memcpy(nameList, inP, recLen);
        if (listPublics) {
            GetRecord();
            if (inType != R_LIBLOC)
                IoError(p->name, "Library 'Locations' record not found");
            GetRecord();
            if (inType != R_LIBDIC)
                IoError(p->name, "Library 'Dictionary' record not found");
        }

        while (nameIn < nameEnd) { /* scan the names until the CRC */
            pstr_t *moduleName  = (pstr_t *)nameIn;
            bool listThisModule = p->modules == NULL;
            for (namelist_t *q = p->modules; q; q = q->next) {
                if (pstrequ(moduleName, q->name)) {
                    q->seen        = true;
                    listThisModule = true;
                    break;
                }
            }

            if (listThisModule) /* list name with 4 leading spaces */
                fprintf(lstFp, "    %.*s\n", moduleName->len, moduleName->str);

            if (listPublics) {
                pstr_t const *pubName = ReadName();
                while (pubName->len) {  /* scan the names until the end of the group */
                    if (listThisModule) /* emit name with 6 leading spaces if required */
                        fprintf(lstFp, "      %.*s\n", pubName->len, pubName->str);
                    pubName = ReadName();
                }
            }
            nameIn += moduleName->len + 1;
        }
        for (namelist_t *q = p->modules; q; q = q->next)
            if (!q->seen)
                fprintf(stderr, "    %s Not found\n", q->name->str);
        free(nameList);
    }
    closeOMFIn();

    if (lstFp != stdout)
        fclose(lstFp);
    SetITrap(0);
}

void cmdReset(int retCode) {
    SetITrap(0);
    longjmp(reset, retCode ? retCode : -1);
}
void Start() {
    bool interactive = *cmdP == '*';
    if (interactive)
        puts("OMF85 LIBRARIAN - " GIT_VERSION);

    GetNonSpaceToken(); // skip the lib cmd itself

    while (1) {
        if (setjmp(reset) == 0)
            break;
        // reset :
        if (!interactive)
            exit(1);
    }
    // main:
    while (1) {
        for (file_t *q, *p = fileHead; p; p = q) {
            q = p->next;
            for (namelist_t *n, *m = p->modules; m; m = n) {
                n = m->next;
                free((void *)m->name);
                free(m);
            }
            // filename is in token shadow copy so no need to free
            free(p);
        }
        fileHead  = NULL;
        fileChain = (file_t *)&fileHead;

        SetITrap(cmdReset);
        if (interactive)
            cmdP = getInteractiveLine() + 1;
        if (!*cmdP) // EOF
            exit(0);
        char *token = GetToken();
        if (stricmp(token, "EXIT") == 0 || stricmp(token, "E") == 0)
            exit(0);
        else if (stricmp(token, "ADD") == 0 || stricmp(token, "A") == 0)
            AddCmd(false);
        else if (stricmp(token, "INIT") == 0 || stricmp(token, "I") == 0)
            AddCmd(true);
        else if (stricmp(token, "CREATE") == 0 || stricmp(token, "C") == 0)
            CreateCmd();
        else if (stricmp(token, "LIST") == 0 || stricmp(token, "L") == 0)
            ListCmd();
        else if (stricmp(token, "DELETE") == 0 || stricmp(token, "D") == 0)
            DeleteCmd();
        else if (stricmp(token, "HELP") == 0 || stricmp(token, "H") == 0)
            usage(NULL);
        else if (*token)
            FatalCmdLineErr("Unrecognised command");
        else if (*cmdP != '\n') {
            SetEndToken(cmdP + 1);
            FatalCmdLineErr("Unexpected text");
        }
        if (!interactive)
            exit(0);
    }
}

