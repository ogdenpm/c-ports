/****************************************************************************
 *  plm2e.c: part of the C port of Intel's ISIS-II plm80                    *
 *  The original ISIS-II application is Copyright Intel                     *
 *                                                                          *
 *  Re-engineered to C by Mark Ogden <mark.pm.ogden@btinternet.com>         *
 *                                                                          *
 *  It is released for academic interest and personal use only              *
 ****************************************************************************/
#include "os.h"
#include "plm.h"
#include <stdio.h>

static uint8_t b7A81[4] = { 0x3C, 0x46, 0x50, 0x5A };

static uint8_t bC28B[4], bC28F;

static void ExchangeDEHL() {
    EncodeFragData(CF_XCHG);
    codeSize++;
    CopyRegisterState(3, 4);
    CopyRegisterState(2, 3);
    CopyRegisterState(4, 2);
}

static void ExchangeHLStack() {
    iCodeArgs[0] = 0xa;
    iCodeArgs[1] = currentStackDepth;
    EncodeFragData(CF_XTHL);
    codeSize++;
    RestoreRegisterFromStack(4);
    SaveRegisterToStack(3);
    CopyRegisterState(4, 3);
}

static void AdjustRegisterPriority(uint8_t slot) {
    if (slot <= 3 && bC28B[slot] < 0xc8)
        bC28B[slot] += bC28F;
}


static uint8_t bC294, curSide, otherSide, bC297, bC298;

static bool IsRegisterUsedByOther(uint8_t slot) {
    return slot <= 3 && slot == exprLoc[otherSide];
}


/**
 * AllocateRegister - Allocate optimal register pair for expression evaluation
 *
 * Core register allocation function that selects the best available register pair
 * for loading an operand value. Uses a priority-based scoring system to choose
 * between register pairs B, D, H, and the stack, then handles register spilling
 * and restoration as needed.
 *
 * The 8080 has three 16-bit register pairs available for expression evaluation:
 * - BC (registers B,C) - Index 0 (IR_B)
 * - DE (registers D,E) - Index 2 (IR_D)
 * - HL (registers H,L) - Index 3 (IR_H)
 * - Stack (pseudo-register) - Index 4
 *
 * Algorithm:
 *
 * 1. Initialize Priority Scores (bC28B array):
 *    For each register pair (slots 0-3):
 *    - If register needs to be saved (registerNeedsSave) OR is in current expression (registerInExpression):
 *      • Set score = 0xC8 (unavailable - register is busy)
 *    - Otherwise:
 *      • Base score from b7A81 table: [0x3C, 0x46, 0x50, 0x5A]
 *      • Add penalty 0xF if register contains a value (boC060)
 *      • Lower scores = higher priority for allocation
 *
 * 2. Special Restrictions:
 *    - If stack top is not empty AND ACC register is active:
 *      • Mark BC (slot 0) as unavailable (0xC8)
 *      • Prevents conflicts with accumulator operations
 *
 * 3. Adjust Priorities Based on Operand Requirements:
 *    For each expression side (Left, Right):
 *    - If operand is on stack AND matches stack top value:
 *      • High penalty (0xCE) - strongly prefer this for efficiency
 *      • Set k = side, clear boC1D8 flag
 *    - Otherwise:
 *      • Normal penalty (0x32)
 *    - Apply penalties to registers that would be used by this operand
 *      (determined via b52B5 and b4C2D lookup tables)
 *
 * 4. Select Best Register:
 *    - Find register with lowest score (n)
 *    - If all registers busy (n == 0xC8):
 *      • Need to spill a register to stack
 *      • If HL is in use: Exchange DE↔HL, update expression locations
 *      • Exchange HL with stack top (XTHL-like operation)
 *      • Use HL (slot 3) as the allocated register
 *    - Otherwise:
 *      • Pop selected register from stack (restores previous value)
 *
 * 5. Post-Allocation Cleanup:
 *    - If register attribute is 0xB (special marker):
 *      • Clear to 0
 *      • If not ACC (slot 0) and contains value:
 *        » Generate MOV L,R or MOV H,R to consolidate register contents
 *
 * 6. Update Expression Tracking:
 *    - If not in conflict mode (boC1D8):
 *      • Update expression location and attribute for allocated side
 *      • Handle case where other side references same stack value
 *      • Update lookup codes via Sub_61A9()
 *
 * Register Priority (from b7A81 base scores):
 * - BC: 0x3C (highest priority - preferred for allocation)
 * - DE: 0x46
 * - HL: 0x50
 * - Stack: 0x5A (lowest priority - avoided when possible)
 *
 * The penalty system ensures:
 * - Registers containing needed values aren't overwritten
 * - Registers already in use for current expression are protected
 * - Stack-based operands are efficiently loaded into optimal registers
 * - Register spilling occurs only when all registers are busy
 *
 * Side effects:
 * - May spill register to stack (if all registers busy)
 * - May restore register from stack (if register was previously saved)
 * - Updates expression locations (exprLoc, exprAttr)
 * - Updates register tracking (registerDataType, registerContents, etc.)
 * - Generates code fragments (XCHG, XTHL, MOV, POP)
 * - Updates code size counter
 *
 * Called from: Expression evaluation when a register is needed for operand loading
 */
void AllocateRegister() {
    uint8_t j, k;

    // Step 1: Analyze current register usage to determine what's available
    AnalyzeRegisterUsage();

    // Step 2: Initialize priority scores for all register pairs
    for (uint8_t slot = 0; slot <= 3; slot++) {
        // If register needs to be saved OR is already in current expression
        if (registerNeedsSave[slot] || registerInExpression[slot])
            bC28B[slot] = 0xc8; // Mark as unavailable (score = 0xC8)
        else
            // Base score + penalty if register contains a value
            // b7A81: [0x3C=BC, 0x46=DE, 0x50=HL, 0x5A=stack]
            // Lower score = higher priority
            bC28B[slot] = b7A81[slot] + (registerHasValue[slot] ? 0xf : 0);
    }

    // Step 3: Special restriction - prevent BC conflicts with accumulator
    // If stack top is not empty AND ACC register is active
    if ((stackRegisterAttrs[currentStackDepth] >> 4) != 0xb && stackNodeContents[currentStackDepth] != 0)
        bC28B[0] = 0xC8; // Mark BC as unavailable

    // Step 4: Adjust priorities based on operand requirements
    for (uint8_t side = Left; side <= Right; side++) {
        if (curExprLoc[side] != 0) {
            // Check if operand is on stack AND matches stack top value
            if (exprLoc[side] == 9 && stackNodeContents[currentStackDepth] == curExprLoc[side]) {
                k      = side;  // Remember which side this is
                bC28F  = 0xce;  // High penalty - strongly prefer stack value
                boC1D8 = false; // Clear conflict flag
            } else
                bC28F = 0x32; // Normal penalty for other cases

            // Apply penalties to registers that would be used by this operand
            // b52B5: maps operand preference to register
            AdjustRegisterPriority(lookupResultLoc[operandFragmentType[side]]);
            // b4C2D: maps operation code to target register (upper 5 bits)
            AdjustRegisterPriority(loadOpTargetReg[operandCategory[side]] >> 5);
        }
    }

    // Step 5: Find register with lowest score (best candidate)
    uint8_t n = 0xc8; // Start with "no register available" value
    for (uint8_t slot = 0; slot <= 3; slot++) {
        if (bC28B[slot] <= n)
            n = bC28B[j = slot]; // j = best register found so far
    }

    // Step 6: Handle register allocation or spilling
    if (n == 0xC8) {
        // All registers busy - need to spill one to stack

        // If HL is in use by an expression, exchange it with DE first
        if (registerInExpression[3]) {
            ExchangeDEHL(); // Generate XCHG instruction

            // Update expression tracking: whichever side was using HL now uses DE
            if (exprLoc[0] == 3) {
                exprLoc[0] = 2; // Left side now in DE
                UpdateExpressionLookup(0);    // Update lookup codes
            } else {
                exprLoc[1] = 2; // Right side now in DE
                UpdateExpressionLookup(1);    // Update lookup codes
            }
        }

        // Exchange HL with stack top (XTHL-like operation)
        ExchangeHLStack();
        j = 3; // Use HL as the allocated register

    } else {
        // Found an available register - restore it from stack if needed
        PopRegisterFromStack(j); // Pop register from stack
    }

    // Step 7: Post-allocation cleanup
    if (registerDataType[j] == 0xb) {
        // Special marker 0xB indicates register needs consolidation
        registerDataType[j] = 0; // Clear the marker

        // If not ACC (slot 0) and register contains a value
        if (j != 0 && registerContents[j] != 0) {
            iCodeArgs[0] = j;
            EncodeFragData(CF_MOVLRHR); // Generate MOV L,R or MOV H,R
            codeSize++;
        }
    }

    // Step 8: Update expression tracking for the allocated register
    if (!boC1D8) { // If not in conflict mode
        // Check if other side references the same stack value
        if (exprLoc[1 - k] == 9) {
            if (curExprLoc[1 - k] == curExprLoc[k]) {
                // Both sides reference same stack value
                exprLoc[1 - k]  = j;        // Update other side location
                exprAttr[1 - k] = registerDataType[j]; // Update other side attribute
                UpdateExpressionLookup(1 - k);            // Update lookup codes
            } else {
                // Different stack values - mark conflict
                boC1D8 = true;
            }
        }

        // Update current side's expression tracking
        exprLoc[k]  = j;        // Set location to allocated register
        exprAttr[k] = registerDataType[j]; // Set attribute from register
        UpdateExpressionLookup(k);            // Update lookup codes
    }
}


/**
 * DetermineOperandOrder - Determine which operand to process first and set operation code
 *
 * Analyzes both operands (Left and Right) to determine the optimal processing order
 * for a binary operation. This function detects register conflicts between operands
 * and adjusts priorities to ensure operands are loaded in the correct sequence,
 * preventing the second operand from overwriting the first.
 *
 * The function performs two main tasks:
 * 1. Detect and penalize register conflicts between operands
 * 2. Select which operand side to process first based on adjusted priorities
 *
 * Algorithm:
 *
 * Phase 1: Register Conflict Detection and Priority Adjustment
 *
 * For each operand side (Left, Right):
 *   - If operand exists (curExprLoc[side] != 0) AND has priority (operandRegisterCost[side] != 0):
 *     • Calculate other side: otherSide = 1 - curSide
 *     • If other side also has priority (operandRegisterCost[otherSide] != 0):
 *       » Check if current operand would use registers needed by other operand
 *       » Two potential conflict registers checked (via lookup tables):
 *         - b4C2D[operandCategory[curSide]] >> 5: Primary target register
 *         - b52B5[operandFragmentType[curSide]]: Secondary register
 *       » If either register conflicts (IsRegisterUsedByOther):
 *         · Increase other side's priority: operandRegisterCost[otherSide] += 10
 *         · Forces other side to be processed FIRST to avoid overwriting
 *
 * Register conflict example:
 *   Left operand needs register D, Right operand currently in register D
 *   → Increase Right's priority by 10
 *   → Right will be processed first (saved/moved)
 *   → Then Left can safely use register D
 *
 * Phase 2: Operand Selection
 *
 * - Compare final priorities: curSide = operandRegisterCost[0] > operandRegisterCost[1] ? Left : Right
 *   • Higher priority value = process FIRST
 *   • Base priorities come from expression complexity/cost
 *   • Conflict penalties (+10) ensure correct ordering
 *
 * - Calculate otherSide: otherSide = Right - curSide
 *   • Simple arithmetic: if curSide=0 (Left), otherSide=1 (Right)
 *   •                    if curSide=1 (Right), otherSide=0 (Left)
 *
 * - Set operation code: bC298 = operandCategory[curSide]
 *   • Lookup operation code for the selected operand
 *   • This code drives subsequent operand loading logic
 *
 * Priority System:
 *
 * Base priorities (from operandRegisterCost array):
 * - Determined by expression complexity, register usage, stack depth
 * - Higher values = more important to process first
 *
 * Conflict penalty (+10):
 * - Added when operand would overwrite another operand's register
 * - Ensures victim operand is saved/processed before being overwritten
 * - Large enough to override most base priority differences
 *
 * Register conflict scenarios:
 *
 * Example 1: Simple conflict
 *   Left: needs D, Right: currently in D, operandRegisterCost[0]=5, operandRegisterCost[1]=3
 *   → Detect conflict: IsRegisterUsedByOther(D) for Left returns true
 *   → Adjust: operandRegisterCost[1] = 3 + 10 = 13
 *   → Select: curSide = (5 > 13) ? Left : Right = Right
 *   → Right processed first (saved/moved), then Left can use D
 *
 * Example 2: No conflict
 *   Left: needs D, Right: in H, operandRegisterCost[0]=5, operandRegisterCost[1]=3
 *   → No conflict detected
 *   → Select: curSide = (5 > 3) ? Left : Right = Left
 *   → Left processed first (higher base priority)
 *
 * Example 3: Commutative operation
 *   Left: in D, Right: in H, operandRegisterCost[0]=4, operandRegisterCost[1]=4
 *   → No conflict (different registers)
 *   → Select: curSide = (4 > 4) ? Left : Right = Right
 *   → Either order works, Right chosen (tie-breaker)
 *
 * Lookup table usage:
 * - operandCategory[]: Maps operand side to operation code
 * - operandFragmentType[]: Maps operand side to register preference code
 * - b4C2D[]: Maps operation code to target register
 * - b52B5[]: Maps preference code to register
 *
 * @global curSide - Output: Which operand to process (Left=0, Right=1)
 * @global otherSide - Output: Other operand (Right=1, Left=0)
 * @global bC298 - Output: Operation code for selected operand
 * @global operandRegisterCost[2] - Input/Modified: Base priorities for Left and Right operands
 * @global curExprLoc[2] - Input: TX2 node indices for both operands
 * @global exprLoc[2] - Input: Current register locations for both operands
 * @global operandCategory[2] - Input: Operation codes for both operands
 * @global operandFragmentType[2] - Input: Register preference codes for both operands
 *
 * Side effects:
 * - Modifies operandRegisterCost[otherSide] if register conflict detected
 * - Sets curSide, otherSide, and bC298 global variables
 * - No code generation - only determines processing order
 *
 * Called from: GenerateOperandCode() at start of operand code generation
 */
static void DetermineOperandOrder() {
    // Phase 1: Detect register conflicts and adjust priorities
    for (curSide = Left; curSide <= Right; curSide++) {
        // Check if current operand exists and has priority
        if (curExprLoc[curSide] != 0 && operandRegisterCost[curSide] != 0) {
            // Check if other operand also exists
            if (operandRegisterCost[otherSide = 1 - curSide] != 0) {
                // Check if current operand would conflict with other operand's registers
                // Two potential conflict points checked:
                // 1. Primary target register: b4C2D[operandCategory[curSide]] >> 5
                // 2. Secondary register: b52B5[operandFragmentType[curSide]]
                if (IsRegisterUsedByOther(loadOpTargetReg[operandCategory[curSide]] >> 5) ||
                    IsRegisterUsedByOther(lookupResultLoc[operandFragmentType[curSide]])) {
                    // Register conflict detected!
                    // Increase other operand's priority by 10 to process it first
                    operandRegisterCost[otherSide] = 10 + operandRegisterCost[otherSide];
                }
            }
        }
    }

    // Phase 2: Select which operand to process first
    // Higher priority value = process first
    curSide   = operandRegisterCost[0] > operandRegisterCost[1] ? Left : Right;
    otherSide = Right - curSide; // Calculate opposite side
    bC298     = operandCategory[curSide];  // Get operation code for selected side
}

/**
 * AdjustOperation - Adjust operation code based on register availability and context
 *
 * Modifies the operation code (bC298) and target register (bC294) based on
 * register availability and operand characteristics. This function handles
 * special cases where the initial operation code needs adjustment to work
 * around register conflicts or to select more efficient instruction sequences.
 *
 * Operation-specific adjustments:
 *
 * 1. Operation 0x13 → 0x15 (HL register conflict)
 *    Condition: HL register (slot 3) needs to be saved OR is in current expression
 *    - If registerNeedsSave[3] (HL needs save) OR registerInExpression[3] (HL in use):
 *      • Changes operation from 0x13 to 0x15
 *      • Selects alternative instruction sequence that doesn't require HL
 *      • Avoids register conflict by using different operation
 *
 *    Example: If HL is busy, use alternative addressing mode
 *
 * 2. Operation 0x14 (Register complement calculation)
 *    - Calculates complementary register: bC294 = 5 - bC297
 *      • Maps register pairs: BC↔HL, DE stays DE
 *      • Used for operations requiring specific register pairing
 *
 *    - If both operands in same register (exprLoc[0] == exprLoc[1]):
 *      • Further adjusts based on operand attribute:
 *        » If uint8_t attribute (exprAttr[curSide] == 0): Use operation 0x0E
 *        » Otherwise (address attribute): Use operation 0x11
 *      • Optimizes for commutative operations where operands can share register
 *
 *    Example: For "a + a" where both operands are in BC, select optimized form
 *
 * 3. Operation 0x08 (Symbol-based operation)
 *    Condition: Current operand has symbol information (tx2[...].left != 0)
 *    - If operand references a symbol:
 *      • Changes operation from 0x08 to 0x06
 *      • Selects symbol-aware instruction sequence
 *      • If target is BC (bC294 == 0): Switch to DE (bC294 = 1)
 *        » Avoids BC conflicts with symbol addressing
 *
 *    Example: Loading address of a variable - use symbol-relative addressing
 *
 * Register complement mapping (5 - register):
 * - bC297 = 0 (BC) → bC294 = 5 - 0 = 5 → (invalid, likely maps to HL in context)
 * - bC297 = 2 (DE) → bC294 = 5 - 2 = 3 (HL)
 * - bC297 = 3 (HL) → bC294 = 5 - 3 = 2 (DE)
 *
 * The adjustments ensure:
 * - Register conflicts are avoided before code generation
 * - More efficient instruction sequences are selected when possible
 * - Symbol-based operations use appropriate addressing modes
 * - Commutative operations are optimized when operands share registers
 *
 * @global bC298 - Operation code (input/output - modified by this function)
 * @global bC294 - Target register (output - set for operations 0x14 and 0x08)
 * @global bC297 - Current register holding operand
 * @global curSide - Current operand side being processed
 * @global registerNeedsSave[3] - HL register save requirement flag
 * @global registerInExpression[3] - HL register usage in expression flag
 * @global exprLoc - Expression locations (Left and Right)
 * @global exprAttr - Expression attributes (Left and Right)
 * @global curExprLoc - Current expression locations for both sides
 * @global tx2 - Expression tree (checked for symbol references)
 *
 * Side effects:
 * - Modifies bC298 (operation code) for cases 0x13, 0x14, 0x08
 * - Sets bC294 (target register) for cases 0x14, 0x08
 * - No code generation - only updates control variables
 *
 * Called from: GenerateOperandCode() after determining target register
 */
static void AdjustOperation() {
    if (bC298 == 0x13) {
        // Case 1: HL register conflict - use alternative operation
        if (registerNeedsSave[3] || registerInExpression[3])
            bC298 = 0x15; // Switch to operation that doesn't need HL

    } else if (bC298 == 0x14) {
        // Case 2: Register complement calculation
        bC294 = 5 - bC297; // Calculate complementary register

        // Optimize for same-register operands (commutative operations)
        if (exprLoc[0] == exprLoc[1])
            bC298 = exprAttr[curSide] == 0 ? 0xE : 0x11;

    } else if (bC298 == 8) {
        // Case 3: Symbol-based operation adjustment
        if (tx2[curExprLoc[curSide]].left != 0) {
            bC298 = 6; // Use symbol-aware operation

            // Avoid BC for symbol operations
            if (bC294 == 0)
                bC294 = 1; // Use DE instead
        }
    }
}
/**
 * SaveConflictingRegisters - Save registers that conflict with operand loading
 *
 * Preserves register values that would be overwritten during operand loading
 * by saving them to the stack or adjusting expression locations. Called before
 * loading an operand to ensure the other operand's value isn't lost.
 *
 * This function handles register conflicts that arise when:
 * - The target register (bC294) already contains a value needed by the other operand
 * - The current register (bC297) needs to be preserved for later use
 * - Register shuffling is required to make room for the new operand
 *
 * Operation-specific handling based on bC298 (operation code):
 *
 * 1. Operations 9-13: Stack offset adjustments
 *    - Calls Sub_63AC(bC297) to adjust stack-relative offsets
 *    - Ensures stack frame references remain valid after operand load
 *    - Applies when operation involves stack-based addressing
 *
 * 2. b4C2D[bC298] & 1 set: Target register needs preservation
 *    - If bC294 != bC297 (different registers):
 *      • Saves target register (bC294) to stack via Sub_5D6B()
 *      • Prevents losing existing value when loading new operand
 *
 *    - If bC294 == bC297 AND operation is 9-13:
 *      • Temporarily marks current side as stack-based (exprLoc[curSide] = 9)
 *      • Re-analyzes register usage with adjusted context
 *      • Saves register to stack
 *      • Restores current side expression location
 *      • Re-analyzes register usage again
 *      • Handles complex stack frame operations safely
 *
 * 3. Operation 0x15: Special HL preservation
 *    - If HL register (slot 3) is in current expression:
 *      • Marks other side as stack-based (exprLoc[otherSide] = 9)
 *      • Allows HL to be used without losing other operand
 *      • Used for operations requiring HL specifically
 *
 * 4. Operation 0x14: Register location swap
 *    - If other operand is in target register:
 *      • Updates other side to use current register instead
 *      • exprLoc[otherSide] = bC297
 *      • Prevents conflict by moving other operand reference
 *      • No actual code generation - just updates tracking
 *
 * Register conflict scenarios handled:
 *
 * Example 1: Loading into register containing other operand
 *   Before: exprLoc[Left] = D, exprLoc[Right] = H, need to load into H
 *   Action: Save H to stack, update Right to point to stack
 *
 * Example 2: Same register for both operands (commutative operation)
 *   Before: exprLoc[Left] = exprLoc[Right] = D
 *   Action: Temporarily mark one as stack-based, save, restore tracking
 *
 * Example 3: Register swap needed
 *   Before: exprLoc[Other] = target register
 *   Action: Update Other to use current register instead
 *
 * @global bC298 - Operation code (determines handling strategy)
 * @global bC294 - Target register for operand load
 * @global bC297 - Current register holding operand
 * @global curSide - Current operand side being processed
 * @global otherSide - Other operand side (not being processed)
 * @global registerInExpression[3] - HL register usage flag
 *
 * Side effects:
 * - May push registers to stack (via Sub_5D6B)
 * - May adjust stack offsets (via Sub_63AC)
 * - Updates expression locations (exprLoc array)
 * - Calls AnalyzeRegisterUsage() to refresh register state
 * - May generate PUSH instructions
 * - Updates stack depth and register tracking
 *
 * Called from: GenerateOperandCode() before encoding operand arguments
 */
static void SaveConflictingRegisters() {
    // Handle operations 9-13: adjust stack offsets
    if (9 <= bC298 && bC298 <= 13)
        AdjustRegisterOffset(bC297);

    // Check if target register needs preservation
    if (loadOpTargetReg[bC298] & 1) {
        if (bC294 != bC297) {
            // Different registers: save target to stack
            SaveOrRedirectRegister(bC294);
        } else if (9 <= bC298 && bC298 <= 13) {
            // Same register with stack operation: special handling
            // Temporarily mark as stack-based
            exprLoc[curSide] = 9;
            AnalyzeRegisterUsage();

            // Save register to stack
            SaveOrRedirectRegister(bC294);

            // Restore expression location
            exprLoc[curSide] = bC297;
            AnalyzeRegisterUsage();
        }
    } else if (bC298 == 0x15) {
        // Operation 0x15: preserve HL if in use
        if (registerInExpression[3])
            exprLoc[otherSide] = 9;
    } else if (bC298 == 0x14) {
        // Operation 0x14: swap register references
        if (exprLoc[otherSide] == bC294)
            exprLoc[otherSide] = bC297;
    }
}
/**
 * EncodeOperandArg - Encode a single operand argument into iCodeArgs array
 *
 * Encodes one argument for an operand load operation based on argument source
 * and encoding format. Called twice per operand to encode both source and
 * destination arguments. Advances iCodeArgsIndex  index as data is written to iCodeArgs array.
 *
 * Argument source encoding (arg1b):
 *
 * 0: Current register (bC297)
 *    - Encodes register holding current operand value
 *    - Used for source register in register-to-register operations
 *
 * 1: Target register (bC294)
 *    - Encodes target register for operation result
 *    - Used for destination register in load operations
 *
 * 2: Register location marker
 *    - Sets iCodeArgs[iCodeArgsIndex ] = LOC_REG
 *    - Advances iCodeArgsIndex  by 2
 *    - Indicates register-based operand
 *
 * 3: Stack location marker
 *    - Sets iCodeArgs[iCodeArgsIndex ] = LOC_STACK
 *    - Sets iCodeArgs[iCodeArgsIndex +1] = currentStackDepth (stack depth)
 *    - Advances iCodeArgsIndex  by 3
 *    - Indicates stack-based operand
 *
 * Other: Operand encoding
 *    - Calls Sub_636A(curSide) to encode current operand
 *    - Uses standard operand encoding
 *
 * Encoding format (arg2b):
 *
 * 0: No encoding
 *    - Returns immediately without adding to iCodeArgs
 *    - Used when argument is not needed
 *
 * 1 or 2: Direct register encoding
 *    - If arg1b == 0: Uses bC297 (current register)
 *    - Otherwise: Uses bC294 (target register)
 *    - Advances iCodeArgsIndex  by 1
 *
 * Other: Standard encoding
 *    - Follows arg1b-based strategy (cases 2, 3, or default)
 *
 * @param arg1b - Argument source/type (0-3, or other for operand)
 * @param arg2b - Encoding format (0 for none, 1-2 for direct register, other for standard)
 *
 * Side effects:
 * - Writes to iCodeArgs array at index iCodeArgsIndex 
 * - Advances iCodeArgsIndex  by 1-3 depending on encoding
 * - May call Sub_636A() for operand encoding
 *
 * Called from: GenerateOperandCode() to build operand argument list
 */
static void EncodeOperandArg(uint8_t argSource, uint8_t encodingFormat) {
    // Check if encoding is needed
    if (encodingFormat == 0)
        return;

    if (encodingFormat == 1 || encodingFormat == 2) {
        // Direct register encoding
        iCodeArgs[registersToSaveCount++] = argSource == 0 ? bC297 : bC294;
    } else if (argSource == 2) {
        // Register location marker
        iCodeArgs[registersToSaveCount] = LOC_REG;
        registersToSaveCount += 2;
    } else if (argSource == 3) {
        // Stack location marker
        iCodeArgs[registersToSaveCount] = LOC_STACK;
        iCodeArgs[registersToSaveCount + 1] = currentStackDepth;
        registersToSaveCount += 3;
    } else {
        // Standard operand encoding
        EncodeExpressionOperand(curSide);
    }
}

/**
 * UpdateRegisterState - Update register state tracking after operand load
 *
 * Updates register tracking data structures to reflect the value loaded into
 * the target register. Called before SetRegisterAttributes() to establish
 * register contents and offsets for subsequent code generation.
 *
 * State update strategy based on b4C15[bC298] >> 4:
 *
 * Case 0: Pop from stack
 *    - Restores register from stack via Sub_5C97(bC294)
 *    - Decrements stack depth: currentStackDepth--
 *    - Used when register value was previously saved
 *
 * Case 1: Exchange with stack top (XTHL-like)
 *    - Performs stack top exchange operation
 *    - Calls Sub_5C97(4), Sub_5C1D(3), Sub_5B96(4, 3)
 *    - Swaps register contents with stack top
 *    - Used for complex stack manipulations
 *
 * Case 2: Track stack-based operand
 *    - Records operand from stack location
 *    - Sets tracking flags:
 *      • registerIsDirect[bC294] = true (valid)
 *      • bC0A8[bC294] = 0 (no offset)
 *      • registerContents[bC294] = curExprLoc[curSide] (TX2 node)
 *      • wC096[bC294] = 0x100 (stack marker)
 *    - Computes stack offset (wC084[bC294]):
 *      • If iCodeArgs[0] == 0xA: -(iCodeArgs[1] * 2)
 *      • Special adjustment for bC298 == 5
 *      • Otherwise: iCodeArgs[3] - currentStackDepth * 2
 *
 * Case 3: Track value-based operand
 *    - Records operand value via GetOperandValue()
 *    - Sets tracking flags:
 *      • registerIsDirect[bC294] = true
 *      • bC0A8[bC294] = 0
 *      • registerContents[bC294] = curExprLoc[curSide]
 *    - Calls GetOperandValue() for value and storage flags
 *
 * Case 4: Track register-based operand
 *    - Records operand already in register
 *    - Sets tracking flags:
 *      • registerIsDirect[bC294] = 0 (direct register)
 *      • registerContents[bC294] = curExprLoc[curSide]
 *    - Handles offset for ADDRESS/ADDRESSIND (exprAttr 4 or 5):
 *      • Extracts offset from bC0C3[tx2[...].extra] & 0xf
 *      • Sign-extends if > 7: OR with 0xf0
 *      • Otherwise: Sets bC0A8[bC294] = 0
 *
 * Case 5: Copy register state
 *    - Copies state from bC297 to bC294
 *    - Calls Sub_5B96(bC297, bC294)
 *    - Used when moving value between registers
 *
 * Case 6: Rotate register state (3→4→2→3)
 *    - Performs circular rotation of register assignments
 *    - Sequence: Sub_5B96(3, 4), Sub_5B96(2, 3), Sub_5B96(4, 2)
 *    - Used for register shuffling operations
 *
 * Case 7: No state update
 *    - Leaves register state unchanged
 *    - Used when no tracking update needed
 *
 * Register state arrays updated:
 * - registerContents[]: TX2 node index (source of value)
 * - bC0A8[]: Register offset adjustment
 * - registerIsDirect[]: Validity/directness flag
 * - wC084[]: Base offset for stack/memory access
 * - wC096[]: Storage class flags
 *
 * @global bC298 - Operation code (index into b4C15 table)
 * @global bC294 - Target register being configured
 * @global curSide - Current operand side (Left/Right)
 * @global curExprLoc - TX2 node indices for both sides
 * @global exprAttr - Expression attributes for both sides
 * @global currentStackDepth - Current stack depth
 * @global iCodeArgs - Encoded operand arguments
 *
 * Side effects:
 * - Updates register tracking arrays (registerContents, bC0A8, registerIsDirect, wC084, wC096)
 * - May decrement currentStackDepth (case 0)
 * - May modify iCodeArgs[2] (case 2, special adjustment)
 * - May call Sub_5C97, Sub_5C1D, Sub_5B96 for register operations
 *
 * Called from: GenerateOperandCode() before SetRegisterAttributes()
 */
static void UpdateRegisterState() {
    switch (loadOpAttributes[bC298] >> 4) {
    case 0:
        // Pop from stack
        RestoreRegisterFromStack(bC294);
        currentStackDepth--;
        break;

    case 1:
        // Exchange with stack top
        RestoreRegisterFromStack(4);
        SaveRegisterToStack(3);
        CopyRegisterState(4, 3);
        break;

    case 2:
        // Track stack-based operand
        registerIsDirect[bC294] = true;
        registerOffset[bC294]  = 0;
        registerContents[bC294]  = curExprLoc[curSide];
        registerStorageClass[bC294]  = 0x100; // Stack marker

        if (iCodeArgs[0] == 0xA) {
            // Stack-relative addressing
            registerStackOffset[bC294] = -(iCodeArgs[1] * 2);

            // Special adjustment for operation 5
            if (stackRegisterAttrs[tx2[curExprLoc[curSide]].extra] == 0xb0)
                if (bC298 == 5) {
                    registerStackOffset[bC294]--;
                    iCodeArgs[2] = iCodeArgs[2] + 1;
                }
        } else {
            // Frame-relative addressing
            registerStackOffset[bC294] = iCodeArgs[3] - currentStackDepth * 2;
        }
        break;

    case 3:
        // Track value-based operand
        registerIsDirect[bC294] = true;
        registerOffset[bC294]  = 0;
        registerContents[bC294]  = curExprLoc[curSide];
        GetOperandValue(registerContents[bC294], &registerStackOffset[bC294], &registerStorageClass[bC294]);
        break;

    case 4:
        // Track register-based operand
        registerIsDirect[bC294] = 0;
        registerContents[bC294]  = curExprLoc[curSide];

        if (exprAttr[curSide] == 4 || exprAttr[curSide] == 5) {
            // ADDRESS or ADDRESSIND: extract offset
            registerOffset[bC294] = stackRegisterAttrs[tx2[curExprLoc[curSide]].extra] & 0xf;

            // Sign-extend if negative
            if (registerOffset[bC294] > 7)
                registerOffset[bC294] = registerOffset[bC294] | 0xf0;
        } else {
            // No offset
            registerOffset[bC294] = 0;
        }
        break;

    case 5:
        // Copy register state
        CopyRegisterState(bC297, bC294);
        break;

    case 6:
        // Rotate register state (3→4→2→3)
        CopyRegisterState(3, 4);
        CopyRegisterState(2, 3);
        CopyRegisterState(4, 2);
        break;

    case 7:
        // No state update needed
        break;
    }
}
/**
 * SetRegisterAttributes - Set data type attributes for target register after load
 *
 * Sets the registerDataType[bC294] attribute to reflect the data type and addressing mode
 * of the value loaded into the target register. Called after register state is
 * updated to complete the register allocation tracking.
 *
 * Attribute assignment based on b4C15[bC298] & 0xf:
 *
 * Case 0: No attribute change
 *    - Leaves registerDataType[bC294] unchanged
 *
 * Case 1: Fixed attribute (1)
 *    - Sets registerDataType[bC294] = 1
 *    - Specific data type indicator
 *
 * Case 2: Byte attribute (0)
 *    - Sets registerDataType[bC294] = 0
 *    - Indicates uint8_t-sized value
 *
 * Case 3: Pointer/Address attribute (6)
 *    - Sets registerDataType[bC294] = 6
 *    - Indicates pointer or address value
 *
 * Case 4: Computed indirect attribute
 *    - If exprLoc[curSide] != 8:
 *      • Computes: IndirectAddr(exprAttr[curSide])
 *      • Converts to indirect addressing mode
 *    - If exprLoc[curSide] == 8:
 *      • Uses 6 if exprAttr == 0
 *      • Otherwise uses exprAttr[curSide] directly
 *
 * Case 5: Derived with adjustment
 *    - Sets registerDataType[bC294] = exprAttr[curSide] - 2
 *    - Increments bC0A8[3]++ (register offset adjustment)
 *    - Used for specific addressing mode conversions
 *
 * The attribute values track:
 * - Data type (uint8_t vs. uint16_t)
 * - Addressing mode (direct vs. indirect)
 * - Pointer semantics
 *
 * @global bC298 - Operation code (index into b4C15 table)
 * @global bC294 - Target register being configured
 * @global curSide - Current operand side (Left/Right)
 * @global exprAttr - Expression attributes for both sides
 * @global exprLoc - Expression locations for both sides
 *
 * Side effects:
 * - Sets registerDataType[bC294] (register attribute)
 * - May increment bC0A8[3] (case 5 only)
 *
 * Called from: GenerateOperandCode() after Sub_8207() updates register state
 */
static void SetRegisterAttributes() {
    switch (loadOpAttributes[bC298] & 0xf) {
    case 0:
        // No attribute change
        break;

    case 1:
        // Fixed attribute
        registerDataType[bC294] = 1;
        break;

    case 2:
        // Byte attribute
        registerDataType[bC294] = 0;
        break;

    case 3:
        // Pointer/Address attribute
        registerDataType[bC294] = 6;
        break;

    case 4:
        // Computed indirect attribute
        if (exprLoc[curSide] != 8) {
            // Convert to indirect addressing mode
            registerDataType[bC294] = IndirectAddr(exprAttr[curSide]);
        } else if (exprAttr[curSide] == 0) {
            // Default to pointer attribute
            registerDataType[bC294] = 6;
        } else {
            // Use existing attribute
            registerDataType[bC294] = exprAttr[curSide];
        }
        break;

    case 5:
        // Derived with adjustment
        registerDataType[bC294] = exprAttr[curSide] - 2;
        registerOffset[3]++; // Adjust register offset
        break;
    }
}
/**
 * GenerateOperandCode - Generate code to load and transform operands into registers
 *
 * Central orchestrator for operand code generation. Analyzes operand requirements,
 * selects optimal registers, handles register conflicts, and emits code to load
 * operands from memory/stack into registers with appropriate transformations.
 *
 * Process:
 * 1. Analyze operand sides and priorities (Sub_7F19)
 *    - Determines which operand to process (curSide, otherSide)
 *    - Computes operation code (bC298) from operandCategory lookup
 *
 * 2. Handle special cases:
 *    - 0x17: Fatal error (ERR214)
 *    - 0x16: Update attributes without code generation
 *    - 0x12: Set boC1D8 flag
 *
 * 3. Normal operand loading (for other bC298 values):
 *    a) Determine target register (bC294):
 *       - From b4C2D table or b52B5 lookup
 *    b) Adjust operation if needed (Sub_7FFC):
 *       - Handle special cases (0x13, 0x14, 0x08)
 *    c) Save conflicting registers (Sub_8086):
 *       - Preserve values needed by other operand
 *    d) Encode fragment arguments (Sub_8148):
 *       - Build iCodeArgs array from operand info
 *    e) Update register state (Sub_8207, Sub_841E):
 *       - Track register contents and attributes
 *    f) Update expression state:
 *       - Set exprAttr[curSide], exprLoc[curSide]
 *       - Update bC0C1 lookup codes (Sub_61A9)
 *    g) Emit code fragment (EncodeFragData):
 *       - Generate actual machine code
 *
 * Register allocation strategy:
 * - Prefers registers not used by other operand
 * - Saves conflicting registers to stack if needed
 * - Tracks register attributes (uint8_t/address/indirect)
 * - Updates reference counts via AnalyzeRegisterUsage()
 *
 * @global bC298 - Operation code (from b4C15, b4C2D, b5012 tables)
 * @global bC294 - Target register for this operand
 * @global bC297 - Current register holding operand
 * @global curSide - Which operand (Left/Right)
 * @global otherSide - Other operand (Right/Left)
 *
 * Side effects:
 * - Modifies register allocation state (registerContents, registerDataType, bC0A8, etc.)
 * - Emits code fragments via EncodeFragData()
 * - Updates expression attributes and locations
 * - May save/restore registers to stack
 * - Updates code size counter
 * - May trigger fatal error (ERR214)
 *
 * Called from: Expression code generation for binary operations
 */
void GenerateOperandCode() {
    // Analyze operand sides and determine processing order
    DetermineOperandOrder();

    if (bC298 == 0x17) {
        // Fatal error case
        HandleFatalError(ERR214);
    } else if (bC298 == 0x16) {
        // Special case: update attributes only
        iCodeArgsIndex[curSide]    = operandFragmentType[curSide];
        exprAttr[curSide] = lookupResultAttr[iCodeArgsIndex[curSide]];
        exprLoc[curSide]  = lookupResultLoc[iCodeArgsIndex[curSide]];
    } else if (bC298 == 0x12) {
        // Special flag case
        boC1D8 = true;
    } else {
        // Normal operand loading

        // Determine target register
        bC294 = loadOpTargetReg[bC298] >> 5;
        if (bC294 > 3)
            bC294 = lookupResultLoc[operandFragmentType[curSide]];

        bC297 = exprLoc[curSide];
        AnalyzeRegisterUsage();

        // Adjust operation for special cases
        AdjustOperation();

        uint8_t frag = loadOpFragment[bC298];

        // Save conflicting registers
        SaveConflictingRegisters();

        // Build fragment arguments
        registersToSaveCount = 0;
        EncodeOperandArg((loadOpTargetReg[bC298] >> 3) & 3, (fragControlBits[frag] >> 4) & 7);
        EncodeOperandArg((loadOpTargetReg[bC298] >> 1) & 3, (fragControlBits[frag] >> 1) & 7);

        // Update register state
        UpdateRegisterState();
        SetRegisterAttributes();

        // Update expression state
        exprAttr[curSide] = registerDataType[bC294];
        exprLoc[curSide]  = bC294;
        UpdateExpressionLookup(Left);
        UpdateExpressionLookup(Right);

        // Emit code fragment
        EncodeFragData(frag);
        codeSize += (fragmentCodeLength[frag] & 0x1f);
    }
}

/**
 * GetOtherSide - Get the expression side NOT using the specified register
 *
 * Determines which expression side (Left=0 or Right=1) is NOT currently
 * using the specified register. Used during fragment argument encoding to
 * select the operand that should be encoded.
 *
 * Logic:
 * - If left side (exprLoc[0]) uses the register → return right side (1)
 * - Otherwise → return left side (0)
 *
 * This effectively returns the "other" side - whichever operand is NOT
 * currently allocated to the given register.
 *
 * @param reg - Register number to check (typically 0 or 3)
 * @return 0 (Left) if right side uses reg, 1 (Right) if left side uses reg
 *
 * Usage examples:
 * - GetOtherSide(3): Returns side NOT using register H (IR_H = 3)
 * - GetOtherSide(0): Returns side NOT using register B (IR_B = 0)
 *
 * Called from: EncodeFragmentArg() for operand side selection
 */
static uint8_t GetOtherSide(uint8_t reg) {
    return exprLoc[0] == reg ? 1 : 0;
}

/**
 * EncodeFragmentArg - Encode a single fragment argument into iCodeArgs array
 *
 * Encodes one argument for a code fragment based on argument type and encoding
 * format. Called twice per fragment to encode both source and destination operands.
 * Advances iCodeArgsIndex  index as data is written to iCodeArgs array.
 *
 * Argument type encoding (arg1b):
 *
 * 0: No argument
 *    - Returns immediately without encoding
 *
 * 1: Left operand (side 0)
 *    - Encodes data for left expression operand
 *
 * 2: Right operand (side 1)
 *    - Encodes data for right expression operand
 *
 * 3: Stack reference
 *    - Encodes stack location:
 *      • iCodeArgs[iCodeArgsIndex ]   = 0xA (stack marker)
 *      • iCodeArgs[iCodeArgsIndex +1] = currentStackDepth (stack depth)
 *      • Advances iCodeArgsIndex  by 3
 *
 * 4: Register check (register 3)
 *    - Determines side: exprLoc[0] == 3 ? side 1 : side 0
 *    - Then encodes based on arg2b format
 *
 * 5: Register check (register 0)
 *    - Determines side: exprLoc[0] == 0 ? side 1 : side 0
 *    - Then encodes based on arg2b format
 *
 * 6: Extra field
 *    - Special handling for tx2[tx2qp].extra:
 *      • If arg2b == 7: Direct encoding (0x10, value)
 *      • Otherwise: Via EncodeOperandInfo()
 *
 * Encoding format (arg2b):
 *
 * 0-3: Direct operand encoding
 *    - Calls Sub_636A(i) to encode operand at side i
 *
 * 4-7: Extended encoding
 *    - Sets iCodeArgs[iCodeArgsIndex ] = arg2b + 9
 *    - If arg2b == 6: Uses tx2[1].right
 *    - Otherwise: Calls GetOperandValue() for value
 *    - Advances iCodeArgsIndex  by 2
 *
 * @param arg1b - Argument type/source (0-6)
 * @param arg2b - Encoding format (0-7)
 *
 * Side effects:
 * - Writes to iCodeArgs array at index iCodeArgsIndex 
 * - Advances iCodeArgsIndex  by 1-3 depending on encoding
 * - May call Sub_636A(), EncodeOperandInfo(), or GetOperandValue()
 * - Exits program on invalid arg1b (>6)
 *
 * Called from: GenerateFragmentCode() to build fragment argument list
 */
static void EncodeFragmentArg(uint8_t argType, uint8_t encodingFormat) {
    uint8_t side;
    uint16_t value;

    switch (argType) {
    case 0:
        // No argument needed
        return;

    case 1:
        // Left operand
        side = 0;
        break;

    case 2:
        // Right operand
        side = 1;
        break;

    case 3:
        // Stack reference
        iCodeArgs[registersToSaveCount] = 0xA;   // Stack marker
        iCodeArgs[registersToSaveCount + 1] = currentStackDepth; // Stack depth
        registersToSaveCount += 3;
        return;

    case 4:
        // Determine side based on register 3 usage
        side = GetOtherSide(3);
        break;

    case 5:
        // Determine side based on register 0 usage
        side = GetOtherSide(0);
        break;

    case 6:
        // Extra field encoding
        if (encodingFormat == 7) {
            // Direct encoding
            iCodeArgs[registersToSaveCount] = 0x10;
            iCodeArgs[registersToSaveCount + 1] = tx2[tx2qp].extra;
            registersToSaveCount += 2;
        } else {
            // Via EncodeOperandInfo
            EncodeOperandInfo((uint8_t)tx2[tx2qp].extra);
        }
        return;

    default:
        // Invalid argument type
        fprintf(stderr, "Invalid argument type in EncodeFragmentArg: %d\n", argType);
        Exit(1);
    }

    // Encode operand based on format
    if (encodingFormat <= 3) {
        // Direct operand encoding
        EncodeExpressionOperand(side);
    } else {
        // Extended encoding format
        iCodeArgs[registersToSaveCount] = encodingFormat + 9;

        if (encodingFormat == 6) {
            // Use common subexpression reference
            iCodeArgs[registersToSaveCount + 1] = tx2[1].right;
        } else {
            // Get operand value
            GetOperandValue(curExprLoc[side], &iCodeArgs[registersToSaveCount + 1], &value);
        }

        registersToSaveCount += 2;
    }
}
/**
 * GenerateFragmentCode - Generate code for fragment with helper function support
 *
 * Generates machine code for a code fragment (cfrag1), handling argument encoding,
 * helper function tracking, and stack usage updates. Used for operations that
 * require runtime library support or complex instruction sequences.
 *
 * Process:
 * 1. Check fragment complexity (only processes cfrag1 > CF_3)
 * 2. Encode fragment arguments via Sub_8698():
 *    - First argument: b42F9[cfrag1] >> 4
 *    - Second argument: b42F9[cfrag1] & 0xf
 *    - Uses fragControl table for encoding parameters
 * 3. Handle special fragment adjustments:
 *    - CF_67, CF_68: Add 2 to last iCodeArgs entry
 * 4. Emit code fragment via EncodeFragData()
 * 5. Update code size counter
 * 6. Track helper function usage and stack requirements:
 *    - CF_DELAY: Mark helper 105 as used, update stack
 *    - CF_171+: Runtime helpers (indexed via b4128, b413B, helperMap)
 *      • Division/modulo (T2_SLASH, T2_MOD, T2_44): +2 words stack
 *      • Other helpers: +1 uint16_t stack
 *
 * Fragment categories:
 * - CF_0 to CF_3: Simple fragments (skipped by this function)
 * - CF_4 to CF_171: Complex operations (encoded with arguments)
 * - CF_DELAY (105): Delay helper function
 * - CF_174+: Runtime library helpers (arithmetic, etc.)
 *
 * Helper function tracking:
 * - Uses helperGroup[b4273[curNodeType]] to find helper group
 * - Indexes into helperMap[grp][i] to get helper ID
 * - Sets helperAddr[helperId] = 1 to mark as used
 * - Ensures helpers are linked into final executable
 *
 * Stack usage calculation:
 * - CF_DELAY: Requires 1 uint16_t on stack
 * - Division/Modulo helpers: Require 2 words on stack
 * - Other helpers: Require 1 uint16_t on stack
 * - Updates global stackUsage if increased
 *
 * @global cfrag1 - Fragment code to generate (from CF_* constants)
 * @global curNodeType - Current operation type (for helper selection)
 * @global currentStackDepth - Current stack depth
 * @global iCodeArgsIndex  - Index into iCodeArgs array (modified by Sub_8698)
 *
 * Side effects:
 * - Builds iCodeArgs array via Sub_8698()
 * - Emits code via EncodeFragData()
 * - Updates codeSize counter
 * - Marks helpers as used (helperAddr array)
 * - May increase stackUsage
 *
 * Called from: Code generation phase for complex operations
 */
void GenerateFragmentCode() {
    // Only process complex fragments
    if (cfrag1 > CF_3) {
        // Encode fragment arguments
        registersToSaveCount = 0;
        EncodeFragmentArg(fragmentArgTypes[cfrag1] >> 4, (fragControlBits[cfrag1] >> 4) & 7);

        // Special adjustments for specific fragments
        if (cfrag1 == CF_67 || cfrag1 == CF_68)
            iCodeArgs[registersToSaveCount - 1] += 2;

        EncodeFragmentArg(fragmentArgTypes[cfrag1] & 0xf, (fragControlBits[cfrag1] >> 1) & 7);

        // Emit code fragment
        EncodeFragData(cfrag1);
        codeSize += (fragmentCodeLength[cfrag1] & 0x1f);

        // Track helper function usage and stack requirements
        if (cfrag1 == CF_DELAY) {
            // DELAY helper function
            helperAddr[105] = 1; // Mark helper 105 as used

            // Ensure stack has room for DELAY call
            if (stackUsage < (currentStackDepth + 1) * 2)
                stackUsage = (currentStackDepth + 1) * 2;

        } else if (cfrag1 > CF_171) {
            // Runtime library helper (arithmetic operations, etc.)

            // Look up helper ID using multiple indirection tables
            uint8_t i   = helperIndexMap[fragmentHelperIndex[cfrag1 - CF_174]];
            uint8_t grp = helperGroup[noteTypeToHelperGroup[curNodeType]];

            // Mark helper as used for linking
            helperAddr[helperMap[grp][i]] = 1;

            // Update stack usage based on helper requirements
            if (curNodeType == T2_SLASH || curNodeType == T2_MOD || curNodeType == T2_44) {
                // Division/modulo helpers need 2 words on stack
                if (stackUsage < (currentStackDepth + 2) * 2)
                    stackUsage = (currentStackDepth + 2) * 2;
            } else {
                // Other helpers need 1 uint16_t on stack
                if (stackUsage < (currentStackDepth + 1) * 2)
                    stackUsage = (currentStackDepth + 1) * 2;
            }
        }
    }
}