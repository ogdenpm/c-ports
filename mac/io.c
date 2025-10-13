#include "mac.h"
#include "utility.h"
#include <ctype.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define _HEXlen 16
uint16_t HEXadr;
uint8_t HEXlen;
uint8_t HEXline[_HEXlen];
uint16_t pageNo;

uint8_t LstPaglen = PAGLEN; // page length - 0 means disabled
uint8_t LstLine;

// %%%%%%%%%%%%%%%%%%
// %% OS Interface %%
// %%%%%%%%%%%%%%%%%%
bool skipFirstLine = true; // to match MAC behaviour for files
int col;

void OpenLib() {
    if (fpLIB)
        fclose(fpLIB);
    if (!(fpLIB = fopen(libFile, "rb")))
        fatal("%s-Cannot open MACLIB file", libFile);
}

void PrepLib() {
    InLIB = true;
    curCh = 0;
}

void IniMAC(int argc, char **argv) {
    fputs("CP/M Macro Assem 2.0\n", stdout);
    TopPtr = 0xfff0;
    SymBot = SymTop = 0x100; // allow large symbol space

    uint8_t cval;
    while (getOpt(argc, argv, "p:s:h:c:t=") != EOF) {
        switch (optOpt) {
        case 'p':
            prnFile = optArg;
            break;
        case 's':
            symFile = optArg;
            break;
        case 'h':
            hexFile = optArg;
            break;
        case 'c':
            char const *s = optArg;
            while (*s) {
                cval = *s == '-'                  ? _DISA
                       : *s == '+'                ? _ENA
                       : (*s == '*' || *s == 'x') ? _PART
                                                  : 0xff;
                if (cval == 0xff)
                    usage("Missing +/-/*/x in controls at %s", s);
                while (*++s == ' ' || *s == '\t')
                    s++;
                switch (tolower(*s)) {
                case 'l':
                    L_opt = cval;
                    break;
                case 'm':
                    M_opt = cval;
                    break;
                case 'q':
                    Q_opt = cval;
                    break;
                case 'r':
                    R_opt = cval;
                    break;
                case '1':
                    _1_opt = cval;
                    break;
                case '\0':
                    usage("Missing control at end of %s", optArg);
                default:
                    usage("Unknown control %c", *s);
                }
                if (cval == _PART && tolower(*s) != 'm')
                    warn("*/x operation only applicable to 'M' control");
                s++;
                while (*s == ' ' || *s == '\t')
                    s++;
            }
            break;
        case 't':
            if (!optArg)
                optArg = "8";
            if ((expandTab = (uint8_t)strtoul(optArg, NULL, 0)) > 8)
                expandTab = 8;
            break;
        }
    }
    if (optInd != argc - 1)
        usage("Expected single source file");
    srcFile = safeStrdup(makeFilename(argv[optInd], ".asm", false));

    if (!prnFile)
        prnFile = safeStrdup(makeFilename(srcFile, ".prn", true));
    if (strcmp(prnFile, ".") != 0) {
        if (strcmp(prnFile, "-") == 0) {
            fpPRN         = stdout;
            skipFirstLine = false;
        } else if (!(fpPRN = fopen(prnFile, "wt")))
            fatal("%s-Cannot Create File", prnFile);
    }
    if (!hexFile)
        hexFile = safeStrdup(makeFilename(srcFile, ".hex", true));
    if (strcmp(hexFile, ".") != 0) {
        if (strcmp(hexFile, "-") == 0)
            fpHEX = stdout;
        else if (!(fpHEX = fopen(hexFile, "wt")))
            fatal("%s-Cannot Create File", hexFile);
    }
}

void IniLine() {
    pageNo = 0;
    P_opt  = true;

    // there is an inconsistency in mac
    // the call from l17f1 to emitLine emits " \n"
    // for devices this is fine, but for files the pendind output buffer
    // is reset so removing the "\n"
    // unfortunately if the option to list on pass one is given
    // the pass 1 listing can be partially overwritten

    if (PassNr)
        Header(); // .. pass 2, give header
    HEXlen = 0;
    if (fpSRC)
        rewind(fpSRC);
    else if (!(fpSRC = fopen(srcFile, "rb")))
        fatal("%s-Cannot Open Source File", srcFile);
}

int mgetc() {
    int c;
    static bool neednl = false;

    if (neednl) {
        neednl = false;
        return '\n';
    }
    if (InLIB) {
        while ((c = getc(fpLIB)) == '\r')
            ;
        if (c == '\n') {
            neednl = true;
            return '\r';
        }
        if (c != EOF && c != eof)
            return c & 0x7f;

        fclose(fpLIB);
        if (Balance)
            fatal("%s - Unbalanced Macro Lib", libFile);
        InLIB = false;
    }
    while ((c = getc(fpSRC)) == '\r')
            ;
    if (c == '\n') {
        neednl = true;
        return '\r';
    }
    if (c == EOF || c == eof) {
        if (ferror(fpSRC))
            warn("%s - unexpected EOF", srcFile);
        if (c == eof)
            ungetc(c, fpSRC); // just incase called again
        return eof;
    }
    return c & 0x7f;
}

void fput_p(uint8_t ch) {
    if (skipFirstLine) {
        if (ch == '\n')
            skipFirstLine = false;
        return;
    }
    if (ch == '\n')
        col = 0;
    else if (ch == '\t' && expandTab) {
        do {
            fput_p(' ');
        } while (col % expandTab);
        return;
    } else if (ch >= ' ')
        col++;
    if (fpPRN && putc(ch, fpPRN) == EOF)
        fatal("%s - PRN File Write Error", prnFile);
}

void fput_x(uint8_t ch) {
    if (fpHEX && putc(ch, fpHEX) == EOF)
        fatal("%s-HEX File Write Error", hexFile);
}

void Header() {
    if (LstPaglen && fpPRN) {
        fput_p('\f');
        LstLine = 0;
        if (Titleptr) {
            if (fprintf(fpPRN, "CP/M Macro Assem 2.0\t#%03d\t%s\n\n", ++pageNo, &BYTE(Titleptr)) <
                0)
                fatal("%s-Write Error", prnFile);
        }
    }
}

void LstPage(uint8_t len) {
    LstPaglen = len;
    if (LstLine < LstPaglen)
        Header();
}

// Put character to list device and console on error
void lstChar(char ch) {
    fput_p(ch);
    if (OutLine[0] != ' ' && PassNr != 2 && fpPRN != stdout)
        putchar(ch);
}

void emitLine() {

    uint8_t emit = 1; // implicit emit
    if (!(_1_opt | PassNr)) {
        if (L_opt && InLIB)
            emit = 2; // explicit emit
        else if (!InLIB || OutLine[0] == ' ')
            emit = 0;
    }
    if (emit == 1) {
        if (OutLine[0] != ' ')
            ;
        else if (!P_opt)
            emit = 0;
        else if (OutLine[5] != '+')
            ;
        else if (M_opt == _DISA)
            emit = 0;
        else if (M_opt == _ENA)
            ;
        else if (OutLine[6] == '#' || OutLine[1] == ' ')
            emit = 0;
        else if (M_opt != _SPENA)
            OutLen = _ASCbyt; // truncate
    }
    if (emit) {
        if (LstLine >= LstPaglen)
            Header();
        LstLine++;
        // trim trailing spaces
        while (OutLen && (OutLine[--OutLen] == ' ' || OutLine[OutLen] == '\t'))
            ;
        for (uint8_t i = 0; i <= OutLen; i++)
            lstChar(OutLine[i]);
        lstChar('\n');
    }

    OutLen = 0;
    memset(OutLine, ' ', LINLEN);
}

void SetErr(uint8_t err) {
    if (OutLine[0] == ' ')
        OutLine[0] = err;
}

void CloseLst() {
    if (fpPRN && fpPRN != stdout)
        fclose(fpPRN);
}

void PrepSYM() {
    if (S_opt != _ENA) {
        if (fpPRN && fpPRN != stdout)
            fclose(fpPRN);
        fpPRN = NULL;
        if (symFile) {
            if (strcmp(symFile, ".") == 0)
                return;
            else if (strcmp(symFile, "-") == 0)
                fpPRN = stdout;
            else
                symFile = safeStrdup(makeFilename(symFile, ".sym", false));
        } else
            symFile = safeStrdup(makeFilename(srcFile, ".sym", true));
        if (!fpPRN && !(fpPRN = fopen(symFile, "wt")))
            fatal("Cannot create symbol file %s", symFile);
    }
    Header();
}

void FClose() {
    if (fpPRN && fpPRN != stdout)
        fclose(fpPRN);
    if (fpHEX) {
        if (HEXlen)
            PutHexRecord(); // flush pending record
        HEXadr = CurHEX;
        PutHexRecord(); // closing record
        if (fpHEX != stdout)
            fclose(fpHEX);
    }
    fputs("End of Assembly\n", stdout);
    exit(0);
}

void putHEX(uint8_t ch) {
    if (fpHEX) {
        if (HEXlen && (HEXlen >= _HEXlen || HEXadr + HEXlen != CurHEX))
            PutHexRecord();
        if (HEXlen == 0)
            HEXadr = CurHEX;
        HEXline[HEXlen++] = ch;
    }
}

void PutHexRecord() {
    fprintf(fpHEX, ":%02X%04X00", HEXlen, HEXadr);
    uint8_t crc = 0 - HEXlen - HEXadr - (HEXadr >> 8);
    for (uint8_t i = 0; i < HEXlen; i++) {
        fprintf(fpHEX, "%02X", HEXline[i]);
        crc -= HEXline[i];
    }
    fprintf(fpHEX, "%02X\n", crc);
    HEXlen = 0;
}
