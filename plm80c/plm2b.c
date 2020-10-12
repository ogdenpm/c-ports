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

static byte bC252; // lifted to file scope

static void Sub_69EB(byte arg1b, wpointer arg2wP)
{
    word p;

    if (*arg2wP != 0) {
        if (arg1b > bC252)
            *arg2wP = 0;
        else {
            p = *arg2wP;
            *arg2wP = tx2qp;

            while (p != 0) {
                p = p - 1;  
                *arg2wP = *arg2wP - 1;
                if (tx2opc[*arg2wP] == T2_LINEINFO)
                    if (tx2op2[*arg2wP] == 0)
                        if (tx2op3[*arg2wP] != 0)
                            p = p - (tx2op3[*arg2wP] - tx2op1[*arg2wP]);
            }
        }
    }
} /* sub69EB */


static void Sub_68E8()
{
    byte i;

    bC252 = bC1D2 & 3;
    if ((bC1D2 & 4) != 0) {
        tx2op2[1] = tx2op1[tx2qp];
        tx2op1[tx2qp] = 1;
    } else
        Sub_69EB(1, &tx2op1[tx2qp]);

    Sub_69EB(2, &tx2op2[tx2qp]);
    if (bC252 == 3) {
        if (curOp == T2_CALL)
            tx2op3[tx2qp] = tx2op3[tx2qp] + botInfo;
        else if (curOp == T2_BYTEINDEX || curOp == T2_WORDINDEX) {
            i = (byte)tx2op1[tx2qp];
            tx2op2[i] = tx2op2[i] + tx2op3[tx2qp] * Sub_575E(tx2op1[i]);
        }
        else
            Sub_69EB(3, &tx2op3[tx2qp]);
    }
    tx2Aux1b[tx2qp] = 0xc;
    tx2Aux2b[tx2qp] = 9;
}

static void Sub_6AA4()
{
    if (curOp == T2_IDENTIFIER) {
        tx2op1[tx2qp] = curInfoP = tx2op1[tx2qp] + botInfo;
        if (TestInfoFlag(F_MEMBER)) {
            curInfoP = GetParentOffset();
            tx2Aux2b[tx2qp] = 4;
        }
        else if (TestInfoFlag(F_AUTOMATIC))
            tx2Aux2b[tx2qp] = 0xa;
        else
            tx2Aux2b[tx2qp] = 4;
        curInfoP = tx2op1[tx2qp];
        tx2op2[tx2qp] = GetLinkVal();
        tx2Aux1b[tx2qp] = b5286[GetType()];
    } else if (curOp <= T2_BIGNUMBER) {
        tx2op2[tx2qp] = tx2op1[tx2qp];
        tx2Aux2b[tx2qp] = 8;
        tx2op1[tx2qp] = 0;
        if (curOp == T2_BIGNUMBER) {
            tx2Aux1b[tx2qp] = 1;
            tx2opc[tx2qp] = T2_NUMBER;
        } else
            tx2Aux1b[tx2qp] = 0;
    } else {
        tx2Aux1b[tx2qp] = 0;
        tx2op2[tx2qp] = 0;
    }
}

void Sub_689E()
{
    for (tx2qp = 4; tx2qp <= bC1BF - 1; tx2qp++) {
        curOp = tx2opc[tx2qp];
        bC1D2 = b5124[curOp];
        if ((bC1D2 & 0xc0) == 0)
            Sub_68E8();
        else if ((bC1D2 & 0xc0) == 0x40)
            Sub_6AA4();
    }
}

