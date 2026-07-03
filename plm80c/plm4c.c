/****************************************************************************
 *  plm4c.c: part of the C port of Intel's ISIS-II plm80                    *
 *  The original ISIS-II application is Copyright Intel                     *
 *                                                                          *
 *  Re-engineered to C by Mark Ogden <mark.pm.ogden@btinternet.com>         *
 *                                                                          *
 *  It is released for academic interest and personal use only              *
 ****************************************************************************/
#include "os.h"
#include "plm.h"
#include <stdio.h>

/**
 * @file plm4c.c
 * @brief Pass-4 fragment decoder and template-driven instruction emitter.
 *
 * This module converts fragment/control records (`cfCode`) into:
 * - object code bytes (`opBytes` -> records),
 * - relocation/fixup records,
 * - listing text (`line`).
 *
 * It interprets compact tables from `plm4a.c`:
 * - `instructionSeq` (fragment -> [start,len] in `codeSeq`)
 * - `codeSeq` (indices into `codeTable`)
 * - `codeTable` (tokenized instruction templates)
 *
 * Main entry:
 * - `EmitCurrentFragment()`
 */

/* ------------------------------------------------------------------------- */
/* Local tables and p-string helpers                                         */
/* ------------------------------------------------------------------------- */

/**
 * Maps parser condition selector (0-5) to opcode condition-bit pattern.
 * Used to merge condition bits into conditional jump/call/return opcodes.
 * Pattern: NC=0x10, C=0x18, Z=8, NZ=0, C=0x18, NC=0x10
 */
static uint8_t conditionCodeBits[]  = { 0x10, 0x18, 8, 0, 0x18, 0x10 };


/**
 * Copy a Pascal-style string (`pstr_t`) to another.
 * Copies both length byte and string data in one operation.
 * 
 * @param dst Destination pstr_t
 * @param src Source pstr_t
 * @return dst pointer for chaining
 */
pstr_t *pstrcpy(pstr_t *dst, pstr_t const *src) {
    /* Copy length byte + string content */
    memcpy(dst, src, src->len + 1);
    return dst;
}

/**
 * Copy a C string into a Pascal-style string (`pstr_t`).
 * Converts null-terminated C string to length-prefixed format.
 * 
 * @param dst Destination pstr_t
 * @param src Source C string
 * @return dst pointer for chaining
 */
pstr_t *pcstrcpy(pstr_t *dst, char const *src) {
    /* Calculate and store length */
    dst->len = (uint8_t)strlen(src);
    /* Copy string data (without null terminator) */
    memcpy(dst->str, src, dst->len);
    return dst;
}

/**
 * Append one Pascal-style string to another.
 * Safe for NULL or zero-length sources.
 * 
 * @param dst Destination pstr_t (modified in place)
 * @param src Source pstr_t to append
 * @return dst pointer for chaining
 */
pstr_t *pstrcat(pstr_t *dst, pstr_t const *src) {
    if (src && src->len) {
        /* Append at end of existing content */
        memcpy(&dst->str[dst->len], src->str, src->len);
        /* Update total length */
        dst->len += src->len;
    }
    return dst;
}

/**
 * Append a C string to a Pascal-style string.
 * Converts and appends null-terminated string.
 * 
 * @param dst Destination pstr_t (modified in place)
 * @param src C string to append
 * @return dst pointer for chaining
 */
pstr_t *pcstrcat(pstr_t *dst, char const *src) {
    if (src)
        while (*src) {
             dst->str[dst->len++] = *src++;
        }
    return dst;
}

/* ------------------------------------------------------------------------- */
/* Template token execution helpers                                          */
/* ------------------------------------------------------------------------- */

/**
 * Apply encoded operand bits into opcode byte and append operand text.
 *
 * Template token format (high bit set indicates operand substitution):
 * - Bits 7-6: always 11 (marks as substitution token)
 * - Bits 5-4: encoding mode (0=regpair, 1=shifting mode, 2-3=direct)
 * - Bits 3-0: operand source selector
 *   - 0-3: use decoded operand from wValAry[]/sValAry[]
 *   - 4+: use architectural register from regNo[]/regPairNo[] tables
 *
 * The encoding mode determines how the selected value is positioned
 * within the opcode byte.
 * 
 * @param token Template token byte (0xC0-0xFF range)
 */
static void ApplyOperandToOpcode(uint8_t token) {
    uint8_t encodedOperandBits;
    pstr_t *operandText;

    /* Extract encoding strategy from token bits 5-4 */
    uint8_t operandEncodeMode  = (token >> 4) & 3;
    /* Extract operand selector from token bits 3-0 */
    uint8_t operandIdx = token & 0xf;
    
    /* Operand slots 0-3: use previously decoded operand values */
    if (operandIdx < 4) {
        encodedOperandBits = (uint8_t)wValAry[operandIdx];
        operandText = sValAry[operandIdx];
    } 
    /* Operand slots 4+: use architectural register tables */
    else if (operandEncodeMode == 0) {
        /* Mode 0: 16-bit register pair (BC, DE, HL, SP) */
        encodedOperandBits = regPairNo[operandIdx - 4];
        operandText = (pstr_t *)regPairStr[operandIdx - 4];
    } else {
        /* Mode 1+: 8-bit register (A, B, C, D, E, H, L, M) */
        encodedOperandBits = regNo[operandIdx - 4];
        operandText = (pstr_t *)regStr[operandIdx - 4];
    }

    /* Apply positional encoding based on mode */
    if (operandEncodeMode == 0)
        /* Mode 0: swap nibbles for register-pair encoding */
        encodedOperandBits = (encodedOperandBits << 4) | (encodedOperandBits >> 4);
    else if (operandEncodeMode == 1)
        /* Mode 1: shift left 3 bits, preserving overflow into bit 5 */
        encodedOperandBits = (encodedOperandBits << 3) | (encodedOperandBits >> 5);
    /* Mode 2-3: use value directly (no shift/swap) */
    
    /* Merge encoded bits into base opcode */
    opBytes[0] |= encodedOperandBits;
    /* Append textual operand to listing line */
    pstrcat((pstr_t *)&line, operandText);
}

/**
 * Append 16-bit operand to object stream and listing text.
 * Sets fixup class for linker to patch address at link time.
 * 
 * @param codeParam Operand slot index (0-3)
 */
static void AppendWordOperand(uint8_t codeParam) {

    fixupSelector = fixupType;
    putWord(&opBytes[opByteCnt], wValAry[codeParam]);
    opByteCnt += 2;
    pstrcat((pstr_t *)&line, sValAry[codeParam]);
}

/**
 * Append helper call target (`@Pnnnn`) and object address/fixup.
 * Helpers are runtime library routines (multiply, divide, etc).
 * 
 * In standalone mode: use direct address with self-fixup.
 * In module mode: use external fixup to link against runtime library.
 * 
 * @param codeParam 1=delay helper, 0=use helper tables
 */
static void AppendHelperCallOperand(uint8_t codeParam) {
    uint16_t helperId;

    /* codeParam=1 is special case for TIME/delay function */
    if (codeParam == 1)
        helperId = 105; // delay
    else {
        /* Look up helper ID using two-level table:
         * - helperGroupIndex selects primary group (from condition in fragment)
         * - helperVariantIndex selects variant within group (from node type) */
        uint8_t grp = helperGroup[0][helperGroupIndex];
        helperId = helperMap[grp][helperVariantIndex];
    }
    
    /* Format helper label for listing: @P0000 through @P9999 */
    line.len += (uint8_t)sprintf(&line.str[line.len], "@P%04d", helperId);
    
    if (standAlone) {
        /* Standalone mode: helper addresses are known at compile time */
        putWord(&opBytes[opByteCnt], helperAddr[helperId]);
        fixupSelector = 1;  // self fixup (relocate within this module)
    } else {
        /* Module mode: helpers are external references */
        putWord(&opBytes[opByteCnt], 0);  // placeholder
        fixupSelector = 5;  // external fixup (link to library)
        curExtId      = (uint8_t)helperAddr[helperId];  // external symbol ID
    }
    opByteCnt += 2;
}

/**
 * Append small immediate constant (inline byte from template stream).
 * 
 * Template provides the byte value directly after the directive token.
 * For certain opcodes, extends to 16-bit zero-extended immediate.
 * 
 * @param num Immediate byte value from template stream
 * @param codeParam Non-zero to extend to 16-bit word
 */
static void AppendSmallImmediate(uint8_t num, uint8_t codeParam) {
    /* Emit low byte */
    opBytes[opByteCnt++] = num;
    /* Optional high byte (zero-extend) */
    if (codeParam)
        opBytes[opByteCnt++] = 0;
    /* Format as decimal in listing */
    line.len += (uint8_t)sprintf(&line.str[line.len], "%d", num);
}


/**
 * Append stack-origin symbolic operand and data-fixup placeholder.
 * 
 * Used for STACK pseudovariable - references top of allocated stack.
 * Requires data-segment fixup at link time.
 */
static void AppendStackOriginOperand() {
    fixupSelector = 3;  // data fixup
    /* Reserve 16-bit address field (to be patched by linker) */
    putWord(&opBytes[opByteCnt], 0);
    opByteCnt += 2;
    /* Append symbolic name to listing */
    pstrcat((pstr_t *)&line, (pstr_t *)stackOrigin);
}

/**
 * Append 8-bit immediate from operand slot.
 * 
 * If operand was originally >255, reformats as truncated hex for listing.
 * (Indicates programmer error or intentional truncation)
 * 
 * @param operandIdx Operand slot (0-3)
 */
static void AppendByteOperand(uint8_t operandIdx) {
    /* Emit low byte only */
    opBytes[opByteCnt++] = (uint8_t)wValAry[operandIdx];
    
    /* Check for truncation - reformat listing string if needed */
    if (wValAry[operandIdx] > 255)
        pcstrcpy(sValAry[operandIdx], hexfmt(false, Low(wValAry[operandIdx])));
    
    /* Append (possibly reformatted) text to listing */
    pstrcat((pstr_t *)&line, sValAry[operandIdx]);
}

/**
 * Append PC-relative target as absolute address + textual `$+/-disp`.
 * 
 * Template provides signed 8-bit displacement. This function:
 * 1. Sign-extends to 16-bit
 * 2. Adds to current PC (baseAddr)
 * 3. Emits absolute address (linker will relocate)
 * 4. Formats as "$±offset" for listing readability
 * 
 * @param offset Signed 8-bit displacement from template stream
 */
static void AppendPcRelativeOperand(uint16_t offset) {
    fixupSelector = 1;  // self fixup (relocate with code segment)
    
    /* Sign-extend 8-bit to 16-bit if negative */
    if (offset > 127)
        offset = offset | 0xff00;  // set high byte to 0xFF
    
    /* Compute absolute target address */
    putWord(&opBytes[opByteCnt], baseAddr + offset);
    opByteCnt += 2;
    
    /* Format as "$" followed by signed displacement */
    line.str[line.len++] = '$';
    AddWrdDisp((pstr_t *)&line, offset);  // "+123" or "-123"
}

/**
 * Apply condition-code bits and append mnemonic text (NC/C/Z/NZ...).
 * 
 * Merges condition selector (from fragment operand) into opcode byte.
 * Used for conditional jumps, calls, and returns.
 */
static void AppendConditionCode() {
    /* Textual mnemonics for six condition codes */
    static char *conditionCodeMnemonics[] = { "NC", "C", "Z", "NZ", "C", "NC" };

    /* Merge condition bits into base opcode */
    opBytes[0] |= conditionCodeBits[conditionSelector];
    /* Append mnemonic to listing */
    pcstrcat((pstr_t *)&line, conditionCodeMnemonics[conditionSelector]);
}

/**
 * Append helper label definition (`@Pnnnn:`) in listing text.
 * 
 * Emits label for helper entry point in listing. Used when generating
 * inline helper code sequences. Post-increments global helperId counter.
 */
static void AppendHelperLabel() {
    line.len += (uint8_t)sprintf(&line.str[line.len], "@P%04d:", helperId++);
}

/**
 * Select helper opcode variant:
 * - Map codeParam -> helper group table
 * - Look up mnemonic/opcode index
 * - Load base opcode byte
 * - Append mnemonic text
 * 
 * Used for operations with multiple implementation variants
 * (e.g., different arithmetic modes, shift directions)
 * 
 * @param codeParam Variant selector (0-3)
 */
static void SelectHelperOpcodeVariant(uint8_t codeParam) {
    /* Map variant selector through helper group table */
    uint8_t index = helperGroup[codeParam][helperGroupIndex];
    
    /* Load opcode byte from indirection table */
    opBytes[0] = helperOpcodeBaseByteMap[index];
    opByteCnt  = 1;
    
    /* Append mnemonic string to listing */
    pcstrcat((pstr_t *)&line, opcodes[index]);
}

/* ------------------------------------------------------------------------- */
/* Instruction template interpreter                                          */
/* ------------------------------------------------------------------------- */

/**
 * Build one instruction from template program.
 * 
 * Walks tokenized template stream (`codeTable`) to produce:
 * - Object bytes in `opBytes[0..opByteCnt-1]`
 * - Listing text in `line`
 * - Fixup classification in `fixupSelector`
 * 
 * Template token classes:
 * - `0x00..0x7F`: Literal listing character (copy to line buffer)
 * - `0x80..0xBF`: Directive token (low nibble = function, bits 4-5 = param)
 * - `0xC0..0xFF`: Operand substitution token (merge bits into opcode)
 * 
 * Directive functions (low nibble):
 * 0 = end of template
 * 1 = append operand text
 * 2 = append word operand + fixup
 * 3 = append helper call
 * 4 = append small immediate (inline byte follows)
 * 5 = append stack origin
 * 6 = append byte operand
 * 7 = append PC-relative target (inline byte follows)
 * 8 = append condition code
 * 9 = emit helper label
 * 10 = append comment string
 * 11 = select helper opcode variant
 * 
 * @param codeIdx Index into `codeSeq` array (start of template)
 */
static void BuildInstructionFromTemplate(uint16_t codeIdx) {
    /* Initialize template pointer from codeSeq indirection table */
    uint8_t *templatePtr = &codeTable[codeSeq[codeIdx]];
    
    /* First byte of template is either:
     * - 0 = phantom instruction (no object code, listing only)
     * - opcode = base instruction byte (operands will merge into this) */
    if (*templatePtr == 0)
        opByteCnt = 0;  // phantom instruction
    else {
        opBytes[0] = *templatePtr;  // base opcode
        opByteCnt  = 1;
    }

    /* Reset output accumulators */
    fixupSelector = 0;   // assume no fixup needed
    line.len      = 0;   // listing starts empty

    /* Walk template stream until end token (0x80) */
    while (1) {
        uint8_t templateToken = *++templatePtr;
        
        /* Three token ranges:
         * 0x00-0x7F: literal listing char
         * 0x80-0xBF: directive (low nibble = function code)
         * 0xC0-0xFF: operand substitution (merge into opcode) */
        
        if (templateToken < 0x80) {
            /* Literal char: copy to listing buffer */
            line.str[line.len++] = templateToken;
        } 
        else if (templateToken >= 0xc0) {
            /* Operand substitution: modify opcode byte */
            ApplyOperandToOpcode(templateToken);
        }
        else {
            /* Directive token: bits 5-4 = parameter, bits 3-0 = function */
            uint8_t codeParam = (templateToken >> 4) & 3;
            
            switch (templateToken & 0xf) {
            case 0:
                /* End of template */
                return;
            case 1:
                /* Append operand text (no object code) */
                pstrcat((pstr_t *)&line, sValAry[codeParam]);
                break;
            case 2:
                /* Append 16-bit operand + fixup */
                AppendWordOperand(codeParam);
                break;
            case 3:
                /* Append helper procedure call */
                AppendHelperCallOperand(codeParam);
                break;
            case 4:
                /* Append small immediate (value follows in stream) */
                AppendSmallImmediate(*++templatePtr, codeParam);
                break;
            case 5:
                /* Append stack-origin reference */
                AppendStackOriginOperand();
                break;
            case 6:
                /* Append byte operand */
                AppendByteOperand(codeParam);
                break;
            case 7:
                /* Append PC-relative operand (offset follows in stream) */
                AppendPcRelativeOperand(*++templatePtr);
                break;
            case 8:
                /* Append condition-code modifier */
                AppendConditionCode();
                break;
            case 9:
                /* Emit helper label definition */
                AppendHelperLabel();
                break;
            case 10:
                /* Append comment annotation */
                pstrcat((pstr_t *)&line, (pstr_t *)commentPStr);
                break;
            case 11:
                /* Select helper opcode variant */
                SelectHelperOpcodeVariant(codeParam);
                break;
            }
        }
    }
}

/**
 * Write current instruction bytes to EXEC record and emit fixup record(s).
 * 
 * Object file format uses multiple record types:
 * - EXEC: executable code bytes
 * - Self-fixup: relocate within this module
 * - Code-fixup: relocate to relocatable code segment
 * - Data-fixup: relocate to data segment
 * - Memory-fixup: relocate to memory segment
 * - External-fixup: link to external symbol
 * 
 * Each record has size limits; flushes to disk when approaching capacity.
 * Fixup records store address of word to be patched (not the value itself).
 * 
 * Fixup address calculation: baseAddr + opByteCnt - 2
 * (Points to last word of instruction, which is typically the operand)
 */
static void WriteInstructionObjectRecord() {
    uint16_t fixupAddress;

    /* Skip if no object code or object output disabled */
    if (opByteCnt == 0 || !OBJECT)
        return;
    
    /* Check if EXEC record is near capacity (1020 byte limit) */
    if (getWord(&recExec[REC_LEN]) + opByteCnt >= 1018)
        FlushRecs();  // write all records to disk
    
    /* Calculate fixup address (points to operand word in instruction) */
    fixupAddress = baseAddr + opByteCnt - 2;
    
    /* Emit appropriate fixup record based on operand class */
    switch (fixupSelector) {
    case 0:
        /* No fixup needed (absolute address or no address operand) */
        break;
        
    case 1:
        /* Self-fixup: relocate within this program module */
        if (getWord(&recSelfFixup[REC_LEN]) + 2 >= 1018)
            FlushRecs();
        RecAddWord(recSelfFixup, 1, fixupAddress);
        break;
        
    case 2:
        /* Code-fixup: relocate to relocatable code segment */
        if (getWord(&recCodeFixup[REC_LEN]) + 2 >= 1017)
            FlushRecs();
        RecAddWord(recCodeFixup, 2, fixupAddress);
        break;
        
    case 3:
        /* Data-fixup: relocate to data segment */
        if (getWord(&recDataFixup[REC_LEN]) + 2 >= 99)
            FlushRecs();
        RecAddWord(recDataFixup, 2, fixupAddress);
        break;
        
    case 4:
        /* Memory-fixup: relocate to memory segment */
        if (getWord(&recMemoryFixup[REC_LEN]) + 2 >= 99)
            FlushRecs();
        RecAddWord(recMemoryFixup, 2, fixupAddress);
        break;
        
    case 5:
        /* External-fixup: link to external symbol (runtime library, etc) */
        if (getWord(&recExtFixup[REC_LEN]) + 4 >= 1018)
            FlushRecs();
        /* External fixup needs both symbol ID and address */
        RecAddWord(recExtFixup, 1, curExtId);      // which external symbol
        RecAddWord(recExtFixup, 1, fixupAddress);  // where to patch
        break;
    }
    
    /* Append instruction bytes to EXEC record */
    for (uint8_t i = 0; i < opByteCnt; i++) {
        RecAddByte(recExec, 3, opBytes[i]);
    }
} /* Sub_654F() */

/**
 * Emit a sequence of template instructions.
 * 
 * Main code generation loop:
 * 1. Build instruction from template
 * 2. Write object code + fixup records
 * 3. Write listing line
 * 4. Advance code address
 * 5. Check for address overflow
 * 
 * Called with instruction sequence descriptor from `instructionSeq` table.
 * Descriptor format: low 12 bits = start index, high 4 bits = count.
 * 
 * @param codeIdx Starting index in `codeSeq` table
 * @param len Number of instructions to emit
 */
void EmitCodeSeq(uint16_t codeIdx, uint8_t len) {
    /* Process each instruction in sequence */
    while (len-- > 0) {
        /* Build instruction from template program */
        BuildInstructionFromTemplate(codeIdx++);
        
        /* Write to object file if object output enabled */
        WriteInstructionObjectRecord();
        
        /* Write to listing file (disassembly + source correlation) */
        ListInstruction();
        
        /* Check for address space overflow (>64KB wrap) */
        if (baseAddr + opByteCnt > 0xffff) {
            /* Report error: program too large for 8080 address space */
            errData.stmt = errData.info = 0;
            errData.num                 = 0xCE;  // CE: code segment overflow
            EmitError();
        }
        
        /* Advance code location counter for next instruction */
        baseAddr += opByteCnt;
    }
}

/* ------------------------------------------------------------------------- */
/* Fragment alias and operand payload decoding                               */
/* ------------------------------------------------------------------------- */

static uint8_t operandEncodingClass;

/**
 * Remap extended fragment aliases to canonical fragment codes.
 * 
 * Some fragment codes (>= CF_174) are aliases that map to:
 * - A canonical fragment code (via cfCodeAliasMap)
 * - A helper variant index (via fragmentHelperIndex + helperIndexMap)
 * 
 * This allows code generation tables to be shared across similar operations
 * while still selecting appropriate helper routines.
 */
static void ResolveFragmentAlias(void) {
    if (cfCode >= CF_174) {
        /* Two-level indirection to resolve helper variant */
        uint8_t index = cfCode - CF_174;
        
        /* Map to canonical fragment code */
        cfCode = cfCodeAliasMap[index];
        
        /* Resolve helper variant index */
        index              = fragmentHelperIndex[index];
        helperVariantIndex = helperIndexMap[index];
    }
}

/* ------------------------------------------------------------------------- */
/* Operand value readers from pass stream                                    */
/* ------------------------------------------------------------------------- */

/**
 * Read 8-bit immediate operand from stream.
 * Formats as hex for listing display.
 * 
 * @param operandIdx Destination slot (0-3) in wValAry/sValAry
 */
static void ReadByteValue(uint8_t operandIdx) {
    /* Read one byte from intermediate file */
    wValAry[operandIdx] = Rd1Byte();
    /* Format as hex string for listing */
    sValAry[operandIdx] = pcstrcpy((pstr_t *)valPStr, hexfmt(false, wValAry[operandIdx]));
}

/**
 * Read 16-bit immediate operand from stream.
 * Formats as hex for listing display.
 * 
 * @param operandIdx Destination slot (0-3)
 */
static void ReadWordValue(uint8_t operandIdx) {
    /* Read two bytes (little-endian) from intermediate file */
    wValAry[operandIdx] = Rd1Word();
    /* Format as hex string for listing */
    sValAry[operandIdx] = pcstrcpy((pstr_t *)valPStr, hexfmt(false, wValAry[operandIdx]));
}

/**
 * Read local label reference from stream.
 * 
 * Local labels are compiler-generated (loop labels, if-then targets, etc).
 * Formatted as "@123" in listing.
 * Requires self-fixup for relocation.
 * 
 * @param operandIdx Destination slot (0-3)
 */
static void ReadLocalLabelValue(uint8_t operandIdx) {
    /* Read label number from stream */
    locLabelNum   = Rd1Word();
    /* Look up resolved address from label table */
    wValAry[operandIdx] = localLabels[locLabelNum];
    /* Format as "@nnn" for listing */
    locPStr[0]    = sprintf(locPStr + 1, "@%d", locLabelNum);
    sValAry[operandIdx] = (pstr_t *)locPStr;
    /* Local labels require relocation (self-fixup) */
    fixupType           = 1;
}

/**
 * Read value with comment tag annotation.
 * 
 * Used for special operands that need both:
 * - A numeric value (for object code)
 * - A symbolic annotation (for listing comment)
 * 
 * Stream format: tag-byte, value-word
 * 
 * @param operandIdx Destination slot (0-3)
 */
static void ReadValueWithCommentTag(uint8_t operandIdx) {
    /* Read tag byte (interpreted by caller) */
    uint8_t i  = Rd1Byte();
    /* Read actual value word */
    uint16_t p = Rd1Word();
    
    /* Build comment string with tag */
    commentPStr[0] = sprintf(commentPStr + 1, "; %d", i);
    
    /* Store numeric value */
    wValAry[operandIdx]  = p;
    /* Format value as decimal for listing */
    valPStr[0]     = sprintf(valPStr + 1, "%d", p);
    sValAry[operandIdx]  = (pstr_t *)valPStr;
}

/**
 * Read symbol-based operand with displacement.
 * 
 * Decodes symbol reference from stream and classifies for fixup:
 * 
 * Symbol classifications:
 * - External: require external-fixup (link to library)
 * - Proc/Label/Data: require self-fixup (relocate within module)
 * - Memory: require memory-fixup (relocate to memory segment)
 * - Code: require code-fixup (relocate to code segment)
 * - Based/Member/Absolute: no fixup (address computed or absolute)
 * 
 * If symbol has no name, formats as "$" (anonymous) and displacement
 * is relative to current instruction.
 * 
 * @param disp Additional offset from symbol base
 * @param operandIdx Destination slot (0-3)
 */
static void ReadSymbolValue(uint16_t disp, uint8_t operandIdx) {
    /* Read symbol table index from stream */
    info = FromIdx(Rd1Word());
    
    /* Compute effective address: symbol base + displacement */
    wValAry[operandIdx] = info->addr + disp;
    
    /* Get symbol name (may be NULL for anonymous/temp symbols) */
    curSym = info->sym;
    
    if (curSym) {
        /* Named symbol: use source name in listing */
        pstrcpy((pstr_t *)valPStr, curSym->name);
    } else {
        /* Anonymous symbol: use "$" prefix */
        valPStr[0] = 1;         // length = 1
        valPStr[1] = '$';
        /* Recompute displacement relative to current instruction */
        disp       = wValAry[operandIdx] - baseAddr;
    }
    
    sValAry[operandIdx] = (pstr_t *)valPStr;
    /* Append displacement if non-zero: "SYMBOL+10" or "SYMBOL-5" */
    AddWrdDisp(sValAry[operandIdx], disp);
    
    /* Classify symbol for fixup record generation */
    if ((info->flag & F_EXTERNAL)) {
        /* External symbol: need external-fixup to link library */
        fixupType = 5;
        curExtId  = info->extId;  // external symbol index
    } 
    else if (info->type == PROC_T || info->type == LABEL_T) {
        /* Procedure or label: self-fixup (code segment) */
        fixupType = 1;
    }
    else if ((info->flag & F_MEMBER) || (info->flag & F_BASED)) {
        /* Structure member or based variable: no fixup needed */
        /* (Address computed at runtime or via base pointer) */
        /* fixupType remains 0 */
    }
    else if ((info->flag & F_DATA)) {
        /* Data symbol: self-fixup (data segment) */
        fixupType = 1;
    }
    else if ((info->flag & F_MEMORY)) {
        /* Memory-mapped variable: memory-fixup */
        fixupType = 4;
    }
    else if (!(info->flag & F_ABSOLUTE)) {
        /* Relocatable code: code-fixup */
        fixupType = 2;
    }
    /* else: absolute symbol, no fixup needed (fixupType = 0) */
}

/**
 * Read absolute value with symbol name as comment.
 * 
 * Used when operand value is absolute (no fixup) but we want to
 * annotate listing with symbolic information for readability.
 * 
 * Stream format: displacement, symbol-index, absolute-value
 * 
 * @param operandIdx Destination slot (0-3)
 */
static void ReadAbsoluteWithSymbolComment(uint8_t operandIdx) {
    /* Read components from stream */
    uint16_t disp = Rd1Word();         // displacement (for comment)
    info          = FromIdx(Rd1Word()); // symbol (for comment)
    wValAry[operandIdx] = Rd1Word();   // actual value (used in object code)
    
    /* Format absolute value as hex for listing */
    sValAry[operandIdx] = pcstrcpy((pstr_t *)valPStr, hexfmt(false, wValAry[operandIdx]));
    
    /* Build comment annotation with symbol name */
    curSym         = info->sym;
    commentPStr[0] = sprintf(commentPStr + 1, "; %s", curSym->name->str);
    /* Append displacement to comment if non-zero */
    AddWrdDisp((pstr_t *)commentPStr, disp);
    
    /* No fixup needed - value is absolute */
}

/**
 * Decode extended operand mode via packed register code.
 * 
 * Extended modes (reg >= 8) dispatch to specialized readers:
 * - 8: byte immediate
 * - 9: word immediate
 * - 10: value with comment tag
 * - 11: symbol + displacement
 * - 12: absolute with symbol comment
 * 
 * @param reg Packed register code (8-12 for extended modes)
 * @param operandIdx Destination slot (0-3)
 */
static void ReadExtendedOperandByRegCode(uint8_t reg, uint8_t operandIdx) {
    switch (reg - 8) {
    case 0:
        /* Extended mode 8: byte immediate */
        ReadByteValue(operandIdx);
        break;
    case 1:
        /* Extended mode 9: word immediate */
        ReadWordValue(operandIdx);
        break;
    case 2:
        /* Extended mode 10: value with comment annotation */
        ReadValueWithCommentTag(operandIdx);
        break;
    case 3:
        /* Extended mode 11: symbol reference with displacement */
        ReadSymbolValue(Rd1Word(), operandIdx);
        break;
    case 4:
        /* Extended mode 12: absolute value with symbol comment */
        ReadAbsoluteWithSymbolComment(operandIdx);
        break;
    }
}

/**
 * Decode operand class and populate value/string arrays.
 * 
 * Operand encoding classes:
 * 0: None (no operand)
 * 1: Register-pair expansion (single packed code -> two 8-bit registers)
 *    Used for operations that implicitly use register pairs (e.g., XCHG)
 * 2: 16-bit register pair (BC, DE, HL, SP)
 * 3: Extended mode (dispatch via packed register code)
 * 4: Byte immediate (8-bit constant)
 * 5: Word immediate (16-bit constant)
 * 6: Local label reference (compiler-generated label)
 * 7: Symbol reference (user-defined variable/procedure)
 * 
 * Classes 1-3 decode register specification from stream.
 * Classes 4-7 decode from respective specialized streams.
 * 
 * Results placed in parallel arrays:
 * - wValAry[operandIdx]: numeric value
 * - sValAry[operandIdx]: formatted string for listing
 * 
 * @param reg Register selector (for classes 1-3) or ignored (for classes 4-7)
 * @param operandIdx Destination slot (0-3)
 */
static void DecodeOperandClass(uint8_t reg, uint8_t operandIdx) {
    switch (operandEncodingClass) {
    case 0:
        /* No operand */
        return;
        
    case 1:
        /* Register-pair expansion: decode single packed code into
         * two 8-bit register slots (e.g., reg=2 -> C and E) */
        wValAry[operandIdx]     = regNo[reg];        // low register (C)
        sValAry[operandIdx]     = (pstr_t *)regStr[reg];
        wValAry[operandIdx + 2] = regNo[reg + 4];    // high register (E)
        sValAry[operandIdx + 2] = (pstr_t *)regStr[reg + 4];
        break;
        
    case 2:
        /* 16-bit register pair: BC, DE, HL, SP */
        wValAry[operandIdx] = regPairNo[reg];
        sValAry[operandIdx] = (pstr_t *)regPairStr[reg];
        break;
        
    case 3:
        /* Extended mode: dispatch to specialized decoder */
        ReadExtendedOperandByRegCode(reg, operandIdx);
        break;
        
    case 4:
        /* Byte immediate from stream */
        ReadByteValue(operandIdx);
        break;
        
    case 5:
        /* Word immediate from stream */
        ReadWordValue(operandIdx);
        break;
        
    case 6:
        /* Local label reference from stream */
        ReadLocalLabelValue(operandIdx);
        break;
        
    case 7:
        /* Symbol reference from stream (no displacement) */
        ReadSymbolValue(0, operandIdx);
        break;
    }
}


/**
 * Read all operands for current fragment from intermediate stream.
 * 
 * Fragment control byte (fragControlBits[cfCode]) encodes:
 * - Bit 7: condition-code present flag
 * - Bits 6-4: first operand encoding class
 * - Bits 3-1: second operand encoding class
 * - Bit 0: (reserved/varies by fragment)
 * 
 * For fragments with condition codes (bit 7 set):
 * - Reads condition selector byte
 * - Maps to helper group via nodeTypeToHelperGroup[]
 * 
 * For fragments with operands:
 * - Reads packed register specification (for register-based classes)
 * - Decodes first operand (high nibble) into slots 0-1
 * - Decodes second operand (low nibble) into slots 2-3
 * 
 * Results:
 * - wValAry[0..3]: numeric operand values
 * - sValAry[0..3]: formatted operand strings
 * - fixupType: relocation class (if applicable)
 * - commentPStr: comment annotation (if applicable)
 */
static void ReadFragmentOperands(void) {
    /* Initialize fixup state: assume no relocation needed */
    fixupType = 0;
    
    /* Check if fragment uses condition code */
    if (fragControlBits[cfCode] & 0x80) {
        /* Read condition selector byte from stream */
        conditionSelector = Rd1Byte();
        /* Map to helper group for conditional operations */
        helperGroupIndex = nodeTypeToHelperGroup[conditionSelector];
    }
    
    /* Clear comment buffer for this fragment */
    commentPStr[0] = 0;
    
    /* Extract first operand encoding class */
    operandEncodingClass = (fragControlBits[cfCode] >> 4) & 7;
    
    if (operandEncodingClass) {
        /* Fragment has operands - read packed register spec if needed */
        uint8_t packedRegisterSpec = operandEncodingClass <= 3 ? Rd1Byte() : 0;
        
        /* Decode first operand (high nibble -> slots 0-1) */
        DecodeOperandClass((packedRegisterSpec >> 4) & 0xf, 0);
        
        /* Extract second operand encoding class */
        operandEncodingClass = (fragControlBits[cfCode] >> 1) & 7;
        
        /* Decode second operand (low nibble -> slots 2-3) */
        DecodeOperandClass(packedRegisterSpec & 0xf, 1);
    }
}


/* ------------------------------------------------------------------------- */
/* Public fragment emission entry point                                      */
/* ------------------------------------------------------------------------- */

/**
 * Emit current fragment (`cfCode`) end-to-end.
 * 
 * Main entry point called from pass-4 coordinator for each fragment
 * record read from intermediate file.
 * 
 * Processing steps:
 * 1. Resolve fragment aliases (extended codes -> canonical + helper variant)
 * 2. Decode operand payload from stream
 * 3. Special case: procedure entry synchronization
 *    - Update baseAddr to procedure's assigned address
 *    - Adjust debug line-number record if DEBUG enabled
 *    - Flush accumulated records before procedure
 * 4. Emit listing prefix (label, source line correlation)
 * 5. Expand instruction sequence via template interpreter
 * 
 * Global state updated:
 * - baseAddr: current code location (advanced by emitted instructions)
 * - opBytes/opByteCnt: instruction bytes (written to EXEC record)
 * - line: formatted listing text (written to listing file)
 * - Various record buffers: EXEC, fixup records, line-number record
 */
void EmitCurrentFragment(void) {
    /* Step 1: Resolve fragment code aliases */
    ResolveFragmentAlias();
    
    /* Step 2: Decode operand values from stream */
    ReadFragmentOperands();
    
    /* Step 3: Handle procedure entry synchronization */
    if (cfCode == T2_PROCEDURE) {
        /* Synchronize code address to procedure's assigned location */
        baseAddr = info->addr;
        
        if (DEBUG) {
            /* Adjust debug line-number record:
             * - Remove spurious statement-location entry (4 bytes)
             * - Add procedure-location entry (2 bytes)
             * Net effect: replace last entry with procedure address */
            putWord(&recLineNum[REC_LEN], getWord(&recLineNum[REC_LEN]) - 4);
            RecAddWord(recLineNum, 1, baseAddr);
            putWord(&recLineNum[REC_LEN], getWord(&recLineNum[REC_LEN]) + 2);
        }
        
        /* Flush all accumulated records before procedure */
        FlushRecs();
    }
    
    /* Step 4: Emit listing prefix (label, source correlation) */
    EmitLinePrefix();
    
    /* Step 5: Expand instruction sequence from template tables
     * instructionSeq[cfCode] packs:
     * - Low 12 bits: starting index in codeSeq[]
     * - High 4 bits: instruction count (1-15) */
    EmitCodeSeq(instructionSeq[cfCode] & 0xfff, instructionSeq[cfCode] >> 12);
}
