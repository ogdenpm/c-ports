/****************************************************************************
 *  main1.c: part of the C port of Intel's ISIS-II plm80c             *
 *  The original ISIS-II application is Copyright Intel                     *
 *																			*
 *  Re-engineered to C by Mark Ogden <mark.pm.ogden@btinternet.com> 	    *
 *                                                                          *
 *  It is released for hobbyist use and for academic interest			    *
 *                                                                          *
 ****************************************************************************/

#include "plm.h"

// static byte copyright[] = "(C) 1976, 1977, 1982 INTEL CORP";

word markedStSP;
word t2CntForStmt;
word procIdx = { 0 };
word curStmtNum;
var_t var;

bool regetTx1Item    = false;
bool tx2LinfoPending = false;
linfo_t linfo;
tx1item_t tx1Item;
byte tx1Aux2;
byte tx1Aux1;

// byte atBuf[512]; use version in plm3a.c
byte b91C0;

static void PrepFiles() {
    blockDepth    = 1;
    procChains[1] = 0;
    /* the opening of all output files has been moved here */
    if (IXREF)
        OpenF(&ixiFile, "IXREF", ixiFileName, "wb+");
    if (OBJECT)
        OpenF(&objFile, "OBJECT", objFileName, "wb+");
    if (PRINT)
        OpenF(&lstFile, isList ? "LIST" : "PRINT", lstFileName, "wt");
}

static void Sub_3F8B() {
    if (tx2LinfoPending)
        Wr2Item(linfo.type, &linfo.lineCnt, sizeof(struct _linfo));
    vfReset(&utf1);
    if (XREF || IXREF || SYMBOLS) {
        vfWbyte(&xrff, '\0');
        vfRewind(&xrff);
    }

    WrAtByte(ATI_END);
    WrAtByte(ATI_EOF);
    vfRewind(&atf);
}

word Start1() {
    if (setjmp(exception) == 0) {
        PrepFiles(); /* create files and preload tx1 */
        dump(&utf1, "uft1_main1");
        ParseLexItems();
    } else {
        /* here longjmp(exception, -1) */
        WrTx2ExtError(b91C0);
        while (tx1Item.type != L_EOF) {
            if (tx1Item.type == L_STMTCNT) {
                t2CntForStmt = 0;
                MapLToT2();
                curStmtNum = tx1Item.dataw[0];
            }
            GetTx1Item();
        }
    }
    Sub_3F8B();
    Sub_6EE0();

    if (hasErrors)
        return 6; // Chain(overlay[6]);
    else
        return 2; // Chain(overlay[2]);
}
