/****************************************************************************
 *  link.c: part of the C port of Intel's ISIS-II link             *
 *  The original ISIS-II application is Copyright Intel                     *
 *																			*
 *  Re-engineered to C by Mark Ogden <mark.pm.ogden@btinternet.com> 	    *
 *                                                                          *
 *  It is released for hobbyist use and for academic interest			    *
 *                                                                          *
 ****************************************************************************/

/*
 * vim:ts=4:shiftwidth=4:expandtab:
 */
#include "link.h"
#include <stdarg.h>
#include <stdio.h>
FILE *objFp;
FILE *outFp;
FILE *lstFp;
char *objName;
char *outName;
char *lstName;
bool mapWanted;
psModName_t moduleName;
byte moduleType;
byte tranId;
byte tranVn;
byte entrySeg;
word entryOffset;
word segLen[6];
byte alignType[6];
byte segmap[256];
pointer inEnd;
pointer inP;
word recNum;
word recLen;
pointer outP;
objFile_t *objFileList;
objFile_t *objFile;
module_t *module;
symbol_t *hashTab[128];
symbol_t *headCommSym;
word unresolved;
word maxExternCnt;
symbol_t *unresolvedList;
int warned     = 0;

byte COPYRIGHT[] = "[C] 1976, 1977, 1979 INTEL CORP'";

_Noreturn void RecError(char const *errMsg) {
    fprintf(stderr, " %s", objName);

    if (module)
        fprintf(stderr, "(%s)", p2cstr(&module->name));

    fprintf(stderr, ", %s\n Record Type %02XH, Record Number ", errMsg, inType);
    if (recNum > 0)
        fprintf(stderr, "%5d\n", recNum);
    else
        fputs("*****\n", stderr);

    Exit(1);
} /* FatalErr() */

_Noreturn void IllFmt() {
    RecError("Illegal record format");
}

_Noreturn void IllegalRelo() {
    RecError("Invalid record");
}

_Noreturn void BadRecordSeq() {
    RecError("Bad record sequence");
}

void pstrcpy(pstr_t const *psrc, pstr_t *pdst) {
    memcpy(pdst, psrc, psrc->len + 1);
} /* pstrcpy() */

byte HashF(pstr_t *pstr) {
    byte i, j;

    j = pstr->len;
    for (i = 0; i < pstr->len; i++)
        j = RorB(j, 1) ^ pstr->str[i];
    return j & 0x7F;
} /* HashF() */

bool Lookup(pstr_t *pstr, symbol_t **symRef, byte mask) {
    symbol_t *p = (symbol_t *)&hashTab[HashF(pstr)];

    *symRef     = p;
    for (p = p->hashChain; p; p = p->hashChain) {
        *symRef = p;
        if ((p->flags & mask) != AUNKNOWN && PStrequ(pstr, &p->name))
            return true;
    }
    return false;
} /* Lookup() */

void Printf(char const *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    if (vfprintf(lstFp, fmt, args) < 0)
        IoError(lstName, "Write error");
    va_end(args);
}

void PrintAndEcho(char const *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    if (vfprintf(lstFp, fmt, args) < 0)
        IoError(lstName, "Write error");
    /* 
    * ISIS II does not have a STDERR device
    * Here if the listing file is not a device
    * echo to stderr
    * in the case where stdout and stderr are left as the console
    * this will do the right thing.
    */
    if (echoToStderr)
        vfprintf( stderr, fmt, args);
    va_end(args);
    warned = 1;
}

void ModuleWarning(char const *msg) {
    PrintAndEcho("%s%s(%s)\n", msg, objFile->name, &module->name);
}

void Start() {
    ParseCmdLine();
    Phase1();
    Phase2();
    Exit(warnOK ? 0 : warned);

} /* Start */

_Noreturn void Exit(int result) {
    if (result) {
        if (outFp)
            fclose(outFp);
        unlink(outName);
    }
    if (lstFp)
        fclose(lstFp);
    exit(result);
}

_Noreturn void usage() {
    printf("Usage: %s inputList (TO | -o) targetFile [link option]*\n"
           "or:    %s (-v | -V | -h)\n",
           invokeName, invokeName);
    printf("Where:\n"
           "-h               Show this help\n"
           "-v / -V          Show simple / extended version information\n"
           "Link options are:\n"
           "-m               Include link map information in listing\n"
           "-n moduleName    Set targetFile module name. '_' is now supported\n"
           "-p listfile      Set listing file rather than use stdout\n"
           "-w               Warnings for unresolved, duplicate and COMMON length conflicts treated as errors\n"
           "MAP              Intel equivalent of -m option\n"
           "NAME(moduleName) Intel equivalent of -n option\n"
           "PRINT(listfile)  Intel equivalent of -p option\n"
           "WERROR           Intel style equivalent of -w option\n"
           "See Intel linker documentation for inputList specification\n"
           "Notes:\n"
           "* File names are of the format [:Fx:]path, where x is a digit and path\n"
           "  can contain directory components but not spaces, commas, ampersand or parenthesis.\n"
           "  The :Fx: maps to a directory prefix from the same named environment variable\n"
           "  It can be used to work around directory character limitations\n"
           "* Response file input for linking is supported by using \"%s <file\"\n"
           "* targetFile is deleted on error, which helps with make builds\n", invokeName);
    exit(0);
}