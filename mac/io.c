#include "mac.h"
#include "utility.h"
#include <ctype.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _MSC_VER
#include <io.h>
#else
#include <unistd.h>
#endif

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
bool squashFirstLine = true; // to match MAC behaviour for files
bool neednl   = false;
int col;

void OpenLib() {
    if (fpLIB)
        fclose(fpLIB);
    if (!(fpLIB = fopen(libFile, "rb")))
        fatal("%s - Cannot open MACLIB file", libFile);
}

void PrepLib() {
    InLIB = true;
    curCh = 0;
}

void IniMAC(int argc, char **argv) {
    fputs("CP/M Macro Assem 2.0\n", stdout);
    topPtr = TOPHEAP;
    SymBot = SymTop = BOTHEAP; // allow large symbol space

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
                    usage("Missing control operation (+,-,* or x) at %s", s);
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
                    warn("* or x operation only applicable to 'M' control");
                s++;
                while (*s == ' ' || *s == '\t')
                    s++;
            }
            break;
        case 't':
            if (!optArg || !*optArg)
                optArg = "8";
            if ((expandTab = (uint8_t)strtoul(optArg, NULL, 0)) > 8)
                expandTab = 8;
            break;
        }
    }
    if (optInd != argc - 1)
        usage("Expected single source file");
    if ((argv[optInd][0] == '.' || argv[optInd][0] == '-') && argv[optInd][1] == '\0')
        usage("Invalid source file name '%s'", argv[optInd]);
    srcFile = makeFilename(argv[optInd], ".asm", false);

    if (!prnFile)
        prnFile = srcFile;
    if (prnFile[0] != '.' || prnFile[1] != '\0') {
        // MAC has a behaviour quirk where
        // the call from l17f1 to emitLine emits " \n"
        // for devices this is fine, but for files the pending output buffer
        // is reset so removing the "\n"
        // unfortunately if the option to list on pass one is given
        // the pass 1 listing can be partially overwritten
        // this is fixed by squashing the first line output to file

        if (prnFile[0] == '-' && prnFile[1] == '\0') {
            fpPRN           = stdout;
            squashFirstLine = false;
        } else {
            prnFile = makeFilename(prnFile, ".prn", prnFile == srcFile);
            if (!(fpPRN = fopen(prnFile, "wt")))
                fatal("%s - Cannot Create File", prnFile);
        }
    }
    if (!hexFile)
        hexFile = srcFile;
    if (hexFile[0] != '.' || hexFile[1] != '\0') {
        if (hexFile[0] == '-' && hexFile[1] == '\0')
            fpHEX = stdout;
        else {
            hexFile = makeFilename(hexFile, ".hex", hexFile == srcFile);
            if (!(fpHEX = fopen(hexFile, "wt")))
                fatal("%s - Cannot Create File", hexFile);
        }
    }
}

void IniPass() {
    pageNo = 0;
    P_opt  = true;

    if (PassNr)
        Header(); // .. pass 2, give header
    HEXlen = 0;
    if (fpSRC)
        rewind(fpSRC);
    else if (!(fpSRC = fopen(srcFile, "rb")))
        fatal("%s - Cannot Open Source File", srcFile);
    neednl = false;
}



int mgetc() {
    int c;


    if (neednl) {
        neednl = false;
        return '\n';
    }
    FILE *fp = InLIB ? fpLIB : fpSRC;
    while ((c = getc(fp)) == '\r') // ignore \r so works for either \n or \r\n line endings
        ;
    if (c == '\n') {
        neednl = true;
        return '\r';
    }
    if (c != EOF && c != cpmEOF)
        return c & 0x7f;
    if (InLIB) {
        fclose(fpLIB);
        if (Balance)
            fatal("%s - Unbalanced Macro Lib", libFile);
        InLIB = false;
        return mgetc();
    }
    if (c == cpmEOF)
        ungetc(c, fpSRC); // just incase called again
    return cpmEOF;
}

void fput_p(uint8_t ch) {
    if (squashFirstLine) {
        if (ch == '\n')
            squashFirstLine = false;
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
    if (putc(ch, fpPRN) == EOF)
        fatal("%s - PRN File Write Error", prnFile);
}

void Header() {
    if (LstPaglen && fpPRN) {
        fput_p('\f');
        LstLine = 0;
        if (Titleptr &&
            fprintf(fpPRN, "CP/M Macro Assem 2.0\t#%-3d\t%s\n\n", ++pageNo, Titleptr) < 0)
            fatal("%s - Write Error", prnFile);
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
    if (OutLine[0] != ' ' && PassNr == 1 && fpPRN != stdout)
        putc(ch, stderr);
}

void emitLine() {
    if (fpPRN) {          // only if not suppressed
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
            while (OutLen && (OutLine[OutLen] == ' ' || OutLine[OutLen] == '\t'))
                OutLen--;
            for (uint8_t i = 0; i <= OutLen; i++)
                lstChar(OutLine[i]);
            lstChar('\n');
        }
    }
    OutLen = 0;
    memset(OutLine, ' ', sizeof(OutLine));
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
        LstPaglen = 0;
        fpPRN = NULL;
        if (!symFile)
            symFile = srcFile;
        if (symFile[0] == '.' && symFile[1] == '\0')
            return;
        else if (symFile[0] == '-' && symFile[1] == '\0')
            fpPRN = stdout;
        else {
            symFile = makeFilename(symFile, ".sym", symFile == srcFile);
            if (!(fpPRN = fopen(symFile, "wt")))
                fatal("Cannot create symbol file %s", symFile);
        }
    } else
        Header();
}

_Noreturn void Exit(int errCode) { // called explicitly or via fatal
    if (fpPRN && fpPRN != stdout)
        fclose(fpPRN);
    if (fpHEX) {
        if (!errCode) {
            if (HEXlen)
                PutHexRecord(); // flush pending record
            HEXadr = CurHEX;
            PutHexRecord(); // closing record
        }
        if (fpHEX != stdout) {
            if (errCode && !isatty(fileno(fpHEX))) {
                fclose(fpHEX);
                remove(hexFile);
            } else
                fclose(fpHEX);
        }
    }
    fputs("End of Assembly\n", stdout);
    exit(errCode);
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
    bool ok     = fprintf(fpHEX, ":%02X%04X00", HEXlen, HEXadr) == 9;

    uint8_t crc = HEXlen + HEXadr + (HEXadr >> 8);
    for (uint8_t i = 0; ok && i < HEXlen; i++) {
        ok = fprintf(fpHEX, "%02X", HEXline[i]) == 2;
        crc += HEXline[i];
    }
    if (!ok || fprintf(fpHEX, "%02X\n", (0 - crc) & 0xff) != 3)
        fatal("%s - HEX File Write Error", hexFile);
    HEXlen = 0;
}
