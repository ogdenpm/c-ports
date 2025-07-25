/****************************************************************************
 *  binobj.c: part of the C port of Intel's ISIS-II binobj             *
 *  The original ISIS-II application is Copyright Intel                     *
 *																			*
 *  Re-engineered to C by Mark Ogden <mark.pm.ogden@btinternet.com> 	    *
 *                                                                          *
 *  It is released for hobbyist use and for academic interest			    *
 *                                                                          *
 ****************************************************************************/

#include <ctype.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "utility.h"

#ifndef _MSC_VER
#define stricmp strcasecmp
#endif

#define MAXNAME 31
#define MODHDR  2
#define MODEND  4
#define CONTENT 6
#define MODEOF  14

char const help[] = "usage : %s binfile [to] objfile";

uint16_t readWord(uint8_t *p) {
    return p[0] + p[1] * 256;
}

void writeWord(uint8_t *p, uint16_t val) {
    *p++ = val % 256;
    *p   = val / 256;
}

/*                                  */
/*  module header record definition */
/*                                  */
struct {
    uint8_t type;
    uint8_t length[2];
    uint8_t nameLen;
    uint8_t name[MAXNAME + 3]; // name + trnId + trnVn + crc
} modhdr = { MODHDR, { 0 }, 0, { 0 } };

/* intermediate storate for length & address */
struct {
    uint8_t length[2];
    uint8_t addr[2];
} binHeader;

struct {
    uint8_t type;
    uint8_t length[2];
    uint8_t segId;
    uint8_t addr[2];
} contentHeader = { CONTENT, { 0 }, 0, { 0 } };

/*                                 */
/*  module end record definition   */
/*                                 */
struct {
    uint8_t recType;
    uint8_t length[2];
    uint8_t type;
    uint8_t segId;
    uint8_t addr[2];
    uint8_t crc;
} modend = { MODEND, { 5, 0 }, 1, 0, { 0 }, 0 };

/*                                 */
/*  module end of file record      */
/*  definition                     */
/*                                 */
struct {
    uint8_t type;
    uint8_t length[2];
    uint8_t crc;
} modeof = { MODEOF, { 1, 0 }, 0xf1 };

/* calc crc */
uint8_t calcCRC(uint8_t *p, int len) {
    uint8_t crc = 0;
    while (len-- > 0)
        crc += *p++;
    return crc;
}

/* *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*
   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*

      BEGINNING OF MAIN PROGRAM.
      Note, unlike the original this version will does not
      limit the output block size

   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*
   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-* */

int main(int argc, char **argv) {
    FILE *fpin, *fpout;
    uint16_t length, addr;

    chkStdOptions(argc, argv);

    if (!(argc == 3 || (argc == 4 && stricmp(argv[2], "TO") == 0)))
        usage("Invalid command line");


    if ((fpin = fopen(argv[1], "rb")) == NULL)
        fatal("can't open %s\n", argv[1]);

    if ((fpout = fopen(argv[argc - 1], "wb")) == NULL) {
        fclose(fpin);
        fatal("can't create %s\n", argv[2]);
    }

    /*
     * output module header record, taking name from input file
     * this version allows upto 31 characters, original limited at 6
     * non alpha numeric are deleted and all alpha is converted to uppercase
     */

    int i = 0;
    for (const char *name = basename(argv[1]); *name && *name != '.'; name++)
        if (isalnum(*name) && i < MAXNAME)
            modhdr.name[i++] = toupper(*name);
    modhdr.nameLen = i;
    writeWord(modhdr.length, i + 4);
    modhdr.name[i++] = 0; // TRN ID
    modhdr.name[i++] = 0; // TRN VN
    modhdr.name[i]   = -calcCRC((uint8_t *)&modhdr, i + 6);
    if ((int)fwrite(&modhdr, 1, i + 5, fpout) != i + 5)
        fatal("failed to write obj file header\n");

    /*
     * read all bin records and write as obj records
     * until length of record is 0
     */
    while (true) {
        if (fread(&binHeader, 1, 4, fpin) != 4)
            fatal("unexpected EOF\n");

        length = readWord(binHeader.length);
        addr   = readWord(binHeader.addr);
        if (length == 0)
            break;
        // now we know length and address of bin block
        // create the corresponding content header
        // type & segId initialised already
        writeWord(contentHeader.length, length + 4); // data + addr + crc
        writeWord(contentHeader.addr, addr);
        if (fwrite(&contentHeader, 1, 6, fpout) != 6)
            fatal("failed to write obj file content\n");

        uint8_t crc = calcCRC((uint8_t *)&contentHeader, 6);
        while (length-- > 0) {
            int c;
            if ((c = getc(fpin)) == EOF)
                fatal("unexpected EOF reading bin file\n");
            crc += c; // update CRC
            putc(c, fpout);
        }
        putc(-crc, fpout);
    }
    /*
        fill in the missing details of the
        output the module end record
    */
    writeWord(modend.addr, addr);
    modend.crc = -calcCRC((uint8_t *)&modend, 7);
    if (fwrite(&modend, 1, 8, fpout) != 8 || fwrite(&modeof, 1, 4, fpout) != 4)
        fatal("failed to write obj file end information\n");

    fclose(fpin);
    fclose(fpout);
    return 0;
}