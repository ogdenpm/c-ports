
#include "plm82.h"

FILE *inFp;
FILE *hexFp;
FILE *outFp;
FILE *polFp;
FILE *lstFp;
FILE *symFp;
char *src;

#ifdef linux
#include <unistd.h>
#endif
void closefiles(void) {
    if (polFp) {
        fclose(polFp);
        if (errorCnt == 0)
            unlink(makeFilename(src, ".pol", true));
    }
    if (symFp) {
        fclose(symFp);
        if (errorCnt == 0)
            unlink(makeFilename(src, ".sym", true));
    }
    if (lstFp)
        fclose(lstFp);
    if (hexFp) {
        fclose(hexFp);
        if (errorCnt)
            unlink(makeFilename(src, ".hex", true));
    }
}

void openfiles(char *srcFile) {
    src = srcFile;
    char *path;
    atexit(closefiles);
    if (!(polFp = fopen(path = makeFilename(srcFile, ".pol", true), "rb"))) {
        fprintf(stderr, "can't open pol file %s\n", path);
        exit(1);
    }
    if (!(symFp = fopen(path = makeFilename(srcFile, ".sym", true), "rb"))) {
        fprintf(stderr, "can't open symbol file %s\n", path);
        exit(1);
    }
    if (!(lstFp = fopen(path = makeFilename(srcFile, ".lst", true), "at"))) {
        fprintf(stderr, "can't open listing file %s\n", path);
        exit(1);
    }
    if (C_HEXFILE && !(hexFp = fopen(path = makeFilename(srcFile, ".hex", true), "at"))) {
        fprintf(stderr, "can't create hex file %s\n", path);
        exit(1);
    }
}

// string msg variant of error
void error(char const *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    errorCnt++;
    fprintf(lstFp, "\n(%05d)  Error ", C_COUNT);
    vfprintf(lstFp, fmt, args);
    putc('\n', lstFp);
    va_end(args);
}

void Fatal(char const *msg, ...) {
    error(msg);
    fprintf(lstFp, "\nCompilation Terminated\n");
    fprintf(stderr, "\nCompilation Terminated\n");
    errflg = true;
}

/*
    simplified as only base 16 is used
    also uses pre decoded instructions to simplify the logic
 */
void dump(int lp, const int u, bool symbolic) {
    // items on line = (line width - address width) / (length of item + a space)
    int itemsOnLine = symbolic ? (C_WIDTH - 5) / 7 : (C_WIDTH - 5) / 4;

    if (itemsOnLine <= 0)
        error("117: line width set too narrow for code dump");

    else {
        uint16_t itemList[32];

        for (int i = 0; i < 32; i++) // initialise line to no values
            itemList[i] = 256;
        uint16_t nsame = 0;
        uint8_t opcnt  = 0;
        while (lp <= u) {
            bool same     = true;
            int lineStart = lp;
            int nItems;
            for (nItems = 0; nItems < itemsOnLine; nItems++) {
                if (lp > u) { // data finishes mid line
                    same = false;
                    break;
                } else {
                    int j = get(lp++);           // check if same
                    if (j != itemList[nItems]) { // if not mark and update value
                        same             = false;
                        itemList[nItems] = j;
                    }
                }
            }
            if (same) {
                if (nsame++ == 0) // single blank line for repeat lines
                    putc('\n', lstFp);
            } else { // non same line
                nsame = 0;
                fprintf(lstFp, "%04XH", lineStart); // print the address line
                int space = 1;
                for (int item = 0; item < nItems; item++) {
                    if (symbolic) {
                        fprintf(lstFp, "%*s", space, "");
                        if (opcnt-- > 0)
                            space = 7 - fprintf(lstFp, "%02XH", itemList[item]);
                        else {
                            space = 7 - fprintf(lstFp, "%s", ctran[itemList[item]].opcode);
                            opcnt = ctran[itemList[item]].extra;
                        }
                    } else
                        fprintf(lstFp, " %02XH", itemList[item]);
                }
                putc('\n', lstFp);
            }
        }
    }
}

// BPNF support removed
void puncod(int start, const int end) {
    for (int remaining = end - start + 1, toWrite = 16; remaining > 0; remaining -= toWrite) {
        if (remaining < 16)
            toWrite = remaining; // adjust to write less than 16 bytes if needed

        fprintf(hexFp, ":%02X%04X00", toWrite, start);
        uint8_t crc = -(toWrite + start % 256 + start / 256);
        for (int i = 0; i < toWrite; i++, start++) {
            int val = get(start);
            crc -= val;
            fprintf(hexFp, "%02X", val);
        }
        fprintf(hexFp, "%02X\n", crc);
    }
}

void puntrailer() { // write end of file record
    if (entry < 0)
        entry = codeBase == preamb && C_STACKHANDLING != 1 ? 0 : codeBase;
    uint8_t crc = -(1 + entry / 256 + entry % 256);
    fprintf(hexFp, ":00%04X01%02X\n", entry, crc);
}

#define MAXLABEL 32
void sydump() {
    /* dump the symbol table for the simulator */
    /* clear the output buffer */
    putc('\n', lstFp);
    int len;

    while ((len = getc(symFp)) > 0) { /* process next symbol table entry */
        int symNo = getc(symFp);
        symNo += getc(symFp) * 256;

        /* write symbol number, symbol */
        char label[MAXLABEL + 1];
        memset(label, '.', MAXLABEL);

        int toRead = (int)(len <= MAXLABEL ? len : MAXLABEL);
        if ((int)fread(label, 1, toRead, symFp) != toRead)
            error("xx: read error in symbol file");

        if (len > MAXLABEL) {
            error("xx: symbol too long - truncated");
            fseek(symFp, len - MAXLABEL, SEEK_CUR);
        }
        label[MAXLABEL] = '\0';
        if (symNo == 5 || symNo > 6) { /* write hex address */
            int j  = symbol[symNo];
            int ch = symbol[j - 1] & 0xf;
            if (ch == PROC || ch == LABEL)
                j -= 2;
            int addr = abs(symbol[j]);
            if (C_HEXFILE)
                fprintf(hexFp, "%-5d %.*s %05XH\n", symNo, len, label, addr);
            if (C_MAP)
                fprintf(lstFp, "%s %04XH\n", label, addr);
        }
    }
    putc('\n', lstFp);

    if (len < 0)
        Fatal("xx: premature EOF reading symbol file");
}

void cmpuse() {
    printf("table usage in pass 2:\n");
    printf("symbol table - max=%-5d, top=%-5d, info=%-5d\n", symax, sytop, syinfo);
}
