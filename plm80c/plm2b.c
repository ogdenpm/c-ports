/****************************************************************************
 *  plm2b.c: De-reference Pass - Convert Relative TX2 References to Absolute
 *
 *  PART OF: Intel ISIS-II PL/M-80 Compiler (C Port)
 *  Original: Copyright Intel
 *  Re-engineered to C by Mark Ogden <mark.pm.ogden@btinternet.com>
 *
 *  Released for academic interest and personal use only
 *
 *  PURPOSE:
 *  --------
 *  This module implements the de-referencing pass that converts compact relative
 *  node references in the TX2 intermediate tree to absolute TX2 node indices.
 *
 *  The TX2 tree uses a space-efficient relative addressing scheme where references
 *  to other nodes are stored as relative distances from the current position. This
 *  pass converts these relative references to absolute indices, making the tree
 *  easier to process in subsequent optimization and code generation phases.
 *
 *  ARCHITECTURE:
 *  ---------------
 *  The TX2 tree is built bottom-up during parsing, which creates a problem:
 *  - When a node is created, referenced nodes haven't been assigned final indices
 *  - References are stored as relative offsets from the current position
 *  - After tree building completes, references must be converted to absolute indices
 *
 *  Example of relative addressing:
 *    tx2qp = 100 (current position)
 *    left = 2 (relative offset: node 100-2 = 98)
 *    right = 5 (relative offset: node 100-5 = 95)
 *
 *  After de-referencing:
 *    left = 98 (absolute index)
 *    right = 95 (absolute index)
 *
 *  COMPILATION FLOW:
 *  -----------------
 *  1. Parse statement → Build TX2 tree with relative references
 *  2. DeRelStmt() → Convert all references to absolute (THIS MODULE)
 *  3. OptimiseStmtNodes() → Apply optimizations (plm2c.c)
 *  4. GenerateStatementCode() → Generate code (plm2h.c)
 *  5. Repeat for next statement
 *
 *  SPECIAL HANDLING:
 *  -----------------
 *  The de-referencing algorithm must handle special cases:
 *
 *  a) T2_LINEINFO nodes:
 *     - Contain line number tracking information
 *     - Have special structure: type=T2_LINEINFO, right=0, extra=block_count
 *     - When backing up through tree, skip over these tracking records
 *     - The 'extra' field contains nesting depth adjustment
 *
 *  b) Array indexing (BYTEINDEX, WORDINDEX):
 *     - Index nodes are scaled by element size
 *     - left operand points to array base
 *     - extra field contains index multiplier
 *     - After de-referencing, update offset: right += extra * element_size
 *
 *  c) JMPFALSE and CASEBLOCK:
 *     - Special case: create dummy LOCALLABEL entry
 *     - Set tx2[1].right = source of jump target
 *     - Replace left reference with dummy entry index (1)
 *
 *  d) CALL nodes:
 *     - Extra field contains info pointer (not relative reference)
 *     - Converted elsewhere by specialized code
 *     - Must not attempt de-referencing of extra field
 *
 *  DATA STRUCTURES:
 *  ----------------
 *  tx2[] - Expression tree nodes:
 *    uint8_t type       - Node type (T2_* constants)
 *    uint16_t left      - Relative/absolute reference to left operand
 *    uint16_t right     - Relative/absolute reference to right operand or value
 *    uint16_t extra     - Additional field (node-type dependent)
 *    uint8_t exprAttr   - Expression attribute (BYTE_A, ADDRESS_A, etc.)
 *    uint8_t exprLoc    - Expression location (LOC_REG, LOC_STACK, LOC_MEM, etc.)
 *    uint8_t cnt        - Reference count (set later by optimizer)
 *
 *  nodeControlMap[] - Classification of node types:
 *    Bits 7-6: Node category
 *      00 = Binary/unary operators
 *      01 = Identifiers and literals
 *      10 = Procedure/statement nodes
 *      11 = Control flow structures
 *    Bits 3-1: Operand configuration
 *      0 = No operands
 *      1 = One operand (left)
 *      2 = Two operands (left, right)
 *      3 = Three operands (left, right, extra)
 *    Bit 0: Special flags
 *
 *  ALGORITHMS:
 *  -----------
 *
 *  1. cvtFromRel(rel, &location):
 *     Converts a single relative reference to absolute index
 *
 *     Parameters:
 *       rel - Relative offset (distance backwards from tx2qp)
 *       location - Pointer to field containing relative reference
 *
 *     Algorithm:
 *       - If location is 0 (null reference), return unchanged
 *       - If relative offset exceeds node size, clear reference (invalid)
 *       - Otherwise, backup through TX2 tree:
 *         • Save starting position in p = *location
 *         • Set *location = tx2qp (marks as processed)
 *         • Loop while p-- > 0:
 *           » Decrement *location to go back through tree
 *           » If node is T2_LINEINFO (line tracking):
 *             · Skip it and any block nesting data it represents
 *             · Adjustment = (extra - left) block count difference
 *
 *     Example backup with LINEINFO:
 *       tx2qp = 100
 *       Start: p = 3 (rel = 3)
 *       Iter 1: --*loc = 99, not LINEINFO, p = 2
 *       Iter 2: --*loc = 98, not LINEINFO, p = 1
 *       Iter 3: --*loc = 97, is LINEINFO with extra=10, left=5
 *               Skip 10-5=5 blocks, so p = 1-5 = -4 (exit)
 *       Result: *location = 97 (actual node after line info)
 *
 *  2. DeRelNode():
 *     De-references all operands of the current node
 *
 *     Steps:
 *       a) Extract node size from nodeControlFlags & 3:
 *          - 0: No operands
 *          - 1: One operand (left)
 *          - 2: Two operands (left, right)
 *          - 3: Three operands (left, right, extra)
 *
 *       b) Check for special case: nodeControlFlags & 4:
 *          - JMPFALSE or CASEBLOCK nodes
 *          - Create dummy LOCALLABEL at tx2[1]
 *          - Set tx2[1].right = source expression
 *          - Replace left reference with index 1
 *
 *       c) De-reference operands:
 *          - left: Always de-referenced (unless JMPFALSE/CASEBLOCK)
 *          - right: De-referenced if nodeSize >= 2
 *          - extra: De-referenced if nodeSize == 3 AND not CALL
 *
 *       d) Special handling for BYTEINDEX/WORDINDEX:
 *          - leftLoc = dereferenced left value (array base)
 *          - Compute element size from array symbol
 *          - Scale index: right += extra * element_size
 *
 *       e) Set expression attributes:
 *          - exprAttr = LIT_A (default for all nodes)
 *          - exprLoc = LOC_SPECIAL (marks as intermediate)
 *
 *  3. simpleVal():
 *     Processes leaf nodes (identifiers and literals)
 *
 *     Cases:
 *       a) IDENTIFIER:
 *          - Look up symbol via FromIdx(tx2[tx2qp].left)
 *          - Set right = info->linkVal (symbol's stack/memory location)
 *          - Set exprAttr from typeExprMap[] based on symbol type
 *          - Set exprLoc = LOC_STACK if AUTOMATIC, else LOC_MEM
 *
 *       b) NUMBER or BIGNUMBER:
 *          - Convert to standard NUMBER node
 *          - Move value from left to right field
 *          - Clear left field
 *          - Set exprAttr = BYTE_A (small) or ADDRESS_A (big)
 *          - Set exprLoc = LOC_REG (immediate value)
 *
 *       c) Other leaf nodes:
 *          - Set exprAttr = BYTE_A
 *          - Clear right field
 *
 *  EXPRESSION ATTRIBUTES (typeExprMap):
 *  -------------------------------------
 *  Map symbol types to expression attributes:
 *    LIT_A (0)       - Literal/constant
 *    LABEL_A (1)     - Label address
 *    BYTE_A (2)      - Byte value
 *    ADDRESS_A (3)   - Address/pointer
 *    STRUCT_A (4)    - Structure member
 *    (others)        - Byte/default
 *
 *  EXPRESSION LOCATIONS (exprLoc):
 *  --------------------------------
 *    LOC_REG (0)     - Value is in register
 *    LOC_STACK (9)   - Value is on stack
 *    LOC_MEM (8)     - Value is in memory
 *    LOC_SPECIAL (other) - Special/intermediate
 *
 *  FLOW THROUGH MODULE:
 *  --------------------
 *  DeRelStmt() {
 *      for each TX2 node in current statement (4 to tx2qNxt-1) {
 *          Determine node type and classification
 *
 *          if (binary/unary operator) {
 *              DeRelNode()  - De-reference operands
 *          } else if (identifier/literal) {
 *              simpleVal()  - Process leaf value
 *          }
 *      }
 *  }
 *
 *  INTEGRATION POINTS:
 *  -------------------
 *  - Called from: main2.c::Start2() - Main compilation loop
 *  - Called after: FillTx2Queue() - Tree building complete
 *  - Called before: OptimiseStmtNodes() - Optimization pass
 *  - Uses: nodeControlMap[] - From data.c
 *  - Uses: FromIdx() - Symbol table lookup
 *  - Uses: GetElementSize() - Array element size calculation
 *
 *  GLOBAL STATE MODIFIED:
 *  ----------------------
 *  - tx2[] - All relative references converted to absolute
 *  - exprAttr[] - Expression attributes set (used later)
 *  - exprLoc[] - Expression locations set (used later)
 *  - No other state modifications
 *
 *  DEPENDENCIES:
 *  --------------
 *  - plm.h - Core data structures and constants
 *  - int.h - Internal type definitions
 *  - Symbol table functions (FromIdx, GetElementSize)
 *  - nodeControlMap[] from data.c
 *
 *  ERROR CONDITIONS:
 *  -----------------
 *  - Invalid relative offset (> nodeSize): Reference cleared to 0
 *  - Invalid CALL node (extra field): Skipped (handled elsewhere)
 *  - Invalid array index: Results in zero element size
 *
 *  OPTIMIZATION NOTES:
 *  --------------------
 *  - T2_LINEINFO nodes are skipped during backward traversal
 *  - Block count tracking allows efficient skip over tracking data
 *  - Relative addressing kept compact during tree building phase
 *  - Single pass conversion provides good performance
 *
 *  MAINTENANCE NOTES:
 *  ------------------
 *  - Adding new node types requires updating nodeControlMap[]
 *  - Changes to tree structure require updating DeRelNode()
 *  - T2_LINEINFO handling is fragile - changes must be coordinated
 *  - Array indexing multiplier must match GetElementSize() results
 *
 *  HISTORICAL NOTES:
 *  -----------------
 *  - Relative addressing scheme dates from original Intel compiler
 *  - Reduces memory usage during tree building phase
 *  - De-referencing pass was originally single monolithic routine
 *  - Modern version splits into separate functions for clarity
 *
 *  VERSION HISTORY:
 *  ----------------
 *  - Original: Intel ISIS-II PLM-80 compiler
 *  - C port: Mark Ogden
 *  - Current: Enhanced with comprehensive documentation
 *
 ****************************************************************************/
#include "plm.h"

static uint8_t nodeSize; // lifted to file scope

static void cvtFromRel(uint8_t rel, uint16_t *ploc) {
    uint16_t p;

    if (*ploc) {
        if (rel > nodeSize)
            *ploc = 0;
        else {
            p     = *ploc;
            *ploc = tx2qp;

            while (p--) {
                // backup, adjusting for lineinfo with just block counts (extra)
                if (tx2[--*ploc].type == T2_LINEINFO && tx2[*ploc].right == 0 &&
                    tx2[*ploc].extra != 0)
                    p -= (tx2[*ploc].extra - tx2[*ploc].left); // skip block nesting data
            }
        }
    }
} /* sub69EB */

static void DeRelNode() {
    nodeSize = nodeControlFlags & 3;
    if ((nodeControlFlags & 4)) {          // JMPFALSE CASEBLOCK
        tx2[1].right    = tx2[tx2qp].left; // set up dummy locallabel entry
        tx2[tx2qp].left = 1;               // reference the dummy
    } else
        cvtFromRel(1, &tx2[tx2qp].left);

    cvtFromRel(2, &tx2[tx2qp].right);
    if (nodeSize == 3) {
        if (curNodeType == T2_BYTEINDEX || curNodeType == T2_WORDINDEX) {
            uint16_t leftLoc = (uint8_t)tx2[tx2qp].left;
            tx2[leftLoc].right += tx2[tx2qp].extra * GetElementSize(FromIdx(tx2[leftLoc].left));
        } else if (curNodeType != T2_CALL)    // T2_CALL convert to info pointer done elsewhere
            cvtFromRel(3, &tx2[tx2qp].extra); // otherwise convert from rel
    }
    tx2[tx2qp].exprAttr = LIT_A;
    tx2[tx2qp].exprLoc  = LOC_SPECIAL;
}

static uint8_t typeExprMap[] = {
    LIT_A, LABEL_A, BYTE_A, ADDRESS_A, STRUCT_A, BYTE_A, BYTE_A
}; // index by info->type (excludes Macro, unk and condvar

static void simpleVal() {
    if (curNodeType == T2_IDENTIFIER) {
        info                = FromIdx(tx2[tx2qp].left);
        tx2[tx2qp].right    = info->linkVal;
        tx2[tx2qp].exprAttr = typeExprMap[info->type];
        tx2[tx2qp].exprLoc =
            (info->flag & (F_MEMBER | F_AUTOMATIC)) == F_AUTOMATIC ? LOC_STACK : LOC_MEM;
    } else if (curNodeType <= T2_BIGNUMBER) {
        tx2[tx2qp].type     = T2_NUMBER;
        tx2[tx2qp].right    = tx2[tx2qp].left;
        tx2[tx2qp].left     = 0;
        tx2[tx2qp].exprAttr = curNodeType == T2_BIGNUMBER ? ADDRESS_A : BYTE_A;
        tx2[tx2qp].exprLoc  = LOC_REG;
    } else {
        tx2[tx2qp].exprAttr = BYTE_A;
        tx2[tx2qp].right    = 0;
    }
}

/**
 * DeRelStmt - Main entry point for de-referencing pass
 *
 * Processes all TX2 nodes in the current statement, converting relative
 * references to absolute indices and setting expression attributes/locations.
 * This is a critical pass that must complete before optimization.
 *
 * Algorithm:
 *   1. Iterate through all TX2 nodes (4 to tx2qNxt-1)
 *      - Node 0-3 are reserved for initialization
 *      - Current statement nodes are from 4 to tx2qNxt-1
 *
 *   2. For each node, classify by nodeControlMap:
 *      - (flags & 0xC0) == 0: Binary/unary operators
 *      - (flags & 0xC0) == 0x40: Identifiers and literals
 *
 *   3. Call appropriate processing function:
 *      - DeRelNode() for operators
 *      - simpleVal() for leaf nodes
 *
 * After this pass, all relative references are converted to absolute,
 * and all expression attributes/locations are properly initialized.
 * The tree is now ready for optimization and code generation.
 *
 * Side effects:
 *   - Updates tx2[].left, tx2[].right, tx2[].extra (relative → absolute)
 *   - Sets tx2[].exprAttr (expression data type)
 *   - Sets tx2[].exprLoc (expression storage location)
 *   - May modify tx2[1] (for JMPFALSE/CASEBLOCK dummy label)
 *
 * Called from: main2.c::Start2() main loop
 * Precondition: FillTx2Queue() has completed (tree is built)
 * Postcondition: Tree is ready for OptimiseStmtNodes()
 */
void DeRelStmt() {
    for (tx2qp = 4; tx2qp < tx2qNxt; tx2qp++) {
        curNodeType      = tx2[tx2qp].type;
        nodeControlFlags = nodeControlMap[curNodeType];
        if ((nodeControlFlags & 0xc0) == 0)
            // LT LE NE EQ GE GT ROL ROR SCL SCR SHL SHR JMPFALSE DOUBLE PLUSSIGN MINUSSIGN STAR
            // SLASH MOD AND OR XOR BASED BYTEINDEX WORDINDEX MEMBER UNARYMINUS NOT LOW HIGH
            // ADDRESSOF PLUS MINUS TIME STKBARG STKWARG DEC COLONEQUALS OUTPUT CASEBLOCK STKARG
            // MOVE RETURNBYTE RETURNWORD RETURN ADDW BEGMOVE CALL CALLVAR SETADDR
            DeRelNode();
        else if ((nodeControlFlags & 0xc0) == 0x40)
            // IDENTIFIER NUMBER BIGNUMBER LENGTH LAST SIZE
            simpleVal();
    }
}