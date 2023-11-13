/****************************************************************************
 *  common.cpp: part of the C port of Intel's ISIS-II plm80             *
 *  The original ISIS-II application is Copyright Intel                     *
 *																			*
 *  Re-engineered to C++ by Mark Ogden <mark.pm.ogden@btinternet.com> 	    *
 *                                                                          *
 *  It is released for hobbyist use and for academic interest			    *
 *                                                                          *
 ****************************************************************************/

// $Id: common.cpp,v 1.2 2004/11/30 23:48:08 Mark Exp $
#include "common.hpp"
#include "plm.hpp"
#include <assert.h>
#ifdef __GNUC__
#include <errno.h>
#endif

const Byte tblOffsets[]  = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2 };
const Byte tblBitFlags[] = { 0x80, 0x40, 0x20, 0x10, 0x10, 8, 8, 4, 2,    1,
                             0x80, 0x40, 0x20, 0x10, 8,    4, 2, 1, 0x80, 0x40 };

const word IOERR_254     = 0xfe;

void alloc(word size1, word size2) {
    blk1Used += size1;
    blk2Used += size2;

    if (blk1Used >= blkSize1 || blk2Used >= blkSize2)
        fatalError(ERR83); // LIMIT EXCEEDED: DYNAMIC STORAGE
}

symbol_pt allocSymbolSpc(word spc) {
    address newbot;
    if ((newbot = (word)botSymbol - spc) < topInfo)
        fatalError(ERR83); // LIMIT EXCEEDED: DYNAMIC STORAGE
    return botSymbol = (symbol_pt)newbot;
}

void setAddr(word val) {
    curInfo_p->addr = val;
}

void setNextInfo(info_pt val) {
    curInfo_p->nextInfoOffset = (val == 0) ? 0 : info2Off(val);
}

void setInfoSymbol(symbol_pt psym) {
    curInfo_p->symbolOffset = (psym == 0) ? 0 : topSymbol - psym;
}

void setInfoScope(word val) {
    curInfo_p->infoScope = val;
}

void setInfoLen(Byte len) {
    curInfo_p->len = len;
}

void setInfoType(Byte val) {
    curInfo_p->type = val;
}

void setFlag(Byte *base, Byte flagId) {
    base[tblOffsets[flagId]] |= tblBitFlags[flagId];
}

Byte num2Asc(word num, Byte width, Byte radix, char *buf) {
    Byte lbuf[18];
    Byte padch = ' ';
    Byte fmt, j, lwidth, firstch, i;

    if (width > 0x7f) {
        padch = '0';
        width = -width;
    }
    lwidth = (width != 0 && width <= 16) ? width : 16;
    fmt    = 0;
    if (radix > 0x7f) {
        fmt   = 0xff;
        radix = -radix;
    }
    if (radix > 16)
        radix = 16;
    firstch = 17 - lwidth;
    FILL(lwidth, lbuf + firstch, padch);
    i = lwidth;
    j = 16;
    while (i-- != 0) {
        lbuf[j] = "0123456789ABCDEF"[num % radix];
        if ((num /= radix) == 0)
            break;
        j--;
    }
    if (width == 0) {
        if (fmt && lbuf[j] > '9')
            lbuf[--j] = '0';
        firstch = j;
        lwidth  = 17 - j;
    }
    if (fmt) {
        lwidth++;
        lbuf[17] = "BXXXXXQXDXXXXXH"[radix - 2];
    }
    movemem(lwidth, lbuf + firstch, buf);
    return lwidth;
}

void cpyTill(const char *src, char *dest, word maxcnt, Byte endch) {
    while (maxcnt-- != 0 && *src != endch)
        *dest++ = *src++;
}

void printStr(char *buf, Byte cnt) {
    word status;
    Write(stdout, buf, cnt, &status);
}

// char *errStrTable = "\x4" "ILLEGAL FILENAME SPECIFICATION\0"
//     "\x5" "ILLEGAL OR UNRECOGNIZED DEVICE SPECIFICATION IN FILENAME\0"
//     "\x0c" "ATTEMPT TO OPEN AN ALREADY OPEN FILE\0"
//     "\x0d" "NO SUCH FILE\0"
//     "\x0e" "FILE IS WRITE PROTECTED\0"
//     "\x13" "FILE IS NOT ON A DISKETTE\0"
//     "\x16" "DEVICE TYPE NOT COMPATIBLE WITH INTENDED FILE USE\0"
//     "\x17" "FILENAME REQUIRED ON DISKETTE FILE\0"
//     "\x1c" "NULL FILE EXTENSION\0"
//     "\xfe" "ATTEMPT TO READ PAST EOF\0";
//
//
// word fatalIO_errcode;
// word fatalIO_off;
// byte fatalIO_len;

void fatalIO(file_t *fp, word errcode) {
    //    fatalIO_errcode = errcode;

    printf("\r\n\nPL/M-80 I/O ERROR --\r\n  FILE: %.6s\r\n", fp->shortName);
    printf("  NAME: %.15s\r\n", fp->fullName);

    printf("  ERROR: %d--%s\r\nCOMPILATION TERMINATED\r\n\n", errcode,
           errcode == 0xfe ? "ATTEMPT TO READ PAST EOF" : strerror(errcode));

    exit(errcode);
}

// void findErrStr()
//{
//     word offset, mark;
//     for (offset = 0; errStrTable[offset]; offset++) {
//	mark = offset;
//	while (errStrTable[offset])
//	    offset++;
//	if (errStrTable[mark] == fatalIO_errcode) {
//	    fatalIO_off = mark + 1;
//	    fatalIO_len = offset - fatalIO_off;
//	    return;
//	}
//     }
//     fatalIO_len = 0;
// }

void lookup(unsigned char *s) {
    word hval = hash(s);
    symbol_pt p;

    curSymbol_p = hashChains_p[hval];

    for (p = 0; curSymbol_p != 0; p = curSymbol_p, curSymbol_p = curSymbol_p->link) {
        if (strncmp((char *)curSymbol_p->name, (char *)s, (Byte)(s[0] + 1)) == 0) {
            if (p != 0) {                                // move to front if not already there
                p->link            = curSymbol_p->link;  // remove curSymbol from chain;
                curSymbol_p->link  = hashChains_p[hval]; // and move to front;
                hashChains_p[hval] = curSymbol_p;
            }
            return;
        }
    }
    alloc(0, (word)(s[0] + 1));
    curSymbol_p = allocSymbolSpc((word)(s[0] + 5));
    movemem((word)(s[0] + 1), s, curSymbol_p->name);
    curSymbol_p->infoChain = 0;
    curSymbol_p->link      = hashChains_p[hval];
    hashChains_p[hval]     = curSymbol_p;
}

word hash(unsigned char *p) {
    Byte c = 0;
    Byte len;

    for (len = *p; len; len--) {
        c = (c << 1) + ((c >> 7) & 1) + *p++;
    }
    return c & 0x3f;
}

void setDATE(const char *buf, Byte len) {
    if (len > 9)
        len = 9;
    FILL(9, DATE, ' ');
    movemem(len, buf, DATE);
}

void setPAGELEN(word plen) {
    PAGELEN = (Byte)plen;
}

void setPAGEWIDTH(word pwidth) {
    PAGEWIDTH = pwidth;
}

// 0 -> Literally
// 1 -> Label
// 2 -> Byte
// 3 -> Address
// 4 -> Structure
// 5 -> Procedure
// 6 -> BuiltIn
// 7 -> Macro (being expanded)
// 9 -> temp
Byte infoLengths[] = { 10, 12, 18, 18, 18, 22, 11, 10, 8, 9 };

info_pt allocInfo(word size) {
    info_pt base;
    alloc(size, size);
    base = topInfo + 1;
    if (botSymbol < topInfo + size)
        fatalError(ERR83); // LIMIT EXCEEDED: DYNAMIC STORAGE
    FILL(size, base, 0);
    topInfo += size;
    return base;
}

void createInfo(word val, Byte type) {
    Byte len  = infoLengths[type];
    curInfo_p = allocInfo(len);
    if (curSymbol_p != 0) {
        setNextInfo(curSymbol_p->infoChain);
        curSymbol_p->infoChain = curInfo_p;
    }
    setInfoType(type);
    setInfoLen(len);
    setInfoScope(val);
    setInfoSymbol(curSymbol_p);
}

Byte getParamCnt() {
    return curInfo_p->type == PROC_T ? curInfo_p->paramCnt : curInfo_p->builtinParamCnt;
}

void setParamCnt(Byte cnt) {
    if (curInfo_p->type == PROC_T)
        curInfo_p->paramCnt = cnt;
    else
        curInfo_p->builtinParamCnt = cnt;
}

Byte getDataType() {
    return (curInfo_p->type == PROC_T) ? curInfo_p->dataType : curInfo_p->builtinDataType;
}

void setDataType(Byte type) {
    if (curInfo_p->type == PROC_T)
        curInfo_p->dataType = type;
    else
        curInfo_p->builtinDataType = type;
}

void chain(char *filename) {
    word status;
    word entry /* = 0 ? */;
    file_t loadFile;

    Load(filename, 0, (word)(debugFlag ? 2 : 1), &entry, &status);
    if (status != 0) {
        initFile(&loadFile, "LOAD ", filename);
        fatalIO(&loadFile, status);
    }
}

void fatal(const char *str, Byte len) {
    printf("\r\n\nPL/M-80 FATAL ERROR --\r\n\n%*s\r\n\nCOMPILATION TERMINATED\r\n\n", len, str);

    exit(1);
}

void initFile(file_t *fp, const char *shortName, const char *fullname) {
    if (fp->aftn != 0)
        fclose(fp->aftn);

    fp->aftn = 0;
    FILL(0x16, fp->shortName, ' ');
    cpyTill(shortName, fp->shortName, 6, ' ');
    cpyTill(fullname, fp->fullName, 15, ' ');
}

void openFile(file_t *fp, Byte access) {
    word status;

    Open(&fp->aftn, fp->fullName, access, 0, &status);
    if (status != 0)
        fatalIO(fp, status);
}

void readFile(file_t *fp, void *buf, word len, word *pactual) {
    word status;

    Read(fp->aftn, buf, len, pactual, &status);
    if (status != 0)
        fatalIO(fp, status);
}

void writeFile(file_t *fp, void *buf, word count) {
    word status;
    Write(fp->aftn, buf, count, &status);
    if (status != 0)
        fatalIO(fp, status);
}

void tellFile(file_t *fp, loc_t *loc) {
    long pos;
    pos = ftell(fp->aftn);
    if (pos < 0)
        fatalIO(fp, errno);
    else {
        loc->block = (word)pos / 128;
        loc->Byte  = (word)pos % 128;
    }
}

void seekFile(file_t *fp, loc_t *loc) {
    if (fseek(fp->aftn, loc->block * 128 + loc->Byte, SEEK_SET) < 0)
        fatalIO(fp, errno);
}

void rewindFile(file_t *fp) {
    rewind(fp->aftn);
}

void backupPos(loc_t *lp, word cnt) { // backup the block/byte pos by cnt bytes
    loc_t loc;

    loc.block = cnt / 128;
    loc.Byte  = cnt % 128;
    lp->block -= loc.block;
    if (loc.Byte > lp->Byte) {
        lp->block--;
        lp->Byte += 128 - loc.Byte;
    } else
        lp->Byte -= loc.Byte;
}

void closeFile(file_t *fp) {
    word status;

    Close(fp->aftn, &status);
    fp->aftn = 0;
    if (status != 0)
        fatalIO(fp, status);
}

// ллллллллллллллл S U B    R O U T I N E ллллллллллллллллллллллллллллллллллллллл

// assignFileBuffer(file$p, buf$p, size$w, mode$b)
// set files buffer and buffer size
// read into buffer if mode$b ==    1

// void assignFileBuffer(file_t * fp, void *buf, word size, byte mode)
//{
//     fp->bufptr = (byte *)buf;
//     fp->bufSize = size;
//     if (mode == 1 && fp == &srcFile)
//		readFile(fp, buf, size, &fp->actual);
//     fp->curoff = 0;
// }

// void flushFile(file_t * fp)
//{
//	fflush(fp->aftn);
//	//if (fp->curoff != 0) {
//	//writeFile(fp, fp->bufptr, fp->curoff);
//	//fp->curoff = 0;
//	//}
// }

void ifwrite(file_t *fp, void *buf, word len) {
    if (fwrite(buf, 1, len, fp->aftn) != len)
        fatalIO(fp, errno);

    // word wcnt;
    // word room = fp->bufSize - fp->curoff;
    //
    // while (1) {
    //	wcnt = room >= len ? len : room;
    //	movemem(wcnt, buf, &fp->bufptr[fp->curoff]);
    //	fp->curoff += wcnt;
    //	len -= wcnt;
    //	if (len == 0)
    //		break;
    //	buf = (byte *)buf + wcnt;
    //	if (fp->curoff >= fp->bufSize)
    //		flushFile(fp);
    //	room = fp->bufSize;
    // }
}

void ifread(file_t *fp, void *buf, word cnt) {
    if (fread(buf, 1, cnt, fp->aftn) != cnt)
        fatalIO(fp, IOERR_254);

    // word inbuf = fp->actual - fp->curoff;

    // for (;;) {
    //	word avail = cnt > inbuf ? inbuf : cnt;
    //	if (fp->actual == 0)
    //		fatalIO(fp, IOERR_254);		// ATTEMPT TO READ PAST EOF
    //	movemem(avail, &fp->bufptr[fp->curoff], buf);
    //	fp->curoff += avail;
    //	if((cnt -= avail) == 0)
    //		break;
    //	buf = (byte *)buf + avail;
    //	if (fp->actual <= fp->curoff) {
    //		readFile(fp, fp->bufptr, fp->bufSize, &fp->actual);
    //		fp->curoff = 0;
    //	}
    //	inbuf = fp->actual;
    // }
}

void deleteFile(file_t *fp) {
    word status;
    Delete(fp->fullName, &status);
}

void SeekEnd(file_t *fp) {
    if (fseek(fp->aftn, 0, SEEK_END) < 0)
        fatalIO(fp, errno);
}

void setInfoFlag(Byte flagId) {
    setFlag(curInfo_p->flags, flagId);
}

void setBuiltinId(Byte val) {
    curInfo_p->builtinId = val;
}

void setCondFlag(Byte val) {
    curInfo_p->cflag = val;
}

void findInfo() {
    word i;
    if (curSymbol_p->infoChain == 0) {
        curInfo_p = 0;
        return;
    }
    for (i = scopeSP; i != 0; i--) {
        findScopedInfo(scopeChains[i]);
        if (curInfo_p != 0)
            return;
    }
}

void findScopedInfo(word val) {
    info_pt p;
    curInfo_p = curSymbol_p->infoChain;
    p         = 0;
    while (curInfo_p != 0) {
        if (val == getInfoScope()) {
            Byte infoType = getInfoType();
            if (infoType == LIT_T || infoType == MACRO_T || !testInfoFlag(F_MEMBER)) {
                if (p != 0) { // not at start of chain
                    info_pt symval = getNextInfo();
                    info_pt q      = curInfo_p;
                    curInfo_p      = p;
                    setNextInfo(symval);
                    curInfo_p = q;
                    setNextInfo(curSymbol_p->infoChain);
                    curSymbol_p->infoChain = curInfo_p;
                }
                return;
            }
        }
        p         = curInfo_p;
        curInfo_p = getNextInfo();
    }
}

word getInfoLen() {
    return curInfo_p->len;
}

word getAddr() {
    return curInfo_p->addr;
}

info_pt getNextInfo() {
    if (curInfo_p->nextInfoOffset)
        return off2Info(curInfo_p->nextInfoOffset);
    else
        return 0;
}

Byte getInfoType() {
    return curInfo_p->type;
}

symbol_pt getInfoSymbol() {
    return curInfo_p->symbolOffset == 0 ? symbol_pt(0)
                                        : symbol_pt(topSymbol - curInfo_p->symbolOffset);
}

word getInfoScope() {
    return curInfo_p->infoScope;
}

Byte testFlag(Byte *base, Byte flagId) {
    return (base[tblOffsets[flagId]] & tblBitFlags[flagId]) ? 0xff : 0;
}

void clrFlags(Byte *base) {
    Byte i;
    for (i = 0; i <= 2; i++)
        base[i] = 0;
}

void clrFlag(Byte *base, Byte flagId) {
    base[tblOffsets[flagId]] &= ~tblBitFlags[flagId];
}

void cpyFlags(Byte *base) {
    Byte i;
    for (i = 0; i <= 2; i++)
        curInfo_p->flags[i] = base[i];
}

Byte testInfoFlag(Byte flagId) {
    return testFlag(curInfo_p->flags, flagId);
}

void setDimension(word val) {
    curInfo_p->dimension = val;
}

info_pt getBase() {
    return curInfo_p->basedOffset == 0 ? 0 : (word)off2Info(curInfo_p->basedOffset);
}

void setBase(info_pt val) {
    curInfo_p->basedOffset = (val == 0) ? 0 : info2Off(val);
}

void setBasedOffset(word val) {
    curInfo_p->basedOffset = val;
}

word getBasedOffset() {
    return curInfo_p->basedOffset;
}

Byte getBuiltinId() {
    return curInfo_p->builtinId;
}

Byte getCondFlag() {
    return curInfo_p->cflag;
}

Byte getInfoExternId() {
    return curInfo_p->externId;
}

void setInfoExternId(Byte val) {
    curInfo_p->externId = val;
}

word getDimension() {
    return curInfo_p->dimension;
}

void findMemberInfo() {
    info_pt tmp = curInfo_p;
    curInfo_p   = curSymbol_p->infoChain; // get the symbol's info

    while (curInfo_p != 0) {
        if (testInfoFlag(F_MEMBER)) // ? structure member
            if (tmp == getOwningStructure())
                return;
        curInfo_p = getNextInfo();
    }
}

word getOwningStructure() {
    if (curInfo_p->type == STRUCT_T)
        return curInfo_p->parentOffset;
    else
        return (curInfo_p->parentOffset == 0) ? 0 : (word)off2Info(curInfo_p->parentOffset);
}

void setOwningStructure(info_pt val) {
    if (curInfo_p->type == STRUCT_T)
        curInfo_p->parentOffset = (word)val;
    else
        curInfo_p->parentOffset = (val == 0) ? 0 : info2Off(val);
}

Byte getProcId() {
    return curInfo_p->procID;
}

Byte getExternId() {
    return curInfo_p->externId;
}

word getParentOffsetOrSize() {
    return curInfo_p->parentOffset;
}

Byte getIntrNo() {
    return curInfo_p->intrNo;
}

void advNextInfo() {
    while (1) {
        curInfo_p += getInfoLen();
        if (curInfo_p >= topInfo) {
            curInfo_p = 0;
            return;
        }
        if (getInfoType() != TEMP_T)
            return;
    }
}