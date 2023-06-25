/****************************************************************************
 *  loc6.c: part of the C port of Intel's ISIS-II locate             *
 *  The original ISIS-II application is Copyright Intel                     *
 *																			*
 *  Re-engineered to C by Mark Ogden <mark.pm.ogden@btinternet.com> 	    *
 *                                                                          *
 *  It is released for hobbyist use and for academic interest			    *
 *                                                                          *
 ****************************************************************************/

#include "loc.h"
#include <ctype.h>

char *cmdP;

int warningMask;
int warnings;
static bool orderDef[256];
static byte nxtSegOrder;
static struct {
    byte type;
    byte aux;
    char *name;
} controls[] = {
    { 1, 1, "CODE" },     { 1, 2, "DATA" },       { 1, 3, "STACK" },     { 1, 4, "MEMORY" },
    { 2, 0, "START" },    { 2, 16, "-s" },        { 3, 1, "STACKSIZE" }, { 3, 17, "-ss" },
    { 4, 2, "RESTART0" }, { 4, 2, "-r" },         { 4, 3, "MAP" },       { 4, 3, "-lm" },
    { 4, 4, "PUBLICS" },  { 4, 4, "-lp" },        { 4, 5, "SYMBOLS" },   { 4, 5, "-ls" },
    { 4, 6, "LINES" },    { 4, 6, "-ll" },        { 4, 7, "PURGE" },     { 4, 7, "-pu" },
    { 5, 8, "NAME" },     { 5, 24, "-n" },        { 6, 0, "PRINT" },     { 6, 16, "-p" },
    { 7, 0, "ORDER" },    { 8, 0, "COLUMNS" },    { 8, 16, "-c" },       { 10, 1, "NOEXTERN" },
    { 10, 1, "-ne" },     { 10, 2, "NOOVERLAP" }, { 10, 2, "-no" }
};

_Noreturn void FatalCmdLineErr(char const *errMsg) {
    if (*cmdP != '\n') /* don't skip past the EOL */
        cmdP++;
    strcpy((char *)cmdP, "#\n"); // mark problem (remove 'const' from cmdP)
    fprintf(stderr, "Command line error near #: %s\n", errMsg);
    printCmdLine(stderr);
    Exit(1);
} /* FatalCmdLineErr() */

void SkipNonArgChars(char *pch) {
    cmdP = SkipSpc(pch);
    /* skip any continuation bits and leading space */
    while (*cmdP == '&') {
        cmdP = SkipSpc(cmdP + 5);
    }
} /* SkipNonArgChars */

void ExpectChar(byte ch, char const *msg) {
    SkipNonArgChars(cmdP);
    if (ch == *cmdP)
        SkipNonArgChars(cmdP + 1);
    else
        FatalCmdLineErr(msg);
} /* ExpectChar */

void ExpectLP() {
    ExpectChar('(', "left parenthesis expected"); /* left parenthesis expected */
} /* ExpectLP */

void ExpectRP() {
    ExpectChar(')', "right parenthesis expected"); /* right parenthesis expected */
} /* ExpectRP */

void ExpectSlash() {
    ExpectChar('/', "invalid syntax, expected '/'"); /* invalid syntax */
} /* ExpectSlash */

/* modified to fixup segOrder post cmdline to avoid multiple inserts */
void ResetSegOrder() {
    memset(orderDef, false, sizeof(orderDef));
    nxtSegOrder = 0;
}

void AddSegOrder(byte seg) {
    if (!orderDef[seg]) {
        segOrder[++nxtSegOrder] = seg;
        orderDef[seg]           = true;
    } else
        FatalCmdLineErr("segment already specified"); /* invalid syntax */
}

static void append(byte seg) { // expect compiler to inline
    if (!orderDef[seg])
        segOrder[++nxtSegOrder] = seg;
}

// add the unspecified segments as per default order
void FixSegOrder() {
    segOrder[0] = SABS;
    append(SCODE);
    append(SSTACK);
    // the common segs in order
    for (int i = SNAMED; i <= SBLANK; i++)
        append(i);
    // finally DATA and MEMORY
    append(SDATA);
    append(SMEMORY);
}

pstr_t const *GetModuleName() {
    pstr_t const *name = toPstr(GetToken());
    if (name->len > 31)
        FatalCmdLineErr("Module name too long");
    if (isdigit(name->str[0]))
        FatalCmdLineErr("Module name cannot start with a digit");
    for (char *s = (char *)name->str; *s; s++) // remove const to allow upper casing
        if (isalnum(*s) || *s == '?' || *s == '@' || *s == '_')
            *s = toupper(*s);
        else
            FatalCmdLineErr("Illegal character in module name");
    return name;
}

void ProcessControls() {
    byte type, aux, seg;
    char *token = GetToken();
    if (*token == '/') // handle common
        type = 9;
    else {
        type = 0;
        for (int i = 0; i < sizeof(controls) / sizeof(controls[0]); i++)
            if (stricmp(token, controls[i].name) == 0) {
                type = controls[i].type;
                aux  = controls[i].aux;
                break;
            }
    }

    switch (type) {
    case 0:
        FatalCmdLineErr("Unrecognised control");
        break;
    case 1:                             /* CODE, DATA, STACK, MEMORY */
        segFlags[aux] |= FHASADDR;      /* note that the address is specified */
        segBases[aux] = ParseLPNumRP(); /* and its value */
        break;
    case 2:                          /* START */
        seen.start = true;           /* got a start address */
        startAddr  = aux & 0x10 ? ParseSimpleNumber() : ParseLPNumRP(); /* and its value */
        break;
    case 3:                                /* STACKSIZE */
        seen.stackSize   = true;           /* got a stack size */
        segSizes[SSTACK] = aux & 0x10 ? ParseSimpleNumber() : ParseLPNumRP(); /* and its value */
        break;
    case 4:                   // RESTART0, MAP, PUBLICS, SYMBOLS, LINES, PURGE
        seen.all[aux] = true; /* simple command seen */
        break;
    case 5:               /* NAME */
        seen.name = true; /* got a module name */
        if (aux & 0x10)
            moduleName = GetModuleName();
        else {
            ExpectLP();
            moduleName = GetModuleName(); /* get its value */
            ExpectRP();
        }
        break;
    case 6: /* PRINT */ /* get the print file */
        if (aux & 0x10)
            lstName = GetToken();
        else {
            ExpectLP();
            lstName = GetToken();
            ExpectRP();
        }
        break;
    case 7: /* ORDER */  /* process the order list */
        ResetSegOrder(); // ORDER resets. Remove this to make additive
        ExpectLP();
        SkipNonArgChars(cmdP);
        while (*(token = GetToken())) {
            if (*token == '/') {
                seg = GetCommonSegId(token);
                AddSegOrder(seg);
            } else {
                int i;
                for (i = 0; i < 4 && stricmp(token, controls[i].name) != 0; i++)
                    ;
                if (i < 4)
                    AddSegOrder(controls[i].aux);
                else
                    FatalCmdLineErr("Unknown segment");
            }
            SkipNonArgChars(cmdP);
            if (*cmdP != ',') // list?
                break;
            cmdP++;
        }
        ExpectRP();
        break;
    case 8: /* COLUMNS */ /* get the number of columns */
        columns = aux & 0x10 ? ParseSimpleNumber() : ParseLPNumRP();
        if (columns < 1 || columns > 3)
            FatalCmdLineErr("Expected number in range 1-3");
        break;
    case 9:                             // common name
        seg = GetCommonSegId(token);    /* look up seg */
        segFlags[seg] |= FHASADDR;      /* note we have an address */
        segBases[seg] = ParseLPNumRP(); /* and its value */
        break;
    case 10:
        warningMask |= aux;
        break;
    }
} /* ProcessControls */
