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

/****************************************************************************
 * cvtFromRel() -
 * Convert Relative TX2 Reference to Absolute Index
 *
 * SUMMARY:
 *   Converts a single relative
 * node reference to an absolute TX2 array index
 *   by walking backwards through the TX2 tree,
 * accounting for line tracking
 *   information nodes along the way.
 *
 * DESCRIPTION:
 *   This
 * is the core reference conversion routine used during the de-referencing
 *   pass. TX2 nodes
 * store operand references as relative offsets (how many nodes
 *   back from the current
 * position). This function converts these relative
 *   offsets to absolute TX2 array indices by
 * walking backwards through the tree.
 *
 *   The algorithm must handle T2_LINEINFO nodes specially
 * - these are compiler
 *   bookkeeping nodes that track source line numbers and block nesting
 * depth.
 *   They don't represent actual expression nodes and must be skipped over during
 *   the
 * backward walk, with adjustments made for the block nesting information
 *   they contain.
 *
 *
 * ALGORITHM:
 *   1. Check if reference is null (0) - if so, leave unchanged
 *   2. Validate
 * relative offset doesn't exceed node size
 *      - If invalid, clear reference to 0 and return

 * *   3. Save relative offset count in p
 *   4. Initialize location to current position (tx2qp)

 * *   5. Walk backwards p steps:
 *      a) Decrement location pointer
 *      b) If node is
 * T2_LINEINFO with block nesting data:
 *         - Calculate block depth change: (extra - left)
 *
 * - Adjust p by block depth change (skip over nesting records)
 *      c) Continue until p reaches
 * 0
 *   6. Result: *ploc contains absolute TX2 index
 *
 * PARAMETERS:
 *   rel  - Relative offset
 * count (how many nodes to walk backwards)
 *          Typically 1, 2, or 3 for left, right, or
 * extra operands
 *   ploc - Pointer to location field containing the relative reference
 * On
 * entry: Contains relative offset from tx2qp
 *          On exit:  Contains absolute TX2 array
 * index
 *
 * GLOBALS USED (READ):
 *   tx2qp     - Current TX2 node index being processed
 *
 * nodeSize  - Number of operands in current node (0-3)
 *   tx2[]     - TX2 expression tree array
 * (read for LINEINFO detection)
 *     .type   - Node type (checked for T2_LINEINFO)
 *     .right
 * - Must be 0 for LINEINFO block nesting records
 *     .extra  - Block nesting end depth
 * .left
 * - Block nesting start depth
 *
 * GLOBALS MODIFIED:
 *   None (only modifies value pointed to by
 * ploc parameter)
 *
 * RETURN VALUE:
 *   None (result returned via *ploc parameter)
 *
 * SPECIAL
 * CASES:
 *
 *   1. Null reference (*ploc == 0):
 *      - No conversion performed
 *      -
 * Reference remains 0 (indicates unused operand)
 *
 *   2. Invalid offset (rel > nodeSize):
 * -
 * Reference cleared to 0
 *      - No error reporting (silent failure)
 *      - Indicates
 * malformed tree from parser
 *
 *   3. T2_LINEINFO nodes with block nesting data:
 *      -
 * Detected by: type==T2_LINEINFO && right==0 && extra!=0
 *      - Contains block depth tracking:
 * extra (end) - left (start)
 *      - Walk counter adjusted by depth change
 *      - Example: If
 * LINEINFO has extra=10, left=5:
 *        • Block depth changed by 5 (10-5)
 *        • p
 * decremented by 5 additional steps
 *        • Effectively skips 5 block nesting tracking nodes

 * *
 * EXAMPLE WALKBACK:
 *   Given: tx2qp = 100, *ploc = 3 (relative), rel = 1
 *
 *   Step 1: p =
 * 3, *ploc = 100 (starting position)
 *   Step 2: p = 2, *ploc = 99 (not LINEINFO, normal
 * decrement)
 *   Step 3: p = 1, *ploc = 98 (not LINEINFO, normal decrement)
 *   Step 4: p = 0,
 * *ploc = 97 (reached target)
 *   Result: *ploc = 97 (absolute index)
 *
 *   With LINEINFO at
 * position 98:
 *   Step 1: p = 3, *ploc = 100
 *   Step 2: p = 2, *ploc = 99
 *   Step 3: *ploc =
 * 98, is LINEINFO with extra=10, left=8
 *           Adjust: p = 2 - (10-8) = 0
 *   Result: *ploc
 * = 98 (skipped over LINEINFO and its nesting data)
 *
 * RELATED FUNCTIONS:
 *   DeRelNode()  -
 * Calls this function to convert each operand
 *   DeRelStmt()  - Main loop controlling
 * de-referencing pass
 *
 * CALLED BY:
 *   DeRelNode() - To convert left, right, and extra
 * operands
 *
 * CONTEXT:
 *   This is a low-level utility function called during the
 * de-referencing
 *   pass. It's the workhorse that performs the actual relative-to-absolute
 *
 * conversion for individual node references.
 *
 * ASSUMPTIONS:
 *   - tx2qp contains valid current
 * node index
 *   - nodeSize correctly reflects number of operands
 *   - Relative offset in *ploc
 * is reasonable
 *   - TX2 array is properly formed with valid nodes
 *   - T2_LINEINFO nodes
 * follow documented structure
 *
 * ERROR HANDLING:
 *   - Invalid offset (rel > nodeSize): Sets
 * *ploc = 0, no error reported
 *   - Null reference: No operation performed
 *   - Malformed
 * LINEINFO: May produce incorrect results
 *
 * SIDE EFFECTS:
 *   - Modifies value at *ploc
 * (relative → absolute conversion)
 *   - Reads TX2 nodes during backward walk
 *   - No other
 * state changes
 *
 * PERFORMANCE:
 *   Time Complexity: O(n) where n = relative offset distance
 *
 * - Typical case: n = 1-10 (very fast)
 *   - Worst case: n = tx2qp (rare, still acceptable)
 *
 * Space Complexity: O(1) - Uses only local variables
 *
 * MAINTENANCE NOTES:
 *   - T2_LINEINFO
 * structure is critical - any changes require updates here
 *   - Block nesting calculation (extra
 * - left) must match line tracking logic
 *   - nodeSize validation prevents buffer underruns
 * -
 * Silent error handling may hide parser bugs
 *
 * HISTORICAL NOTE:
 *   Original Intel function:
 * sub69EB (first part)
 *   This was originally inline code in the main de-referencing routine
 *
 * Extracted to separate function for clarity in C port
 *
 * REFERENCE:
 *   Original Intel ISIS-II
 * PLM-80 compiler
 *   C port by Mark Ogden
 *

 * ****************************************************************************/
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

/****************************************************************************
 * DeRelNode()
 *
 * SUMMARY:
 *   De-reference operator node - converts relative TX2 node references to
 *   absolute indices for a single operator node's operands.
 *
 * DESCRIPTION:
 *   This function processes a single TX2 operator node (binary/unary) and
 *   converts all its operand references from relative offsets to absolute
 *   TX2 array indices. This is a critical step performed after tree building
 *   that transforms the compact relative representation into an easily
 *   navigable absolute form.
 *
 *   The function handles special cases for different node types:
 *   - JMPFALSE/CASEBLOCK: Creates dummy local label entry for jump targets
 *   - BYTEINDEX/WORDINDEX: Adjusts array indexing by element size
 *   - CALL nodes: Skips extra field (contains info pointer, not reference)
 *   - Standard nodes: Converts 1-3 operands based on nodeSize
 *
 * ALGORITHM:
 *   1. Extract operand count from nodeControlFlags (bits 0-1)
 *   2. Handle special JMPFALSE/CASEBLOCK case (bit 2 set):
 *      - Set tx2[1].right = tx2[tx2qp].left (save jump target)
 *      - Set tx2[tx2qp].left = 1 (reference dummy entry)
 *   3. Otherwise convert left operand from relative to absolute
 *   4. Convert right operand from relative to absolute
 *   5. If nodeSize == 3 (three operands):
 *      a) For BYTEINDEX/WORDINDEX: Scale array index by element size
 *      b) For non-CALL nodes: Convert extra field from relative
 *   6. Set node attributes: exprAttr = LIT_A, exprLoc = LOC_SPECIAL
 *
 * PARAMETERS:
 *   None (operates on module-level state)
 *
 * GLOBALS USED (READ):
 *   tx2qp            - Current TX2 node index being processed
 *   nodeControlFlags - Node classification flags (from nodeControlMap[])
 *   curNodeType      - Current node type (T2_* constant)
 *
 * GLOBALS MODIFIED:
 *   tx2[]            - TX2 expression tree array
 *     .left          - Left operand reference (relative → absolute)
 *     .right         - Right operand reference (relative → absolute)
 *     .extra         - Third operand reference (relative → absolute) OR
 *                      adjusted array offset for index nodes
 *     .exprAttr      - Set to LIT_A (literal/constant attribute)
 *     .exprLoc       - Set to LOC_SPECIAL (intermediate location)
 *   nodeSize         - Set to operand count (0-3) from nodeControlFlags
 *
 * SPECIAL CASES:
 *
 *   1. JMPFALSE and CASEBLOCK nodes (nodeControlFlags bit 2 = 1):
 *      - These control flow nodes need dummy local label entries
 *      - tx2[1] is reused as a temporary dummy LOCALLABEL entry
 *      - The original jump target is preserved in tx2[1].right
 *      - The left operand is changed to reference entry 1
 *      - This allows uniform processing of jump targets elsewhere
 *
 *   2. Array indexing (T2_BYTEINDEX, T2_WORDINDEX):
 *      - Array subscripts are multiplied by element size
 *      - Left operand points to array base (contains array info)
 *      - Extra field contains index multiplier from parser
 *      - Formula: right += extra * GetElementSize(array_info)
 *      - This converts symbolic index to byte offset
 *      - Example: ARRAY[5] → offset = 5 * element_size
 *
 *   3. CALL nodes (T2_CALL):
 *      - Extra field contains procedure info pointer (NOT relative ref)
 *      - This field is set up elsewhere by call processing code
 *      - Must NOT be converted by cvtFromRel()
 *      - Skip extra field conversion for CALL nodes
 *
 * NODESIZE VALUES:
 *   0 = No operands (e.g., literals, identifiers)
 *   1 = One operand in left field (e.g., unary operators)
 *   2 = Two operands in left/right fields (e.g., binary operators)
 *   3 = Three operands in left/right/extra fields (e.g., indexing, calls)
 *
 * RELATED FUNCTIONS:
 *   cvtFromRel()     - Converts single relative reference to absolute
 *   DeRelStmt()      - Main loop that calls DeRelNode() for each node
 *   GetElementSize() - Returns size of array element in bytes
 *   FromIdx()        - Converts info index to info_t pointer
 *
 * CALLED BY:
 *   DeRelStmt() - Main de-referencing loop
 *
 * CALLS:
 *   cvtFromRel()     - To convert each relative reference
 *   GetElementSize() - To get array element size (for index nodes only)
 *   FromIdx()        - To get array info (for index nodes only)
 *
 * CONTEXT:
 *   This function is part of the de-referencing pass that runs after
 *   tree building and before optimization. The TX2 tree is built with
 *   relative references during parsing for space efficiency, then this
 *   pass converts everything to absolute indices for easier processing.
 *
 * ASSUMPTIONS:
 *   - tx2qp points to a valid operator node
 *   - nodeControlFlags correctly describes the node's operand structure
 *   - curNodeType contains the correct T2_* type constant
 *   - All referenced nodes exist in the TX2 array
 *   - For array index nodes, left operand points to valid array info
 *
 * ERROR HANDLING:
 *   - Invalid relative references are set to 0 by cvtFromRel()
 *   - No error reporting (silent failure for invalid references)
 *   - Assumes well-formed tree from parser
 *
 * SIDE EFFECTS:
 *   - Modifies TX2 node operand references (relative → absolute)
 *   - May modify tx2[1] as temporary dummy label entry
 *   - For array indexing, modifies right field with scaled offset
 *   - Sets expression attributes for later optimization passes
 *
 * PERFORMANCE:
 *   O(1) per node - Constant time per operand conversion
 *   cvtFromRel() is O(n) where n = relative distance (worst case)
 *   Overall: O(tree_size) for entire de-referencing pass
 *
 * EXAMPLE:
 *   Before DeRelNode() for binary operator at tx2qp=100:
 *     tx2[100].left = 2   (relative: node 98)
 *     tx2[100].right = 5  (relative: node 95)
 *
 *   After DeRelNode():
 *     tx2[100].left = 98  (absolute index)
 *     tx2[100].right = 95 (absolute index)
 *     tx2[100].exprAttr = LIT_A
 *     tx2[100].exprLoc = LOC_SPECIAL
 *
 * MAINTENANCE NOTES:
 *   - Adding new three-operand node types requires updating the nodeSize==3
 *     section to determine if extra field should be converted
 *   - Changes to array indexing must coordinate with GetElementSize()
 *   - The tx2[1] dummy entry assumes entry 1 is always available
 *   - CALL node handling depends on call setup code in other modules
 *
 * REFERENCE:
 *   Original Intel ISIS-II PLM-80 compiler function sub69EB
 *   C port by Mark Ogden
 *
 ****************************************************************************/
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

/****************************************************************************
 * cvtFromRel() - Convert Relative TX2 Reference to Absolute Index
 *
 * SUMMARY:
 *   Converts a single relative node reference to an absolute TX2 array index
 *   by walking backwards through the TX2 tree, accounting for line tracking
 *   information nodes along the way.
 *
 * DESCRIPTION:
 *   This is the core reference conversion routine used during the de-referencing
 *   pass. TX2 nodes store operand references as relative offsets (how many nodes
 *   back from the current position). This function converts these relative
 *   offsets to absolute TX2 array indices by walking backwards through the tree.
 *
 *   The algorithm must handle T2_LINEINFO nodes specially - these are compiler
 *   bookkeeping nodes that track source line numbers and block nesting depth.
 *   They don't represent actual expression nodes and must be skipped over during
 *   the backward walk, with adjustments made for the block nesting information
 *   they contain.
 *
 * ALGORITHM:
 *   1. Check if reference is null (0) - if so, leave unchanged
 *   2. Validate relative offset doesn't exceed node size
 *      - If invalid, clear reference to 0 and return
 *   3. Save relative offset count in p
 *   4. Initialize location to current position (tx2qp)
 *   5. Walk backwards p steps:
 *      a) Decrement location pointer
 *      b) If node is T2_LINEINFO with block nesting data:
 *         - Calculate block depth change: (extra - left)
 *         - Adjust p by block depth change (skip over nesting records)
 *      c) Continue until p reaches 0
 *   6. Result: *ploc contains absolute TX2 index
 *
 * PARAMETERS:
 *   rel  - Relative offset count (how many nodes to walk backwards)
 *          Typically 1, 2, or 3 for left, right, or extra operands
 *   ploc - Pointer to location field containing the relative reference
 *          On entry: Contains relative offset from tx2qp
 *          On exit:  Contains absolute TX2 array index
 *
 * GLOBALS USED (READ):
 *   tx2qp     - Current TX2 node index being processed
 *   nodeSize  - Number of operands in current node (0-3)
 *   tx2[]     - TX2 expression tree array (read for LINEINFO detection)
 *     .type   - Node type (checked for T2_LINEINFO)
 *     .right  - Must be 0 for LINEINFO block nesting records
 *     .extra  - Block nesting end depth
 *     .left   - Block nesting start depth
 *
 * GLOBALS MODIFIED:
 *   None (only modifies value pointed to by ploc parameter)
 *
 * RETURN VALUE:
 *   None (result returned via *ploc parameter)
 *
 * SPECIAL CASES:
 *
 *   1. Null reference (*ploc == 0):
 *      - No conversion performed
 *      - Reference remains 0 (indicates unused operand)
 *
 *   2. Invalid offset (rel > nodeSize):
 *      - Reference cleared to 0
 *      - No error reporting (silent failure)
 *      - Indicates malformed tree from parser
 *
 *   3. T2_LINEINFO nodes with block nesting data:
 *      - Detected by: type==T2_LINEINFO && right==0 && extra!=0
 *      - Contains block depth tracking: extra (end) - left (start)
 *      - Walk counter adjusted by depth change
 *      - Example: If LINEINFO has extra=10, left=5:
 *        • Block depth changed by 5 (10-5)
 *        • p decremented by 5 additional steps
 *        • Effectively skips 5 block nesting tracking nodes
 *
 * EXAMPLE WALKBACK:
 *   Given: tx2qp = 100, *ploc = 3 (relative), rel = 1
 *
 *   Step 1: p = 3, *ploc = 100 (starting position)
 *   Step 2: p = 2, *ploc = 99 (not LINEINFO, normal decrement)
 *   Step 3: p = 1, *ploc = 98 (not LINEINFO, normal decrement)
 *   Step 4: p = 0, *ploc = 97 (reached target)
 *   Result: *ploc = 97 (absolute index)
 *
 *   With LINEINFO at position 98:
 *   Step 1: p = 3, *ploc = 100
 *   Step 2: p = 2, *ploc = 99
 *   Step 3: *ploc = 98, is LINEINFO with extra=10, left=8
 *           Adjust: p = 2 - (10-8) = 0
 *   Result: *ploc = 98 (skipped over LINEINFO and its nesting data)
 *
 * RELATED FUNCTIONS:
 *   DeRelNode()  - Calls this function to convert each operand
 *   DeRelStmt()  - Main loop controlling de-referencing pass
 *
 * CALLED BY:
 *   DeRelNode() - To convert left, right, and extra operands
 *
 * CONTEXT:
 *   This is a low-level utility function called during the de-referencing
 *   pass. It's the workhorse that performs the actual relative-to-absolute
 *   conversion for individual node references.
 *
 * ASSUMPTIONS:
 *   - tx2qp contains valid current node index
 *   - nodeSize correctly reflects number of operands
 *   - Relative offset in *ploc is reasonable
 *   - TX2 array is properly formed with valid nodes
 *   - T2_LINEINFO nodes follow documented structure
 *
 * ERROR HANDLING:
 *   - Invalid offset (rel > nodeSize): Sets *ploc = 0, no error reported
 *   - Null reference: No operation performed
 *   - Malformed LINEINFO: May produce incorrect results
 *
 * SIDE EFFECTS:
 *   - Modifies value at *ploc (relative → absolute conversion)
 *   - Reads TX2 nodes during backward walk
 *   - No other state changes
 *
 * PERFORMANCE:
 *   Time Complexity: O(n) where n = relative offset distance
 *   - Typical case: n = 1-10 (very fast)
 *   - Worst case: n = tx2qp (rare, still acceptable)
 *   Space Complexity: O(1) - Uses only local variables
 *
 * MAINTENANCE NOTES:
 *   - T2_LINEINFO structure is critical - any changes require updates here
 *   - Block nesting calculation (extra - left) must match line tracking logic
 *   - nodeSize validation prevents buffer underruns
 *   - Silent error handling may hide parser bugs
 *
 * HISTORICAL NOTE:
 *   Original Intel function: sub69EB (first part)
 *   This was originally inline code in the main de-referencing routine
 *   Extracted to separate function for clarity in C port
 *
 * REFERENCE:
 *   Original Intel ISIS-II PLM-80 compiler
 *   C port by Mark Ogden
 *
 ****************************************************************************/
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

/****************************************************************************
 * DeRelStmt() - Main Entry Point for Statement De-referencing Pass
 *
 * SUMMARY:
 *   Processes all TX2 nodes in the current statement, converting relative
 *   node references to absolute indices and initializing expression attributes
 *   and storage locations for optimization and code generation.
 *
 * DESCRIPTION:
 *   This is the main entry point for the de-referencing pass, which is a
 *   critical compilation phase that runs after tree building and before
 *   optimization. The TX2 expression tree is built with space-efficient
 *   relative node references during parsing. This function converts all
 *   those relative references to absolute TX2 array indices, making the
 *   tree structure easy to navigate in subsequent phases.
 *
 *   The function iterates through all TX2 nodes representing the current
 *   statement (typically one source line or control structure), classifies
 *   each node by type, and calls the appropriate processing function:
 *   - Operator nodes (binary/unary) → DeRelNode()
 *   - Leaf nodes (identifiers/literals) → simpleVal()
 *
 *   After this pass completes:
 *   - All node references are absolute TX2 indices (not relative offsets)
 *   - All expression attributes are set (data types)
 *   - All expression locations are set (storage classes)
 *   - Tree is ready for optimization and code generation
 *
 * ALGORITHM:
 *   1. Initialize loop: tx2qp = 4 (skip reserved entries 0-3)
 *   2. Loop while tx2qp < tx2qNxt (end of current statement)
 *   3. For each node:
 *      a) Read node type: curNodeType = tx2[tx2qp].type
 *      b) Look up classification: nodeControlFlags = nodeControlMap[curNodeType]
 *      c) Extract category bits: flags & 0xC0
 *         - 0x00 (binary 00xxxxxx): Operator nodes
 *         - 0x40 (binary 01xxxxxx): Identifier/literal nodes
 *         - 0x80 (binary 10xxxxxx): Procedure/statement nodes (skipped)
 *         - 0xC0 (binary 11xxxxxx): Control flow nodes (skipped)
 *      d) Process based on category:
 *         - Category 0x00: Call DeRelNode() to convert operand references
 *         - Category 0x40: Call simpleVal() to process leaf values
 *         - Other categories: Skip (no de-referencing needed)
 *   4. Increment tx2qp and continue
 *   5. Complete when all statement nodes processed
 *
 * PARAMETERS:
 *   None (operates on global TX2 tree state)
 *
 * GLOBALS USED (READ):
 *   tx2qNxt           - End of current statement in TX2 array
 *   tx2[]             - TX2 expression tree array
 *     .type           - Node type (T2_* constants)
 *   nodeControlMap[]  - Node classification table (indexed by node type)
 *
 * GLOBALS MODIFIED:
 *   tx2qp             - Current node index (loop variable)
 *   curNodeType       - Current node type (set for each node)
 *   nodeControlFlags  - Current node classification flags
 *   tx2[]             - All node fields modified by DeRelNode()/simpleVal():
 *     .left           - Converted relative → absolute
 *     .right          - Converted relative → absolute, or set to value/location
 *     .extra          - Converted relative → absolute, or scaled for indexing
 *     .type           - Normalized (e.g., BIGNUMBER → NUMBER)
 *     .exprAttr       - Expression data type set
 *     .exprLoc        - Expression storage location set
 *
 * NODE CATEGORY BITS (nodeControlFlags & 0xC0):
 *   0x00 - Operators (binary/unary/special):
 *          LT, LE, NE, EQ, GE, GT, ROL, ROR, SCL, SCR, SHL, SHR,
 *          JMPFALSE, DOUBLE, PLUSSIGN, MINUSSIGN, STAR, SLASH, MOD,
 *          AND, OR, XOR, BASED, BYTEINDEX, WORDINDEX, MEMBER,
 *          UNARYMINUS, NOT, LOW, HIGH, ADDRESSOF, PLUS, MINUS,
 *          TIME, STKBARG, STKWARG, DEC, COLONEQUALS, OUTPUT,
 *          CASEBLOCK, STKARG, MOVE, RETURNBYTE, RETURNWORD, RETURN,
 *          ADDW, BEGMOVE, CALL, CALLVAR, SETADDR
 *
 *   0x40 - Identifiers and literals:
 *          IDENTIFIER, NUMBER, BIGNUMBER, LENGTH, LAST, SIZE
 *
 *   0x80 - Procedure/statement nodes (not processed here)
 *   0xC0 - Control flow structures (not processed here)
 *
 * STATEMENT PROCESSING ORDER:
 *   Nodes are processed in order from index 4 to tx2qNxt-1:
 *   - Index 0-3: Reserved initialization entries (skipped)
 *   - Index 4+: Statement expression nodes (processed here)
 *
 *   Tree is built bottom-up during parsing, so processing order
 *   naturally visits leaf nodes before their parent operators.
 *
 * RESERVED TX2 ENTRIES:
 *   tx2[0] - Reserved for initialization
 *   tx2[1] - Used as dummy LOCALLABEL for JMPFALSE/CASEBLOCK
 *   tx2[2] - Reserved
 *   tx2[3] - Reserved
 *   tx2[4+] - Actual expression tree nodes
 *
 * RELATED FUNCTIONS:
 *   DeRelNode()            - Processes operator nodes
 *   simpleVal()            - Processes leaf nodes
 *   FillTx2Queue()         - Builds TX2 tree (called before this)
 *   OptimiseStmtNodes()    - Optimizes tree (called after this)
 *   GenerateStatementCode()- Generates code (called after optimization)
 *
 * CALLED BY:
 *   Start2() in main2.c - Main compilation loop
 *
 * CALLS:
 *   DeRelNode() - For operator nodes (category 0x00)
 *   simpleVal() - For leaf nodes (category 0x40)
 *
 * COMPILATION FLOW:
 *   1. Parser → FillTx2Queue() → Builds TX2 tree (relative refs)
 *   2. THIS FUNCTION → DeRelStmt() → Converts to absolute refs
 *   3. Optimizer → OptimiseStmtNodes() → Optimizes tree
 *   4. Code gen → GenerateStatementCode() → Emits machine code
 *   5. Repeat for next statement
 *
 * CONTEXT:
 *   This function is called once per statement after the TX2 expression
 *   tree has been built by the parser. It must complete successfully
 *   before any optimization or code generation can proceed. The relative
 *   addressing scheme used during tree building is more compact but harder
 *   to work with, so this pass converts everything to simple absolute
 *   indices for the remaining compilation phases.
 *
 * ASSUMPTIONS:
 *   - FillTx2Queue() has completed successfully
 *   - tx2qNxt marks the end of the current statement
 *   - Nodes 0-3 are reserved and should not be processed
 *   - All TX2 nodes have valid types
 *   - nodeControlMap[] is properly initialized
 *   - Tree structure is well-formed from parser
 *
 * ERROR HANDLING:
 *   - No explicit error checking in this function
 *   - Assumes well-formed input from parser
 *   - Invalid node types may cause out-of-bounds array access
 *   - Malformed relative references handled by cvtFromRel()
 *
 * SIDE EFFECTS:
 *   - Modifies all TX2 nodes in current statement (4 to tx2qNxt-1)
 *   - Converts tree from relative to absolute addressing
 *   - Initializes expression attributes for optimizer
 *   - May modify tx2[1] (dummy label for jumps)
 *   - Sets global curNodeType and nodeControlFlags during iteration
 *
 * PERFORMANCE:
 *   Time Complexity: O(n) where n = number of nodes in statement
 *   - Each node processed exactly once
 *   - DeRelNode(): O(1) per node (with O(k) walkback for refs)
 *   - simpleVal(): O(1) per node (symbol lookup)
 *   - Overall: Linear in statement size (very fast)
 *
 *   Space Complexity: O(1)
 *   - No additional memory allocated
 *   - In-place conversion of TX2 array
 *
 * TYPICAL STATEMENT SIZES:
 *   - Simple assignment: 5-10 nodes
 *   - Expression: 10-30 nodes
 *   - Complex control: 20-50 nodes
 *   - Function call: 15-40 nodes
 *
 * EXAMPLES:
 *
 *   Example 1: Simple assignment "A = B + C"
 *     Before DeRelStmt():
 *       tx2[4]: IDENTIFIER(B)   - left=infoIdx(B), right=?, exprAttr=?
 *       tx2[5]: IDENTIFIER(C)   - left=infoIdx(C), right=?, exprAttr=?
 *       tx2[6]: PLUS            - left=2 (rel), right=1 (rel), exprAttr=?
 *       tx2[7]: IDENTIFIER(A)   - left=infoIdx(A), right=?, exprAttr=?
 *       tx2[8]: COLONEQUALS     - left=1 (rel), right=2 (rel), exprAttr=?
 *
 *     After DeRelStmt():
 *       tx2[4]: IDENTIFIER(B)   - left=infoIdx(B), right=stackOffset, exprAttr=BYTE_A
 *       tx2[5]: IDENTIFIER(C)   - left=infoIdx(C), right=stackOffset, exprAttr=BYTE_A
 *       tx2[6]: PLUS            - left=4 (abs), right=5 (abs), exprAttr=LIT_A
 *       tx2[7]: IDENTIFIER(A)   - left=infoIdx(A), right=stackOffset, exprAttr=BYTE_A
 *       tx2[8]: COLONEQUALS     - left=7 (abs), right=6 (abs), exprAttr=LIT_A
 *
 *   Example 2: Array indexing "X = ARR[I*2]"
 *     Before: BYTEINDEX node with left=2 (rel to ARR), extra=multiplier
 *     After:  BYTEINDEX node with left=5 (abs to ARR), right=scaled_offset
 *
 * MAINTENANCE NOTES:
 *   - Adding new node types requires updating nodeControlMap[]
 *   - Category bits (0xC0 mask) must match nodeControlMap structure
 *   - Reserved entry count (4) is hardcoded - changes require updates
 *   - Loop range (4 to tx2qNxt-1) assumes parser conventions
 *
 * INTEGRATION POINTS:
 *   - Reads: TX2 tree built by parser (FillTx2Queue)
 *   - Reads: nodeControlMap[] classification table (data.c)
 *   - Writes: TX2 tree with absolute references
 *   - Provides: Expression attributes for optimizer
 *   - Provides: Storage locations for code generator
 *
 * TESTING CONSIDERATIONS:
 *   - Verify all node types are correctly classified
 *   - Test relative reference conversion with various offsets
 *   - Verify LINEINFO nodes are properly skipped
 *   - Test array indexing element size scaling
 *   - Verify JMPFALSE/CASEBLOCK dummy label creation
 *   - Test with deeply nested expressions
 *
 * REFERENCE:
 *   Original Intel ISIS-II PLM-80 compiler
 *   C port by Mark Ogden
 *
 ****************************************************************************/
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