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

static uint8_t basePriorityScore[4] = { 0x3C, 0x46, 0x50, 0x5A };

static uint8_t priorityScore[4], penalty;

static void ExchangeDEHL() {
    EncodeFragData(CF_XCHG);
    codeSize++;
    registerState[IR_TMP] = registerState[IR_H];
    registerState[IR_H] = registerState[IR_D];
    registerState[IR_D] = registerState[IR_TMP];
}

static void ExchangeHLStack() {
    iCodeArgs[0] = 0xa;
    iCodeArgs[1] = currentStackDepth;
    EncodeFragData(CF_XTHL);
    codeSize++;
    RestoreRegisterFromStack(IR_TMP);
    SaveRegisterToStack(IR_H);
    registerState[IR_H] = registerState[IR_TMP];
}

static void AdjustRegisterPriority(uint8_t slot) {
    if (slot <= IR_H && priorityScore[slot] < 200)
        priorityScore[slot] += penalty;
}


static uint8_t targetRegister;     // bC294 - Target register for operand load
static uint8_t currentOperandSide; // curSide - Current operand being processed (Left=0/Right=1)
static uint8_t otherOperandSide;   // otherSide - Other operand (opposite of current)
static uint8_t currentRegister;    // bC297 - Current register holding operand
static uint8_t operationCode;      // bC298 - Operation code for load

static bool IsRegisterUsedByOther(uint8_t slot) {
    return slot <= 3 && slot == exprLoc[otherOperandSide];
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
 * 1. Initialize Priority Scores (priorityScore array):
 *    For each register pair (slots 0-3):
 *    - If register needs to be saved (registerNeedsSave) OR is in current expression (registerInExpression):
 *      • Set score = 200 (unavailable - register is busy)
 *    - Otherwise:
 *      • Base score from basePriorityScore table: [0x3C, 0x46, 0x50, 0x5A]
 *      • Add penalty 0xF if register contains a value (boC060)
 *      • Lower scores = higher priority for allocation
 *
 * 2. Special Restrictions:
 *    - If stack top is not empty AND ACC register is active:
 *      • Mark BC (regIdx 0) as unavailable (200)
 *      • Prevents conflicts with accumulator operations
 *
 * 3. Adjust Priorities Based on Operand Requirements:
 *    For each expression side (Left, Right):
 *    - If operand is on stack AND matches stack top value:
 *      • High penalty (0xCE) - strongly prefer this for efficiency
 *      • Set currentSide = side, clear conflictMode flag
 *    - Otherwise:
 *      • Normal penalty (0x32)
 *    - Apply penalties to registers that would be used by this operand
 *      (determined via b52B5 and b4C2D lookup tables)
 *
 * 4. Select Best Register:
 *    - Find register with lowest score (n)
 *    - If all registers busy (n == 200):
 *      • Need to spill a register to stack
 *      • If HL is in use: Exchange DE↔HL, update expression locations
 *      • Exchange HL with stack top (XTHL-like operation)
 *      • Use HL (regIdx 3) as the allocated register
 *    - Otherwise:
 *      • Pop selected register from stack (restores previous value)
 *
 * 5. Post-Allocation Cleanup:
 *    - If register attribute is 0xB (special marker):
 *      • Clear to 0
 *      • If not ACC (regIdx 0) and contains value:
 *        » Generate MOV L,R or MOV H,R to consolidate register contents
 *
 * 6. Update Expression Tracking:
 *    - If not in conflict mode (conflictMode):
 *      • Update expression location and attribute for allocated side
 *      • Handle case where other side references same stack value
 *      • Update lookup codes via Sub_61A9()
 *
 * Register Priority (from basePriorityScore base scores):
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
    uint8_t bestRegister, currentSide;

    // Step 1: Analyze current register usage to determine what's available
    AnalyzeRegisterUsage();

    // Step 2: Initialize priority scores for all register pairs
    for (uint8_t regIdx = IR_PSW; regIdx <= IR_H; regIdx++) {
        // If register needs to be saved OR is already in current expression
        if (registerNeedsSave[regIdx] || registerInExpression[regIdx])
            priorityScore[regIdx] = 200; // Mark as unavailable (score = 200
        else
            // Base score + penalty if register contains a value
            // basePriorityScore: [0x3C=BC, 0x46=DE, 0x50=HL, 0x5A=stack]
            // Lower score = higher priority
            priorityScore[regIdx] = basePriorityScore[regIdx] + (registerHasValue[regIdx] ? 0xf : 0);
    }

    // Step 3: Special restriction - prevent BC conflicts with accumulator
    // If stack top is not empty AND ACC register is active
    if ((stackRegisterAttrs[currentStackDepth] >> 4) != 0xb && stackNodeContents[currentStackDepth] != 0)
        priorityScore[IR_PSW] = 200; // Mark PSW as unavailable

    // Step 4: Adjust priorities based on operand requirements
    for (uint8_t side = Left; side <= Right; side++) {
        if (curExprLoc[side] != 0) {
            // Check if operand is on stack AND matches stack top value
            if (exprLoc[side] == 9 && stackNodeContents[currentStackDepth] == curExprLoc[side]) {
                currentSide      = side;  // Remember which side this is
                penalty  = 206;  // High penalty - strongly prefer stack value
                conflictMode = false; // operands match - no conflict
            } else
                penalty = 50; // Normal penalty for other cases

            // Apply penalties to registers that would be used by this operand
            // b52B5: maps operand preference to register
            AdjustRegisterPriority(lookupResultLoc[operandFragmentType[side]]);
            // b4C2D: maps operation code to target register (upper 5 bits)
            AdjustRegisterPriority(loadOpTargetReg[operandCategory[side]] >> 5);
        }
    }

    // Step 5: Find register with lowest score (best candidate)
    uint8_t n = 200; // Start with "no register available" value
    for (uint8_t regIdx = IR_PSW; regIdx <= IR_H; regIdx++) {
        if (priorityScore[regIdx] <= n)
            n = priorityScore[bestRegister = regIdx]; // best register found so far
    }

    // Step 6: Handle register allocation or spilling
    if (n == 200) {
        // All registers busy - need to spill one to stack

        // If HL is in use by an expression, exchange it with DE first
        if (registerInExpression[IR_H]) {
            ExchangeDEHL(); // Generate XCHG instruction

            // Update expression tracking: whichever side was using HL now uses DE
            if (exprLoc[Left] == IR_H) {
                exprLoc[Left] = IR_D; // Left side now in DE
                UpdateExpressionLookup(Left);    // Update lookup codes
            } else {
                exprLoc[Right] = IR_D; // Right side now in DE
                UpdateExpressionLookup(Right);    // Update lookup codes
            }
        }

        // Exchange HL with stack top (XTHL-like operation)
        ExchangeHLStack();
        bestRegister = IR_H; // Use HL as the allocated register

    } else {
        // Found an available register - restore it from stack if needed
        PopRegisterFromStack(bestRegister); // Pop register from stack
    }

    // Step 7: Post-allocation cleanup
    if (registerState[bestRegister].dataType == 0xb) {
        // Special marker 0xB indicates register needs consolidation
        registerState[bestRegister].dataType = 0; // Clear the marker

        // If not ACC (regIdx 0) and register contains a value
        if (bestRegister != 0 && registerState[bestRegister].contents != 0) {
            iCodeArgs[0] = bestRegister;
            EncodeFragData(CF_MOVLRHR); // Generate MOV L,R or MOV H,R
            codeSize++;
        }
    }

    // Step 8: Update expression tracking for the allocated register
    if (!conflictMode) { // If not in conflict mode
        // Check if other side references the same stack value
        if (exprLoc[1 - currentSide] == 9) {
            if (curExprLoc[1 - currentSide] == curExprLoc[currentSide]) {
                // Both sides reference same stack value
                exprLoc[1 - currentSide]  = bestRegister;        // Update other side location
                exprAttr[1 - currentSide] = registerState[bestRegister].dataType; // Update other side attribute
                UpdateExpressionLookup(1 - currentSide);            // Update lookup codes
            } else {
                // Different stack values - mark conflict
                conflictMode = true;
            }
        }

        // Update current side's expression tracking
        exprLoc[currentSide]  = bestRegister;        // Set location to allocated register
        exprAttr[currentSide] = registerState[bestRegister].dataType; // Set attribute from register
        UpdateExpressionLookup(currentSide);            // Update lookup codes
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
 *     • Calculate other side: otherOperandSide = 1 - curSide
 *     • If other side also has priority (operandRegisterCost[otherOperandSide] != 0):
 *       » Check if current operand would use registers needed by other operand
 *       » Two potential conflict registers checked (via lookup tables):
 *         - b4C2D[operandCategory[currentOperandSide]] >> 5: Primary target register
 *         - b52B5[operandFragmentType[currentOperandSide]]: Secondary register
 *       » If either register conflicts (IsRegisterUsedByOther):
 *         · Increase other side's priority: operandRegisterCost[otherOperandSide] += 10
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
 * - Compare final priorities: currentOperandSide = operandRegisterCost[0] > operandRegisterCost[1] ? Left : Right
 *   • Higher priority value = process FIRST
 *   • Base priorities come from expression complexity/cost
 *   • Conflict penalties (+10) ensure correct ordering
 *
 * - Calculate otherOperandSide: otherOperandSide = Right - currentOperandSide
 *   • Simple arithmetic: if currentOperandSide=0 (Left), otherOperandSide=1 (Right)
 *   •                    if currentOperandSide=1 (Right), otherOperandSide=0 (Left)
 *
 * - Set operation code: operationCode = operandCategory[currentOperandSide]
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
 *   → Select: currentOperandSide = (5 > 13) ? Left : Right = Right
 *   → Right processed first (saved/moved), then Left can use D
 *
 * Example 2: No conflict
 *   Left: needs D, Right: in H, operandRegisterCost[0]=5, operandRegisterCost[1]=3
 *   → No conflict detected
 *   → Select: currentOperandSide = (5 > 3) ? Left : Right = Left
 *   → Left processed first (higher base priority)
 *
 * Example 3: Commutative operation
 *   Left: in D, Right: in H, operandRegisterCost[0]=4, operandRegisterCost[1]=4
 *   → No conflict (different registers)
 *   → Select: currentOperandSide = (4 > 4) ? Left : Right = Right
 *   → Either order works, Right chosen (tie-breaker)
 *
 * Lookup table usage:
 * - operandCategory[]: Maps operand side to operation code
 * - operandFragmentType[]: Maps operand side to register preference code
 * - b4C2D[]: Maps operation code to target register
 * - b52B5[]: Maps preference code to register
 *
 * @global currentOperandSide - Output: Which operand to process (Left=0, Right=1)
 * @global otherOperandSide - Output: Other operand (Right=1, Left=0)
 * @global operationCode - Output: Operation code for selected operand
 * @global operandRegisterCost[2] - Input/Modified: Base priorities for Left and Right operands
 * @global curExprLoc[2] - Input: TX2 node indices for both operands
 * @global exprLoc[2] - Input: Current register locations for both operands
 * @global operandCategory[2] - Input: Operation codes for both operands
 * @global operandFragmentType[2] - Input: Register preference codes for both operands
 *
 * Side effects:
 * - Modifies operandRegisterCost[otherOperandSide] if register conflict detected
 * - Sets currentOperandSide, otherOperandSide, and operationCode global variables
 * - No code generation - only determines processing order
 *
 * Called from: GenerateOperandCode() at start of operand code generation
 */
static void DetermineOperandOrder() {
    // Phase 1: Detect register conflicts and adjust priorities
    for (currentOperandSide = Left; currentOperandSide <= Right; currentOperandSide++) {
        // Check if current operand exists and has priority
        if (curExprLoc[currentOperandSide] != 0 && operandRegisterCost[currentOperandSide] != 0) {
            // Check if other operand also exists
            if (operandRegisterCost[otherOperandSide = 1 - currentOperandSide] != 0) {
                // Check if current operand would conflict with other operand's registers
                // Two potential conflict points checked:
                // 1. Primary target register: b4C2D[operandCategory[currentOperandSide]] >> 5
                // 2. Secondary register: b52B5[operandFragmentType[currentOperandSide]]
                if (IsRegisterUsedByOther(loadOpTargetReg[operandCategory[currentOperandSide]] >> 5) ||
                    IsRegisterUsedByOther(lookupResultLoc[operandFragmentType[currentOperandSide]])) {
                    // Register conflict detected!
                    // Increase other operand's priority by 10 to process it first
                    operandRegisterCost[otherOperandSide] = 10 + operandRegisterCost[otherOperandSide];
                }
            }
        }
    }

    // Phase 2: Select which operand to process first
    // Higher priority value = process first
    currentOperandSide   = operandRegisterCost[0] > operandRegisterCost[1] ? Left : Right;
    otherOperandSide = 1 - currentOperandSide; // Calculate opposite side
    operationCode     = operandCategory[currentOperandSide];  // Get operation code for selected side
}

/**
 * AdjustOperation - Adjust operation code based on register availability and context
 *
 * Modifies the operation code (operationCode) and target register (targetRegister) based on
 * register availability and operand characteristics. This function handles
 * special cases where the initial operation code needs adjustment to work
 * around register conflicts or to select more efficient instruction sequences.
 *
 * Operation-specific adjustments:
 *
 * 1. Operation 0x13 → 0x15 (HL register conflict)
 *    Condition: HL register (regIdx 3) needs to be saved OR is in current expression
 *    - If registerNeedsSave[3] (HL needs save) OR registerInExpression[3] (HL in use):
 *      • Changes operation from 0x13 to 0x15
 *      • Selects alternative instruction sequence that doesn't require HL
 *      • Avoids register conflict by using different operation
 *
 *    Example: If HL is busy, use alternative addressing mode
 *
 * 2. Operation 0x14 (Register complement calculation)
 *    - Calculates complementary register: targetRegister = 5 - currentRegister
 *      • Maps register pairs: BC↔HL, DE stays DE
 *      • Used for operations requiring specific register pairing
 *
 *    - If both operands in same register (exprLoc[0] == exprLoc[1]):
 *      • Further adjusts based on operand attribute:
 *        » If uint8_t attribute (exprAttr[currentOperandSide] == 0): Use operation 0x0E
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
 *      • If target is BC (targetRegister == 0): Switch to DE (targetRegister = 1)
 *        » Avoids BC conflicts with symbol addressing
 *
 *    Example: Loading address of a variable - use symbol-relative addressing
 *
 * Register complement mapping (5 - register):
 * - currentRegister = 0 (BC) → targetRegister = 5 - 0 = 5 → (invalid, likely maps to HL in context)
 * - currentRegister = 2 (DE) → targetRegister = 5 - 2 = 3 (HL)
 * - currentRegister = 3 (HL) → targetRegister = 5 - 3 = 2 (DE)
 *
 * The adjustments ensure:
 * - Register conflicts are avoided before code generation
 * - More efficient instruction sequences are selected when possible
 * - Symbol-based operations use appropriate addressing modes
 * - Commutative operations are optimized when operands share registers
 *
 * @global operationCode - Operation code (input/output - modified by this function)
 * @global targetRegister - Target register (output - set for operations 0x14 and 0x08)
 * @global currentRegister - Current register holding operand
 * @global currentOperandSide - Current operand side being processed
 * @global registerNeedsSave[3] - HL register save requirement flag
 * @global registerInExpression[3] - HL register usage in expression flag
 * @global exprLoc - Expression locations (Left and Right)
 * @global exprAttr - Expression attributes (Left and Right)
 * @global curExprLoc - Current expression locations for both sides
 * @global tx2 - Expression tree (checked for symbol references)
 *
 * Side effects:
 * - Modifies operationCode (operation code) for cases 0x13, 0x14, 0x08
 * - Sets targetRegister (target register) for cases 0x14, 0x08
 * - No code generation - only updates control variables
 *
 * Called from: GenerateOperandCode() after determining target register
 */
static void AdjustOperation() {
    if (operationCode == 0x13) {
        // Case 1: HL register conflict - use alternative operation
        if (registerNeedsSave[3] || registerInExpression[3])
            operationCode = 0x15; // Switch to operation that doesn't need HL

    } else if (operationCode == 0x14) {
        // Case 2: Register complement calculation
        targetRegister = 5 - currentRegister; // Calculate complementary register

        // Optimize for same-register operands (commutative operations)
        if (exprLoc[0] == exprLoc[1])
            operationCode = exprAttr[currentOperandSide] == 0 ? 0xE : 0x11;

    } else if (operationCode == 8) {
        // Case 3: Symbol-based operation adjustment
        if (tx2[curExprLoc[currentOperandSide]].left != 0) {
            operationCode = 6; // Use symbol-aware operation

            // Avoid BC for symbol operations
            if (targetRegister == 0)
                targetRegister = 1; // Use DE instead
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
 * - The target register (targetRegister) already contains a value needed by the other operand
 * - The current register (currentRegister) needs to be preserved for later use
 * - Register shuffling is required to make room for the new operand
 *
 * Operation-specific handling based on operationCode (operation code):
 *
 * 1. Operations 9-13: Stack offset adjustments
 *    - Calls Sub_63AC(currentRegister) to adjust stack-relative offsets
 *    - Ensures stack frame references remain valid after operand load
 *    - Applies when operation involves stack-based addressing
 *
 * 2. b4C2D[operationCode] & 1 set: Target register needs preservation
 *    - If targetRegister != currentRegister (different registers):
 *      • Saves target register (targetRegister) to stack via Sub_5D6B()
 *      • Prevents losing existing value when loading new operand
 *
 *    - If targetRegister == currentRegister AND operation is 9-13:
 *      • Temporarily marks current side as stack-based (exprLoc[currentOperandSide] = 9)
 *      • Re-analyzes register usage with adjusted context
 *      • Saves register to stack
 *      • Restores current side expression location
 *      • Re-analyzes register usage again
 *      • Handles complex stack frame operations safely
 *
 * 3. Operation 0x15: Special HL preservation
 *    - If HL register (regIdx 3) is in current expression:
 *      • Marks other side as stack-based (exprLoc[otherOperandSide] = 9)
 *      • Allows HL to be used without losing other operand
 *      • Used for operations requiring HL specifically
 *
 * 4. Operation 0x14: Register location swap
 *    - If other operand is in target register:
 *      • Updates other side to use current register instead
 *      • exprLoc[otherOperandSide] = bC297
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
 * @global operationCode - Operation code (determines handling strategy)
 * @global targetRegister - Target register for operand load
 * @global currentRegister - Current register holding operand
 * @global currentOperandSide - Current operand side being processed
 * @global otherOperandSide - Other operand side (not being processed)
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
    // Handle stack frame operations 9-13: adjust stack offsets
    if (9 <= operationCode && operationCode <= 13)
        AdjustRegisterOffset(currentRegister);

    // Check if target register needs preservation
    if (loadOpTargetReg[operationCode] & 1) {
        if (targetRegister != currentRegister) {
            // Different registers: save target to stack
            SaveOrRedirectRegister(targetRegister);
        } else if (9 <= operationCode && operationCode <= 13) {
            // Same register with stack operation: special handling
            // Temporarily mark as stack-based
            exprLoc[currentOperandSide] = 9;
            AnalyzeRegisterUsage();

            // Save register to stack
            SaveOrRedirectRegister(targetRegister);

            // Restore expression location
            exprLoc[currentOperandSide] = currentRegister;
            AnalyzeRegisterUsage();
        }
    } else if (operationCode == 0x15) {
        // Operation 0x15: preserve HL if in use
        if (registerInExpression[3])
            exprLoc[otherOperandSide] = 9;
    } else if (operationCode == 0x14) {
        // Operation 0x14: swap register references
        if (exprLoc[otherOperandSide] == targetRegister)
            exprLoc[otherOperandSide] = currentRegister;
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
 * 0: Current register (currentRegister)
 *    - Encodes register holding current operand value
 *    - Used for source register in register-to-register operations
 *
 * 1: Target register (targetRegister)
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
 *    - Calls Sub_636A(currentOperandSide) to encode current operand
 *    - Uses standard operand encoding
 *
 * Encoding format (arg2b):
 *
 * 0: No encoding
 *    - Returns immediately without adding to iCodeArgs
 *    - Used when argument is not needed
 *
 * 1 or 2: Direct register encoding
 *    - If arg1b == 0: Uses currentRegister (current register)
 *    - Otherwise: Uses targetRegister (target register)
 *    - Advances iCodeArgsIndex  by 1
 *
 * Other: Standard encoding
 *    - Follows arg1b-based strategy (cases 2, 3, or default)
 *
 * @param argSource - Argument source/type (0-3, or other for operand)
 * @param encodingFormat - Encoding format (0 for none, 1-2 for direct register, other for standard)
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
        iCodeArgs[iCodeArgsIdx++] = argSource == 0 ? currentRegister : targetRegister;
    } else if (argSource == 2) {
        // Register location marker
        iCodeArgs[iCodeArgsIdx] = LOC_REG;
        iCodeArgsIdx += 2;
    } else if (argSource == 3) {
        // Stack location marker
        iCodeArgs[iCodeArgsIdx] = LOC_STACK;
        iCodeArgs[iCodeArgsIdx + 1] = currentStackDepth;
        iCodeArgsIdx += 3;
    } else {
        // Standard operand encoding
        EncodeExpressionOperand(currentOperandSide);
    }
}

/**
 * UpdateRegisterState - Update register state tracking after operand load
 *
 * Updates register tracking data structures to reflect the value loaded into
 * the target register. Called before SetRegisterAttributes() to establish
 * register contents and offsets for subsequent code generation.
 *
 * State update strategy based on loadOpAttributes[operationCode] >> 4:
 *
 * Case 0: Pop from stack
 *    - Restores register from stack via Sub_5C97(targetRegister)
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
 *      • registerState[targetRegister].isDirect = true (valid)
 *      • bC0A8[targetRegister] = 0 (no offset)
 *      • registerState[targetRegister].contents = curExprLoc[currentOperandSide] (TX2 node)
 *      • wC096[targetRegister] = 0x100 (stack marker)
 *    - Computes stack offset (wC084[targetRegister]):
 *      • If iCodeArgs[0] == 0xA: -(iCodeArgs[1] * 2)
 *      • Special adjustment for operationCode == 5
 *      • Otherwise: iCodeArgs[3] - currentStackDepth * 2
 *
 * Case 3: Track value-based operand
 *    - Records operand value via GetOperandValue()
 *    - Sets tracking flags:
 *      • registerState[targetRegister].isDirect = true
 *      • bC0A8[targetRegister] = 0
 *      • registerState[targetRegister].contents = curExprLoc[curSide]
 *    - Calls GetOperandValue() for value and storage flags
 *
 * Case 4: Track register-based operand
 *    - Records operand already in register
 *    - Sets tracking flags:
 *      • registerState[targetRegister].isDirect = false (direct register)
 *      • registerState[targetRegister].contents = curExprLoc[curSide]
 *    - Handles offset for ADDRESS/ADDRESSIND (exprAttr 4 or 5):
 *      • Extracts offset from bC0C3[tx2[...].extra] & 0xf
 *      • Sign-extends if > 7: OR with 0xf0
 *      • Otherwise: Sets bC0A8[targetRegister] = 0
 *
 * Case 5: Copy register state
 *    - Copies state from currentRegister to targetRegister
 *    - Calls Sub_5B96(currentRegister, targetRegister)
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
 * - registerState[].contents: TX2 node index (source of value)
 * - bC0A8[]: Register offset adjustment
 * - registerState[].isDirect: Validity/directness flag
 * - wC084[]: Base offset for stack/memory access
 * - wC096[]: Storage class flags
 *
 * @global operationCode - Operation code (index into b4C15 table)
 * @global targetRegister - Target register being configured
 * @global currentOperandSide - Current operand side (Left/Right)
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
    switch (loadOpAttributes[operationCode] >> 4) { // state
    case 0:
        // Pop from stack
        RestoreRegisterFromStack(targetRegister);
        currentStackDepth--;
        break;

    case 1:
        // Exchange with stack top
        RestoreRegisterFromStack(IR_TMP);
        SaveRegisterToStack(IR_H);
        registerState[IR_H] = registerState[IR_TMP];
        break;

    case 2:
        // Track stack-based operand
        registerState[targetRegister].isDirect = true;
        registerState[targetRegister].offset  = 0;
        registerState[targetRegister].contents  = curExprLoc[currentOperandSide];
        registerState[targetRegister].storageClass  = 0x100; // Stack marker

        if (iCodeArgs[0] == OP_STK) {
            // Stack-relative addressing
            registerState[targetRegister].stackOffset = -(iCodeArgs[1] * 2);

            // Special adjustment for operation 5
            if (stackRegisterAttrs[tx2[curExprLoc[currentOperandSide]].extra] == 0xb0)
                if (operationCode == 5) {
                    registerState[targetRegister].stackOffset--;
                    iCodeArgs[2] = iCodeArgs[2] + 1;
                }
        } else {
            // Frame-relative addressing
            registerState[targetRegister].stackOffset = iCodeArgs[3] - currentStackDepth * 2;
        }
        break;

    case 3:
        // Track value-based operand
        registerState[targetRegister].isDirect = true;
        registerState[targetRegister].offset  = 0;
        registerState[targetRegister].contents  = curExprLoc[currentOperandSide];
        GetOperandValue(registerState[targetRegister].contents, &registerState[targetRegister].stackOffset, &registerState[targetRegister].storageClass);
        break;

    case 4:
        // Track register-based operand
        registerState[targetRegister].isDirect = false;
        registerState[targetRegister].contents  = curExprLoc[currentOperandSide];

        if (exprAttr[currentOperandSide] == 4 || exprAttr[currentOperandSide] == 5) {
            // ADDRESS or ADDRESSIND: extract offset
            registerState[targetRegister].offset = stackRegisterAttrs[tx2[curExprLoc[currentOperandSide]].extra] & 0xf;

            // Sign-extend if negative
            if (registerState[targetRegister].offset > 7)
                registerState[targetRegister].offset = registerState[targetRegister].offset | 0xf0;
        } else {
            // No offset
            registerState[targetRegister].offset = 0;
        }
        break;

    case 5:
        // Copy register state
        registerState[targetRegister] = registerState[currentRegister];
        break;

    case 6:
        // Rotate register state swap DE & HL
        registerState[IR_TMP] = registerState[IR_H];
        registerState[IR_H] = registerState[IR_D];
        registerState[IR_D] = registerState[IR_TMP];
        break;

    case 7:
        // No state update needed
        break;
    }
}
/**
 * SetRegisterAttributes - Set data type attributes for target register after load
 *
 * Sets the registerState[targetRegister].dataType attribute to reflect the data type and addressing mode
 * of the value loaded into the target register. Called after register state is
 * updated to complete the register allocation tracking.
 *
 * Attribute assignment based on loadOpAttributes[operationCode] & 0xf:
 *
 * Case 0: No attribute change
 *    - Leaves registerState[targetRegister].dataType unchanged
 *
 * Case 1: Fixed attribute (1)
 *    - Sets registerState[targetRegister].dataType = 1
 *    - Specific data type indicator
 *
 * Case 2: Byte attribute (0)
 *    - Sets registerState[targetRegister].dataType = 0
 *    - Indicates uint8_t-sized value
 *
 * Case 3: Pointer/Address attribute (6)
 *    - Sets registerState[targetRegister].dataType = 6
 *    - Indicates pointer or address value
 *
 * Case 4: Computed indirect attribute
 *    - If exprLoc[currentOperandSide] != 8:
 *      • Computes: IndirectAddr(exprAttr[currentOperandSide])
 *      • Converts to indirect addressing mode
 *    - If exprLoc[currentOperandSide] == 8:
 *      • Uses 6 if exprAttr == 0
 *      • Otherwise uses exprAttr[currentOperandSide] directly
 *
 * Case 5: Derived with adjustment
 *    - Sets registerState[targetRegister].dataType = exprAttr[currentOperandSide] - 2
 *    - Increments bC0A8[3]++ (register offset adjustment)
 *    - Used for specific addressing mode conversions
 *
 * The attribute values track:
 * - Data type (uint8_t vs. uint16_t)
 * - Addressing mode (direct vs. indirect)
 * - Pointer semantics
 *
 * @global operationCode - Operation code (index into b4C15 table)
 * @global targetRegister - Target register being configured
 * @global currentOperandSide - Current operand side (Left/Right)
 * @global exprAttr - Expression attributes for both sides
 * @global exprLoc - Expression locations for both sides
 *
 * Side effects:
 * - Sets registerState[targetRegister].dataType (register attribute)
 * - May increment bC0A8[3] (case 5 only)
 *
 * Called from: GenerateOperandCode() after Sub_8207() updates register state
 */
static void SetRegisterAttributes() {
    switch (loadOpAttributes[operationCode] & 0xf) {
    case 0:
        // No attribute change
        break;

    case 1:
        // Fixed attribute
        registerState[targetRegister].dataType = 1;
        break;

    case 2:
        // Byte attribute
        registerState[targetRegister].dataType = 0;
        break;

    case 3:
        // Pointer/Address attribute
        registerState[targetRegister].dataType = 6;
        break;

    case 4:
        // Computed indirect attribute
        if (exprLoc[currentOperandSide] != 8) {
            // Convert to indirect addressing mode
            registerState[targetRegister].dataType = IndirectAddr(exprAttr[currentOperandSide]);
        } else if (exprAttr[currentOperandSide] == 0) {
            // Default to pointer attribute
            registerState[targetRegister].dataType = 6;
        } else {
            // Use existing attribute
            registerState[targetRegister].dataType = exprAttr[currentOperandSide];
        }
        break;

    case 5:
        // Derived with adjustment
        registerState[targetRegister].dataType = exprAttr[currentOperandSide] - 2;
        registerState[3].offset++; // Adjust register offset
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
 *    - Determines which operand to process (currentOperandSide, otherOperandSide)
 *    - Computes operation code (operationCode) from operandCategory lookup
 *
 * 2. Handle special cases:
 *    - 0x17: Fatal error (ERR214)
 *    - 0x16: Update attributes without code generation
 *    - 0x12: Set conflictMode flag
 *
 * 3. Normal operand loading (for other operationCode values):
 *    a) Determine target register (targetRegister):
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
 *       - Set exprAttr[currentOperandSide], exprLoc[currentOperandSide]
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
 * @global operationCode - Operation code (from b4C15, b4C2D, b5012 tables)
 * @global targetRegister - Target register for this operand
 * @global currentRegister - Current register holding operand
 * @global currentOperandSide - Which operand (Left/Right)
 * @global otherOperandSide - Other operand (Right/Left)
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

/*
GenerateOperandCode()
    ├─ DetermineOperandOrder()
    │   └─ operationCode = operandCategory[currentOperandSide]  [0-23]
    │
    ├─ Check special cases:
    │   ├─ 0x17 → HandleFatalError(ERR214)
    │   ├─ 0x16 → Update attributes only, return
    │   └─ 0x12 → Set conflictMode flag
    │
    └─ Normal path:
        ├─ AdjustOperation()
        │   ├─ 0x13 → 0x15 (if HL busy)
        │   ├─ 0x14 → 0x0E/0x11 (if commutative)
        │   └─ 0x08 → 0x06 (if symbol-based)
        │
        ├─ SaveConflictingRegisters()
        │   └─ Check: 9 <= operationCode <= 13 (stack ops)
        │
        ├─ UpdateRegisterState()
        │   └─ switch (loadOpAttributes[operationCode] >> 4)
        │
        └─ SetRegisterAttributes()
            └─ switch (loadOpAttributes[operationCode] & 0xf)
*/
void GenerateOperandCode() {
    // Analyze operand sides and determine processing order
    DetermineOperandOrder();

    if (operationCode == 0x17) {
        // Fatal error case
        HandleFatalError(ERR214);
    } else if (operationCode == 0x16) {
        // Special case: update attributes only
        iCodeArgsIndex[currentOperandSide]    = operandFragmentType[currentOperandSide];
        exprAttr[currentOperandSide] = lookupResultAttr[iCodeArgsIndex[currentOperandSide]];
        exprLoc[currentOperandSide]  = lookupResultLoc[iCodeArgsIndex[currentOperandSide]];
    } else if (operationCode == 0x12) {
        // Special flag case
        conflictMode = true;
    } else {
        // Normal operand loading

        // Determine target register
        targetRegister = loadOpTargetReg[operationCode] >> 5;
        if (targetRegister > 3)
            targetRegister = lookupResultLoc[operandFragmentType[currentOperandSide]];

        currentRegister = exprLoc[currentOperandSide];
        AnalyzeRegisterUsage();

        // Adjust operation for special cases
        AdjustOperation();

        uint8_t frag = loadOpFragment[operationCode];

        // Save conflicting registers
        SaveConflictingRegisters();

        // Build fragment arguments
        iCodeArgsIdx = 0;
        EncodeOperandArg((loadOpTargetReg[operationCode] >> 3) & 3, (fragControlBits[frag] >> 4) & 7);
        EncodeOperandArg((loadOpTargetReg[operationCode] >> 1) & 3, (fragControlBits[frag] >> 1) & 7);

        // Update register state
        UpdateRegisterState();
        SetRegisterAttributes();

        // Update expression state
        exprAttr[currentOperandSide] = registerState[targetRegister].dataType;
        exprLoc[currentOperandSide]  = targetRegister;
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
    return exprLoc[Left] == reg ? Right : Left;
}

/**
 * EncodeFragmentArg - Encode a single fragment argument into iCodeArgs array
 *
 * Encodes one argument for a code fragment based on argument type and encoding
 * format. Called twice per fragment to encode both source and destination operands.
 * Advances iCodeArgsIndex  index as data is written to iCodeArgs array.
 *
 * Argument type encoding (argType):
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
 *    - Determines side: exprLoc[Left] == IR_H ? Right : Left
 *    - Then encodes based on arg2b format
 *
 * 5: Register check (register 0)
 *    - Determines side: exprLoc[Left] == IR_PSW ? Right : Left
 *    - Then encodes based on arg2b format
 *
 * 6: Extra field
 *    - Special handling for tx2[tx2qp].extra:
 *      • If encodingFormat == 7: Direct encoding (0x10, value)
 *      • Otherwise: Via EncodeOperandInfo()
 *
 * Encoding format (encodingFormat):
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
        side = Left;
        break;

    case 2:
        // Right operand
        side = Right;
        break;

    case 3:
        // Stack reference
        iCodeArgs[iCodeArgsIdx] = OP_STK;   // Stack marker
        iCodeArgs[iCodeArgsIdx + 1] = currentStackDepth; // Stack depth
        iCodeArgsIdx += 3;
        return;

    case 4:
        // Determine side based on register HL usage
        side = GetOtherSide(IR_H);
        break;

    case 5:
        // Determine side based on register PSW usage
        side = GetOtherSide(IR_PSW);
        break;

    case 6:
        // Extra field encoding
        if (encodingFormat == 7) {
            // Direct encoding
            iCodeArgs[iCodeArgsIdx] = OP_DIRECT;
            iCodeArgs[iCodeArgsIdx + 1] = tx2[tx2qp].extra;
            iCodeArgsIdx += 2;
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
        iCodeArgs[iCodeArgsIdx] = encodingFormat + 9;

        if (encodingFormat == 6) {
            // Use common subexpression reference
            iCodeArgs[iCodeArgsIdx + 1] = tx2[1].right;
        } else {
            // Get operand value
            GetOperandValue(curExprLoc[side], &iCodeArgs[iCodeArgsIdx + 1], &value);
        }

        iCodeArgsIdx += 2;
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
        iCodeArgsIdx = 0;
        EncodeFragmentArg(fragmentArgTypes[cfrag1] >> 4, (fragControlBits[cfrag1] >> 4) & 7);

        // Special adjustments for specific fragments
        if (cfrag1 == CF_67 || cfrag1 == CF_68)
            iCodeArgs[iCodeArgsIdx - 1] += 2;

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