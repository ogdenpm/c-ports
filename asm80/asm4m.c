/****************************************************************************
 *  asm4m.c: part of the C port of Intel's ISIS-II asm80                    *
 *  The original ISIS-II application is Copyright Intel                     *
 *                                                                          *
 *  Re-engineered to C by Mark Ogden <mark.pm.ogden@btinternet.com>         *
 *                                                                          *
 *  It is released for academic interest and personal use only              *
 ****************************************************************************/
#include "asm80.h"

bool StrUcEqu(char const *s, char const *t) {
    while (*s) {
        if (*s++ != (*t++ & 0x5f))
            return false;
    }
    return true;
}

bool IsSkipping(void) {
    return (mSpoolMode & 1) || skipIf[0];
}

/**
 * Validates and normalizes token sizing for the current source line before finalization.
 *
 * Behavior:
 * - Advances `spIdx` to the next token index (`NxtTokI()`).
 * - Raises a syntax error if an operand is still expected at end-of-line.
 * - When tokens remain, requires either:
 *   - the current token to be `O_DATA`, or
 *   - `lineNumberEmitted` to already be set;
 *   otherwise reports syntax error.
 * - Applies size normalization rules:
 *   - For `DB`/`DW` contexts (`inDB || inDW`):
 *     - if exactly one token was parsed, no blank assembler error code exists,
 *       and `token.size != 1`, force `token.size = 2`.
 *   - For non-`DB`/`DW` contexts:
 *     - if no blank assembler error code exists and tokens remain,
 *       clamp `tokenStk[spIdx].size` to a maximum of 3.
 *
 * Notes:
 * - Uses global parser/assembler state (`spIdx`, `tokenIdx`, `token`, `tokenStk`,
 *   `expectOperand`, `lineNumberEmitted`, `inDB`, `inDW`).
 * - This routine mutates token size metadata used later by line-finalization/output logic.
 */
void ValidateAndNormalizeLineTokens(void) {
    spIdx = NxtTokI();
    if (expectOperand)
        SyntaxError();
    if (HaveTokens())
        if (!(tokenStk[spIdx].type == O_DATA || lineNumberEmitted))
            SyntaxError();
    if (inDB || inDW) {
        if (tokenIdx == 1 && !BlankAsmErrCode() && token.size != 1)
            token.size = 2;
    } else if (!BlankAsmErrCode() && HaveTokens())
        if (tokenStk[spIdx].size > 3)
            tokenStk[spIdx].size = 3;
}

void FinishLine(void) {
    ValidateAndNormalizeLineTokens();
    if (IsPhase2Print()) { /* update the line number */
        lineNo++;

        if (ShowLine() || !BlankAsmErrCode())
            PrintLine();
    }

    if (skipRuntimeError) {
        exit(1);
    }

    if (!isControlLine) {
        int i     = tokenIdx < 2 || inDB || inDW ? 0 : 2;

        int w6BCE = IsSkipping() || !isInstr ? 0 : tokenStk[i].start + tokenStk[i].size;

        if (ChkGenObj())
            Ovl8();
        b6B2C                  = true;
        segLocation[activeSeg] = effectiveAddr = (word)(segLocation[activeSeg] + w6BCE);
    }

    if (controls.xref && haveUserSymbol && phase == 1)
        EmitXref(XREF_REF, name);

    FlushM(false);

    while (tokenIdx > 0) {
        PopToken();
    }

    InitLine();
    if (b6B33) {
        finished = true;
        if (IsPhase2Print() && controls.symbols)
            PrintSymbols();

        if (ChkGenObj())
            ReinitFixupRecs();
    }
}