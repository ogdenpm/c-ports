/****************************************************************************
 *  link.c: part of the C port of Intel's ISIS-II link                      *
 *  The original ISIS-II application is Copyright Intel                     *
 *                                                                          *
 *  Re-engineered to C by Mark Ogden <mark.pm.ogden@btinternet.com>         *
 *                                                                          *
 *  It is released for academic interest and personal use only              *
 ****************************************************************************/
#include "link.h"
#include <stdarg.h>
#include <stdio.h>

bool mapWanted;
pstr_t const *moduleName;
uint8_t moduleType;
uint8_t tranId;
uint8_t tranVn;
uint8_t entrySeg;
uint16_t entryOffset;
uint16_t segLen[6];
uint8_t alignType[6];
uint8_t segmap[256];
objFile_t *objFileList;
objFile_t *objFile;
module_t *module;
symbol_t *hashTab[128];
symbol_t *headCommSym;
uint16_t unresolved;
uint16_t maxExternCnt;
symbol_t *unresolvedList;
bool externOk         = false; // if false then warnings are teated as errors
int warnings          = 0;

uint8_t COPYRIGHT[] = "[C] 1976, 1977, 1979 INTEL CORP'";

uint8_t HashF(pstr_t const *pstr) {
    uint8_t i, j;

    j = pstr->len;
    for (i = 0; i < pstr->len; i++)
        j = RorB(j, 1) ^ pstr->str[i];
    return j & 0x7F;
} /* HashF() */

bool Lookup(pstr_t const *pstr, symbol_t **symRef, uint8_t mask) {
    symbol_t *p = (symbol_t *)&hashTab[HashF(pstr)];

    *symRef     = p;
    for (p = p->hashChain; p; p = p->hashChain) {
        *symRef = p;
        if ((p->flags & mask) != AUNKNOWN && pstrequ(pstr, p->name))
            return true;
    }
    return false;
} /* Lookup() */

void ModuleWarning(char const *msg) {
    PrintfAndLog("%s%s(%s)\n", msg, objFile->name, module->name->str);
}

void Start() {
    ParseCmdLine();
    Phase1();
    Phase2();
    Exit(externOk ? 0 : warnings);
} /* Start */

void usage() {
    printf("Usage: %s inputList (TO | -o) targetFile [link option]*\n"
           "or:    %s (-v | -V | -h)\n",
           invokeName, invokeName);
    printf("Where:\n"
           "-h               Show this help\n"
           "-v / -V          Show simple / extended version information\n"
           "Link options are:\n"
           "MAP              Include link map information in listing\n"
           "NAME(moduleName) Set targetFile module name. '_' is now supported\n"
           "PRINT(listfile)  Set listing file rather than use stdout\n"
           "EXTERNOK         Don't treat unresolved warnings as errors\n"
           "                 and COMMON length conflicts\n"
           "See Intel linker documentation for inputList specification\n"
           "Notes:\n"
           "* File names are of the format [:Fx:]path, where x is a digit and path\n"
           "  The :Fx: maps to a directory prefix from the same named environment variable\n"
           "* Response file input for linking is supported by using \"%s <file\"\n"
           "* targetFile is deleted on error, which helps with make builds\n",
           invokeName);
}