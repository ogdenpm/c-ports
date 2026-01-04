/****************************************************************************
 *  plm2d.c: part of the C port of Intel's ISIS-II plm80                    *
 *  The original ISIS-II application is Copyright Intel                     *
 *                                                                          *
 *  Re-engineered to C by Mark Ogden <mark.pm.ogden@btinternet.com>         *
 *                                                                          *
 *  It is released for academic interest and personal use only              *
 ****************************************************************************/
#include "plm.h"

// lifted to file scope
static uint16_t lhsAcc, lhsAccFlag, rhsAcc, rhsAccFlag;
static uint8_t operand;
static uint8_t arithOp;
static uint8_t resAttrib, resOpFlag;


static bool CanFoldOperand(uint8_t arg1b, uint16_t arg2w) {
    /**
     * CanFoldOperand - Validate if an operand can participate in constant folding
     * 
     * Checks whether an operand meets the requirements for constant folding based on:
     * - Operand location (LOC_MEM, LOC_REG)
     * - Operand flags (compatibility between left and right operands)
     * - Special constraints (0x4000 flag check)
     * 
     * Validation rules (arg1b parameter):
     * - 0: Requires LOC_MEM location
     * - 2: Rejects if arg2w != 0
     * - 3: Special handling - rejects if arg2w == 0x4000
     * - 4: Requires arg2w == 0 (must be pure constant)
     * - 5: Requires lhsAccFlag == rhsAccFlag (operands must have same storage class)
     * - Other: Requires LOC_REG location and performs additional flag checks
     * 
     * @param arg1b - Validation rule index from constFoldRules table
     * @param arg2w - Operand flag from GetOperandValue()
     * @return true if operand can be folded, false otherwise
     */
    if (arg1b == 0) {
        if (tx2[operand].exprLoc != LOC_MEM)
            return false;
    } else if (arg1b != 3) {
        if (tx2[operand].exprLoc != LOC_REG)
            return false;
        if (arg2w != 0) {
            if (arg1b == 2)
                return false;
            if (arg1b == 4) {
                if (lhsAccFlag != 0)
                    return false;
            } else if (arg1b == 5) {
                if (lhsAccFlag != rhsAccFlag)
                    return false;
            }
        }
    } else if (arg2w == 0x4000)
        return false;
    return true;
}

static uint16_t evalConst() {
    switch (arithOp) {
    case 0: // DOUBLE
        return lhsAcc;
    case 1: // PLUSSIGN
        return lhsAcc + rhsAcc;
    case 2: // MINUSSIGN
        return lhsAcc - rhsAcc;
    case 3: // STAR
        return lhsAcc * rhsAcc;
    case 4: // SLASH
        return lhsAcc / rhsAcc;
    case 5: // MOD
        return lhsAcc % rhsAcc;
    case 6: // AND
        return lhsAcc & rhsAcc;
    case 7: // OR
        return lhsAcc | rhsAcc;
    case 8: // XOR
        return lhsAcc ^ rhsAcc;
    case 9: // BASED
        return lhsAcc + rhsAcc;
    case 10: // BYTEINDEX
        return lhsAcc + rhsAcc;
    case 11: // WORDINDEX
        return lhsAcc + rhsAcc + rhsAcc;
    case 12: // MEMBER
        return lhsAcc + rhsAcc;
    case 13: // UNARY MINUS
        return -lhsAcc;
    case 14: // NOT
        return ~lhsAcc;
    case 15: // LOW
        return Low(lhsAcc);
    case 16: // HIGH
        return High(lhsAcc);
    case 17: // ADDRESSOF
        return lhsAcc;
    }
    return lhsAcc; // added to prevent compiler warning C4715
}

/**
 * DetermineResultType - Determine the data type and storage location of folded constant result
 * 
 * Analyzes the operation type and operand attributes to determine:
 * - resAttrib: Result data type (BYTE_A or ADDRESS_A)
 * - resOpFlag: Result location (LOC_REG, LOC_MEM, or LOC_STACK)
 * - info: Associated symbol information (if applicable)
 * 
 * Uses constFoldRules lookup table to apply type inference rules based on operation.
 * Special handling for MEMBER access with stack-based structs.
 */
static void DetermineResultType() {
    if (curNodeType == T2_MEMBER && tx2[tx2[tx2qp].left].exprLoc == LOC_STACK) {
        resAttrib = tx2[tx2[tx2qp].right].exprAttr;
        resOpFlag = LOC_STACK;
    } else
        switch (constFoldRules[arithOp] & 7) {
        case 0: // LOW HIGH
            resAttrib = BYTE_A;
            resOpFlag = LOC_REG;
            break;
        case 1: // PLUSSIGN MINUSSIGN AND OR XOR
            if (tx2[tx2[tx2qp].left].exprAttr == BYTE_A && tx2[tx2[tx2qp].right].exprAttr == BYTE_A)
                resAttrib = BYTE_A;
            else
                resAttrib = ADDRESS_A;
            resOpFlag = LOC_REG;
            break;
        case 2: // DOUBLE STAR SLASH MOD ADDRESSOF
            resAttrib = ADDRESS_A;
            resOpFlag = LOC_REG;
            break;
        case 3:                                        // BYTEINDEX WORDINDEX UNARYMINUS NOT
            resAttrib = tx2[tx2[tx2qp].left].exprAttr; // propagate attribute of item op applied to
            resOpFlag = tx2[tx2[tx2qp].left].exprLoc;  // and the opFlag
            break;
        case 4:                                         // MEMBER
            resAttrib = tx2[tx2[tx2qp].right].exprAttr; // get the member's attribute
            resOpFlag = tx2[tx2[tx2qp].right].exprLoc;  // and opFlag
            break;
        case 5: // BASED
            resAttrib = IndirectAddr(tx2[tx2[tx2qp].right].exprAttr);
            resOpFlag = LOC_REG;
            break;
        }
    if (lhsAccFlag == rhsAccFlag)
        info = NULL;
    else if (rhsAccFlag == 0)
        info = FromIdx(tx2[tx2[tx2qp].left].left);
    else
        info = FromIdx(tx2[tx2[tx2qp].right].left);
}

 /**
 * FoldConstantExpr - Perform constant folding optimization on type conversion expressions
 * 
 * This function implements constant folding, a classic compiler optimization that evaluates
 * expressions with constant operands at compile-time, replacing the expression tree with
 * a single constant value. This reduces code size and improves runtime performance by
 * eliminating unnecessary runtime computations.
 * 
 * Handles the following operations (T2_DOUBLE through T2_ADDRESSOF):
 *   - Type conversions: DOUBLE, ADDRESSOF
 *   - Arithmetic: PLUSSIGN, MINUSSIGN, STAR, SLASH, MOD, UNARY MINUS
 *   - Bitwise: AND, OR, XOR, NOT
 *   - Byte operations: LOW, HIGH
 *   - Address calculations: BASED, BYTEINDEX, WORDINDEX, MEMBER
 * 
 * Process:
 * 1. Extract left operand value and flags using GetOperandValue()
 * 2. Validate left operand is suitable for folding (must be constant or in register)
 * 3. Extract right operand value and flags (for binary operations)
 * 4. Validate right operand is suitable for folding
 * 5. Evaluate the constant expression at compile-time using evalConst()
 * 6. Determine result type and storage class using Sub_73C5()
 * 7. Replace the expression subtree with a single constant node
 * 
 * Conditions for successful folding:
 * - Both operands must be compile-time constants (accFlag == 0)
 * - Or operands must meet specific criteria based on constFoldRules lookup table
 * - Expression location must be compatible (LOC_REG or LOC_MEM as appropriate)
 * 
 * Side effects:
 * - Replaces tx2[tx2qp] node with T2_NUMBER or T2_IDENTIFIER containing folded value
 * - Updates nodeControlFlags to reflect new node type
 * - Decrements reference counts via Sub_611A()
 * 
 * Called from: GenerateOperatorCode() for type conversion operators
 * 
 * Example transformation:
 *   Before: Expression tree for "5 + 3 * 2"
 *   After:  Single T2_NUMBER node with value 11
 */
void FoldConstantExpr() {
    uint16_t constVal;

    arithOp = curNodeType - T2_DOUBLE;
    operand = (uint8_t)tx2[tx2qp].left;
    GetOperandValue(operand, &lhsAcc, &lhsAccFlag);
    
    // Check if left operand can be folded
    if (CanFoldOperand(constFoldRules[arithOp] >> 6, lhsAccFlag)) {
        operand = (uint8_t)tx2[tx2qp].right;
        GetOperandValue(operand, &rhsAcc, &rhsAccFlag);
        
        // Check if right operand can be folded
        if (CanFoldOperand((constFoldRules[arithOp] >> 3) & 7, rhsAccFlag)) {
            // Both operands are foldable, proceed with constant folding
            DecrementExprRefs();
            constVal = evalConst();
            DetermineResultType();
            if (resAttrib == BYTE_A && resOpFlag == LOC_REG)
                constVal &= 0xFF;
            CreateConstantOrIdNode(constVal, info, resAttrib, resOpFlag);
            nodeControlFlags = nodeControlMap[curNodeType = tx2[tx2qp].nodeType];
        }
    }
}

static uint8_t optResult;
static uint16_t acc, accFlag;
static uint8_t curStepActions;

static bool ExpectedAttr(uint8_t nodeLoc) {
    if ((curStepActions & 0x40))
        return tx2[nodeLoc].exprAttr == ADDRESS_A || tx2[nodeLoc].exprAttr == ADDRESSIND_A;
    return tx2[nodeLoc].exprAttr == BYTE_A || tx2[nodeLoc].exprAttr == BYTEIND_A ||
           tx2[nodeLoc].exprAttr == STRUCT_A;
}


/**
 * TryEliminateCopyAssignment - Attempt to eliminate redundant copy in assignment chains
 *
 * Detects and optimizes the pattern:
 *   temp := expression
 *   variable := temp      // where temp is used only once
 *
 * Optimizes to:
 *   variable := expression
 *   (eliminated)
 *
 * Process:
 * 1. Skip forward past any T2_OPTBACKREF nodes
 * 2. Check if next statement is assignment (T2_COLONEQUALS)
 * 3. Verify pattern matches (nodeIdx is LHS, tx2qp is RHS, single reference)
 * 4. If pattern matches:
 *    - Set optResult = 6 (optimization performed flag)
 *    - Convert assignment to T2_SEMICOLON (no-op)
 *    - Clear reference counts
 *
 * Called from: Sub_7801() during peephole optimization
 *
 * @param nodeIdx - Index of the node being assigned from (the temporary)
 */
static void TryEliminateCopyAssignment(uint8_t nodeIdx) {
    uint8_t i;

    // Skip past any back-reference nodes
    for (i = tx2qp + 1; tx2[i].nodeType == T2_OPTBACKREF && i < tx2qNxt; i++)
        ;

    // Check if we have the pattern: nodeIdx := tx2qp, where tx2qp has single reference
    if (tx2[i].nodeType == T2_COLONEQUALS && nodeIdx == tx2[i].left && tx2[i].right == tx2qp &&
        tx2[tx2qp].cnt == 1) {
        optResult       = 6;            // Signal optimization was performed
        tx2[i].nodeType = T2_SEMICOLON; // Eliminate the copy assignment
        tx2[tx2qp].cnt  = 0;            // Clear reference count
        tx2[nodeIdx].cnt--;             // Decrement LHS reference
    }
}



/**
 * ValidateOptimisationStep - Validate whether an optimization step can be applied
 *
 * Checks if the current optimization step meets all required conditions before
 * transformation. Performs multiple validation checks based on curStepActions
 * flags and computes the optResult value used by ApplyTransformation().
 *
 * Validation checks (based on curStepActions flags):
 *
 * 1. Basic operand requirements:
 *    - accFlag must be 0 (constant or simple value)
 *    - For nodes < T2_BASED: left operand must be LOC_REG
 *
 * 2. Right operand attribute check (flag 0x20):
 *    - Verifies right operand has expected attribute via ExpectedAttr()
 *
 * 3. Operand attribute compatibility (flag 0x01):
 *    - Ensures left and right operands have matching attributes
 *
 * 4. Value-specific validation (flag 0x04):
 *    a) Non-zero check (flag 0x08):
 *       - Requires acc != 0
 *    b) Expected constant match:
 *       - Compares acc against lookup table: w502A[b5048[stepIdx] >> 3]
 *    c) Optional copy elimination (flag 0x80):
 *       - Triggers TryEliminateCopyAssignment() for assignment chains
 *
 * 5. Compute optimization result (flag 0x40):
 *    - Sets optResult = acc (if acc <= 4)
 *    - Or: optResult = acc % w502A[...] (modulo operation)
 *
 * The optResult value computed here is used by ApplyTransformation() to
 * determine the new node type: nodeType = b5221[stepIdx] + optResult
 *
 * @param stepIdx - Index into lookup tables (b5048, w502A)
 * @param left - Left operand node index in TX2 array
 * @param right - Right operand node index in TX2 array
 * @return true if step validation succeeded, false if optimization should abort
 *
 * Side effects:
 * - May set optResult (used by ApplyTransformation)
 * - May trigger TryEliminateCopyAssignment() (flag 0x80)
 * - Reads global variables: accFlag, curNodeType, curStepActions
 *
 * Called from: ApplyStepOptimization() for each step in optimization sequence
 */
static bool ValidateOptimisationStep(uint8_t stepIdx, uint8_t left, uint8_t right) {
    // Check basic operand requirements
    if (accFlag != 0 || (curNodeType < T2_BASED && tx2[left].exprLoc != LOC_REG))
        return false;

    // Validate right operand attribute if required
    if ((curStepActions & 0x20) && !ExpectedAttr(right))
        return false;

    // Check operand attribute compatibility
    if ((curStepActions & 1) && ExpectedAttr(left) != ExpectedAttr(right))
        return false;

    // Value-specific validation and optResult computation
    if ((curStepActions & 4)) {
        // Non-zero value check
        if ((curStepActions & 8))
            return acc != 0;

        // Expected constant value check
        if (commonConstants[peepholePatterns[stepIdx] >> 3] != acc)
            return false;

        // Optional copy elimination optimization
        if ((curStepActions & 0x80))
            TryEliminateCopyAssignment(right);

        return true;
    }

    // Compute optResult for transformation
    if (!(curStepActions & 0x40)) {
        // Modulo operation to determine result variant
        optResult = acc % commonConstants[peepholePatterns[stepIdx] >> 3];
        return true;
    } else if (acc <= 4) {
        // Direct value assignment for small constants
        optResult = (uint8_t)acc;
        return true;
    } else {
        // Value out of range for this optimization
        return false;
    }
}
/**
 * ApplyTransformation - Apply the validated optimization transformation to TX2 node
 *
 * Executes the actual transformation after ApplyStepOptimization() validates
 * the optimization is legal. Uses lookup tables to determine the new node type
 * and how to rewire operands.
 *
 * Two transformation types:
 *
 * 1. Constant Replacement (nodeType == T2_NUMBER):
 *    - Replaces entire expression with constant value
 *    - Sets acc = 0 if operation requires it (j == 0)
 *    - Decrements reference counts for original operands
 *    - Determines result type (BYTE_A or ADDRESS_A) based on:
 *      • Operation type (not STAR/SLASH/MOD)
 *      • Value range (acc < 0x100)
 *      • Operand attributes (ExpectedAttr checks)
 *    - Creates new T2_NUMBER node via Sub_5F4B()
 *
 * 2. Node Rewriting (nodeType != T2_NUMBER):
 *    - Modifies node type based on: b5221[stepIdx] + optResult
 *    - Rewires operands based on b5048[stepIdx] & 3:
 *      • j == 1: Use right operand, decrement left refcount
 *      • j != 1: Clear right operand, may swap with tx2[1].right
 *    - Handles comparison inversion (boC20F flag):
 *      • Computes inverted operation: 0x43 - nodeType
 *      • Used for relational operation optimization
 *    - Sets right operand to 0 (unary result)
 *
 * @param stepIdx - Index into lookup tables (decremented by 1 before use)
 * @param left - Left operand node index
 * @param right - Right operand node index
 *
 * Side effects:
 * - Modifies tx2[tx2qp] node (type, operands)
 * - Decrements reference counts for replaced operands
 * - Updates global variables: curNodeType, nodeControlFlags
 * - May clear boC20F flag (comparison inversion)
 * - May modify acc value
 *
 * Called from: ApplyStepOptimization() after validation succeeds
 */
static void ApplyTransformation(uint8_t stepIdx, uint8_t left, uint8_t right) {
    uint8_t j;

    stepIdx--; // Adjust index for lookup tables
    uint8_t nodeType = step2ActionTable[stepIdx] + optResult;
    j             = peepholePatterns[stepIdx] & 3;

    if (nodeType == T2_NUMBER) {
        // Transformation Type 1: Replace expression with constant
        if (j == 0)
            acc = 0;

        DecrementExprRefs();

        // Determine result data type
        curStepActions = 0; // Forces ExpectedAttr to test for BYTE types
        uint8_t exprAttr  = ADDRESS_A;

        // Use BYTE_A if all conditions met:
        // - Not multiplication/division/modulo
        // - Value fits in uint8_t
        // - Operands have uint8_t attributes
        if (curNodeType != T2_STAR && curNodeType != T2_SLASH && curNodeType != T2_MOD &&
            acc < 0x100 && ExpectedAttr(left) && (!right || ExpectedAttr(right)))
            exprAttr = BYTE_A;

        // Replace node with constant
        CreateConstantOrIdNode(acc, NULL, exprAttr, LOC_REG);
    } else {
        // Transformation Type 2: Rewrite node structure
        if (j == 1) {
            // Use right operand only
            tx2[tx2qp].left = right;
            tx2[left].cnt--;
        } else {
            // Clear right operand
            if (tx2[tx2qp].right != 0)
                tx2[tx2[tx2qp].right].cnt--;

            // Check if we need to swap with common subexpression
            if ((nodeControlFlags & 4)) {
                tx2[tx2qp].left = tx2[1].right;

                // Handle comparison inversion for relational ops
                if (boC20F) {
                    nodeType = 0x43 - nodeType; // Invert comparison
                    boC20F   = false;
                }
            }
        }

        // Update node type and clear right operand (unary result)
        tx2[tx2qp].nodeType = nodeType;
        tx2[tx2qp].right    = 0;
    }

    // Update control flags for new node type
    nodeControlFlags = nodeControlMap[curNodeType = nodeType];
}

/**
 * ApplyStepOptimization - Apply table-driven peephole optimization to expression
 *
 * Executes a sequence of optimization steps from the stepTable to transform
 * the current expression node. Uses a table-driven approach to apply various
 * peephole optimizations based on the operation type and operand characteristics.
 *
 * Process:
 * 1. Extract left operand value and flags via GetOperandValue()
 * 2. Initialize optimization result (optResult = 0)
 * 3. Iterate through stepTable entries starting at stepIdx:
 *    - Read step actions from stepTable[stepIdx]
 *    - If actions == 0xFF, optimization failed (return false)
 *    - Check operand attributes via ExpectedAttr() if flag 0x10 set
 *    - Validate optimization conditions via Sub_7801()
 *    - Continue to next step if conditions not met
 * 4. Apply transformation via Sub_7925() once all steps validated
 *
 * The step tables (step1Map, step2Map) provide starting indices for different
 * operations, allowing operation-specific optimization sequences.
 *
 * Optimization examples:
 * - Constant folding: x + 0 → x, x * 1 → x
 * - Strength reduction: x * 2 → x + x
 * - Dead code elimination: unused intermediate values
 * - Copy propagation: eliminate redundant assignments
 *
 * @param stepIdx - Starting index into stepTable for this optimization sequence
 * @param left - Left operand node index in TX2 array
 * @param right - Right operand node index in TX2 array
 * @return true if optimization applied successfully, false otherwise
 *
 * Side effects:
 * - May modify TX2 nodes (via Sub_7925)
 * - Sets global variables: acc, accFlag, optResult, curStepActions
 * - May decrement reference counts
 * - May eliminate copy assignments (via TryEliminateCopyAssignment)
 */
static bool ApplyStepOptimisation(uint8_t stepIdx, uint8_t left, uint8_t right) {
    // Extract left operand value and storage flags
    GetOperandValue(left, &acc, &accFlag);
    optResult = 0;

    // Iterate through optimization steps until valid transformation found
    for (bool more = true; more; stepIdx++) {
        // Read step actions from table
        if ((curStepActions = optimisationStepTable[stepIdx]) == 0xff)
            return false; // No valid optimization found

        // Check if step requires attribute validation
        if ((curStepActions & 0x10)) {
            // First check: verify left operand has expected attribute
            more = !ExpectedAttr(left);

            // If attribute check passed and step needs value check
            if ((curStepActions & 4) && !more)
                more = !ValidateOptimisationStep(stepIdx, left, right);
        } else {
            // No attribute check needed, just validate optimization conditions
            more = !ValidateOptimisationStep(stepIdx, left, right);
        }
    }

    // All steps validated, apply the transformation
    ApplyTransformation(stepIdx, left, right);
    return true;
}
/**
 * OptimiseExpression - Apply optimizations to the current expression node
 *
 * Performs expression-level optimizations on the current TX2 node (tx2qp).
 * Handles two types of optimizations:
 *
 * 1. Relational Operation Optimization (curNodeType ≤ T2_GT):
 *    Detects pattern: relational_op followed by JMPFALSE
 *    When the relational result has single reference (cnt == 1):
 *    - Converts to direct conditional jump (T2_JNZ)
 *    - Eliminates intermediate boolean value
 *    - Inverts comparison if needed (boC20F flag)
 *
 *    Example transformation:
 *      a < b        →  JNZ <label>, comparison=LT
 *      JMPFALSE <label>
 *
 * 2. General Expression Optimization:
 *    Applies peephole optimizations via step tables:
 *    - Tries step1Map optimization first (right, left order)
 *    - Falls back to step2Map if needed (left, right order)
 *    - Uses lookup tables for operation-specific transformations
 *    - Can fold constants, simplify operations, eliminate redundant copies
 *
 * Called from: Code generation phase for each expression node
 *
 * Side effects:
 * - May modify TX2 nodes (type, operands, reference counts)
 * - May set global flags (boC20F, optResult)
 * - May convert relational operations to conditional jumps
 */
void OptimiseExpression() {
    if (curNodeType <= T2_GT) { // Relational operation
        // Check for relational + JMPFALSE pattern that can be optimized
        if (tx2[tx2qp + 1].nodeType == T2_JMPFALSE && tx2[tx2qp].cnt == 1) {
            // Convert to direct conditional jump
            tx2[tx2qp + 1].nodeType = T2_JNZ;
            tx2[tx2qp + 1].left     = tx2[1].right;

            // Handle comparison inversion if needed
            if (boC20F) {
                tx2[tx2qp + 1].right = bC209[curNodeType]; // Inverted comparison
                boC20F               = false;
            } else {
                tx2[tx2qp + 1].right = curNodeType; // Normal comparison
            }

            tx2[tx2qp].cnt = 0; // Mark relational node as dead
        } else {
            // Convert to conditional value (boolean result)
            tx2[tx2qp].nodeType = curNodeType += T2_LT_VAL;
        }
    } else {
        // Apply general peephole optimizations
        // Try step1 (right, left operand order) first
        if (!ApplyStepOptimisation(optimsationStep1Map[curNodeType], (uint8_t)tx2[tx2qp].right, (uint8_t)tx2[tx2qp].left)) {
            // Fall back to step2 (left, right operand order)
            ApplyStepOptimisation(optimisationStep2Map[curNodeType], (uint8_t)tx2[tx2qp].left, (uint8_t)tx2[tx2qp].right);
        }
    }
}

