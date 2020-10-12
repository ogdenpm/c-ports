/****************************************************************************
 *  plm80: C port of Intel's ISIS-II PLM80 v4.0                             *
 *  Copyright (C) 2020 Mark Ogden <mark.pm.ogden@btinternet.com>            *
 *                                                                          *
 *  This program is free software; you can redistribute it and/or           *
 *  modify it under the terms of the GNU General Public License             *
 *  as published by the Free Software Foundation; either version 2          *
 *  of the License, or (at your option) any later version.                  *
 *                                                                          *
 *  This program is distributed in the hope that it will be useful,         *
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of          *
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the           *
 *  GNU General Public License for more details.                            *
 *                                                                          *
 *  You should have received a copy of the GNU General Public License       *
 *  along with this program; if not, write to the Free Software             *
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,              *
 *  MA  02110-1301, USA.                                                    *
 *                                                                          *
 ****************************************************************************/


#include "plm.h"

static byte copyright[] = "(C) 1976, 1977, 1982 INTEL CORP";


word markedStSP;
word t2CntForStmt;
word curProcInfoP = {0};
word curStmtNum;
var_t var;

bool regetTx1Item = false;
bool tx2LinfoPending = false;
linfo_t linfo;
tx1item_t tx1Item;
byte tx1Aux2;
byte tx1Aux1;
byte tx1RdBuf[512];
//byte tx2Buf[512]; use larger buf in main6.c
byte xrfBuf[512];
// byte atBuf[512]; use version in plm3a.c
byte b91C0;


static void PrepFiles()
{
    CreatF(&tx1File, tx1RdBuf, 512, READ_MODE);
    OpenF(&tx2File, UPDATE_MODE);
    CreatF(&tx2File, tx2Buf, 512, WRITE_MODE);
    if (XREF || IXREF || SYMBOLS)
    {
        OpenF(&xrfFile, WRITE_MODE);
        CreatF(&xrfFile, xrfBuf, 512, WRITE_MODE);
    }
    OpenF(&atFile, UPDATE_MODE);
    CreatF(&atFile, atBuf, 16, WRITE_MODE);
    blockDepth = 1;
    procChains[1] = 0;
}

static void Sub_3F8B()
{
    if (tx2LinfoPending) 
        WrTx2File((pointer)&linfo, 7);
    CloseF(&tx1File);
#ifdef _DEBUG
    copyFile(tx1File.fNam, "plmtx1.tmp_main1");
#endif
    DeletF(&tx1File);
    OpenF(&tx1File, 3);
    if (XREF || IXREF || SYMBOLS)
    {
        Fwrite(&xrfFile, "\0", 1);
        Fflush(&xrfFile);
        CloseF(&xrfFile);
    }
    WrAtFileByte(ATI_END);
    WrAtFileByte(ATI_EOF);
    Fflush(&atFile);
    Rewind(&atFile);
}

word Start1()
{

    if (setjmp(exception) == 0) {
        PrepFiles();	/* create files and preload tx1 */
        ParseLexItems();
    } else {
        /* here longjmp(exception, -1) */
        WrTx2ExtError(b91C0);
        while (tx1Item.type != L_EOF) {
            if (tx1Item.type == L_STMTCNT)
            {
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
