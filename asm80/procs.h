/****************************************************************************
 *  asm80: C port of ASM80 v4.1                                             *
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


void AsmComplete(void);
void BalanceError(void);
bool BlankAsmErrCode(void);
bool MPorNoErrCode(void);
bool ChkGenObj(void);
void ChkInvalidRegOperand(void);
void ChkLF(void);
void Close(word conn, wpointer statusP);
void CloseF(word conn);
void CollectByte(byte rc);
void CommandError(void);
void Delete(char const *pathP, wpointer statusP);
void DoPass(void);
void EmitXref(byte xrefMode, char const *name);
void Error(word ErrorNum);
NORETURN(Exit(int retCode));
void ExpressionError(void);
void FileError(void);
void FinishAssembly(void);
void FinishLine(void);
void FinishPrint(void);
void FlushM(void);
void Flushout(void);
void GenAsxref(void);
void GetAsmFile(void);
byte GetCh(void);
byte GetChClass(void);
byte GetCmdCh(void);
void GetId(byte type);
void GetNum(void);
word GetNumVal(void);
byte GetPrec(byte topOp);
byte GetSrcCh(void);
void GetStr(void);
byte HaveTokens(void);
void IllegalCharError(void);
void InitLine(void);
void InitRecTypes(void);
void InsertByteInMacroTbl(byte c);
void InsertCharInMacroTbl(byte c);
void IoErrChk(void);
void IoError(char const *path);
bool IsComma(void);
bool IsCR(void);
bool IsGT(void);
bool IsLT(void);
bool IsPhase1(void);
bool IsPhase2Print(void);
bool IsReg(byte type);
bool IsRParen(void);
bool IsSkipping(void);
bool IsTab(void);
bool IsWhite(void);
void Load(char const *pathP, word LoadOffset, word swt, word entryP, wpointer statusP);
void LocationError(void);
byte Lookup(byte tableId);
pointer MemCk(void);
void MkCode(byte arg1b);
void MultipleDefError(void);
void Nest(byte arg1b);
void NestingError(void);
byte Nibble2Ascii(byte n);
byte NonHiddenSymbol(void);
byte NxtTokI(void);
void OperandError(void);
void Open(wpointer connP, const char *pathP, word access, word echo, wpointer statusP);
void OpenSrc(void);
void Outch(char c);
void OutStrN(char const *s, byte n);
void Ovl11(void);
void Ovl8(void);
void ParseControlLines(void);
void PackToken(void);
void ParseControls(void);
void PhaseError(void);
pointer Physmem(void);
void PopToken(void);
void PrintCmdLine(void);
void PrintDecimal(word n);
void PrintLine(void);
void PushToken(byte type);
void Read(word conn, char *buffP, word count, wpointer actualP, wpointer statusP);
void ReadM(word blk);
void ReinitFixupRecs(void);
void Rescan(word conn, wpointer statusP);
void ResetData(void);
void RuntimeError(byte errCode);
word SafeOpen(char const *pathP, word access);
void Seek(word conn, word mode, wpointer blockP, wpointer byteP, wpointer statusP);
void SetExpectOperands(void);
bool ShowLine(void);
void Skip2EOL(void);
void Skip2NextLine(void);
void SkipWhite(void);
void SkipWhite_2(void);
void SourceError(byte errCh);
void StackError(void);
void Start(void);
bool StrUcEqu(char const *s, char const * t);
void ResultType(void);
void Sub546F(void);
void InsertMacroSym(word val, byte type);
void Sub7041_8447(void);
void DoIrpX(byte mtype);
void initMacroParam(void);
void GetMacroToken(void);
void DoMacro(void);
void DoMacroBody(void);
void DoEndm(void);
void DoExitm(void);
void DoIterParam(void);
void DoRept(void);
void DoLocal(void);
void Sub78CE(void);
void SyntaxError(void);
void sysInit(int argc, char **argv);
bool TestBit(byte bitIdx, pointer bitVector);
void Tokenise(void);
void UndefinedSymbolError(void);
void UnNest(byte arg1b);
void UnpackToken(wpointer src, char *dst);
void UpdateSymbolEntry(word val, byte type);
void ValueError(void);
void Write(word conn, const void *buffP, word count, wpointer statusP);
void WriteExtName(void);
void WriteM(void);
void WriteModend(void);
void WriteModhdr(void);
void WriteRec(pointer recP);

#define move(cnt, src, dst)	memcpy(dst, src, cnt)

void DumpSymbols(byte tableId);
void DumpOpStack(void);
void DumpTokenStack(bool pop);

void DumpTokenStackItem(int i, bool pop);
void ShowYYType(void);
