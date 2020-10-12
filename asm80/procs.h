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


void AsmComplete();
void BalanceError();
bool BlankAsmErrCode();
bool MPorNoErrCode();
bool ChkGenObj();
void ChkInvalidRegOperand();
void ChkLF();
void Close(word conn, wpointer statusP);
void CloseF(word conn);
void CollectByte(byte rc);
void CommandError();
void Delete(pointer pathP, wpointer statusP);
void DoPass();
void EmitXref(byte xrefMode, pointer name);
void Error(word ErrorNum);
NORETURN(Exit());
void ExpressionError();
void FileError();
void FinishAssembly();
void FinishLine();
void FinishPrint();
void FlushM();
void Flushout();
void GenAsxref();
void GetAsmFile();
byte GetCh();
byte GetChClass();
byte GetCmdCh();
void GetId(byte type);
void GetNum();
word GetNumVal();
byte GetPrec(byte topOp);
byte GetSrcCh();
void GetStr();
byte HaveTokens();
void IllegalCharError();
void InitLine();
void InitRecTypes();
void InsertByteInMacroTbl(byte c);
void InsertCharInMacroTbl(byte c);
void IoErrChk();
void IoError(pointer path);
bool IsComma();
bool IsCR();
bool IsGT();
bool IsLT();
bool IsPhase1();
bool IsPhase2Print();
bool IsReg(byte type);
bool IsRParen();
bool IsSkipping();
bool IsTab();
bool IsWhite();
void Load(pointer pathP, word LoadOffset, word swt, word entryP, wpointer statusP);
void LocationError();
byte Lookup(byte tableId);
pointer MemCk();
void MkCode(byte arg1b);
void MultipleDefError();
void Nest(byte arg1b);
void NestingError();
byte Nibble2Ascii(byte n);
byte NonHiddenSymbol();
byte NxtTokI();
void OperandError();
void Open(wpointer connP, const pointer pathP, word access, word echo, wpointer statusP);
void OpenSrc();
void Outch(byte c);
void OutStrN(pointer s, byte n);
void Ovl11();
void Ovl8();
void ParseControlLines();
void PackToken();
void ParseControls();
void PhaseError();
pointer Physmem();
void PopToken();
void PrintCmdLine();
void PrintDecimal(word n);
void PrintLine();
void PushToken(byte type);
void Read(word conn, pointer buffP, word count, wpointer actualP, wpointer statusP);
void ReadM(word blk);
void ReinitFixupRecs();
void Rescan(word conn, wpointer statusP);
void ResetData();
void RuntimeError(byte errCode);
word SafeOpen(pointer pathP, word access);
void Seek(word conn, word mode, wpointer blockP, wpointer byteP, wpointer statusP);
void SetExpectOperands();
bool ShowLine();
void Skip2EOL();
void Skip2NextLine();
void SkipWhite();
void SkipWhite_2();
void SourceError(byte errCh);
void StackError();
void Start();
bool StrUcEqu(pointer s, pointer t);
void ResultType();
void Sub546F();
void InsertMacroSym(word val, byte type);
void Sub7041_8447();
void DoIrpX(byte mtype);
void initMacroParam();
void GetMacroToken();
void DoMacro();
void DoMacroBody();
void DoEndm();
void DoExitm();
void DoIterParam();
void DoRept();
void DoLocal();
void Sub78CE();
void SyntaxError();
void sysInit(int argc, char **argv);
bool TestBit(byte bitIdx, pointer bitVector);
void Tokenise();
void UndefinedSymbolError();
void UnNest(byte arg1b);
void UnpackToken(wpointer src, pointer dst);
void UpdateSymbolEntry(word val, byte type);
void ValueError();
void Write(word conn, const pointer buffP, word count, wpointer statusP);
void WriteExtName();
void WriteM();
void WriteModend();
void WriteModhdr();
void WriteRec(pointer recP);

#define move(cnt, src, dst)	memcpy(dst, src, cnt)

void DumpSymbols(byte tableId);
void DumpOpStack();
void DumpTokenStack(bool pop);

void DumpTokenStackItem(int i, bool pop);
void ShowYYType();
