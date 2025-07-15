#include "plm81.h"




void putSymInt(int n, int width) {
    do {
        int digit = n % 32;                                  // convert to base 32
        putc(digit < 10 ? '0' + digit : 'A' + digit - 10, symFp); // convert to ASCII
        width--;
    } while (n /= 32);
    while (width-- > 0)
        putc('0', symFp); // pad with zeros
}

void putSymHex2(int n, bool tag) {
    static const char digits[] = "0123456789ABCDEFGHIJKLMNOPQRSTUV";
    putc(digits[HIGHNIBBLE(n) + (tag ? 16 : 0)], symFp);
    putc(digits[LOWNIBBLE(n)], symFp);
}

// to map to original code info
#define MkInfo(packed, prec, type) (((int)(packed) << 8) | ((prec) << 4) | (type))

void dumpsy() {
    int ic;

    ic = C_SYMBOLS;
    if (ic != 0) {
        if (ic > 1)
            fprintf(lstFp, "\nSYMBOL  ADDR STR CHRS   LENGTH PR TY");
        for (int symIdx = symNext - 1; symIdx > 0; symIdx--) {
            /*     quick check for zero length name */
            if (ic >= 2 || symbol[symIdx].strLen > 0)
                fprintf(lstFp, "S%05d", symIdx);

            if (ic >= 2) {
                fprintf(lstFp, "%c ",
                        symbol[symIdx].outOfScope ? '*' : ' '); // * if now out of scope
                fprintf(lstFp, "%04d %3d %4d ", symIdx, symbol[symIdx].strId,
                        symbol[symIdx].strLen);
                fprintf(lstFp, "%c ", symbol[symIdx].based ? 'B' : 'R'); // based or regular
                fprintf(lstFp, "%06d%3d%3d", symbol[symIdx].len, symbol[symIdx].prec,
                        symbol[symIdx].type);
            }
            putc(' ', lstFp);

            if (symbol[symIdx].strId) { // exists
                int type = symbol[symIdx].type;
                if (type == LITER)
                    putc('\'', lstFp);
                fprintf(lstFp, "%.*s", symbol[symIdx].strLen, idToStr(symbol[symIdx].strId));
                if (type == LITER)
                    putc('\'', lstFp);
            }
            putc('\n', lstFp);
        }
        putc('\n', lstFp);
    }

    if (errorCnt)
        return;
    /*     write the interrupt procedure names */
    putc('/', symFp);
    for (int intProcNum = 0; intProcNum < 8; intProcNum++) {
        int intProcAddr = intpro[intProcNum]; /* interrupt procedure address */
        if (intProcAddr > 0) {                /* write intnumber symbolnum (4 base-32 digits) */
            putc(intProcNum + '0', symFp);
            putSymInt(intProcAddr, 3);
            putc('/', symFp);
        }
    }
    putc('/', symFp);

    // mark internal labels
    for (int symIdx = 1; symIdx < symNext; symIdx++) {
        if (symbol[symIdx].type == LABEL && symbol[symIdx].len == 0)
            setInfo(symIdx, 1, CompilerLabel, LABEL); // set to compiler label
    }

    putc('/', symFp);
    for (int symIdx = 1; symIdx < symNext; symIdx++) {
        int info = MkInfo(symbol[symIdx].len, symbol[symIdx].prec, symbol[symIdx].type);
        if (symbol[symIdx].based) {
            putc('-', symFp);
            putSymInt(info, 0); // print the Info word
            putc(' ', symFp);
            putSymInt(symbol[symIdx].based, 0); // print the Info word
        } else {
            putc(' ', symFp);
            putSymInt(info, 0); // print the Info word
        }
        putc('/', symFp);
    }

    putc('/', symFp);
}



int wrdata(const int sy) {
    /*     if sy is negative, the call comes from synth -- data is inserted */
    /*     inline by calling LIT with each byte value. */

    /*     if sy is positive, the call comes from dumpin -- */
    /*     wrdata writes data into the output file from symbol at location */
    /*     'sy'  each byte value is written as a pair of base 32 digits. */
    /*     the high order bit of the first digit is 1, and all remaining high */
    /*     order digits are zero. the value returned by wrdata is the total */
    /*     number of bytes written. */
    /*      global tables */
    uint32_t symIdx = iabs(sy);

    /*     check precision of value */
    /*     set dflag to true if we are dumping a variable or LABEL name */
    int type   = symbol[symIdx].type;
    bool dflag = type == LABEL || type == VARB || type == PROC;

    int len    = symbol[symIdx].prec;
    if (len <= 2 && !dflag) { // single or double byte constant
        uint16_t n = symbol[symIdx].len;
        if (len) {
            if (sy < 0) {
                if (len == 2)
                    emit(HIGHBYTE(n), LIT);
                emit(LOWBYTE(n), LIT);
            } else {
                if (len == 2)
                    putSymHex2(HIGHBYTE(n), true);
                putSymHex2(LOWBYTE(n), len != 2);
            }
        }
    } else {

        len             = symbol[symIdx].strLen;
        uint8_t *symStr = idToStr(symbol[symIdx].strId);

        for (int lp = 0; lp < len; lp++) {
            if (dflag) /*     write out the identifier name */
                putc(symStr[lp], symFp);
            else if (sy < 0) /*     emit string data inline */
                emit(symStr[lp], LIT);
            else
                putSymHex2(symStr[lp], lp == 0);
        }
    }
    return len;
}

void dumpch() {
    /*     dump the symbolic names for the simulator */

    putc('/', symFp);
    if (C_MEMORY) {
        int i = 2;
        for (int symIdx = 1; symIdx < symNext; symIdx++) {
            if (!symbol[symIdx].based) {
                int type = symbol[symIdx].type;
                if (type == LABEL || type == VARB || type == PROC) {
                    /* check if real symbol */
                    if (symbol[symIdx].strLen) {
                        putSymInt(symIdx, 3); /* write symbol number */
                        wrdata(symIdx);       /* now write the string */
                        putc('/', symFp);
                    }
                }
            }
        }
    }
    putc('/', symFp);
}


void dumpin() {
    /*     dump the initialization table */
    /*     wrdata(x) writes the data at location x in symbol table */
    /*     and returns the number of bytes written */
    if (C_SYMBOLS == 2) {

        for (int i = 0; i < initialDataSP;) {
            fprintf(lstFp, "\nSYMBOL S%05d =", initialData[i] >> 15);
            int col = 15;
            for (int jp = initialData[i++] & 0x7fff; jp > 0; jp--) {
                if (col + 7 > C_WIDTH) {
                    fprintf(lstFp, "\n%.*s", 15, "");
                    col = 15;
                }
                /*         get the symbol number */
                col += fprintf(lstFp, " S%05d", initialData[i++] >> 16);
            }
        }
    }
    putc('\n', lstFp);
    /*     ready to write the initialization table */
    if (errorCnt)
        return;
    putc('/', symFp);

    for (int i = 0; i < initialDataSP;) {
        putSymInt(initialData[i] >> 15, 3);
        for (int jp = initialData[i++] & 0x7fff; jp > 0; jp--)
            wrdata(initialData[i++] & 0xffff);
        putc('/', symFp);
    }
    putc('/', symFp);

    return;
}

void emit(const int val, const int typ) {

    static int polcnt     = 0;
    static char *polchr[] = { "OPR", "ADR", "VAL", "DEF", "LIT", "LIN" };
    static char *opcval[] = { "NOP", "ADD", "ADC", "SUB", "SBC", "MUL", "DIV", "REM", "NEG",
                              "AND", "IOR", "XOR", "NOT", "EQL", "LSS", "GTR", "NEQ", "LEQ",
                              "GEQ", "INX", "TRA", "TRC", "PRO", "RET", "STO", "STD", "XCH",
                              "DEL", "DAT", "LOD", "BIF", "INC", "CSE", "END", "ENB", "ENP",
                              "HAL", "RTL", "RTR", "SFL", "SFR", "HIV", "LOV", "CVA", "ORG",
                              "DRT", "ENA", "DIS", "AX1", "AX2", "AX3" };

    /*     typ      meaning */
    /*      0      operator */
    /*      1      load address */
    /*      2      load value */
    /*      3      define location */
    /*      4      literal value */
    /*      5      line number */
    /*      6      unused */
    /*      7        " */

    if (C_GENERATE != 0) {
        fprintf(lstFp, "%5d %s ", ++polcnt, polchr[typ]);
        switch (typ) {
        case OPR:
            fprintf(lstFp, opcval[val]);
            break;
        case ADR:
        case VLU:
        case DEF:
            fprintf(lstFp, "S%05d", val);
            break;
        case LIT:
        case LIN:
            fprintf(lstFp, " %05d", val);
        }
        /*     now store the polish element in the polish array. */
        putc('\n', lstFp);
    }

    uint16_t pol = (val << 3) + typ;
    fwrite(&pol, sizeof(pol), 1, polFp); // write the polish element
    return;
}
