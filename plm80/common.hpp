/****************************************************************************
 *  oldplm80: Old C++ port of PLM80 v4.0                                    *
 *  Copyright (C) 2020 Mark Ogden <mark.pm.ogden@btinternet.com>            *
 *                                                                          *
 *  This program is free software; you can redistribute it and/or           *
 *  modify it under the terms of the GNU General Public License             *
 *  as published by the Free Software Foundation; either version 2          *
 *  of the License, or (at your option) any later version.                  *
 *                                                                          *
 *  This program is distributed in the hope that it will be useful,         *
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of          *
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the           *
 *  GNU General Public License for more details.                            *
 *                                                                          *
 *  You should have received a copy of the GNU General Public License       *
 *  along with this program; if not, write to the Free Software             *
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,              *
 *  MA  02110-1301, USA.                                                    *
 *                                                                          *
 ****************************************************************************/


// $Id: common.hpp,v 1.2 2004/11/30 23:48:08 Mark Exp $
extern const byte tblBitFlags[];
extern const byte tblOffsets[];


enum PASS { MAIN = 0, OV0, OV1, OV2 };

extern PASS pass;

extern void (*fatalError)(byte errcode);

void alloc(word size1, word size2);
info_pt allocInfo(word size);
symbol_pt allocSymbolSpc(word spc);
void backupPos(loc_t * lp, word cnt);
void chain(char *filename);
void clrFlag(byte *base, byte flagId);
void clrFlags(byte *base);
void closeFile(file_t * fp);
void cpyTill(const char *src, char *dest, word maxcnt, byte endch);
void cpyFlags(byte *base);
void assignFileBuffer(file_t * fp, void *buf, word size, byte mode);
void createInfo(word val, byte type);
void deleteFile(file_t *fp);
void fatal(const char *str, byte len);
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
byte getDataType();
word getDimension();
byte getInfoExternId();
byte getBuiltinId();
byte getCondFlag();
word getInfoLen();
byte getInfoType();
byte getIntrNo();
symbol_pt getInfoSymbol();
word getInfoScope();
word getAddr();
info_pt getNextInfo();
word getOwningStructure();
byte getParamCnt();

word hash(unsigned char *p);
void initFile(file_t * fp, const char *shortName, const char *fullname);
void lookup(unsigned char *s);
byte num2Asc(word num, byte width, byte radix, char *buf);
void openFile(file_t * fp, byte access);
void printStr(char *buf, byte cnt);
void readFile(file_t * fp, void *buf, word len, word * pactual);
void rewindFile(file_t * fp);
void seekFile(file_t * fp, loc_t * loc);
void SeekEnd(file_t *fp);
void setBase(info_pt  val);
void setBasedOffset(word  val);
void setDATE(const char *buf, byte len);
void setDataType(byte type);
void setDimension(word val);
void setInfoExternId(byte val);
void setFlag(byte *base, byte flagId);
void setInfoFlag(byte flagId);
void setBuiltinId(byte val);
void setCondFlag(byte val);
void setInfoLen(byte len);
void setInfoScope(word val);
void setInfoSymbol(symbol_pt  psym);
void setInfoType(byte val);
void setAddr(word val);
void setNextInfo(info_pt  val);
void setOwningStructure(info_pt  val);
void setPAGELEN(word plen);
void setPAGEWIDTH(word pwidth);
void setParamCnt(byte cnt);
void tellFile(file_t * fp, loc_t * loc);
byte testFlag(byte *base, byte flagId);
byte testInfoFlag(byte flagId);
void writeFile(file_t * fp, void *  buf, word count);

byte getProcId();
byte getExternId();
word getParentOffsetOrSize();
void advNextInfo();
