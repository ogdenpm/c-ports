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
 *  [Previous documentation sections omitted for brevity]
 ****************************************************************************/
#include "plm.h"

// Local state for operand analysis
static uint8_t operandNodeType;      // Type of operand node being analyzed
static uint16_t calculatedOffset;    // Calculated offset for stack/memory operands
static uint16_t first, last;         // Range of valid fragments [first..last]

/**
 * CanUseRegisterDirect - Check if any fragment accepts direct register operand
 *
 * Scans all valid fragments in the range [first..last] to determine if at least
 * one fragment can accept the operand directly in a register (no load needed).
 *
 * @param lrIdx: Left (0) or Right (1) operand index
 * @return: true if any fragment accepts direct register (types 0-3, 12-14)
 *
 * Fragment operand types:
 *   0-3: Direct register operands (BC, -, DE, HL)
 *   12-14: Special register variants
 *   Other: Requires memory/immediate/indirect addressing
 */
static bool CanUseRegisterDirect(uint8_t lrIdx) {
    // Iterate through all valid fragments for this operation
    for (currentFragmentCode = first; currentFragmentCode <= last; currentFragmentCode++) {
        // Get the operand type requirement for this fragment
        uint8_t i = GetFragmentOperandType(lrIdx);
        
        // Check if fragment accepts direct register operand
        if (i <= 3 || (12 <= i && i <= 14))
            return true; // Found at least one fragment that accepts register
    }
    return false; // No fragments accept direct register operand
} /* CanUseRegisterDirect() */

/**
 * TryUseExistingRegister - Attempt to reuse a register already containing operand
 *
 * Optimization: If the operand value is already loaded in a register with
 * compatible type, reuse that register instead of loading again.
 *
 * @param exprIdx: Expression index (tx2 array index) to find
 * @param lrIdx: Left (0) or Right (1) operand index
 *
 * Strategy:
 *   1. Check if any fragment can use direct register operand
 *   2. Search registers (BC, DE, HL) for one containing exprIdx
 *   3. Verify type compatibility (BYTE_A=0, LABEL_A=1, ADDRESS_A=6)
 *   4. Ensure no conflict with other operand
 *   5. If found, update exprAttr and exprLoc to reuse register
 */
static void TryUseExistingRegister(uint8_t exprIdx, uint8_t lrIdx) {
    // Only proceed if at least one fragment accepts direct register
    if (CanUseRegisterDirect(lrIdx)) {
        // Scan all physical registers (0=BC, 2=DE, 3=HL; 1 is unused)
        for (int regIdx = 0; regIdx < 4; regIdx++) {
            // Check if this register contains our target expression
            if (registerState[regIdx].contents == exprIdx) {
                // Type must be compatible: 0=BYTE_A, 1=LABEL_A, 6=ADDRESS_A
                if (registerState[regIdx].dataType == 0 || registerState[regIdx].dataType == 1 ||
                    registerState[regIdx].dataType == 6) {
                    // Update operand to use this register's type and location
                    exprAttr[lrIdx] = registerState[regIdx].dataType;
                    exprLoc[lrIdx]  = regIdx;
                    
                    // Verify no conflict: can't use same register for both operands
                    // (unless it's a unary operation or intentionally using same register)
                    if (exprLoc[1 - lrIdx] != regIdx)
                        return; // Success - operand will reuse this register
                }
            }
        }
    }
} /* TryUseExistingRegister() */

/**
 * TryUseIndirectRegister - Attempt to use register-indirect addressing
 *
 * If operand is in memory and we have a register pointing to it (indirect),
 * use register-indirect addressing instead of direct memory addressing.
 * This is more efficient on 8080 (e.g., MOV A,M vs MOV A,[addr]).
 *
 * @param exprIdx: Expression index to find
 * @param lrIdx: Left (0) or Right (1) operand index
 *
 * Strategy:
 *   1. Check if operand not already in register (exprLoc > 3)
 *   2. Search for register containing address of operand
 *   3. Verify register has indirect type (2 or 3)
 *   4. Update to use register-indirect addressing
 */
static void TryUseIndirectRegister(uint8_t exprIdx, uint8_t lrIdx) {
    // Only useful if operand not already directly in a register
    // exprLoc values: 0-3=registers, 4=immediate, 8=memory, 9=stack, 0xA=SP-relative
    if (exprLoc[lrIdx] > 3) {
        // Scan registers DE, HL (BC typically not used for indirect)
        for (int regIdx = 1; regIdx < 4; regIdx++) {
            // Check if register contains address of our expression
            if (registerState[regIdx].contents == exprIdx &&
                // Type 2 or 3 = indirect/pointer types
                (registerState[regIdx].dataType == 2 || registerState[regIdx].dataType == 3)) {
                // Switch to indirect addressing mode
                exprAttr[lrIdx] = registerState[regIdx].dataType;
                exprLoc[lrIdx]  = regIdx;
                
                // Verify no conflict with other operand
                if (exprLoc[1 - lrIdx] != regIdx)
                    return; // Success - will use register-indirect addressing
            }
        }
    }
} /* TryUseIndirectRegister() */

/**
 * AnalyzeStackOperand - Analyze stack-based operand and detect conflicts
 *
 * For operands on the runtime stack, calculate their offset and determine
 * if there are register conflicts preventing efficient access.
 *
 * @param exprIdx: Expression index being analyzed
 * @param lrIdx: Left (0) or Right (1) operand index
 *
 * Stack operand types:
 *   exprLoc == 9: Stack-based (absolute stack position)
 *   exprLoc == 0xA: SP-relative (offset from stack pointer)
 *
 * Conflict detection:
 *   If too many registers are active between current position and operand
 *   location, accessing the operand becomes complex. If < 4 active registers,
 *   set conflictMode to trigger register allocation.
 */
static void AnalyzeStackOperand(uint8_t exprIdx, uint8_t lrIdx) {
    uint8_t activeRegCount;   // Count of active registers preventing stack access
    uint16_t stackLevel;      // Current stack depth being examined

    // Handle SP-relative addressing
    if (exprLoc[lrIdx] == 0xA)
        calculatedOffset = tx2[exprIdx].right; // Offset stored in right field
    
    // Handle absolute stack position
    else if (exprLoc[lrIdx] == 9) {
        calculatedOffset = tx2[exprIdx].extra; // Stack position stored in extra field
        
        // Check if we need conflict resolution:
        // - If left operand register needs saving but not in expression
        // - If registers already saved
        // - If operand not at current stack depth
        if ((!registerInExpression[Left] && registerNeedsSave[Left]) || savedRegisterCount > 0 ||
            calculatedOffset != currentStackDepth) {
            
            // Count how many registers are "active" (need to be preserved)
            activeRegCount = savedRegisterCount + exprRegisterCount;
            
            // Count stack slots that contain values between operand and top
            for (stackLevel = calculatedOffset; stackLevel <= currentStackDepth; stackLevel++) {
                if (stackNodeContents[stackLevel] != 0)
                    activeRegCount++;
            }
            
            // If < 4 active registers, we have room to resolve conflict
            if (activeRegCount < 4)
                conflictMode = true; // Trigger register allocation
            else
                exprLoc[lrIdx] = 0xA; // Use SP-relative addressing instead
        }
        
        // Convert stack position to negative offset (stack grows downward)
        calculatedOffset = -(calculatedOffset * 2); // Each stack slot is 2 bytes
    }
} /* AnalyzeStackOperand() */

/**
 * FindOptimalRegister - Find best register for operand with minimum offset
 *
 * For operands requiring register loading, find the register that minimizes
 * the offset adjustment needed. This reduces instruction complexity.
 *
 * @param lrIdx: Left (0) or Right (1) operand index
 *
 * Strategy:
 *   1. Determine operand offset and storage class
 *   2. Scan all registers for one with matching storage class
 *   3. Calculate offset delta for each candidate register
 *   4. Select register with minimum offset adjustment
 *   5. Update register state to contain operand
 *
 * Operand types handled:
 *   - SP-relative (exprLoc == 0xA): Stack pointer offset
 *   - Memory label (exprLoc == 8, attr == 1): Symbol table lookup
 *   - Immediate indirect (exprLoc == 4, special attr): Constant pointer
 */
static void FindOptimalRegister(uint8_t lrIdx) {
    uint16_t operandOffset;   // Offset of operand in memory/stack
    uint16_t storageFlags;    // Storage class of operand (stack/memory/register)
    uint8_t minDistance;      // Minimum offset distance found so far
    uint8_t targetAttr;       // Target attribute for register
    uint16_t offsetDelta;     // Difference between register offset and operand offset

    // Case 1: SP-relative operand (offset from stack pointer)
    if (exprLoc[lrIdx] == 0xA) {
        operandOffset = calculatedOffset;
        storageFlags  = 0x100;              // Stack storage class
        minDistance   = 4;                  // Maximum acceptable distance
        targetAttr    = IndirectAddr(exprAttr[lrIdx]); // Convert to indirect type
    } 
    // Case 2: Memory label operand (symbol address)
    else if (exprLoc[lrIdx] == 8 && exprAttr[lrIdx] == 1) {
        GetOperandValue(curExprLoc[lrIdx], &operandOffset, &storageFlags);
        minDistance = 2;
        targetAttr  = ADDRESS_A;            // Address/pointer type
    } 
    // Case 3: Immediate indirect operand (constant pointer)
    else if (exprLoc[lrIdx] == 4 &&
               (exprAttr[lrIdx] == 0 || exprAttr[lrIdx] == 8 || !CanUseRegisterDirect(lrIdx))) {
        GetOperandValue(curExprLoc[lrIdx], &operandOffset, &storageFlags);
        minDistance = 2;
        targetAttr  = IndirectAddr(exprAttr[lrIdx]); // Convert to indirect
    } 
    else
        return; // No optimization needed for this operand type

    // Scan candidate registers (DE=2, HL=3; BC=1 rarely used for indirect)
    for (int regIdx = 1; regIdx < 4; regIdx++) {
        // Special case: if both operands are same expression and not assignment
        if (registerInExpression[regIdx]) {
            if (curExprLoc[Left] == curExprLoc[Right] && curNodeType != T2_COLONEQUALS)
                if (exprLoc[lrIdx] > 3)
                    exprLoc[lrIdx] = regIdx; // Reuse the expression register
        } 
        // Check if register is available and has compatible state
        else if (!registerNeedsSave[regIdx] &&
                   registerState[regIdx].storageClass == storageFlags &&
                   registerState[regIdx].isDirect && 
                   1 <= registerState[regIdx].dataType &&
                   registerState[regIdx].dataType <= 6) {
            
            // Calculate how far register's current offset is from operand
            offsetDelta =
                registerState[regIdx].stackOffset + registerState[regIdx].offset - operandOffset;
            
            // Convert to absolute distance
            if (offsetDelta > 0xff)
                offsetDelta = -offsetDelta;
            
            // Track register with minimum distance
            if (offsetDelta < minDistance) {
                exprLoc[lrIdx] = regIdx;
                minDistance    = (uint8_t)offsetDelta;
            }
        }
    }
    
    // If we found a suitable register, update its state
    if (exprLoc[lrIdx] <= 3) {
        int regIdx                     = exprLoc[lrIdx];
        
        // Set register type and contents
        registerState[regIdx].dataType = exprAttr[lrIdx] = targetAttr;
        registerState[regIdx].contents                   = curExprLoc[lrIdx];
        
        // Adjust offset to minimize distance to operand
        registerState[regIdx].offset =
            registerState[regIdx].stackOffset + registerState[regIdx].offset - operandOffset;
        registerState[regIdx].stackOffset = operandOffset;
    }
} /* FindOptimalRegister() */

/**
 * AnalyzeOperandLocations - Main operand analysis pass
 *
 * Analyzes both left and right operands to determine their current location
 * and optimal access method. Orchestrates all operand location strategies.
 *
 * Algorithm:
 *   Phase 1: Determine current operand locations
 *     - Check for special types (stack pointer, local label)
 *     - Get location and attribute from TX2 node
 *     - Try reusing existing register (direct)
 *     - Try reusing register-indirect
 *
 *   Phase 2: Optimize operand access
 *     - Analyze register usage conflicts
 *     - Handle stack operand offsets
 *     - Find optimal register for loading
 *     - Update expression lookup tables
 *
 * Side effects:
 *   - Updates exprLoc[Left/Right]: Where operands are
 *   - Updates exprAttr[Left/Right]: How to access them
 *   - Updates registerState: Register allocation info
 *   - May set conflictMode: Triggers register allocation
 */
static void AnalyzeOperandLocations() {
    uint8_t exprIdx, lrIdx;
    
    // Initialize: assume both operands in memory
    exprLoc[Left]  = 8;
    exprLoc[Right] = 8;
    
    // === Phase 1: Determine current operand locations ===
    for (lrIdx = Left; lrIdx <= Right; lrIdx++) {
        // Get expression index for this operand
        if ((exprIdx = curExprLoc[lrIdx]) == 0)
            // No operand (unary operation or missing)
            exprAttr[lrIdx] = 0xC; // Uninitialized/missing
        
        else if ((operandNodeType = tx2[exprIdx].type) == T2_STACKPTR)
            // Stack pointer special handling
            exprAttr[lrIdx] = 0xA; // SP-relative addressing
        
        else if (operandNodeType == T2_LOCALLABEL)
            // Local label reference
            exprAttr[lrIdx] = 9; // Label type
        
        else {
            // Normal operand - get stored location and attribute
            exprAttr[lrIdx] = tx2[exprIdx].exprAttr;
            exprLoc[lrIdx]  = tx2[exprIdx].exprLoc;
            
            // Try optimization strategies
            TryUseExistingRegister(exprIdx, lrIdx); // Reuse register with value?
            TryUseIndirectRegister(exprIdx, lrIdx); // Use register-indirect?
        }
    }
    
    // === Phase 2: Optimize operand access ===
    for (lrIdx = Left; lrIdx <= Right; lrIdx++) {
        exprIdx = curExprLoc[lrIdx];
        
        AnalyzeRegisterUsage();          // Check register availability/conflicts
        AnalyzeStackOperand(exprIdx, lrIdx); // Handle stack operands
        FindOptimalRegister(lrIdx);      // Find best register for loading
        UpdateExpressionLookup(lrIdx);   // Update lookup tables
    }
} /* AnalyzeOperandLocations() */

/**
 * GetTypeConversion - Get type conversion cost for operand
 *
 * Determines what type conversion is needed to convert operand to the type
 * required by the current fragment.
 *
 * @param lrIdx: Left (0) or Right (1) operand index
 * @return: Type conversion index (used to look up cost in exprComplexity[])
 *
 * Special cases:
 *   - curExprLoc 0 or 1: Return conversion index 1
 *   - Otherwise: Look up in typeConversionTable based on:
 *       • Current operand type (iCodeArgsIndex[lrIdx])
 *       • Required fragment operand type (GetFragmentOperandType)
 */
static uint8_t GetTypeConversion(uint8_t lrIdx) {
    // Special case: operand is 0 or 1 (simple constant or flag)
    if (curExprLoc[lrIdx] == 0 || curExprLoc[lrIdx] == 1)
        return 1; // Simple type conversion
    
    // Look up conversion needed from current type to fragment's required type
    return typeConversionTable[iCodeArgsIndex[lrIdx]][GetFragmentOperandType(lrIdx)];
} /* GetTypeConversion() */

/**
 * SetOperandCosts - Copy cost metrics from source to operand
 *
 * Updates operand cost arrays with values from expression complexity analysis.
 * These costs guide fragment selection.
 *
 * @param lrIdx: Left (0) or Right (1) operand index
 * @param srcIdx: Source index in expr arrays to copy from
 */
static void SetOperandCosts(uint8_t lrIdx, uint8_t srcIdx) {
    operandComplexity[lrIdx]   = exprComplexity[srcIdx];      // Code size cost
    operandRegisterCost[lrIdx] = exprRegisterCost[srcIdx];    // Register pressure
    operandCategory[lrIdx]     = nodeExprCategory[srcIdx];    // Expression category
} /* SetOperandCosts() */

/**
 * SelectOptimalFragment - Core optimization: select best code fragment
 *
 * Iterates through all valid fragments for this operation and selects the one
 * with minimum total cost. This is the heart of the code generator's optimizer.
 *
 * Cost calculation:
 *   totalCost = leftConversionCost + rightConversionCost + fragmentCodeSize
 *
 * Algorithm:
 *   1. Initialize minCost to maximum (198)
 *   2. For each fragment in valid range [first..last]:
 *      a. Get type conversion cost for left operand
 *      b. Get type conversion cost for right operand
 *      c. Get fragment code length (lower 5 bits of fragmentCodeLength)
 *      d. Calculate totalCost = sum of all costs
 *      e. If totalCost < minCost, this fragment is new best
 *   3. Set operand costs based on best fragment's type conversions
 *
 * Side effects:
 *   - Sets cfrag1: Selected fragment code
 *   - Sets selectedOperatorIdx: Operator encoding index
 *   - Sets operandFragmentType[Left/Right]: Required operand types
 *   - Sets operandComplexity/Cost: Operand cost metrics
 */
static void SelectOptimalFragment() {
    uint8_t bestLeftType, bestRightType; // Best type conversions found
    uint8_t minCost;                     // Minimum cost found so far
    uint8_t leftConvType, rightConvType; // Type conversions for current fragment
    uint8_t totalCost;                   // Total cost of current fragment
    
    bestLeftType = bestRightType = 0; // Initialize (avoid compiler warning)
    minCost                      = 198; // Start with very high cost

    // === Iterate through all valid fragments ===
    for (currentFragmentCode = first; currentFragmentCode <= last; currentFragmentCode++) {
        // Calculate cost components for this fragment:
        
        // 1. Type conversion cost for left operand
        leftConvType  = GetTypeConversion(Left);
        
        // 2. Type conversion cost for right operand
        rightConvType = GetTypeConversion(Right);
        
        // 3. Total cost = conversions + code size
        //    fragmentCodeLength lower 5 bits = code size in bytes
        totalCost     = exprComplexity[leftConvType] +      // Left conversion cost
                        exprComplexity[rightConvType] +     // Right conversion cost
                        (fragmentCodeLength[opcodeEncodingMap[currentFragmentCode]] & 0x1f); // Code size
        
        // If this fragment is better than previous best, select it
        if (totalCost < minCost) {
            minCost                    = totalCost;
            bestLeftType               = leftConvType;
            bestRightType              = rightConvType;
            
            // Record selected fragment info
            cfrag1                     = opcodeEncodingMap[currentFragmentCode];
            selectedOperatorIdx        = fragmentOpcodeIndex[currentFragmentCode];
            
            // Record required operand types for this fragment
            operandFragmentType[Left]  = GetFragmentOperandType(Left);
            operandFragmentType[Right] = GetFragmentOperandType(Right);
        }
    }
    
    // Set operand costs based on best fragment's requirements
    SetOperandCosts(Left, bestLeftType);
    SetOperandCosts(Right, bestRightType);
} /* SelectOptimalFragment() */

/**
 * AdjustOperandOffsets - Normalize register offsets for operands
 *
 * After operands are in registers, adjust their offsets to zero if needed.
 * This ensures registers point exactly at the operand data.
 *
 * Side effects:
 *   - Calls AdjustRegisterOffset for each non-zero operand
 *   - May generate offset adjustment code
 */
static void AdjustOperandOffsets() {
    // Adjust left operand register offset if present
    if (curExprLoc[Left] != 0)
        AdjustRegisterOffset(exprLoc[Left]);

    // Adjust right operand register offset if present
    if (curExprLoc[Right] != 0)
        AdjustRegisterOffset(exprLoc[Right]);
} /* AdjustOperandOffsets() */

/**
 * HandleSpecialStatements - Handle special statement types needing custom logic
 *
 * Some statement types require special handling beyond normal expression code
 * generation (e.g., calls, returns, stack adjustments).
 *
 * Statement types handled:
 *   - T2_STKARG/STKBARG/STKWARG: Push argument on stack
 *   - T2_CALL: Procedure call
 *   - T2_CALLVAR: Variable (indirect) call
 *   - T2_RETURN/RETURNBYTE/RETURNWORD: Return from procedure
 *   - T2_JMPFALSE: Conditional jump (may optimize comparison)
 *   - T2_CASEBLOCK: Case statement
 *   - T2_MOVE: Memory move operation
 *
 * Side effects:
 *   - Updates currentStackDepth: Runtime stack depth
 *   - Updates stackUsage: Maximum stack usage tracking
 *   - Updates callStackBase: Call frame base pointer
 *   - May generate return sequence code
 *   - May modify cfrag1: Selected fragment code
 *   - May set nextReturnState: Return generation flag
 */
static void HandleSpecialStatements() {
    uint16_t p; // Temporary for stack size calculation

    // === Stack argument push operations ===
    if (curNodeType == T2_STKARG || curNodeType == T2_STKBARG || curNodeType == T2_STKWARG) {
        // Adjust stack pointer for additional argument
        AdjustStackOnReturn(-(callStackBase[procCallDepth] * 2));
        callStackBase[procCallDepth]++; // Increment argument count
        currentStackDepth++;            // Track stack depth
    } 
    
    // === Procedure call ===
    else if (curNodeType == T2_CALL) {
        // Adjust stack for call frame
        AdjustStackOnReturn(-(callStackBase[procCallDepth] * 2));
        
        // Get procedure info
        info = FromIdx(tx2[tx2qp].extra);
        
        // Calculate stack usage for this call
        if ((info->flag & F_EXTERNAL))
            // External procedure: only arguments on stack
            p = (callStackBase[procCallDepth] + 1) * 2;
        else
            // Internal procedure: arguments + local variables + callee's stack
            p = (callStackDepth[procCallDepth] + 1) * 2 + info->stackUsage;
        
        // Update maximum stack usage tracking
        if (p > stackUsage)
            stackUsage = p;
    } 
    
    // === Variable (indirect) procedure call ===
    else if (curNodeType == T2_CALLVAR) {
        // Adjust stack for call frame
        AdjustStackOnReturn(-(callStackBase[procCallDepth] * 2));
        
        // Update maximum stack usage (conservative estimate)
        if (stackUsage < currentStackDepth * 2)
            stackUsage = currentStackDepth * 2;
    } 
    
    // === Return statement ===
    else if (curNodeType == T2_RETURN || curNodeType == T2_RETURNBYTE ||
               curNodeType == T2_RETURNWORD) {
        nextReturnState = true;         // Flag: return being generated
        GenerateReturnSequence();       // Generate return code
    } 
    
    // === Conditional jump (may have optimized comparison) ===
    else if (curNodeType == T2_JMPFALSE) {
        AdjustStackOnReturn(0);
        
        // If comparison was inverted for optimization, change jump to JMPTRUE
        if (invertComparison) {
            cfrag1 = CF_JMPTRUE;        // Change fragment to jump-if-true
            invertComparison = false;   // Reset flag
        }
    } 
    
    // === Case statement ===
    else if (curNodeType == T2_CASEBLOCK)
        AdjustStackOnReturn(0);
    
    // === Memory move operation ===
    else if (curNodeType == T2_MOVE) {
        // If stack depth changed since call, need to adjust
        if (callStackBase[procCallDepth] != currentStackDepth) {
            AdjustStackOnReturn(-((callStackBase[procCallDepth] + 1) * 2));
            PopRegisterFromStack(3); // Pop HL from stack
        }
        
        // Optimize: if HL has label, use optimized MOVE fragment
        if (registerState[3].dataType == 1)
            cfrag1 = CF_MOVE_HL;
    }
} /* HandleSpecialStatements() */

/**
 * InvalidateTargetRegisters - Invalidate registers containing assignment target
 *
 * When assigning to a variable, any registers containing direct (non-pointer)
 * values of that variable become invalid. This prevents stale data reuse.
 *
 * Algorithm:
 *   - Scan all registers (0-3 = BC, DE, HL; IR_H = temp registers)
 *   - If register contains left operand (assignment target)
 *   - And register type is direct value (0, 1, 6, 7)
 *   - Then invalidate register (set contents = 0)
 *
 * Notes:
 *   - Types 2-5 are indirect/pointer types, which remain valid
 *   - Only direct value registers are invalidated
 */
static void InvalidateTargetRegisters() {
    // Scan all register slots including temporaries
    for (int regIdx = 0; regIdx <= IR_H; regIdx++) {
        // Check if register contains assignment target
        if (registerState[regIdx].contents == curExprLoc[Left])
            // If direct value type (not indirect), invalidate
            if (registerState[regIdx].dataType < 2 || 5 < registerState[regIdx].dataType)
                registerState[regIdx].contents = 0; // Mark register as invalid
    }
}

/**
 * UpdateRegisterState - Update register state after operation completes
 *
 * This is the final phase of code generation: updating register tracking to
 * reflect the result of the operation just generated.
 *
 * Algorithm:
 *   1. Extract operator precedence and type rules
 *   2. Process temporary register slots (IR_SAVPSW to IR_SAVH)
 *      according to rules:
 *      - Rule 0-3: Copy from physical register
 *      - Rule 4: New result value
 *      - Rule 5: Special stack offset value
 *      - Other: Invalidate register
 *   3. Apply precedence adjustments (increment offset, make indirect)
 *   4. Handle special cases (assignment invalidation)
 *   5. Copy temporary registers to physical registers
 *
 * Register state fields updated:
 *   - contents: Which expression value is in register
 *   - dataType: Type of value (BYTE_A, ADDRESS_A, etc.)
 *   - offset: Offset adjustment from base
 *   - isDirect: Direct value vs indirect/pointer
 *   - storageClass: Stack/memory/register
 *   - stackOffset: Stack frame offset
 *
 * Side effects:
 *   - Updates registerState[]: Full register state
 *   - Updates tx2[].exprAttr: Result expression attribute
 *   - May invalidate registers after assignment
 */
static void UpdateRegisterState() {
    // Extract operator rules from lookup tables
    uint16_t precedenceBits = operatorPrecedence[selectedOperatorIdx] * 16;
    uint16_t typeRuleBits   = operatorTypeRules[selectedOperatorIdx];
    uint8_t resultRegIdx    = 0; // Which register holds result
    
    // === Special case: Assignment statement ===
    if (curNodeType == T2_COLONEQUALS) {
        // Invalidate any direct values of assignment target
        InvalidateTargetRegisters();
        
        // Optimization: Track assignment value in register for reuse
        if (tx2[curExprLoc[Right]].cnt == 0)          // Right operand no longer needed
            if (tx2[curExprLoc[Left]].cnt > 0) {       // Left target still used
                // Track byte assignment in register
                if (cfrag1 == CF_MOVMLR || cfrag1 == CF_STA) {
                    registerState[exprLoc[Right]].dataType = BYTE_A;
                    registerState[exprLoc[Right]].contents = curExprLoc[Left];
                } 
                // Track word/address assignment in register
                else if (cfrag1 == CF_SHLD || cfrag1 == CF_MOVMRP) {
                    registerState[exprLoc[Right]].dataType = ADDRESS_A;
                    registerState[exprLoc[Right]].contents = curExprLoc[Left];
                }
            }
    } 
    // === Special case: Some operations invalidate target ===
    else if (T2_51 <= curNodeType && curNodeType <= T2_56)
        InvalidateTargetRegisters();
    
    // === Process each temporary register slot ===
    // Temporary slots (IR_SAVPSW to IR_SAVH) hold intermediate results
    // before being copied to physical registers
    for (int tmpRegIdx = IR_SAVPSW; tmpRegIdx <= IR_SAVH; tmpRegIdx++) {
        // Extract rule for this register from packed bit fields
        uint8_t precedenceRule = precedenceBits >> 13; // Top 3 bits
        uint8_t ruleValue      = typeRuleBits >> 12;   // Top 4 bits
        precedenceBits <<= 3;  // Shift to next register's rule
        typeRuleBits <<= 4;
        
        // === Apply type rule ===
        if (ruleValue <= 3) {
            // Rule 0-3: Copy state from physical register
            registerState[tmpRegIdx] = registerState[ruleValue];
            
            // Apply precedence adjustment
            if (precedenceRule == 1)
                registerState[tmpRegIdx].offset++; // Increment offset
            else if (precedenceRule == 2) {
                // Make indirect: convert value to address
                if (registerState[tmpRegIdx].dataType == BYTE_A) {
                    registerState[tmpRegIdx].dataType = 6; // Byte pointer
                } else {
                    registerState[tmpRegIdx].dataType = ADDRESS_A; // Word pointer
                    registerState[tmpRegIdx].isDirect = false;
                }
            }
        } 
        else if (ruleValue == 4) {
            // Rule 4: New result value
            registerState[resultRegIdx = tmpRegIdx].isDirect = false;
            
            // If expression still referenced, track result
            if (0 < tx2[tx2qp].cnt) {
                registerState[tmpRegIdx].contents = tx2qp; // Expression in this register
                registerState[tmpRegIdx].dataType = tx2[tx2qp].exprAttr =
                    fragmentCodeLength[cfrag1] >> 5; // Result type from fragment
                registerState[tmpRegIdx].offset = 0;
            } else
                registerState[tmpRegIdx].contents = 0; // Result not needed
        } 
        else if (ruleValue == 5) {
            // Rule 5: Special stack offset value
            registerState[tmpRegIdx].contents     = 0;
            registerState[tmpRegIdx].storageClass = 0;
            registerState[tmpRegIdx].offset       = 0;
            registerState[tmpRegIdx].isDirect     = true;
            registerState[tmpRegIdx].dataType     = BYTE_A;
            registerState[tmpRegIdx].stackOffset  = precedenceRule; // Stack offset in precedence field
        } 
        else {
            // Other rules: Invalidate register
            registerState[tmpRegIdx].contents = 0;
            registerState[tmpRegIdx].isDirect = false;
        }
    }
    
    // === Handle orphaned result ===
    // If no register marked as result (resultRegIdx == 0) but expression still used
    if (resultRegIdx == 0 && tx2[tx2qp].cnt > 0) {
        // Find an empty indirect register to hold result
        for (int tmpRegIdx = IR_SAVPSW; tmpRegIdx <= IR_SAVH; tmpRegIdx++) {
            if (registerState[tmpRegIdx].contents == 0)
                if (!registerState[resultRegIdx = tmpRegIdx].isDirect)
                    break; // Found empty indirect slot
        }
        
        // If found slot, mark it as holding result
        if (resultRegIdx != 0) {
            registerState[resultRegIdx].contents = tx2qp;
            registerState[resultRegIdx].isDirect = false;
            registerState[resultRegIdx].dataType = BYTE_A; // Default to byte
            tx2[tx2qp].exprAttr                  = BYTE_A;
            registerState[resultRegIdx].offset   = 0;
        }
    }
    
    // === Copy temporary registers to physical registers ===
    // Registers 0-3 (BC, -, DE, HL) get updated from temporaries 5-8
    for (int n = 0; n <= IR_H; n++)
        registerState[n] = registerState[n + 5]; // Copy temp slot to physical
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
    // === 1. Extract operands from TX2 node ===
    curExprLoc[Left]  = (uint8_t)tx2[tx2qp].left;  // Left operand expression index
    curExprLoc[Right] = (uint8_t)tx2[tx2qp].right; // Right operand expression index
    
    // === 2. Determine valid fragment range for this operation ===
    first             = nodeTypeToAttribute[curNodeType];      // First valid fragment
    last              = first + operatorResultType[curNodeType] - 1; // Last valid fragment

    // === 3. Analyze where operands currently are ===
    AnalyzeOperandLocations();

    // === 4. Iterative optimization loop ===
    // Keep trying until operands are in optimal locations for code generation
    while (1) {
        // Select best code fragment based on current operand state
        SelectOptimalFragment();
        
        // If both operands ready (complexity == 0), we're done optimizing
        if (operandComplexity[Left] == 0 && operandComplexity[Right] == 0)
            break; // Ready for code generation
        
        // Operands not optimal - generate code to load them
        if (conflictMode)
            AllocateRegister();     // Resolve register conflicts
        else
            GenerateOperandCode();  // Generate operand loading code
    }
    
    // === 5. Finalize operand register offsets ===
    AdjustOperandOffsets();
    
    // === 6. Update reference counts ===
    DecrementExprRefs(); // Decrement usage count for operands
    
    // === 7. Invalidate registers as specified by operator ===
    // Upper 4 bits of operatorPrecedence specify register invalidation mask
    InvalidateRegistersByMask(operatorPrecedence[selectedOperatorIdx] >> 12);
    
    // === 8. Handle special statement types ===
    HandleSpecialStatements();
    
    // === 9. Generate the actual machine code ===
    GenerateFragmentCode(); // Emit selected fragment code
    
    // === 10. Update register state with result ===
    UpdateRegisterState(); // Track which registers now hold what values
} /* GenerateExpressionCode() */

/**
 * c_procedure - Handle procedure entry and initialization
 *
 * Processes T2_PROCEDURE nodes to set up new procedure scope.
 * Manages block structure, symbol information, and entry code generation.
 *
 * Algorithm:
 *   1. Enter new block scope
 *   2. Save current code size and stack state
 *   3. Link block into procedure chain
 *   4. Initialize new procedure context
 *   5. Emit procedure entry code
 *
 * Side effects:
 *   - Updates blk[] array: Block structure
 *   - Updates blkId: Current block ID
 *   - Updates codeSize: Reset for new procedure
 *   - Updates curExtProcId: Current procedure ID
 *   - Generates procedure entry code
 *
 * Called from: GenerateStatementCode() for T2_PROCEDURE nodes
 */
void c_procedure() {
    // Attempt to enter new block scope
    if (EnterBlk()) {
        // Save state from previous block
        blk[blkId].codeSize    = codeSize;           // Code size of previous block
        blk[blkId].wB4B0       = currentStackDepth;  // Stack depth at entry
        blk[blkId].stackSize   = stackUsage;         // Maximum stack used
        blk[blkId].extProcId   = curExtProcId;       // Previous procedure ID
        
        // Link new block into procedure chain
        blk[activeGrpCnt].next = blkId;              // Previous block -> new block
        blkId                  = activeGrpCnt;       // Update current block ID
        
        // Get procedure symbol info and initialize new procedure context
        info = blk[activeGrpCnt].info = FromIdx(tx2[tx2qp].left); // Procedure symbol
        curExtProcId                  = info->procId;              // New procedure ID
        
        // Reset code generation state for new procedure
        codeSize                      = 0;           // Start counting from 0
        
        // Generate procedure entry code
        EmitTopItem();                               // Emit procedure header
        GenerateProcedureEntry();                   // Generate entry sequence (stack frame setup)
    }
}
