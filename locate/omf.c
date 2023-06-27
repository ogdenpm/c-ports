#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "os.h"
#include "omf.h"

// omf record handlers for basic records



uint8_t outRec[1060];       // allows for accidental overrun by long name
uint8_t *outP;              // uint8_t * to current location in outRec
char *omfOutName;
FILE *omfOutFp;

// helper function for none le systems
uint16_t putWord(uint8_t * buf, uint16_t val) {
    buf[0] = val & 0xff;
    buf[1] = val >> 8;
    return val;
}

void openOMFOut() {
    if (!(omfOutFp = Fopen(omfOutName, "wb+")))
        IoError(omfOutName, "Create error");

}



void InitRecord(uint8_t type) {
    outRec[REC_TYPE] = type;
    outP             = outRec + REC_DATA;
}

void WriteByte(uint8_t val) {
    *outP++ = val;
}

void WriteWord(uint16_t val) {
    *outP++ = (uint8_t)val;
    *outP++ = val >> 8;
}

void WriteName(pstr_t const *name) {
    memcpy(outP, name, name->len + 1);
    outP += name->len + 1;
}

void EndRecord(void) {
    // 1025 (+3 for type & len, -1 for pending crc)
    if (putWord(&outRec[REC_LEN], (uint16_t)(outP - outRec - 2)) > 1025)
        FatalError("%s: Record length > 1025", omfOutName);
    uint8_t crc;
    uint8_t *p;
    for (crc = 0, p = outRec; p < outP; crc -= *p++) /* calculate and insert crc */
        ;
    *outP++ = crc; // add the crc
    if (fwrite(outRec, 1, outP - outRec, omfOutFp) != outP - outRec)
        IoError(omfOutName, "Write error");
}


uint8_t inRec[0x10002]; // let VM handle the biggest possible inRecord
uint8_t *inP;
uint8_t *inEnd;
uint16_t recLen;
int recNum;
uint8_t inType;
char *omfInName;
FILE *omfInFp;
static pstr_t const *inModuleName;

// helper functions to allow portability
uint16_t getWord(uint8_t *buf) {
    return buf[0] + buf[1] * 256;
}

_Noreturn void RecError(char const *errMsg) {
    fprintf(stderr, " %s", omfInName);

    if (inModuleName)
        fprintf(stderr, "(%s)", inModuleName->str);

    fprintf(stderr, ", %s\n Record Type %02XH, Record Number ", errMsg, inType);
    if (recNum > 0)
        fprintf(stderr, "%5d\n", recNum);
    else
        fputs("*****\n", stderr);

    Exit(1);
} 

_Noreturn void IllegalRecord(void) {
    RecError("unknown record type");
}

_Noreturn void IllegalReloc(void) {
    RecError("illegal record format");
}

_Noreturn void BadRecordSeq(void) {
    RecError("unexpected record");
}


void openOMFIn(char const *name) {
    if (omfInFp && fclose(omfInFp))
        IoError(omfInName, "Close error");
    if (!(omfInFp = Fopen(name, "rb")))
        IoError(name, "Open error");
    free(omfInName);
    omfInName = xstrdup(name);
    inModuleName = NULL;
}

void closeOMFIn() {
    if (omfInFp && fclose(omfInFp))
        IoError(omfInName, "Close error");
    omfInFp = NULL;
}

pstr_t const *ReadName(void) {
    pstr_t *pname = (pstr_t *)inP;
    if (inP >= inEnd || (inP += pname->len + 1) > inEnd)
        IllegalReloc();
    return pname; /* read a name */
}

/* read in 4 byte block num and byte num inP points to the data, return a uint32_t offset */
uint32_t ReadLocation(void) {
    uint32_t offset = ReadWord() * 128;
    return offset + ReadWord();
}
uint16_t ReadWord(void) {
    if ((inP += 2) > inEnd)
        IllegalReloc();
    return getWord(inP - 2);
}

uint8_t ReadByte(void) {
    if (inP >= inEnd)
        IllegalReloc();
    return *inP++;
}

void GetRecord(void) {

    if (fread(inRec, 1, 3, omfInFp) != 3)
        FatalError("%s: Premature EOF", omfInName);
    recLen = getWord(inRec + REC_LEN);
    inP   = inRec + REC_DATA;
    inEnd = inRec + recLen + 2; // exclude CRC
    if (fread(inP, 1, recLen, omfInFp) != recLen)
        FatalError("%s: Premature EOF", omfInName);
    recNum++;

    uint8_t crc = 0;
    for (int i = 0; i < recLen + REC_DATA; i++)
        crc += inRec[i];
    if (crc)
        RecError("Checksum error");
    inType = inRec[0];
    if (inType == 0 || inType > R_COMDEF || (inType & 1)) /* other invalid caught elsewhere */
        IllegalReloc();
    /* check for special case handling */
    if (recLen > 1025 && inType != R_MODDAT && (inType < R_LIBNAM || inType == R_COMDEF))
        RecError("Record length > 1025");
    if (inType == R_MODHDR)
        inModuleName = pstrdup((pstr_t *)inP);

}


void SeekOMFIn(long loc) {

    if (fseek(omfInFp, loc, SEEK_SET))
        IoError(omfInName, "Seek error");

    recNum = 0; /* reset vars */
}




void CopyRecord(void) {
    if (fwrite(inRec, 1, recLen + 3, omfOutFp) != recLen + 3)
        IoError(omfOutName, "Write error");
}


pstr_t const *GetModuleName(char const *token) {
    pstr_t const *name = c2pstrdup(token);
    if (name->len > 31)
        FatalCmdLineErr("Module name too long");
    if (isdigit(name->str[0]))
        FatalCmdLineErr("Module name cannot start with a digit");
    for (char *s = (char *)name->str; *s; s++) // remove const to allow upper casing
        if (isalnum(*s) || *s == '?' || *s == '@' || *s == '_')
            *s = toupper(*s);
        else
            FatalCmdLineErr("Illegal character in module name");
    return name;
}

//  pascal style string handling for OMF

/* creates a copy of pstr also adding a trailing '\0' for C string */
pstr_t const *pstrdup(pstr_t const *pstr) {
    pstr_t *ps = (pstr_t *)xmalloc(pstr->len + 2);
    memcpy(ps, pstr, pstr->len + 1);
    ps->str[ps->len] = '\0';
    return ps;
}
// create a copy of a C string with a pascal string len prefix
pstr_t const *c2pstrdup(char const *s) {
    uint8_t len = (uint8_t)strlen(s);
    pstr_t *ps  = (pstr_t *)xmalloc(len + 2);
    ps->len     = len;
    strcpy(ps->str, s);
    return ps;
}

bool pstrequ(pstr_t const *s, pstr_t const *t) {
    return s->len == t->len && strncmp(s->str, t->str, s->len) == 0;
}