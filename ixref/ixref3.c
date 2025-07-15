/****************************************************************************
 *  ixref3.c: part of the C port of Intel's ISIS-II ixref                   *
 *  The original ISIS-II application is Copyright Intel                     *
 *                                                                          *
 *  Re-engineered to C by Mark Ogden <mark.pm.ogden@btinternet.com>         *
 *                                                                          *
 *  It is released for academic interest and personal use only              *
 ****************************************************************************/
#include "ixref.h"
#include "os.h"
#include <stdlib.h>
#include <string.h>

char const *dotSpace = " . . . . . . . . . . . . . . . .";
char const *dashes   = "--------------------------------";

char *types[]        = { "",          "LABEL", "BYTE",    "ADDRESS", "STRUCTURE",
                         "PROCEDURE", "WORD",  "INTEGER", "REAL",    "POINTER" };

static char const *OutDotFill(char const *s) {
    static char line[33];
    strcpy(line, dotSpace);
    memcpy(line, s, strlen(s));
    return line;
}

static void PrintXref(sym_t *p) {
    int pass    = 0;
    xref_t *def = p->xrefList;
    do {
        for (; def; def = def->next)
            if (def->isDef)
                break;
        if (!def) {
            if (pass == 0 && showExternals)
                def = p->xrefList;
            else
                return;
        }

        /* display name */
        OutNewLine();
        OutPrintf("   %.*s", maxIdLen + 1, OutDotFill(p->name));
        /*
            types
            1	LABEL
            2	BYTE
            3	ADDRESS
            4	STRUCTURE
            5	PROCEDURE
            6	WORD
            7	INTEGER
            8	REAL
            9	POINTER
        */

        if (T_LABEL <= def->type && def->type <= T_POINTER) {
            OutPrintf(" %s", types[def->type]);
            if (def->type == T_PROCEDURE) {
                if (def->dimRtype == T_BYTE || def->dimRtype == T_ADDRESS)
                    OutPrintf("  %s", types[def->dimRtype]);
            } else if (def->dimRtype)
                OutPrintf("(%d)", def->dimRtype);
        } else
            OutPrintf(" TYPE(%d)", def->type);

        OutStr(";  ");
        if (showPublics) {
            if (def->isDef) {
                OutStr(def->module);
                if (pass)
                    OutStr("** DUPLICATE **");
            } else
                OutStr("** UNRESOLVED **");
        }
        if (pass == 0 && showExternals) {
            for (xref_t *ref = p->xrefList; ref; ref = ref->next) {
                if (!ref->isDef) {
                    OutStr("  ");
                    OutStr(ref->module);
                }
            }
        }
        def = def->next;
        pass++;
    } while (showPublics);
}

static void ListModules() {
    OutNNewLines(5);
    OutStr("MODULE DIRECTORY\n"
           "----------------\n\n\n");

    OutPrintf("   MODULE NAME%.*s FILE NAME    DISKETTE NAME\n", maxIdLen - 10, dotSpace + 1);
    OutPrintf("   %.*s%26s\n\n", maxIdLen + 1, dashes, dashes);
    /*
        modDirEntries format
        ptr -> link, modName(len byte, name(len)), diskName(10)
    */
    for (int i = 0; i < modCnt; i++) {
        OutPrintf("   %.*s", maxIdLen + 1, OutDotFill(modtab[i].name));
        char const *fname = modtab[i].fname;
        if (*fname == ' ') // long file name
            OutStr(fname);
        else {
            OutPrintf(" %.10s", fname); // print filename
            int len = (int)strlen(fname);
            if (len > 10)
                OutPrintf("  %.6s", fname + 10);
            if (len > 16)
                OutPrintf(".%.3s", fname + 16);
        }
        OutStr("\n");
    }
}

int cmpMod(void const *r1, void const *r2) {
    return strcmp(((mod_t *)r1)->name, ((mod_t *)r2)->name);
}

void sub5317() {
    OutStr("\n\nINTER-MODULE CROSS-REFERENCE LISTING\n"
           "------------------------------------\n\n\n");
    OutPrintf("   NAME%.*s ATTRIBUTES;  MODULE NAMES\n", maxIdLen - 3, dotSpace);
    OutPrintf("   ----%.*s%.*s\n", maxIdLen - 3, dashes, 26, dashes);

    for (sym_t *p = symlist; p; p = p->next)
        PrintXref(p);

    if (modCnt) {
        qsort(modtab, modCnt, sizeof(mod_t), cmpMod);
        ListModules();
    }
    fclose(outFp);
}