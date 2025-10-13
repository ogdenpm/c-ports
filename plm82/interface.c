#include "plm82.h"

int32_t getSym32() {
    int32_t val;
    if (fread(&val, sizeof(val), 1, symFp) != 1) {
        Fatal("112: symbol table read error");
        return 0; // to avoid compiler warning
    }
    return val;
}

uint16_t getSym16() {
    uint16_t val;
    if (fread(&val, sizeof(val), 1, symFp) != 1) {
        Fatal("113: symbol table read error");
        return 0; // to avoid compiler warning
    }
    return val;
}

/*
 * SYMBOL TABLE FORMAT
 *
 *
 * symbol[++sytop] =  syinfo                    // address
 *                    symbol[--syinfo]          // attributes aka info from pass 1
 *              VARB
 *               opt  symbol[--syinfo]          // based symbol
 *                    symbol[--syinfo]          // 1 entry for variable
 *
 *
 *               PROC symbol[--syinfo]          // 3 entries for procedure
 *                    symbol[--syinfo]
 *                    symbol[--syinfo]
 *
 *
 *              LABEL symbol[--syinfo]         // simple label
 *               opt symbol[--syinfo]        // HL tracker
 */
void loadsy() {
    int ch;
    bool ok = false;

    // binary format now dumps the 8 interrupt symbols in native format
    // 0 implies none
    if (fread(intpro, sizeof(intpro[0]), 8, symFp) != 8) {
        Fatal("114: symbol table read error");
        return; // to avoid compiler warning
    }
    if (C_SYMBOLS >= 2)
        for (int i = 0; i < 8; i++)
            if (intpro[i])
                fprintf(lstFp, "I%d=S%05d\n", i, intpro[i]);

    /* interrupt procedures are handled. */

    while ((ch = getc(symFp))) { // process next symbol table entry
        if (ch != 1 && ch != 2)
            goto badData;
        if (++sytop >= syinfo) {
            Fatal("108: pass-2 symbol table overflow");
            syinfo = symax;
        }
        if (C_SYMBOLS >= 2) // write symbol number AND symbol table address
            fprintf(lstFp, "S%05d", sytop);

        symbol[sytop] = syinfo--;
        if (syinfo - ch <= sytop) {
            Fatal("109: symbol table overflow");
            syinfo = symax;
        }

        int info = getSym32();
        if (C_SYMBOLS >= 2) // write symbol table address AND entry
            fprintf(lstFp, "    %05d (%d,%d,%d)", syinfo, INFO_ECNT(info), INFO_PREC(info), INFO_TYPE(info));
        symbol[syinfo--] = ch == 1 ? info : -info;
        if (ch == 2) {
            symbol[syinfo--] = info = getSym32();
            if (C_SYMBOLS >= 2) // write symbol table address AND entry
                fprintf(lstFp, " based S%05d", info);
            
        }


        if (C_SYMBOLS >= 2)
            putc('\n', lstFp);
        /* check for special case at END of an entry */
        // allocate additional cell count
        switch (INFO_TYPE(symAttrib(sytop))) {
        case VARB:
            syinfo -= 1;
            break;
        case PROC:
            syinfo -= 3;
            break;
        case LABEL:
            syinfo -= INFO_ECNT(symAttrib(sytop)) == 1 ? 2 : 1;
            break; // check for single reference to the label
        }
    }

    /* assign relative memory addresses to variables IN symbol table */
    lmem = 0xff00;
    for (int i = sytop; i > 0; i--) { /* process symbols (backwards) */
        int addr   = -1;
        int attrib = symAttrib(i);

        if (attrib >= 0 && INFO_TYPE(attrib) == VARB) {
            int prec = INFO_PREC(attrib);
            int ecnt = INFO_ECNT(attrib);

            if (prec > 2) /* probably an inline data variable */
                prec = -1;
            else {
                if (prec == 2) // align words to even boundary
                    lmem &= ~1;

                if ((lmem -= prec * ecnt) < 0) {
                    error("110: data storage too big");
                    lmem = 0xff00;
                }
                addr = lmem;
                if (C_SYMBOLS && i > 4 && i != 6) /* write address assignment for real variables */
                    fprintf(lstFp, "S%05d=%05d\n", i, addr);
            }
        }
        symAddr(i) = addr;
    }
    ok = true;
badData:
    if (!ok)
        error("111: inline data format error");

    /* now assign the last address to the variable 'memory' */
    /* ** note that 'memory' must be at location 5 IN the symbol table ** */
    symAddr(5) = 0xff00;
}

int loadin() // modified for V4
{
    int addr   = -1;
    int marker = getc(symFp);
    while (marker == 4) {

        // we have a new block
        int symNo  = getSym16();
        int attrib = symAttrib(symNo);
        int prec   = INFO_PREC(attrib);
        int sAddr  = symAddr(symNo);
        if (codloc > sAddr)
            error("123: initialized storage overlaps previously initialized storage at %04XH",
                  sAddr);
        while (codloc < sAddr)
            put(codloc++, 0);
        if (addr < 0)
            addr = sAddr;
        // load in data
        while ((marker = getc(symFp)) >= 1 && marker <= 3) {
            int ch1 = getc(symFp);
            switch (marker) {
            case 1: // byte
                put1(ch1, prec);
                break;
            case 2: // address
                put2(ch1, getc(symFp), prec);
                break;
            case 3: // string
                while (ch1 > 0) {
                    int ch2;
                    if ((ch2 = getc(symFp)) <= 0) {
                        put1(ch1, prec);
                        break;
                    }
                    put2(ch1, ch2, prec);
                    ch1 = getc(symFp);
                }
                break;
            }
        }
    }
    if (marker != 0)
        error("124: initialization table format error");
    return addr;
}

int polCnt = 0;

int32_t getNextPol() {
    uint8_t typ  = getc(polFp);
    uint16_t val = getc(polFp);
    val += getc(polFp) * 256;
    polCnt++;
    return feof(polFp) ? EOF : (val << 3) + typ;
}

void inldat() {
    int symIdx = 0; // assign to appease GCC

    /* emit data inline */
    int startAddr  = codloc;
    int cnt = 0;
    for (;;) {
        int type = 0;
        int val  = 0;
        while (lapol != FIN) {
            val  = lapol / 8;
            type = lapol % 8;
            if ((lapol = getNextPol()) == EOF) {
                lapol = FIN;
                error("125: inline data error");
                return;
            }

            if (cnt == 0) { /* define inline data symbol */
                if (type == DEF) {
                    if (val)
                        symIdx = val; /* this is a symbol reference */
                    else {            /* inline constant -- set up symbol entry */
                        if (syinfo - 2 < ++sytop)
                            break; // overflow
                        symIdx        = -sytop;
                        symbol[sytop] = syinfo;
                        syinfo -= 2; // reserve 2 entries
                    }
                } else
                    break;
            } else if (type == OPR && val == DAT) {
                /* backstuff jump address */
                /* now fix symbol table entries */
                symAddr(abs(symIdx))   = -startAddr;
                int ecnt               = INFO_ECNT(symAttrib(abs(symIdx)));
                symAttrib(abs(symIdx)) = PACK_ATTRIB(--cnt, 1, VARB); // fill in the symbol table
                if (symIdx < 0) /* this is an address reference to a constant, so.. */
                    parseStk[++sp] = stkItem(0, 2, symIdx, startAddr);
                else if (ecnt != cnt) /* check size declared against size read */
                    break;
                return;
            } else if (type == LIT)
                emit(0, val, 0);
            else
                break;
            cnt++;
        }

        if (type == LIN)
            C_COUNT = val;
        else {
            error("125: inline data error");
            return;
        }
    }
}