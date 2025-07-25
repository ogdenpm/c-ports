/****************************************************************************
 *  loc6.c: part of the C port of Intel's ISIS-II locate                    *
 *  The original ISIS-II application is Copyright Intel                     *
 *                                                                          *
 *  Re-engineered to C by Mark Ogden <mark.pm.ogden@btinternet.com>         *
 *                                                                          *
 *  It is released for academic interest and personal use only              *
 ****************************************************************************/
#include "loc.h"
#include <ctype.h>

int warningMask;
int warnings;
static bool orderDef[256];
static uint8_t nxtSegOrder;
static struct {
    uint8_t type;
    uint8_t aux;
    char *name;
} controls[] = {
    { 1, 1, "CODE" },      { 1, 2, "DATA" },       { 1, 3, "STACK" },    { 1, 4, "MEMORY" },
    { 2, 0, "START" },     { 3, 1, "STACKSIZE" },  { 4, 2, "RESTART0" }, { 4, 3, "MAP" },
    { 4, 4, "PUBLICS" },   { 4, 5, "SYMBOLS" },    { 4, 6, "LINES" },    { 4, 7, "PURGE" },
    { 5, 8, "NAME" },      { 6, 0, "PRINT" },      { 7, 0, "ORDER" },    { 8, 0, "COLUMNS" },
    { 10, 1, "EXTERNOK" }, { 10, 2, "OVERLAPOK" },
};

void ExpectLP(void) {
    ExpectChar('(', "left parenthesis expected"); /* left parenthesis expected */
}

void ExpectRP(void) {
    ExpectChar(')', "right parenthesis expected"); /* right parenthesis expected */
}

void ExpectSlash(void) {
    ExpectChar('/', "invalid syntax, expected '/'"); /* invalid syntax */
}

/* modified to fixup segOrder post cmdline to avoid multiple inserts */
void ResetSegOrder(void) {
    memset(orderDef, false, sizeof(orderDef));
    nxtSegOrder = 0;
}

void AddSegOrder(uint8_t seg) {
    if (!orderDef[seg]) {
        segOrder[++nxtSegOrder] = seg;
        orderDef[seg]           = true;
    } else
        FatalCmdLineErr("segment already specified"); /* invalid syntax */
}

static void append(uint8_t seg) { // expect compiler to inline
    if (!orderDef[seg])
        segOrder[++nxtSegOrder] = seg;
}

// add the unspecified segments as per default order
void FixSegOrder(void) {
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

void ProcessControls(void) {
    uint8_t type, aux, seg;
    char *token = GetToken();
    if (*token == '/') // handle common
        type = 9;
    else {
        type = 0;
        for (uint32_t i = 0; i < sizeof(controls) / sizeof(controls[0]); i++)
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
        startAddr  = ParseLPNumRP(); /* and its value */
        break;
    case 3:                                /* STACKSIZE */
        seen.stackSize   = true;           /* got a stack size */
        segSizes[SSTACK] = ParseLPNumRP(); /* and its value */
        break;
    case 4:                   // RESTART0, MAP, PUBLICS, SYMBOLS, LINES, PURGE
        seen.all[aux] = true; /* simple command seen */
        break;
    case 5:               /* NAME */
        seen.name = true; /* got a module name */
        ExpectLP();
        moduleName = GetModuleName(GetToken()); /* get its value */
        ExpectRP();
        break;
    case 6: /* PRINT */ /* get the print file */
        ExpectLP();
        lstName = GetText();
        ExpectRP();
        break;
    case 7: /* ORDER */  /* process the order list */
        ResetSegOrder(); // ORDER resets. Remove this to make additive
        {
            ExpectLP();
            bool needSeg = true;
            do {
                token = GetToken();
                if (!*token)
                    break;

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
                if ((needSeg = *cmdP == ','))
                    cmdP++;
            } while (*cmdP && *cmdP != ')');

            if (needSeg)
                FatalCmdLineErr("Expected Segment Name");
            ExpectRP();
        }
        break;
    case 8: /* COLUMNS */ /* get the number of columns */
        columns = ParseLPNumRP();
        if (columns < 1 || columns > 4)
            FatalCmdLineErr("Expected number in range 1-4");
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
}