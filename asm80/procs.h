/****************************************************************************
 *  procs.h: part of the C port of Intel's ISIS-II asm80             *
 *  The original ISIS-II application is Copyright Intel                     *
 *																			*
 *  Re-engineered to C by Mark Ogden <mark.pm.ogden@btinternet.com> 	    *
 *                                                                          *
 *  It is released for hobbyist use and for academic interest			    *
 *                                                                          *
 ****************************************************************************/


void AsmComplete(FILE *fp);
void BalanceError(void);
bool BlankAsmErrCode(void);
bool MPorNoErrCode(void);
bool ChkGenObj(void);
void ChkInvalidRegOperand(void);
void Close(word conn, wpointer statusP);
void CollectByte(byte rc);
void CommandError(void);
void DoPass(void);
void EmitXref(byte xrefMode, char const *name);
void ExpressionError(void);
void FileError(void);
void FinishAssembly(void);
void FinishLine(void);
void FinishPrint(void);
void FlushM(bool fin);
void GenAsxref(void);
void PrepSrcFile(char *srcName);
byte GetCh(void);
byte GetChClass(void);
byte GetCmdCh(void);
void GetTokenText(byte type);
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
void InsertByteInMacroTbl(byte c);
_Noreturn void IoError(char const *path, char const *msg);
bool IsComma(void);
bool IsEOL(void);
bool IsGT(void);
bool IsLT(void);
bool IsPhase1(void);
bool IsPhase2Print(void);
bool IsReg(byte type);
bool IsRParen(void);
bool IsSkipping(void);
bool IsTab(void);
bool IsWhite(void);
void LocationError(void);
byte Lookup(byte tableId);
pointer MemCk(void);
void MkCode(byte arg1b);
void MultipleDefError(void);
void Nest(byte arg1b);
void NestingError(void);
byte NonHiddenSymbol(void);
byte NxtTokI(void);
void OperandError(void);
FILE *Fopen(const char *pathP, char *access);
void OpenSrc(void);
void Outch(char c);
void OutStr(char const *s);
void Ovl11(void);
void Ovl8(void);
void ParseControlLines(void);
void PackToken(void);
void ParseControls(void);
void PhaseError(void);
void PopToken(void);
void PrintCmdLine(void);
void PrintLine(void);
void PushToken(byte type);
void ReadM(word blk);
void ReinitFixupRecs(void);
void ResetData(void);
void RuntimeError(byte errCode);
FILE *SafeOpen(char const *pathP, char *access);
void SetExpectOperands(void);
bool ShowLine(void);
void Skip2EOL(void);
void Skip2EOL(void);
void SkipWhite(void);
void SkipNextWhite(void);
void SourceError(byte errCh);
void StackError(void);
void Start(char *srcName);
bool StrUcEqu(char const *s, char const * t);
void ResultType(void);
void Sub546F(void);
void InsertMacroSym(word val, byte type);
void PrintSymbols(void);
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
void UnpackToken(wpointer src, byte *dst);
void UpdateSymbolEntry(word val, byte type);
void ValueError(void);
void WriteExtName(void);
void WriteM(pointer buf, int cnt);
void WriteModend(void);
void WriteModhdr(void);
void WriteRec(pointer recP);

#define move(cnt, src, dst)	memcpy(dst, src, cnt)

void DumpSymbols(byte tableId);
void DumpOpStack(void);
void DumpTokenStack(bool pop);

void DumpTokenStackItem(int i, bool pop);
void ShowYYType(void);

// helper functions for none le systems
word setWord(pointer buf, word val);
word getWord(pointer buf);
word addRecLen(pointer buf, word val);
word getRecLen(pointer buf);

void wrapUp(void);
int Printf(char const *fmt, ...);

void InsertXref(bool isDef, const char *name, word lineNum);
_Noreturn void FatalError(char const *fmt, ...);
char *basename(char *path);

tokensym_t *in_word_set(register const char *str);

char const *AllocStr(char const *s, bool isTemp);
void resetTmpStrings();
void *xmalloc(size_t n);
void *xrealloc(void *p, size_t n);
void Warn(char const *fmt, ...);
void PrintChar(char c);