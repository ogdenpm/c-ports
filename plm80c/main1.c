/****************************************************************************
 *  main1.c: part of the C port of Intel's ISIS-II plm80                    *
 *  The original ISIS-II application is Copyright Intel                     *
 *                                                                          *
 *  Re-engineered to C by Mark Ogden <mark.pm.ogden@btinternet.com>         *
 *                                                                          *
 *  It is released for academic interest and personal use only              *
 ****************************************************************************/
#include "os.h"
#include "plm.h"

// static byte copyright[] = "(C) 1976, 1977, 1982 INTEL CORP";

word markedStSP;
word stmtT2Cnt;
info_t *curProc;
word curStmtNum;
var_t var;

bool regetTx1Item    = false;
bool tx2LinfoPending = false;
linfo_t linfo;
tx1item_t tx1Item;
byte tx1Attr;
byte tx1ICode;

static void PrepFiles() {
    scopeSP        = 1;
    scopeChains[1] = 0;
    /* the opening of all output files has been moved here after command line processing */
    if (IXREF)
        OpenF(&ixiFile, "IXREF", ixiFileName, "wb+");
    if (OBJECT) {
        OpenF(&objFile, "OBJECT", objFileName, "wb+");
        RmOnError(objFileName);
    }
    if (PRINT)
        OpenF(&lstFile, isList ? "LIST" : "PRINT", lstFileName, "wt");
}

static void FinaliseAt() {
    if (tx2LinfoPending)
        Wr2Item(linfo.type, &linfo.lineCnt, sizeof(struct _linfo));
    vfReset(&utf1);
    WrAtByte(ATI_END);
    WrAtByte(ATI_EOF);
    vfRewind(&atf); // rewind for var allocation
}

word Start1() {
#ifdef TRACE
    DumpT1Stream();
#endif
    if (setjmp(exception) == 0) {
        PrepFiles(); /* create files */
                     //        dump(&utf1, "uft1_main1");    // diagnostic dump
        ParseLexItems();
    } else {
        /* here longjmp(exception, -1) */
        Wr2TokError(fatalCode);
        while (tx1Item.type != T1_EOF) {
            if (tx1Item.type == T1_STMTCNT) {
                stmtT2Cnt = 0;
                MapLToT2();
                curStmtNum = tx1Item.dataw[0];
            }
            GetTx1Item();
        }
    }
    FinaliseAt();
    AllocateVars();

    if (hasErrors)
        return 6; // Chain(overlay[6]);
    else
        return 2; // Chain(overlay[2]);
}
