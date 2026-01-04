/****************************************************************************
 *  plm2c.c: part of the C port of Intel's ISIS-II plm80                    *
 *  The original ISIS-II application is Copyright Intel                     *
 *                                                                          *
 *  Re-engineered to C by Mark Ogden <mark.pm.ogden@btinternet.com>         *
 *                                                                          *
 *  It is released for academic interest and personal use only              *
 ****************************************************************************/
#include "plm.h"

static uint8_t expensiveAccessStart; // bC259 - Start position for expensive access optimization
static uint8_t cheapAccessStart;     // bC25A - Start position for cheap access optimization


/*
Optimization Strategy Documentation
Access Cost Classification
Expensive Accesses (narrow CSE window):
•	T2_BASED - Base+offset addressing
•	F_AT flag - Memory-mapped I/O
•	Dynamic array indexing
•	Complex structure member access
Cheap Accesses (wide CSE window):
•	Direct variable access
•	Simple automatic variables
•	Static storage
•	Constants
CSE Search Window Strategy

Statement: [4] ... [cheapAccessStart] ... [expensiveAccessStart] ... [tx2qp]
                   └─ Wide window ──────────┘
                                             └─ Narrow window ──┘

This two-tier strategy balances optimization aggressiveness with code correctness, preventing
invalid CSE across side effects while maximizing common subexpression reuse for safe cases.

*/


/**
 * IncrementNodeReference - Increment reference count for expression node
 *
 * Increments the reference count (cnt field) of a TX2 expression node if it exists.
 * This reference counting is critical for later optimization phases, register allocation,
 * and code generation decisions.
 *
 * The reference count tracks how many times an expression node is used:
 * - cnt = 0: Node unused or only used once (can be inlined)
 * - cnt = 1: Node used twice (might need register preservation)
 * - cnt > 1: Node used multiple times (definitely needs register or memory storage)
 *
 * Usage Context:
 *
 * This function is called during statement optimization (OptimiseStmtNodes) to build
 * reference counts for all expression nodes. It's invoked in several scenarios:
 *
 * 1. After Common Subexpression Elimination (CSE):
 *    - OptimiseNodes() increments counts for left/right operands
 *    - When T2_OPTBACKREF nodes are created, the original node's count increases
 *
 * 2. During Assignment Processing (T2_COLONEQUALS):
 *    - ProcessAssignmentNode() increments counts for both operands
 *    - Special handling ensures assignment targets are tracked
 *
 * 3. Within Procedure Call Context:
 *    - ProcessOperatorNode() increments counts when procCallDepth > 0
 *    - Prevents optimization across call boundaries
 *
 * 4. For Output and Timer Operations:
 *    - ProcessOperatorNode() handles T2_OUTPUT and T2_TIME specially
 *    - Both operands get reference counts incremented
 *
 * Reference Count Impact on Code Generation:
 *
 * Later compilation phases use these counts to make critical decisions:
 *
 * a) Register Allocation (in plm2a.c - AnalyzeRegisterUsage):
 *    - If tx2[k].cnt > 1: Register marked as needing preservation (boC072[i] = true)
 *    - Prevents register reuse that would lose multi-referenced values
 *
 * b) Common Subexpression Reuse:
 *    - High cnt indicates value should be computed once and reused
 *    - Low cnt suggests direct computation might be more efficient
 *
 * c) Stack Spilling Decisions:
 *    - Multi-referenced nodes (cnt > 1) are candidates for stack storage
 *    - Single-use nodes can be kept in registers temporarily
 *
 * Example Scenarios:
 *
 * Scenario 1: Single Use (cnt = 0 after increment → cnt = 1)
 * ```plm
 * a = b + c;
 * ```
 * - Node for "b" gets cnt incremented once (final cnt = 1)
 * - Node for "c" gets cnt incremented once (final cnt = 1)
 * - Both can be loaded into registers and used immediately
 *
 * Scenario 2: Multiple Use (cnt > 1)
 * ```plm
 * a = b + b;
 * ```
 * - Node for "b" gets cnt incremented twice (final cnt = 2)
 * - Compiler knows "b" is referenced multiple times
 * - Must preserve "b" in register or memory for second use
 *
 * Scenario 3: Common Subexpression (after CSE)
 * ```plm
 * a = b + c;
 * d = b + c;
 * ```
 * - First "b + c" expression node gets normal increments
 * - Second "b + c" becomes T2_OPTBACKREF pointing to first
 * - Original expression node's cnt reflects total usage
 *
 * Scenario 4: Complex Expression
 * ```plm
 * a = (b + c) * (b + c);
 * ```
 * - After CSE, second "(b + c)" becomes T2_OPTBACKREF
 * - Original "(b + c)" node has cnt = 2
 * - Nodes "b" and "c" each have cnt = 2 (used in one expr, expr used twice)
 *
 * Zero Check Purpose:
 *
 * The `if (nodeLoc)` check serves multiple purposes:
 *
 * 1. Safety: Prevents incrementing invalid/null nodes (nodeLoc = 0)
 * 2. Sentinel: Node index 0 is reserved, never represents a valid expression
 * 3. Optimization: Avoids processing empty/placeholder references
 *
 * Integration with Optimization Pipeline:
 *
 * Reference counting is part of a multi-phase optimization strategy:
 *
 * Phase 1 (OptimiseStmtNodes):
 * - Initialize all cnt fields to 0
 * - Call IncrementNodeReference for each node usage
 * - Build complete reference count map
 *
 * Phase 2 (Code Generation):
 * - Use cnt to determine register allocation strategy
 * - Decide when to save/restore register values
 * - Optimize for single-use vs. multi-use patterns
 *
 * Phase 3 (Register Spilling):
 * - Multi-referenced nodes (cnt > 1) trigger preservation logic
 * - Single-referenced nodes can be computed on-demand
 *
 * @param nodeLoc - Index of TX2 node to increment reference count for
 *                  (0 = no node, 1-255 = valid node index)
 *
 * @global tx2[] - Expression tree array
 *
 * Side effects:
 * - Increments tx2[nodeLoc].cnt if nodeLoc is non-zero
 * - No effect if nodeLoc is 0 (null/invalid node)
 *
 * Called from:
 * - OptimiseNodes() - After CSE, for left/right operands
 * - ProcessAssignmentNode() - During assignment processing
 * - ProcessOperatorNode() - In procedure call context
 * - ProcessOperatorNode() - For OUTPUT and TIME operations
 *
 * Related Functions:
 * - OptimiseStmtNodes() - Main optimization loop (initializes cnt to 0)
 * - OptimiseNodes() - Common subexpression elimination
 * - AnalyzeRegisterUsage() - Uses cnt for register allocation
 * - SaveOrRedirectRegister() - Checks cnt to determine preservation
 *
 * Historical Note:
 * This is one of the core functions in the PLM/80 optimizer, directly
 * implementing the reference counting strategy described in Intel's
 * compiler design. The simplicity of this function belies its critical
 * importance in generating efficient 8080 assembly code.
 */
static void IncrementNodeReference(uint8_t nodeLoc) {
    if (nodeLoc)
        tx2[nodeLoc].cnt++;
}

/*
* 1.	Common Subexpression Elimination (CSE)
•	OptimiseNodes() scans backward to find identical expressions
•	Replaces duplicates with T2_OPTBACKREF nodes pointing to the first occurrence
•	Only enabled when OPTIMIZE is true

*/
static void OptimiseNodes(uint8_t nodeLoc) {
    if (OPTIMIZE)
        while (tx2qp > ++nodeLoc) {
            if (tx2[nodeLoc].nodeType == curNodeType && tx2[nodeLoc].left == tx2[tx2qp].left &&
                tx2[nodeLoc].right == tx2[tx2qp].right && tx2[nodeLoc].exprAttr == tx2[tx2qp].exprAttr) {
                if (tx2[nodeLoc].extra != 0xff00) {
                    tx2[tx2qp].nodeType = T2_OPTBACKREF;
                    tx2[tx2qp].left = nodeLoc;
                    return;
                }
            }
        }
    if ((nodeControlFlags & 0xc0) == 0) {
        // LT LE NE EQ GE GT ROL ROR SCL SCR SHL SHR JMPFALSE DOUBLE PLUSSIGN MINUSSIGN STAR
        // SLASH MOD AND OR XOR BASED BYTEINDEX WORDINDEX MEMBER UNARYMINUS NOT LOW HIGH
        // ADDRESSOF PLUS MINUS TIME STKBARG STKWARG DEC COLONEQUALS OUTPUT CASEBLOCK STKARG
        // MOVE RETURNBYTE RETURNWORD RETURN ADDW BEGMOVE CALL CALLVAR SETADDR
        IncrementNodeReference((uint8_t)tx2[tx2qp].left);
        IncrementNodeReference((uint8_t)tx2[tx2qp].right);
    }
    tx2[tx2qp].extra = tx2qp;
}

static bool isExpensive;

static void FixBackRef(wpointer pNodeIdx) {
    if (*pNodeIdx && tx2[*pNodeIdx].nodeType == T2_OPTBACKREF)
        *pNodeIdx = tx2[*pNodeIdx].left;
}

static void chkComponentAccess(uint8_t nodeLoc) {
    uint8_t nodeType;
    if ((nodeType = tx2[nodeLoc].nodeType) == T2_BASED)
        isExpensive = true;
    else if (nodeType == T2_BYTEINDEX || nodeType == T2_WORDINDEX) {
        if (tx2[tx2[nodeLoc].right].nodeType != T2_NUMBER)
            isExpensive = true;
        else {
            info = FromIdx(tx2[tx2[nodeLoc].left].left);
            if (tx2[tx2[nodeLoc].right].right >= info->dim || (info->flag & F_AT))
                isExpensive = true;
        }
    } else if (nodeType == T2_IDENTIFIER) {
        info = FromIdx(tx2[nodeLoc].left);
        if ((info->flag & F_AT))
            isExpensive = true;
    }
} /* Sub_6F20() */

/*
* 3.	Expensive Access Detection
•	chkVarAccess() identifies costly memory accesses (BASED, AT, array indexing)
•	Affects the optimization search range
*/

static void chkVarAccess(uint8_t nodeLoc) {
    isExpensive = false;
    if (tx2[nodeLoc].nodeType == T2_MEMBER) {
        chkComponentAccess((uint8_t)tx2[nodeLoc].left);
        chkComponentAccess((uint8_t)tx2[nodeLoc].right);
    } else
        chkComponentAccess(nodeLoc);
}

static uint8_t searchBoundary; // bC263 - Dynamic CSE search boundary

/**
 * UpdateSearchBoundary - Update CSE search boundary based on node position
 *
 * Adjusts the CSE search boundary (searchBoundary) to ensure it doesn't extend
 * beyond nodes that might have side effects (procedure calls) or complex
 * dependencies. For T2_CALL nodes, uses the call position; for others,
 * uses the node's extra field which tracks original position.
 *
 * @param nodeIdx - TX2 node index to check
 */
static void UpdateSearchBoundary(uint8_t nodeIdx) {
    if (tx2[nodeIdx].nodeType != T2_CALL)
        nodeIdx = (uint8_t)tx2[nodeIdx].extra;
    if (nodeIdx && nodeIdx > searchBoundary)
        searchBoundary = nodeIdx;
}

/**
 * GetOptimizationStartPos - Determine CSE search starting position
 *
 * Returns the starting position for common subexpression elimination (CSE)
 * search based on whether the current expression involves expensive memory
 * accesses. Expensive accesses (BASED, AT, indexed arrays) get a more
 * restrictive search range to avoid complex dependencies.
 *
 * @return Starting TX2 node index for CSE backward scan
 */
static uint8_t GetOptimizationStartPos() {
    searchBoundary = isExpensive ? expensiveAccessStart : cheapAccessStart;

    UpdateSearchBoundary((uint8_t)tx2[tx2qp].left);
    UpdateSearchBoundary((uint8_t)tx2[tx2qp].right);
    return searchBoundary;
}

/**
 * IsAutomaticVariableAccess - Check if assignment target is automatic variable
 *
 * Determines if the assignment target involves an automatic (stack-based)
 * variable or has expensive memory access patterns. Used to control
 * optimization boundaries for assignments to ensure stack frame consistency.
 *
 * @param nodeLoc - TX2 node index of assignment target
 * @return true if target is automatic variable or has expensive access
 */
static bool IsAutomaticVariableAccess(uint8_t nodeLoc) {
    chkVarAccess((uint8_t)tx2[nodeLoc].left);
    if (isExpensive)
        return true;

    if (tx2[tx2[nodeLoc].left].nodeType == T2_IDENTIFIER) {
        info = FromIdx(tx2[tx2[nodeLoc].left].left);
        if ((info->flag & F_AUTOMATIC))
            return true;
    }
    return false;
}

/**
 * ProcessAssignmentNode - Handle special processing for assignment operators
 *
 * Performs special reference counting and optimization boundary updates for
 * assignment operations (T2_COLONEQUALS). Updates cheapAccessStart if the
 * assignment target is an automatic variable to prevent inappropriate CSE
 * across stack frame modifications.
 *
 * Called from: ProcessOperatorNode() when curNodeType == T2_COLONEQUALS
 */
void ProcessAssignmentNode() {
    if (IsAutomaticVariableAccess(tx2qp))
        cheapAccessStart = tx2qp;

    // Mark assignment target's extra field if not already marked
    if (tx2[tx2[expensiveAccessStart = tx2qp].left].extra != 0xff00)
        tx2[tx2[tx2qp].left].extra = tx2qp;

    // Increment reference counts for both operands
    IncrementNodeReference((uint8_t)tx2[tx2qp].left);
    IncrementNodeReference((uint8_t)tx2[tx2qp].right);
}
/*
•	Fixes back-references from previous optimizations
•	Handles special cases for assignment (T2_COLONEQUALS)
•	Manages procedure call depth tracking
•	Performs common subexpression elimination via OptimiseNodes()
•	Optimizes JMPFALSE(NOT(x)) patterns by removing the redundant NOT
*/
static void ProcessOperatorNode() {
    FixBackRef(&tx2[tx2qp].left);
    FixBackRef(&tx2[tx2qp].right);
    if (curNodeType == T2_COLONEQUALS)
        ProcessAssignmentNode();
    else if (procCallDepth > 0) {
        IncrementNodeReference((uint8_t)tx2[tx2qp].left);
        IncrementNodeReference((uint8_t)tx2[tx2qp].right);
        if (curNodeType == T2_CALL)
            procCallDepth--;
        else if (curNodeType == T2_MOVE || curNodeType == T2_CALLVAR) {
            procCallDepth--;
            FixBackRef(&tx2[tx2qp].extra);
            IncrementNodeReference((uint8_t)tx2[tx2qp].extra);
        } else
            tx2[tx2qp].extra = 0xff00;
    } else if (curNodeType == T2_OUTPUT || curNodeType == T2_TIME) {
        IncrementNodeReference((uint8_t)tx2[tx2qp].left);
        IncrementNodeReference((uint8_t)tx2[tx2qp].right);
    } else {
        tx2[tx2qp].extra = 0;
        chkVarAccess(tx2qp);
        OptimiseNodes(GetOptimizationStartPos());
        if (curNodeType == T2_JMPFALSE && tx2[tx2qp - 1].nodeType == T2_NOT) {
            boC20F        = true;
            tx2[tx2qp].right = tx2[tx2qp - 1].left;
            MoveTx2(tx2qp, tx2qp - 1);
            tx2[tx2qp].nodeType = T2_SEMICOLON;
        }
    }
}

/*
* •	Marks nodes appropriately based on procedure call context
•	Optimizes simple expressions
*/
static void OptimiseLeafNode() {
    if (procCallDepth > 0)
        tx2[tx2qp].extra = 0xff00;
    else {
        tx2[tx2qp].extra = 0;
        if (curNodeType == T2_IDENTIFIER) {
            info = FromIdx(tx2[tx2qp].left);
            OptimiseNodes((info->flag & F_AT) ? expensiveAccessStart : cheapAccessStart);
        } else
            OptimiseNodes(cheapAccessStart);
    }
}

/*
* Iterates through all TX2 nodes in the current statement (from position 4 to tx2qNxt - 1):
1.	Initializes each node with a reference count of 0
2.	Classifies nodes using nodeControlMap[nodeType] to determine processing strategy
3.	Routes processing based on node control flags

optimisation features
4.	Procedure Call Tracking
•	Maintains procCallDepth to handle nested calls
•	Prevents certain optimizations across call boundaries
*/

void OptimiseStmtNodes() {
    expensiveAccessStart = 4;
    cheapAccessStart = 4;
    for (tx2qp = 4; tx2qp <= tx2qNxt - 1; tx2qp++) {
        tx2[tx2qp].cnt = 0;
        curNodeType          = tx2[tx2qp].nodeType;
        nodeControlFlags          = nodeControlMap[curNodeType];
        if ((nodeControlFlags & 0xc0) == 0) // binary/unary operators
            // LT LE NE EQ GE GT ROL ROR SCL SCR SHL SHR JMPFALSE DOUBLE PLUSSIGN MINUSSIGN STAR
            // SLASH MOD AND OR XOR BASED BYTEINDEX WORDINDEX MEMBER UNARYMINUS NOT LOW HIGH
            // ADDRESSOF PLUS MINUS TIME STKBARG STKWARG DEC COLONEQUALS OUTPUT CASEBLOCK STKARG
            // MOVE RETURNBYTE RETURNWORD RETURN ADDW BEGMOVE CALL CALLVAR SETADDR
            ProcessOperatorNode();
        else if ((nodeControlFlags & 0xc0) == 0x40) // identifiers and literals
            OptimiseLeafNode();
        if (curNodeType == T2_BEGCALL || curNodeType == T2_BEGMOVE)
            procCallDepth++;
    }
}
