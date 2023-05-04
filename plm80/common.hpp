/****************************************************************************
 *  common.hpp: part of the C port of Intel's ISIS-II plm80             *
 *  The original ISIS-II application is Copyright Intel                     *
 *																			*
 *  Re-engineered to C++ by Mark Ogden <mark.pm.ogden@btinternet.com> 	    *
 *                                                                          *
 *  It is released for hobbyist use and for academic interest			    *
 *                                                                          *
 ****************************************************************************/
#pragma once
#include "plm.hpp"

// $Id: common.hpp,v 1.2 2004/11/30 23:48:08 Mark Exp $
extern const Byte tblBitFlags[];
extern const Byte tblOffsets[];


enum PASS { MAIN = 0, OV0, OV1, OV2 };

extern PASS pass;

extern void (*fatalError)(Byte errcode);

void alloc(word size1, word size2);
info_pt allocInfo(word size);
symbol_pt allocSymbolSpc(word spc);
void backupPos(loc_t * lp, word cnt);
void chain(char *filename);
void clrFlag(Byte *base, Byte flagId);
void clrFlags(Byte *base);
void closeFile(file_t * fp);
void cpyTill(const char *src, char *dest, word maxcnt, Byte endch);
void cpyFlags(Byte *base);
void assignFileBuffer(file_t * fp, void *buf, word size, Byte mode);
void createInfo(word val, Byte type);
void deleteFile(file_t *fp);
void fatal(const char *str, Byte len);
void fatalIO(file_t * fp, word errcode);
void findErrStr();
void findInfo();
void findMemberInfo();
void findScopedInfo(word val);
//void flushFile(file_t * fp);
void ifread(file_t *fp, void *buf, word cnt);
void ifwrite(file_t * fp, void *buf, word len);
info_pt getBase();
word getBasedOffset();
Byte getDataType();
word getDimension();
Byte getInfoExternId();
Byte getBuiltinId();
Byte getCondFlag();
word getInfoLen();
Byte getInfoType();
Byte getIntrNo();
symbol_pt getInfoSymbol();
word getInfoScope();
word getAddr();
info_pt getNextInfo();
word getOwningStructure();
Byte getParamCnt();

word hash(unsigned char *p);
void initFile(file_t * fp, const char *shortName, const char *fullname);
void lookup(unsigned char *s);
Byte num2Asc(word num, Byte width, Byte radix, char *buf);
void openFile(file_t * fp, Byte access);
void printStr(char *buf, Byte cnt);
void readFile(file_t * fp, void *buf, word len, word * pactual);
void rewindFile(file_t * fp);
void seekFile(file_t * fp, loc_t * loc);
void SeekEnd(file_t *fp);
void setBase(info_pt  val);
void setBasedOffset(word  val);
void setDATE(const char *buf, Byte len);
void setDataType(Byte type);
void setDimension(word val);
void setInfoExternId(Byte val);
void setFlag(Byte *base, Byte flagId);
void setInfoFlag(Byte flagId);
void setBuiltinId(Byte val);
void setCondFlag(Byte val);
void setInfoLen(Byte len);
void setInfoScope(word val);
void setInfoSymbol(symbol_pt  psym);
void setInfoType(Byte val);
void setAddr(word val);
void setNextInfo(info_pt  val);
void setOwningStructure(info_pt  val);
void setPAGELEN(word plen);
void setPAGEWIDTH(word pwidth);
void setParamCnt(Byte cnt);
void tellFile(file_t * fp, loc_t * loc);
Byte testFlag(Byte *base, Byte flagId);
Byte testInfoFlag(Byte flagId);
void writeFile(file_t * fp, void *  buf, word count);

Byte getProcId();
Byte getExternId();
word getParentOffsetOrSize();
void advNextInfo();
