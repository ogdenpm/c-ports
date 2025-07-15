/****************************************************************************
 *  objhex.c: part of the C port of Intel's ISIS-II objhex             *
 *  The original ISIS-II application is Copyright Intel                     *
 *																			*
 *  Re-engineered to C by Mark Ogden <mark.pm.ogden@btinternet.com> 	    *
 *                                                                          *
 *  It is released for hobbyist use and for academic interest			    *
 *                                                                          *
 ****************************************************************************/

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "utility.h"
#ifdef _WIN32
#define DIRSEP "/\\:"
#else
#define DIRSEP "/"
#endif
#ifndef _MSC_VER
#define stricmp strcasecmp
#endif

#define MODHDR  2
#define MODEND  4
#define CONTENT 6
#define RELOC   0x22

#define MAXHEX  16

char const help[] = "Usage: %s objfile [to] hexfile";

uint8_t getByte(FILE *fp) {
    int c = getc(fp);
    if (c == EOF)
        fatal("Unexpected EOF in obj file\n");
    return (uint8_t)c;
}

uint16_t getAddress(FILE *fp) {
    uint16_t val = getByte(fp);
    return val + getByte(fp) * 256;
}

// return the trailing filename part of the passed in path
const char *basename(const char *path) {
    const char *t;
    while ((t = strpbrk(
                path,
                DIRSEP))) // allow windows & unix separators - will fail for unix if : in filename!!
        path = t + 1;
    return path;
}

int main(int argc, char **argv) {
    chkStdOptions(argc, argv);

    if (!(argc == 3 || (argc == 4 && stricmp(argv[2], "TO") == 0)))
        usage("Bad command line");

    FILE *fpin, *fpout;

    if ((fpin = fopen(argv[1], "rb")) == NULL)
        fatal("Cannot open obj file %s\n", argv[1]);

    if ((fpout = fopen(argv[argc - 1], "wt")) == NULL) {
        fclose(fpin);
        fatal("Cannot create hex file %s\n", argv[2]);
    }

    /*
       READ OBJECT RECORDS, WRITE HEXADECIBAL RECORDS.
    */
    bool error = false;
    for (; !error;) {
        uint16_t addr;
        uint8_t type = getByte(fpin);

        if (type < MODHDR || type >= RELOC || (type & 1))
            fatal("Unsupported type %02x\n", type);

        uint16_t reclen = getAddress(fpin);
        if (type == MODEND) {
            getByte(fpin);
            getByte(fpin);
            addr  = getAddress(fpin);
            error = fprintf(fpout, ":00%04X01%02X\n", addr, 0xff & -((addr >> 8) + addr + 1)) < 0;
            break;
        }
        if (type != CONTENT)
            error = fseek(fpin, reclen, SEEK_CUR) != 0;
        else {
            getByte(fpin);
            addr = getAddress(fpin);
            reclen -= 4;
            while (reclen) {
                uint8_t hexlen = reclen > MAXHEX ? MAXHEX : reclen;
                reclen -= hexlen;
                uint8_t crc = hexlen + (addr >> 8) + addr;
                error       = fprintf(fpout, ":%02X%04X00", hexlen, addr) < 0;

                for (int i = 0; !error && i < hexlen; i++) {
                    uint8_t c = getByte(fpin);
                    crc += c;
                    error = fprintf(fpout, "%02X", c) < 0;
                }
                if (!error)
                    error = fprintf(fpout, "%02X\n", (-crc) & 0xff) < 0;
                addr += hexlen;
            }
            getByte(fpin); // junk crc
        }
    }
    if (error)
        fputs("Error writing hex file\n", stderr);
    fclose(fpin);
    fclose(fpout);
    return 0;
}