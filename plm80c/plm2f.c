/****************************************************************************
 *  plm2f.c: Expression Code Generation - Fragment Selection & Optimization
 *
 *  PART OF: Intel ISIS-II PL/M-80 Compiler (C Port)
 *  Original: Copyright Intel
 *  Re-engineered to C by Mark Ogden <mark.pm.ogden@btinternet.com>
 *
 *  Released for academic interest and personal use only
 *
 *  PURPOSE:
 *  --------
 *  This module implements the core expression code generation pipeline,
 *  responsible for:
 *
 *  1. Analyzing operand storage locations and register usage
 *  2. Optimally selecting code fragments for operation implementation
 *  3. Managing register state transitions after operations
 *  4. Handling special statement types (calls, jumps, assignments)
 *  5. Coordinating with lower-level code generation modules
 *
 *  The module bridges high-level expression analysis (plm2a.c) with low-level
 *  code generation (plm2e.c), making sophisticated optimization decisions to
 *  produce efficient 8080 assembly code from abstract expression trees.
 *
 *  COMPILATION FLOW:
 *  -----------------
 *  Start2() (main2.c)
 *    ├─ FillTx2Queue() - Build expression tree
 *    ├─ DeRelStmt() (plm2b.c) - Convert relative → absolute references
 *    ├─ OptimiseStmtNodes() (plm2c.c) - Common subexpr elimination
 *    ├─ GenerateStatementCode() (plm2h.c)
 *    │   └─ GenerateExpressionCode() ← THIS MODULE
 *    │       ├─ AnalyzeOperandLocations() - Where are operands?
 *    │       ├─ SelectOptimalFragment() - Which fragment is best?
 *    │       ├─ GenerateOperandCode() (plm2e.c) - Load operands
 *    │       ├─ GenerateFragmentCode() (plm2e.c) - Emit fragment
 *    │       └─ UpdateRegisterState() - Track register changes
 *    └─ Next statement
 *
 *  ARCHITECTURE:
 *  ---------------
 *  Expression code generation follows a multi-pass optimization strategy:
 *
 *  PASS 1: Analyze Operand Locations
 *    - Determine where each operand is currently stored
 *    - Check if it's in a register, on stack, or in memory
 *    - Try to reuse existing registers when possible
 *    - Find optimal register for new values
 *    - Update expression attributes and locations
 *
 *  PASS 2: Select Optimal Code Fragment
 *    - Iterate through all valid code fragments for the operation
 *    - For each fragment, calculate total implementation cost:
 *      • Type conversion cost for left operand
 *      • Type conversion cost for right operand
 *      • Fragment code size/complexity
 *    - Select fragment with minimum total cost
 *
 *  PASS 3: Generate Operand Loading Code (plm2e.c)
 *    - If operands need loading, load them into registers
 *    - Handle register conflicts and spilling
 *    - Continue until all operands are optimal
 *
 *  PASS 4: Generate Operation Code (plm2e.c)
 *    - Emit selected code fragment
 *    - May require helper functions for complex operations
 *
 *  PASS 5: Update Register State
 *    - Track which registers now hold which values
 *    - Update register type information
 *    - Manage temporary register allocation
 *
 *  KEY CONCEPTS:
 *  ---------------
 *
 *  1. OPERAND LOCATIONS (exprLoc values):
 *     - 0-3: Register indices (BC, -, DE, HL)
 *     - 4: Immediate value (in iCodeArgs)
 *     - 8: Memory reference (via symbol table)
 *     - 9: Stack-based (on runtime stack)
 *     - 0xA: Stack pointer relative
 *
 *  2. EXPRESSION ATTRIBUTES (exprAttr values):
 *     - BYTE_A (0): 8-bit value
 *     - LABEL_A (1): Label address
 *     - 2: Indirect addressing
 *     - 3: Register indirect
 *     - ADDRESS_A (6): 16-bit address/pointer
 *     - 0xC: Unknown/uninitialized
 *
 *  3. FRAGMENT CODES:
 *     - Represent different instruction sequences for operations
 *     - Each has associated cost (code size in bytes)
 *     - Each has type conversion rules for operands
 *     - Selected based on operand types and storage
 *
 *  4. REGISTER STATE TRACKING:
 *     - registerState[] - Array of 9 register state structures
 *       • Indices 0-3: BC, -, DE, HL (actual registers)
 *       • Indices 5-8: Temporary slots for operation results
 *     - Contents, dataType, offset, stackOffset tracked
 *     - Used for value reuse and optimization
 *
 *  DATA FLOW:
 *  -----------
 *  
 *  Expression: a + b
 *
 *  1. ANALYZE OPERAND LOCATIONS
 *     - 'a' at location? (register, memory, stack?)
 *     - 'b' at location? (can reuse same register?)
 *     - Try reusing existing registers (TryUseExistingRegister)
 *     - Try indirect registers (TryUseIndirectRegister)
 *     - Find optimal register (FindOptimalRegister)
 *
 *  2. SELECT OPTIMAL FRAGMENT
 *     - Fragment 1: MOV H,left + ADD H,right (cost: 5 bytes)
 *     - Fragment 2: MOV D,left + XCHG + ADD H,D (cost: 7 bytes)
 *     - Fragment 3: Load + ADD + Store (cost: 10 bytes)
 *     → Select Fragment 1 (minimum cost)
 *
 *  3. GENERATE OPERAND CODE
 *     - If 'a' not in H, load it
 *     - If 'b' not in appropriate location, load it
 *     - Iterate until ready
 *
 *  4. GENERATE FRAGMENT CODE
 *     - Emit selected instructions
 *     - May call helper for complex operations
 *
 *  5. UPDATE REGISTER STATE
 *     - Result now in register H
 *     - H.contents = tx2[tx2qp]
 *     - H.dataType = result type
 *     - H.offset = any offset adjustment
 *
 *  OPTIMIZATION STRATEGIES:
 *  ------------------------
 *
 *  1. REGISTER REUSE:
 *     - If operand already in register with compatible type
 *     - Avoid redundant load operations
 *     - Check for conflicts with other operand
 *
 *  2. OPTIMAL OFFSET SELECTION:
 *     - When loading from stack/memory
 *     - Find register that minimizes offset adjustment
 *     - Reduces instruction complexity
 *
 *  3. FRAGMENT SELECTION:
 *     - Consider all valid fragments for operation
 *     - Calculate total cost including type conversions
 *     - Select minimum cost option
 *
 *  4. INDIRECT ADDRESSING:
 *     - Some operations better with indirect (register-indirect) addressing
 *     - Checked in TryUseIndirectRegister
 *     - Enables more efficient memory operations
 *
 *  5. COMMUTATIVE OPERATION OPTIMIZATION:
 *     - For operations like addition, multiplication
 *     - Both operand orders possible
 *     - Can select order based on current register state
 *
 *  ALGORITHMS:
 *  -----------
 *
 *  CanUseRegisterDirect(lrIdx):
 *    - Scans all fragments in range
 *    - Returns true if any fragment accepts direct register operand
 *    - Determines feasibility of register reuse
 *
 *  TryUseExistingRegister(exprIdx, lrIdx):
 *    - If fragment can use direct registers
 *    - Search registers for one containing exprIdx
 *    - Check type compatibility (0, 1, or 6)
 *    - Avoid conflict with other operand
 *    - Reuse register if found
 *
 *  TryUseIndirectRegister(exprIdx, lrIdx):
 *    - If operand not already in register (exprLoc > 3)
 *    - Search registers for one containing exprIdx
 *    - Check for indirect/address type (2 or 3)
 *    - Update location and attribute
 *
 *  AnalyzeStackOperand(exprIdx, lrIdx):
 *    - If operand on stack (exprLoc == 9 or 0xA)
 *    - Calculate stack offset (calculatedOffset)
 *    - Check if conflict resolution needed
 *    - Count active registers preventing reuse
 *
 *  FindOptimalRegister(lrIdx):
 *    - For operands not easily reusable
 *    - Scan all registers looking for one with compatible offset
 *    - Find register with minimum offset adjustment needed
 *    - Update register state with new content
 *
 *  SelectOptimalFragment():
 *    - Iterate through all valid fragments (first to last)
 *    - For each, calculate type conversion costs
 *    - Add fragment code size/complexity
 *    - Track fragment with minimum total cost
 *    - Record operand type requirements
 *
 *  UpdateRegisterState():
 *    - Process operator precedence rules
 *    - Update temporary register slots (5-8)
 *    - Copy results to actual registers (0-3)
 *    - Handle special cases (assignments, returns)
 *    - Track which register holds result
 *
 *  SPECIAL CASE HANDLING:
 *  ----------------------
 *
 *  1. STACK ARGUMENT OPERATIONS (T2_STKARG, T2_STKBARG, T2_STKWARG):
 *     - Manage stack frame for argument passing
 *     - Adjust stack pointers
 *     - Update call depth tracking
 *
 *  2. PROCEDURE CALLS (T2_CALL):
 *     - Calculate stack space needed for call
 *     - Handle external vs. internal calls
 *     - Track return value location
 *     - Update stack usage maximum
 *
 *  3. VARIABLE CALLS (T2_CALLVAR):
 *     - Indirect procedure call through variable
 *     - Adjust stack for call frame
 *     - Update stack depth tracking
 *
 *  4. RETURN STATEMENTS (T2_RETURN, T2_RETURNBYTE, T2_RETURNWORD):
 *     - Generate return sequence via GenerateReturnSequence()
 *     - Set flag to prevent duplicate return generation
 *
 *  5. ASSIGNMENTS (T2_COLONEQUALS):
 *     - Special register state handling
 *     - Can optimize by tracking assigned values
 *     - Invalidate conflicting register contents
 *
 *  6. JUMP STATEMENTS (T2_JMPFALSE):
 *     - Handle inverted comparison optimization
 *     - Change to T2_JMPTRUE if comparison was inverted
 *     - Update fragment code accordingly
 *
 *  7. MOVE OPERATIONS (T2_MOVE):
 *     - Handle MOVE blocks with special register state
 *     - Optimize register usage for large moves
 *
 *  REGISTER STATE MACHINE:
 *  ------------------------
 *
 *  After operation, register contents transition:
 *
 *  Before:
 *    Reg 0 (BC): value X
 *    Reg 2 (DE): value Y
 *    Reg 3 (HL): value Z
 *
 *  Operation: Add BC to HL → result in HL
 *
 *  After:
 *    Reg 0 (BC): value X (unchanged)
 *    Reg 2 (DE): value Y (unchanged)
 *    Reg 3 (HL): result (new value, replaces Z)
 *
 *  Register state also tracks:
 *    - Whether register is "direct" (contains actual value)
 *    - Offset adjustments needed (for stack/memory refs)
 *    - Stack offset for values on stack
 *    - Storage class flags (register, stack, memory)
 *
 *  COST MODEL:
 *  -----------
 *
 *  Fragment selection uses multi-factor cost calculation:
 *
 *  totalCost = typeConversionCost[leftType] +
 *              typeConversionCost[rightType] +
 *              fragmentCodeLength
 *
 *  Where:
 *    - Type conversion costs from exprComplexity[] table
 *    - Costs range from 0 (compatible, no conversion) to high
 *    - Fragment code length is byte count (0-31)
 *
 *  Example:
 *    Fragment A: 2 bytes, left cost 1, right cost 2 = total 5
 *    Fragment B: 5 bytes, left cost 0, right cost 0 = total 5
 *    Fragment C: 3 bytes, left cost 0, right cost 1 = total 4 ← selected
 *
 *  INTEGRATION POINTS:
 *  --------------------
 *  - Called from: plm2h.c::GenerateStatementCode()
 *  - Calls: plm2e.c::GenerateOperandCode() - Load operands
 *  - Calls: plm2e.c::GenerateFragmentCode() - Emit code
 *  - Calls: AnalyzeRegisterUsage() - Register tracking
 *  - Calls: AdjustRegisterOffset() - Normalize offsets
 *  - Calls: AdjustStackOnReturn() - Stack adjustments
 *  - Calls: AdjustRegistersByMask() - Register state changes
 *  - Uses: nodeTypeToAttribute[] - Node classification
 *  - Uses: operatorPrecedence[], operatorTypeRules[] - Rules
 *  - Uses: exprComplexity[], exprRegisterCost[] - Cost models
 *  - Uses: typeConversionTable[][] - Type compatibility
 *
 *  GLOBAL STATE MODIFIED:
 *  ----------------------
 *  - curExprLoc[] - Current operand locations
 *  - exprLoc[], exprAttr[] - Expression attributes
 *  - operandComplexity[], operandRegisterCost[] - Operand costs
 *  - operandCategory[], operandFragmentType[] - Operand types
 *  - cfrag1 - Selected code fragment
 *  - selectedOperatorIdx - Operator encoding index
 *  - registerState[] - Register tracking
 *  - currentStackDepth - Stack depth tracking
 *  - stackUsage - Maximum stack usage
 *  - codeSize - Generated code size
 *  - conflictMode - Register conflict flag
 *
 *  PERFORMANCE CONSIDERATIONS:
 *  ---------------------------
 *  1. Fragment selection loop: O(F) where F = fragment count
 *     - Typically 2-5 fragments per operation
 *     - Cost calculation done for each
 *
 *  2. Register scanning: O(R) where R = register count
 *     - Typically 4 registers searched
 *     - Fast linear scan sufficient
 *
 *  3. Operand code generation: Iterative
 *     - Continues until operands optimal
 *     - Usually 1-3 iterations
 *     - Bounded by available registers
 *
 *  4. Overall: Near O(1) for most operations
 *     - Bounded problem size (2 operands, ~4 registers, ~5 fragments)
 *     - Fast enough for single-pass compilation
 *
 *  DEBUGGING NOTES:
 *  ----------------
 *  - Add tracing in SelectOptimalFragment() to see cost calculations
 *  - Check exprLoc[] values to verify operand locations
 *  - Monitor registerState[] to track register allocation
 *  - Watch conflictMode flag for register conflict scenarios
 *  - Review GenerateOperandCode() iteration count
 *
 *  FUTURE IMPROVEMENTS:
 *  ---------------------
 *  1. More sophisticated cost model considering:
 *     - Cache locality
 *     - Register pressure
 *     - Instruction pipeline effects
 *
 *  2. Partial code generation:
 *     - Generate fragments for subset of operand types
 *     - Only emit code for used cases
 *
 *  3. Adaptive fragment selection:
 *     - Learn which fragments work best in context
 *     - Adjust costs based on actual performance
 *
 *  VERSION HISTORY:
 *  ----------------
 *  - Original: Intel ISIS-II PLM-80 compiler
 *  - C port: Mark Ogden
 *  - Current: Fully renamed and documented
 *
 ****************************************************************************/
#include "plm.h"

static uint8_t operandNodeType;
static uint16_t calculatedOffset, first, last;

static bool CanUseRegisterDirect(uint8_t lrIdx) {
    for (currentFragmentCode = first; currentFragmentCode <= last; currentFragmentCode++) {
        uint8_t i = GetFragmentOperandType(lrIdx);
        if (i <= 3 || (12 <= i && i <= 14))
            return true;
    }
    return false;
} /* CanUseRegisterDirect() */

static void TryUseExistingRegister(uint8_t exprIdx, uint8_t lrIdx) {
    if (CanUseRegisterDirect(lrIdx)) {
        for (int regIdx = 0; regIdx < 4; regIdx++) {
            if (registerState[regIdx].contents == exprIdx) {
                if (registerState[regIdx].dataType == 0 || registerState[regIdx].dataType == 1 ||
                    registerState[regIdx].dataType == 6) {
                    exprAttr[lrIdx] = registerState[regIdx].dataType;
                    exprLoc[lrIdx]  = regIdx;
                    if (exprLoc[1 - lrIdx] != regIdx)
                        return;
                }
            }
        }
    }
} /* TryUseExistingRegister() */

static void TryUseIndirectRegister(uint8_t exprIdx, uint8_t lrIdx) {
    if (exprLoc[lrIdx] > 3) {
        for (int regIdx = 1; regIdx < 4; regIdx++) {
            if (registerState[regIdx].contents == exprIdx &&
                (registerState[regIdx].dataType == 2 || registerState[regIdx].dataType == 3)) {
                exprAttr[lrIdx] = registerState[regIdx].dataType;
                exprLoc[lrIdx]  = regIdx;
                if (exprLoc[1 - lrIdx] != regIdx)
                    return;
            }
        }
    }
} /* TryUseIndirectRegister() */

static void AnalyzeStackOperand(uint8_t exprIdx, uint8_t lrIdx) {
    uint8_t activeRegCount;
    uint16_t stackLevel;

    if (exprLoc[lrIdx] == 0xA)
        calculatedOffset = tx2[exprIdx].right;
    else if (exprLoc[lrIdx] == 9) {
        calculatedOffset = tx2[exprIdx].extra;
        if ((!registerInExpression[Left] && registerNeedsSave[Left]) || savedRegisterCount > 0 ||
            calculatedOffset != currentStackDepth) {
            activeRegCount = savedRegisterCount + exprRegisterCount;
            for (stackLevel = calculatedOffset; stackLevel <= currentStackDepth; stackLevel++) {
                if (stackNodeContents[stackLevel] != 0)
                    activeRegCount++;
            }
            if (activeRegCount < 4)
                conflictMode = true;
            else
                exprLoc[lrIdx] = 0xA;
        }
        calculatedOffset = -(calculatedOffset * 2);
    }
} /* AnalyzeStackOperand() */

static void FindOptimalRegister(uint8_t lrIdx) {
    uint16_t operandOffset, storageFlags;
    uint8_t minDistance, targetAttr;
    uint16_t offsetDelta;

    if (exprLoc[lrIdx] == 0xA) {
        operandOffset = calculatedOffset;
        storageFlags  = 0x100;
        minDistance   = 4;
        targetAttr    = IndirectAddr(exprAttr[lrIdx]);
    } else if (exprLoc[lrIdx] == 8 && exprAttr[lrIdx] == 1) {
        GetOperandValue(curExprLoc[lrIdx], &operandOffset, &storageFlags);
        minDistance = 2;
        targetAttr  = ADDRESS_A;
    } else if (exprLoc[lrIdx] == 4 &&
               (exprAttr[lrIdx] == 0 || exprAttr[lrIdx] == 8 || !CanUseRegisterDirect(lrIdx))) {
        GetOperandValue(curExprLoc[lrIdx], &operandOffset, &storageFlags);
        minDistance = 2;
        targetAttr  = IndirectAddr(exprAttr[lrIdx]);
    } else
        return;

    for (int regIdx = 1; regIdx < 4; regIdx++) {
        if (registerInExpression[regIdx]) {
            if (curExprLoc[Left] == curExprLoc[Right] && curNodeType != T2_COLONEQUALS)
                if (exprLoc[lrIdx] > 3)
                    exprLoc[lrIdx] = regIdx;
        } else if (!registerNeedsSave[regIdx] &&
                   registerState[regIdx].storageClass == storageFlags &&
                   registerState[regIdx].isDirect && 1 <= registerState[regIdx].dataType &&
                   registerState[regIdx].dataType <= 6) {
            offsetDelta =
                registerState[regIdx].stackOffset + registerState[regIdx].offset - operandOffset;
            if (offsetDelta > 0xff)
                offsetDelta = -offsetDelta;
            if (offsetDelta < minDistance) {
                exprLoc[lrIdx] = regIdx;
                minDistance    = (uint8_t)offsetDelta;
            }
        }
    }
    if (exprLoc[lrIdx] <= 3) {
        int regIdx                     = exprLoc[lrIdx];
        registerState[regIdx].dataType = exprAttr[lrIdx] = targetAttr;
        registerState[regIdx].contents                   = curExprLoc[lrIdx];
        registerState[regIdx].offset =
            registerState[regIdx].stackOffset + registerState[regIdx].offset - operandOffset;
        registerState[regIdx].stackOffset = operandOffset;
    }
} /* FindOptimalRegister() */

static void AnalyzeOperandLocations() {
    uint8_t exprIdx, lrIdx;
    exprLoc[Left]  = 8;
    exprLoc[Right] = 8;
    for (lrIdx = 0; lrIdx <= 1; lrIdx++) {
        if ((exprIdx = curExprLoc[lrIdx]) == 0)
            exprAttr[lrIdx] = 0xC;
        else if ((operandNodeType = tx2[exprIdx].type) == T2_STACKPTR)
            exprAttr[lrIdx] = 0xA;
        else if (operandNodeType == T2_LOCALLABEL)
            exprAttr[lrIdx] = 9;
        else {
            exprAttr[lrIdx] = tx2[exprIdx].exprAttr;
            exprLoc[lrIdx]  = tx2[exprIdx].exprLoc;
            TryUseExistingRegister(exprIdx, lrIdx);
            TryUseIndirectRegister(exprIdx, lrIdx); /*  checked */
        }
    }
    for (lrIdx = 0; lrIdx <= 1; lrIdx++) {
        exprIdx = curExprLoc[lrIdx];
        AnalyzeRegisterUsage();
        AnalyzeStackOperand(exprIdx, lrIdx);
        FindOptimalRegister(lrIdx);
        UpdateExpressionLookup(lrIdx);
    }
} /* AnalyzeOperandLocations() */

static uint8_t GetTypeConversion(uint8_t lrIdx) {

    if (curExprLoc[lrIdx] == 0 || curExprLoc[lrIdx] == 1)
        return 1;
    return typeConversionTable[iCodeArgsIndex[lrIdx]][GetFragmentOperandType(lrIdx)];
} /* GetTypeConversion() */

static void SetOperandCosts(uint8_t lrIdx, uint8_t srcIdx) {
    operandComplexity[lrIdx]   = exprComplexity[srcIdx];
    operandRegisterCost[lrIdx] = exprRegisterCost[srcIdx];
    operandCategory[lrIdx]     = nodeExprCategory[srcIdx];
} /* SetOperandCosts() */

static void SelectOptimalFragment() {
    uint8_t bestLeftType, bestRightType, minCost, leftConvType, rightConvType, totalCost;
    bestLeftType = bestRightType = 0; // set to avoid compiler warning

    minCost                      = 198;
    for (currentFragmentCode = first; currentFragmentCode <= last; currentFragmentCode++) {
        leftConvType  = GetTypeConversion(Left);
        rightConvType = GetTypeConversion(Right);
        totalCost     = exprComplexity[leftConvType] + exprComplexity[rightConvType] +
                    (fragmentCodeLength[opcodeEncodingMap[currentFragmentCode]] & 0x1f);
        if (totalCost < minCost) {
            minCost                    = totalCost;
            bestLeftType               = leftConvType;
            bestRightType              = rightConvType;
            cfrag1                     = opcodeEncodingMap[currentFragmentCode];
            selectedOperatorIdx        = fragmentOpcodeIndex[currentFragmentCode];
            operandFragmentType[Left]  = GetFragmentOperandType(Left);
            operandFragmentType[Right] = GetFragmentOperandType(Right);
        }
    }
    SetOperandCosts(Left, bestLeftType);
    SetOperandCosts(Right, bestRightType);
} /* SelectOptimalFragment() */

static void AdjustOperandOffsets() {
    if (curExprLoc[Left] != 0)
        AdjustRegisterOffset(exprLoc[Left]);

    if (curExprLoc[Right] != 0)
        AdjustRegisterOffset(exprLoc[Right]);
} /* AdjustOperandOffsets() */

static void HandleSpecialStatements() {
    uint16_t p;

    if (curNodeType == T2_STKARG || curNodeType == T2_STKBARG || curNodeType == T2_STKWARG) {
        AdjustStackOnReturn(-(callStackBase[procCallDepth] * 2));
        callStackBase[procCallDepth]++;
        currentStackDepth++;
    } else if (curNodeType == T2_CALL) {
        AdjustStackOnReturn(-(callStackBase[procCallDepth] * 2));
        info = FromIdx(tx2[tx2qp].extra);
        if ((info->flag & F_EXTERNAL))
            p = (callStackBase[procCallDepth] + 1) * 2;
        else
            p = (callStackDepth[procCallDepth] + 1) * 2 + info->stackUsage;
        if (p > stackUsage)
            stackUsage = p;
    } else if (curNodeType == T2_CALLVAR) {
        AdjustStackOnReturn(-(callStackBase[procCallDepth] * 2));
        if (stackUsage < currentStackDepth * 2)
            stackUsage = currentStackDepth * 2;
    } else if (curNodeType == T2_RETURN || curNodeType == T2_RETURNBYTE ||
               curNodeType == T2_RETURNWORD) {
        nextReturnState = true;
        GenerateReturnSequence();
    } else if (curNodeType == T2_JMPFALSE) {
        AdjustStackOnReturn(0);
        if (invertComparison) {
            cfrag1 = CF_JMPTRUE;
            invertComparison = false;
        }
    } else if (curNodeType == T2_CASEBLOCK)
        AdjustStackOnReturn(0);
    else if (curNodeType == T2_MOVE) {
        if (callStackBase[procCallDepth] != currentStackDepth) {
            AdjustStackOnReturn(-((callStackBase[procCallDepth] + 1) * 2));
            PopRegisterFromStack(3);
        }
        if (registerState[3].dataType == 1)
            cfrag1 = CF_MOVE_HL;
    }
} /* HandleSpecialStatements() */

static void InvalidateTargetRegisters() {
    for (int regIdx = 0; regIdx <= IR_H; regIdx++) {
        if (registerState[regIdx].contents == curExprLoc[Left])
            if (registerState[regIdx].dataType < 2 || 5 < registerState[regIdx].dataType)
                registerState[regIdx].contents = 0;
    }
}

static void UpdateRegisterState() {

    uint16_t precedenceBits = operatorPrecedence[selectedOperatorIdx] * 16;
    uint16_t typeRuleBits   = operatorTypeRules[selectedOperatorIdx];
    uint8_t resultRegIdx    = 0;
    if (curNodeType == T2_COLONEQUALS) {
        InvalidateTargetRegisters();
        if (tx2[curExprLoc[Right]].cnt == 0)
            if (tx2[curExprLoc[Left]].cnt > 0) {
                if (cfrag1 == CF_MOVMLR || cfrag1 == CF_STA) {
                    registerState[exprLoc[Right]].dataType = BYTE_A;
                    registerState[exprLoc[Right]].contents = curExprLoc[Left];
                } else if (cfrag1 == CF_SHLD || cfrag1 == CF_MOVMRP) {
                    registerState[exprLoc[Right]].dataType = ADDRESS_A;
                    registerState[exprLoc[Right]].contents = curExprLoc[Left];
                }
            }
    } else if (T2_51 <= curNodeType && curNodeType <= T2_56)
        InvalidateTargetRegisters();
    for (int tmpRegIdx = IR_SAVPSW; tmpRegIdx <= IR_SAVH; tmpRegIdx++) {
        uint8_t precedenceRule = precedenceBits >> 13;
        uint8_t ruleValue      = typeRuleBits >> 12;
        precedenceBits <<= 3;
        typeRuleBits <<= 4;
        if (ruleValue <= 3) {
            registerState[tmpRegIdx] = registerState[ruleValue];
            if (precedenceRule == 1)
                registerState[tmpRegIdx].offset++;
            else if (precedenceRule == 2) {
                if (registerState[tmpRegIdx].dataType == BYTE_A) {
                    registerState[tmpRegIdx].dataType = 6;
                } else {
                    registerState[tmpRegIdx].dataType = ADDRESS_A;
                    registerState[tmpRegIdx].isDirect = false;
                }
            }
        } else if (ruleValue == 4) {
            registerState[resultRegIdx = tmpRegIdx].isDirect = false;
            if (0 < tx2[tx2qp].cnt) {
                registerState[tmpRegIdx].contents = tx2qp;
                registerState[tmpRegIdx].dataType = tx2[tx2qp].exprAttr =
                    fragmentCodeLength[cfrag1] >> 5;
                registerState[tmpRegIdx].offset = 0;
            } else
                registerState[tmpRegIdx].contents = 0;
        } else if (ruleValue == 5) {
            registerState[tmpRegIdx].contents     = 0;
            registerState[tmpRegIdx].storageClass = 0;
            registerState[tmpRegIdx].offset       = 0;
            registerState[tmpRegIdx].isDirect     = true;
            registerState[tmpRegIdx].dataType     = BYTE_A;
            registerState[tmpRegIdx].stackOffset  = precedenceRule;
        } else {
            registerState[tmpRegIdx].contents = 0;
            registerState[tmpRegIdx].isDirect = false;
        }
    }
    if (resultRegIdx == 0 && tx2[tx2qp].cnt > 0) {
        for (int tmpRegIdx = IR_SAVPSW; tmpRegIdx <= IR_SAVH; tmpRegIdx++) {
            if (registerState[tmpRegIdx].contents == 0)
                if (!registerState[resultRegIdx = tmpRegIdx].isDirect)
                    break;
        }
        if (resultRegIdx != 0) {
            registerState[resultRegIdx].contents = tx2qp;
            registerState[resultRegIdx].isDirect = false;
            registerState[resultRegIdx].dataType = BYTE_A;
            tx2[tx2qp].exprAttr                  = BYTE_A;
            registerState[resultRegIdx].offset   = 0;
        }
    }
    for (int n = 0; n <= IR_H; n++)
        registerState[n] = registerState[n + 5];
} /* UpdateRegisterState() */

/**
 * GenerateExpressionCode - Main orchestrator for expression code generation
 *
 * Central entry point for expression compilation. Coordinates the complete
 * pipeline from operand analysis through code generation and register state
 * updates.
 *
 * Algorithm:
 *   1. Extract operands from current TX2 node
 *   2. Determine valid fragment range based on operation type
 *   3. Analyze operand locations (registers, memory, stack)
 *   4. Iteratively:
 *      a. Select optimal code fragment (minimum cost)
 *      b. If operands not optimal, generate loading code
 *      c. Repeat until ready for code generation
 *   5. Normalize register offsets
 *   6. Decrement operand reference counts
 *   7. Invalidate registers as needed by operation
 *   8. Handle special statement types
 *   9. Generate actual machine code
 *   10. Update register state for result
 *
 * Side effects:
 *   - Updates exprLoc[], exprAttr[] (operand info)
 *   - Updates registerState[] (register allocation)
 *   - Generates code via GenerateFragmentCode()
 *   - Updates codeSize counter
 *   - Updates currentStackDepth
 *   - Updates stackUsage
 *
 * Called from: plm2h.c::GenerateStatementCode()
 * Postcondition: Code generated for expression, register state updated
 */
void GenerateExpressionCode() {
    curExprLoc[Left]  = (uint8_t)tx2[tx2qp].left;
    curExprLoc[Right] = (uint8_t)tx2[tx2qp].right;
    first             = nodeTypeToAttribute[curNodeType];
    last              = first + operatorResultType[curNodeType] - 1;
    AnalyzeOperandLocations();

    while (1) {
        SelectOptimalFragment(); /*  OK */
        if (operandComplexity[Left] == 0 && operandComplexity[Right] == 0)
            break;
        if (conflictMode)
            AllocateRegister();
        else
            GenerateOperandCode();
    }
    AdjustOperandOffsets();
    DecrementExprRefs();
    InvalidateRegistersByMask(operatorPrecedence[selectedOperatorIdx] >> 12);
    HandleSpecialStatements();
    GenerateFragmentCode();
    UpdateRegisterState();
} /* GenerateExpressionCode() */

/**
 * c_procedure - Handle procedure entry and initialization
 *
 * Processes T2_PROCEDURE nodes to set up new procedure scope.
 * Manages block structure, symbol information, and entry code generation.
 *
 * Called from: GenerateStatementCode() for T2_PROCEDURE nodes
 */
void c_procedure() {
    if (EnterBlk()) {
        blk[blkId].codeSize    = codeSize;
        blk[blkId].wB4B0       = currentStackDepth;
        blk[blkId].stackSize   = stackUsage;
        blk[blkId].extProcId   = curExtProcId;
        blk[activeGrpCnt].next = blkId;
        blkId                  = activeGrpCnt;
        info = blk[activeGrpCnt].info = FromIdx(tx2[tx2qp].left);
        curExtProcId                  = info->procId;
        codeSize                      = 0;
        EmitTopItem();
        GenerateProcedureEntry();
    }
}
