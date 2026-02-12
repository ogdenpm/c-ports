/****************************************************************************
 *  plm2h.c: Statement-Level Code Generation Dispatcher
 *
 *  PART OF: Intel ISIS-II PL/M-80 Compiler (C Port)
 *  Original: Copyright Intel
 *  Re-engineered to C by Mark Ogden <mark.pm.ogden@btinternet.com>
 *
 *  Released for academic interest and personal use only
 *
 *  PURPOSE:
 *  --------
 *  This module implements the top-level statement code generation dispatcher,
 *  responsible for:
 *
 *  1. Main statement processing loop
 *  2. Node type classification and routing
 *  3. Expression code generation coordination
 *  4. Procedure/block structure management
 *  5. Special statement handling (case, loops, calls)
 *  6. Builtin function processing (LENGTH, SIZE)
 *  7. Code generation state management
 *
 *  This is the "glue" between the expression analysis phase (plm2f.c) and
 *  the complete code generation system, acting as the primary dispatcher that
 *  routes different statement types to their appropriate handlers.
 *
 *  COMPILATION FLOW:
 *  -----------------
 *  Start2() (main2.c)
 *    └─ Main compilation loop for each statement:
 *       1. FillTx2Queue() - Build TX2 tree
 *       2. DeRelStmt() (plm2b.c) - Convert relative references
 *       3. OptimiseStmtNodes() (plm2c.c) - Optimize tree
 *       4. GenerateStatementCode() ← THIS MODULE
 *          ├─ Process each TX2 node (4 to tx2qNxt-1)
 *          ├─ Classify by nodeControlMap >> 6
 *          ├─ Route to specific handlers
 *          └─ Generate code sequences
 *
 *  NODE CLASSIFICATION SYSTEM:
 *  ---------------------------
 *  Nodes are routed based on nodeControlMap >> 6:
 *
 *  Category 0 (0xc0 == 0x00): Binary/Unary Operators & Special Operations
 *    ├─ T2_CALL: Direct procedure call → GenerateCallCode()
 *    ├─ T2_CALLVAR: Indirect call → GenerateIndirectCallCode()
 *    ├─ T2_BEGMOVE: Move block start → BeginMoveOperation()
 *    └─ Other operators → GenerateOperatorCode()
 *
 *  Category 1 (0xc0 == 0x40): Builtin Functions
 *    ├─ T2_LENGTH: Array length → GenerateLengthBuiltin(0)
 *    ├─ T2_LAST: Last element index → GenerateLengthBuiltin(1)
 *    └─ T2_SIZE: Element size → GenerateSizeBuiltin()
 *
 *  Category 2 (0xc0 == 0x80): Procedure/Statement Nodes
 *    ├─ T2_PROCEDURE: Procedure definition → c_procedure()
 *    └─ Other statements → ProcessSpecialNodes()
 *
 *  Category 3 (0xc0 == 0xc0): Control Flow Structures
 *    ├─ T2_CASE: Begin case block → EnterCaseBlock()
 *    ├─ T2_ENDCASE: End case block → ExitCaseBlock()
 *    ├─ T2_ENDPROC: End procedure → ExitProcedure()
 *    └─ T2_BEGCALL: Begin call nesting → BeginProcedureCall()
 *
 *  KEY CONCEPTS:
 *  ---------------
 *
 *  1. RETURN GENERATION TRACKING:
 *     - returnGenerated flag: Prevents duplicate return code generation
 *     - nextReturnState: Buffer for return state update
 *     - Set by GenerateReturnSequence() calls
 *     - Reset in ResetCodeGenState() at statement start
 *     - Prevents multiple RET instructions in same procedure
 *
 *  2. PROCEDURE CALL DEPTH TRACKING:
 *     - procCallDepth: Tracks nested call nesting (0-10 max)
 *     - callStackDepth[]: Stack depth for each nesting level
 *     - callStackBase[]: Base stack pointer for each level
 *     - Prevents stack overflow scenarios
 *     - Limits to prevent compiler exhaustion
 *
 *  3. EXPRESSION INLINING OPTIMIZATION:
 *     - TryInlineExpression() attempts to inline simple expressions
 *     - Checks: Binary operator with reference count ≤ 1
 *     - Benefits: Reduces tree depth, fewer memory operations
 *     - Tradeoff: Code duplication for single-use expressions
 *
 *  4. PARAMETER TYPE ENCODING:
 *     - addressTypeEncoding[] = {12, 13} for ADDRESS_T params
 *     - byteTypeEncoding[] = {1, 2} for BYTE_T params
 *     - Used to optimize procedure call sequences
 *     - Encoded in fragmentOpcodeTable for call handlers
 *
 *  5. CODE GENERATION STATE LIFECYCLE:
 *     - ResetCodeGenState(): Initialize for new statement
 *     - RegisterState[].dataType = 0xC (unknown)
 *     - RegisterState[].contents = 0 (empty)
 *     - RegisterState[].isDirect = false
 *     - nextReturnState = false
 *
 *  ALGORITHMS:
 *  -----------
 *
 *  TryInlineExpression():
 *    Purpose: Optimize by replacing expression reference with actual expression
 *
 *    Conditions for success:
 *      1. Left operand is binary/unary operator
 *      2. Reference count ≤ 1 (not shared)
 *      3. Update stack tracking if needed
 *
 *    Actions:
 *      - Move expression from exprNodeIdx to tx2qp
 *      - Preserve reference count from tx2qp
 *      - Update all register contents pointing to old node
 *      - Decrement original node's reference count
 *
 *    Result: Expression evaluated inline, saved tree space
 *
 *  GenerateOperatorCode():
 *    Purpose: Main dispatcher for operator code generation
 *
 *    Flow:
 *      1. Extract left/right operand indices
 *      2. Handle type conversions (T2_DOUBLE → T2_ADDRESSOF)
 *      3. Attempt expression optimization
 *      4. Try expression inlining
 *      5. Generate code based on operator type:
 *         - Binary/unary: GenerateExpressionCode()
 *         - Statement: ProcessSpecialNodes()
 *      6. Special handling: MOVE resets procCallDepth
 *
 *  GenerateCallCode():
 *    Purpose: Generate direct procedure call code
 *
 *    Algorithm:
 *      1. Get called procedure info
 *      2. Set return type attribute
 *      3. Build parameter type encoding:
 *         - For each parameter (scanning backwards):
 *           • Get parameter type
 *           • Encode as ADDRESS or BYTE type
 *           • Combine into fragmentOpcodeTable entry
 *      4. Adjust encoding for 1-parameter case
 *      5. Generate operator code for call
 *      6. Restore stack depth
 *
 *    Special handling:
 *      - ADDRESS_T returns → ADDRESS_A attribute
 *      - BYTE_T returns → BYTE_A attribute
 *      - Parameter encoding order-dependent
 *      - Stack depth restored after call
 *
 *  GenerateIndirectCallCode():
 *    Purpose: Generate indirect (variable) procedure call
 *
 *    Algorithm:
 *      1. Determine call variable type:
 *         - T2_IDENTIFIER: Check F_AUTOMATIC flag
 *           → Type 3 if automatic, 4 if static
 *         - Stack/memory: Type 5 if matches callStackBase
 *         - Default: Type 2 (generic)
 *      2. Set attribute based on type
 *      3. Build parameter type encoding
 *      4. Generate operator code for call
 *      5. Restore stack depth
 *
 *    Differences from direct call:
 *      - Variable call target, not fixed address
 *      - Simpler parameter handling
 *      - Different type encoding scheme
 *
 *  GenerateStatementCode():
 *    Purpose: Main statement processing loop
 *
 *    Algorithm:
 *      1. Reset code generation state
 *      2. For each TX2 node (4 to tx2qNxt-1):
 *         a. Get node type and control flags
 *         b. Classify by nodeControlMap >> 6
 *         c. Switch on category:
 *            - 0: Operators → route to handler
 *            - 1: Builtins → GenerateLengthBuiltin/Size
 *            - 2: Procedures → c_procedure() or ProcessSpecialNodes()
 *            - 3: Control flow → route to handler
 *         d. Clear extra field
 *      3. Final cleanup: AdjustStackOnReturn(0)
 *      4. Update returnGenerated = nextReturnState
 *
 *    State managed:
 *      - registerState[] reset and updated
 *      - currentStackDepth adjusted
 *      - codeSize accumulated
 *      - returnGenerated updated
 *
 *  SPECIAL STATEMENT HANDLING:
 *  ---------------------------
 *
 *  1. CASE BLOCKS (EnterCaseBlock / ExitCaseBlock):
 *     Entry: Save case index, enter new scope
 *     Exit: Generate case label table
 *           For each case label:
 *              - Emit DW (word) directive
 *              - Record label address
 *           Validate: At least one case required
 *           Restore: Restore case index
 *
 *  2. PROCEDURE ENTRY/EXIT (ExitProcedure):
 *     Exit sequence:
 *        - If not returnGenerated: Generate return sequence
 *        - Emit RET instruction
 *     Exit handling:
 *        - Calculate procedure size and stack usage
 *        - Write procedure end marker (0xA4)
 *        - Restore block context
 *        - Reset locals and stack tracking
 *
 *  3. MOVE BLOCKS (BeginMoveOperation):
 *     Setup: procCallDepth = 1
 *     Generate: Operator code for MOVE
 *     Track: Save stack base for MOVE cleanup
 *
 *  4. PROCEDURE CALLS (BeginProcedureCall):
 *     Increment: procCallDepth++
 *     Validate: Check ≤ 10 (max nesting)
 *     Setup: Invalidate all registers, save stack depth
 *     Error: ERR203 if exceeds limit
 *
 *  5. BUILTIN FUNCTIONS:
 *     LENGTH/LAST:
 *        - Get array dimension from symbol table
 *        - Calculate: dim - adjust (0 for LENGTH, 1 for LAST)
 *        - Create constant node with result
 *     SIZE:
 *        - Get element size via GetElementSize()
 *        - Create constant node with size
 *
 *  RETURN STATE MANAGEMENT:
 *  -------------------------
 *
 *  Why separate returnGenerated and nextReturnState?
 *
 *  returnGenerated: Current return state (used in code generation)
 *  nextReturnState: Buffer for return state updates
 *
 *  Flow:
 *    1. Statement starts: returnGenerated = false
 *    2. During processing:
 *       - GenerateReturnSequence() calls set nextReturnState = true
 *       - But don't update returnGenerated yet
 *    3. Statement ends:
 *       - returnGenerated = nextReturnState
 *       - Prevents duplicate returns
 *    4. Next statement starts fresh
 *
 *  Benefits:
 *    - Atomic return state updates
 *    - Prevents intermediate states
 *    - Clear statement boundaries
 *    - Correct cleanup on early returns
 *
 *  REGISTER STATE RESET:
 *  ----------------------
 *
 *  At start of each statement (ResetCodeGenState):
 *
 *    registerState[0-3].dataType = 0xC
 *      - 0xC = unknown/uninitialized
 *      - Indicates no valid type information
 *      - Forces re-analysis before use
 *
 *    registerState[0-3].contents = 0
 *      - Clears previous register contents
 *      - Marks as empty
 *      - Previous values no longer available
 *
 *    registerState[0-3].isDirect = false
 *      - Clears "direct value" flag
 *      - New statement, new tracking
 *      - Must re-establish after operations
 *
 *  Why full reset?
 *    - Statements are independent code units
 *    - Registers can change between statements
 *    - Must not assume register contents persist
 *    - Clean state prevents subtle bugs
 *
 *  INTEGRATION POINTS:
 *  --------------------
 *  - Called from: main2.c::Start2() main loop
 *  - Calls: GenerateExpressionCode() (plm2f.c) - expr code gen
 *  - Calls: ProcessSpecialNodes() (plm2g.c) - special statements
 *  - Calls: c_procedure() (plm2f.c) - procedure entry
 *  - Calls: GenerateReturnSequence() - return code
 *  - Calls: FoldConstantExpr() - constant optimization
 *  - Calls: OptimiseExpression() - expr optimization
 *  - Uses: nodeControlMap[] - node classification
 *  - Uses: fragmentOpcodeTable[] - code fragment encoding
 *  - Updates: codeSize - track generated code
 *  - Updates: currentStackDepth - stack tracking
 *  - Updates: returnGenerated - return state flag
 *
 *  GLOBAL STATE MODIFIED:
 *  ----------------------
 *  - codeSize: Incremented for generated code
 *  - currentStackDepth: Updated during code gen
 *  - stackUsage: Maximum stack depth tracked
 *  - localVariableSize: Local variable allocation
 *  - registerState[]: Register tracking
 *  - curExprLoc[], exprLoc[], exprAttr[]: Expression tracking
 *  - procCallDepth: Procedure call nesting
 *  - callStackDepth[], callStackBase[]: Call tracking
 *  - returnGenerated: Return code flag
 *  - Block structure (blk[], activeGrpCnt, blkId)
 *
 *  CONTROL FLOW GRAPH:
 *  --------------------
 *
 *  GenerateStatementCode()
 *    ├─ ResetCodeGenState()
 *    ├─ For each TX2 node:
 *    │  ├─ Get node type/flags
 *    │  └─ Switch on category:
 *    │     ├─ 0 (Binary/Unary):
 *    │     │  ├─ T2_CALL → GenerateCallCode()
 *    │     │  │   ├─ Encode parameters
 *    │     │  │   └─ GenerateOperatorCode()
 *    │     │  ├─ T2_CALLVAR → GenerateIndirectCallCode()
 *    │     │  ├─ T2_BEGMOVE → BeginMoveOperation()
 *    │     │  └─ Other → GenerateOperatorCode()
 *    │     │     ├─ TryInlineExpression() (if eligible)
 *    │     │     └─ GenerateExpressionCode()
 *    │     ├─ 1 (Builtins):
 *    │     │  ├─ T2_LENGTH → GenerateLengthBuiltin(0)
 *    │     │  ├─ T2_LAST → GenerateLengthBuiltin(1)
 *    │     │  └─ T2_SIZE → GenerateSizeBuiltin()
 *    │     ├─ 2 (Procedures):
 *    │     │  ├─ T2_PROCEDURE → c_procedure()
 *    │     │  └─ Other → ProcessSpecialNodes()
 *    │     └─ 3 (Control):
 *    │        ├─ T2_CASE → EnterCaseBlock()
 *    │        ├─ T2_ENDCASE → ExitCaseBlock()
 *    │        ├─ T2_ENDPROC → ExitProcedure()
 *    │        └─ T2_BEGCALL → BeginProcedureCall()
 *    ├─ AdjustStackOnReturn(0)
 *    └─ returnGenerated = nextReturnState
 *
 *  PERFORMANCE CONSIDERATIONS:
 *  ---------------------------
 *  1. Statement processing: O(N) where N = TX2 nodes
 *  2. Node classification: O(1) via nodeControlMap lookup
 *  3. Call handling: O(P) where P = parameter count
 *  4. Code generation: Varies by statement type
 *
 *  Optimizations:
 *    - Early returns for inlined expressions
 *    - Single pass through TX2 nodes
 *    - Efficient node classification via bit flags
 *    - Parameter encoding combined with generation
 *
 *  DEBUGGING NOTES:
 *  ----------------
 *  - Trace nodeControlFlags >> 6 to see routing
 *  - Watch procCallDepth for call nesting issues
 *  - Monitor returnGenerated for duplicate returns
 *  - Check registerState[] reset effectiveness
 *  - Verify parameter encoding in GenerateCallCode
 *
 *  FUTURE IMPROVEMENTS:
 *  ---------------------
 *  1. Tail call optimization
 *     - Detect last statement in procedure
 *     - Replace CALL/RET with JMP
 *
 *  2. Register pressure analysis
 *     - Predict register usage before generation
 *     - Guide operand ordering decisions
 *
 *  3. Early loop detection
 *     - Identify loops during code generation
 *     - Apply loop-specific optimizations
 *
 *  4. Procedure inlining
 *     - Small procedures inlined at call sites
 *     - Reduce call overhead
 *
 *  VERSION HISTORY:
 *  ----------------
 *  - Original: Intel ISIS-II PLM-80 compiler
 *  - C port: Mark Ogden
 *  - Current: Fully renamed and documented
 *
 ****************************************************************************/
#include "plm.h"

/**
 * TryInlineExpression - Attempt to inline simple expression
 *
 * Optimizes by replacing an expression reference with the actual expression
 * when beneficial. Reduces tree depth and memory operations for single-use
 * expressions.
 *
 * Conditions:
 *   1. Left operand is binary/unary operator (not leaf node)
 *   2. Reference count ≤ 1 (expression used only here)
 *   3. Update stack tracking if expression was on stack
 *
 * @return true if expression was successfully inlined, false otherwise
 *
 * Side effects:
 *   - Updates TX2 tree structure
 *   - Updates register state
 *   - Modifies reference counts
 *
 * Called from: GenerateOperatorCode() when eligible
 */
static bool TryInlineExpression() {
    uint8_t exprNodeIdx, regIdx, savedRefCount;

    exprNodeIdx = (uint8_t)tx2[tx2qp].left;
    if ((nodeControlMap[tx2[exprNodeIdx].type] & 0xc0) == 0) {
        if (tx2[exprNodeIdx].cnt > 1)
            return false;
        if (tx2[exprNodeIdx].extra != 0)
            stackNodeContents[tx2[exprNodeIdx].extra] = tx2qp;
    }
    savedRefCount = (uint8_t)tx2[tx2qp].cnt;
    MoveTx2(exprNodeIdx, tx2qp);
    tx2[tx2qp].cnt = savedRefCount;
    tx2[exprNodeIdx].cnt--;
    for (regIdx = IR_PSW; regIdx <= IR_H; regIdx++) {
        if (registerState[regIdx].contents == exprNodeIdx)
            registerState[regIdx].contents = tx2qp;
    }
    return true;
}

/**
 * GenerateOperatorCode - Dispatcher for operator code generation
 *
 * Routes operator nodes to appropriate code generation handlers,
 * with special handling for optimizations like inlining and constant folding.
 *
 * Algorithm:
 *   1. Extract operands (left and right indices)
 *   2. Handle type conversions (T2_DOUBLE through T2_ADDRESSOF)
 *   3. Optimize expressions via FoldConstantExpr
 *   4. Attempt expression inlining if eligible
 *   5. Route by operator category:
 *      - Binary/Unary: Generate expression code
 *      - Statement-level: Process special nodes
 *   6. Special handling for MOVE (reset call depth)
 *
 * Side effects:
 *   - Generates code for operators
 *   - Updates expression state
 *   - Modifies register state
 *   - May inline expressions
 *
 * Called from: GenerateStatementCode() for operators
 */
void GenerateOperatorCode() {
    curExprLoc[Left]  = (uint8_t)tx2[tx2qp].left;
    curExprLoc[Right] = (uint8_t)tx2[tx2qp].right;
    if (T2_DOUBLE <= curNodeType && curNodeType <= T2_ADDRESSOF) // type conversion
        FoldConstantExpr();
    if (curNodeType <= T2_MEMBER) {
        OptimiseExpression();
        if (curNodeType == T2_65)
            if (TryInlineExpression()) // attempt inline optimisation
                return;
    }
    if ((nodeControlFlags & 0xc0) == 0) { // binary/unary operator
        GenerateExpressionCode();
        if (curNodeType == T2_MOVE)
            procCallDepth = 0;
    } else if ((nodeControlFlags & 0xc0) == 0x80) // statement level node
        ProcessSpecialNodes();                    // handle statement-level codegeneration
}

static uint8_t addressTypeEncoding[2] = { 12, 13 };
static uint8_t byteTypeEncoding[2]    = { 1, 2 };

/**
 * GenerateCallCode - Generate direct procedure call code
 *
 * Generates code for direct CALL instructions including parameter
 * type encoding for the called procedure.
 *
 * Algorithm:
 *   1. Get called procedure info from symbol table
 *   2. Set return type: ADDRESS_A or BYTE_A
 *   3. Build parameter type encoding:
 *      - Scan parameters in reverse order
 *      - For each parameter: Get type (ADDRESS_T or BYTE_T)
 *      - Encode as appropriate type code
 *      - Combine into fragmentOpcodeTable entry
 *   4. Special case: Single parameter gets left-shifted encoding
 *   5. Generate operator code for call expression
 *   6. Restore stack depth to pre-call state
 *
 * Special handling:
 *   - Parameter count 0: Simple call
 *   - Parameter count 1: Single parameter, special encoding
 *   - Parameter count 2+: Multiple parameters, normal encoding
 *   - Return type affects attribute mapping
 *
 * Side effects:
 *   - Generates CALL code fragment
 *   - Updates fragmentOpcodeTable with parameter encoding
 *   - Updates codeSize
 *   - Restores stack depth
 *
 * Called from: GenerateStatementCode() for T2_CALL nodes
 */
void GenerateCallCode() {
    uint8_t remainingParams, paramIdx;
    pointer encodingPtr;
    uint8_t totalParams;

    if (procCallDepth <= 10) {
        info                         = FromIdx(tx2[tx2qp].extra);
        nodeTypeToAttribute[T2_CALL] = info->returnType == ADDRESS_T ? ADDRESS_A : BYTE_A;
        remainingParams = totalParams = info->paramCnt;
        encodingPtr                   = &fragmentOpcodeTable[nodeTypeToAttribute[T2_CALL]];
        paramIdx                      = 0;

        while (remainingParams > 0) {
            AdvNxtInfo();
            if (--remainingParams < 2) {
                *encodingPtr =
                    ((*encodingPtr << 4) | (info->type == ADDRESS_T ? addressTypeEncoding[paramIdx]
                                                                    : byteTypeEncoding[paramIdx]));
                paramIdx = 1;
            }
        }

        if (totalParams == 1)
            *encodingPtr <<= 4;
        GenerateOperatorCode();
        currentStackDepth = callStackDepth[procCallDepth];
    }
    procCallDepth--;
}

/**
 * GetParameterTypeEncoding - Get type encoding for parameter
 *
 * Returns the appropriate type encoding (ADDRESS or BYTE) for a parameter
 * based on its expression index and side (Left or Right).
 *
 * @param exprIdx - Expression node index (0 = null)
 * @param lrIdx - Operand side (0=Left, 1=Right)
 * @return Type encoding value
 *
 * Called from: GenerateIndirectCallCode() for parameter encoding
 */
static uint8_t GetParameterTypeEncoding(uint8_t exprIdx, uint8_t lrIdx) {
    return exprIdx == 0                      ? 0
           : tx2[exprIdx].exprAttr == BYTE_A ? byteTypeEncoding[lrIdx]
                                             : addressTypeEncoding[lrIdx];
}

/**
 * GenerateIndirectCallCode - Generate indirect procedure call code
 *
 * Generates code for CALLVAR instructions (indirect/variable procedure calls).
 * Handles calls through procedure variables and function pointers.
 *
 * Algorithm:
 *   1. Determine call variable type:
 *      - If T2_IDENTIFIER: Check F_AUTOMATIC flag
 *        • Type 3 if automatic (stack) variable
 *        • Type 4 if static/memory variable
 *      - If matches callStackBase: Type 5 (stack-based)
 *      - Otherwise: Type 2 (generic)
 *   2. Set attribute from call variable type
 *   3. Encode parameter types for Left and Right operands
 *   4. Generate operator code for call expression
 *   5. Restore stack depth
 *
 * Differences from direct call:
 *   - Call target is variable, not fixed address
 *   - Simpler parameter handling (0, 1, or 2)
 *   - Call variable type determines encoding strategy
 *
 * Side effects:
 *   - Generates CALLVAR code fragment
 *   - Updates fragmentOpcodeTable entry
 *   - Updates codeSize
 *   - Restores stack depth
 *
 * Called from: GenerateStatementCode() for T2_CALLVAR nodes
 */
void GenerateIndirectCallCode() {
    uint8_t callVarNodeIdx;

    if (procCallDepth <= 10) {
        uint8_t callVariableType;
        callVarNodeIdx = (uint8_t)tx2[tx2qp].extra;
        if (tx2[callVarNodeIdx].type == T2_IDENTIFIER) {
            info = FromIdx(tx2[callVarNodeIdx].left);
            callVariableType =
                info->flag & F_AUTOMATIC ? 3 : 4; // automatic or static/memory variable
        } else if (tx2[callVarNodeIdx].extra == callStackBase[procCallDepth]) {
            callVariableType = 5; // stack/memory variable
            callStackDepth[procCallDepth]--;
        } else
            callVariableType = 2; // generic call variable

        nodeTypeToAttribute[T2_CALLVAR] = callVariableType;
        fragmentOpcodeTable[callVariableType] =
            (GetParameterTypeEncoding((uint8_t)tx2[tx2qp].left, Left) << 4) |
            GetParameterTypeEncoding((uint8_t)tx2[tx2qp].right, Right);
        GenerateOperatorCode();
        currentStackDepth = callStackDepth[procCallDepth];
    }
    procCallDepth--;
}

/**
 * BeginMoveOperation - Initialize MOVE block operation
 *
 * Sets up state for MOVE (bulk data transfer) operations.
 * MOVE blocks are special multi-statement operations that copy data
 * between memory areas.
 *
 * Setup:
 *   - procCallDepth = 1 (special state for MOVE)
 *   - Generate operator code to start MOVE
 *   - Save current stack depth as MOVE base
 *
 * Side effects:
 *   - Updates procCallDepth
 *   - Generates MOVE start code
 *   - Updates callStackBase
 *
 * Called from: GenerateStatementCode() for T2_BEGMOVE nodes
 */
void BeginMoveOperation() {
    procCallDepth = 1;
    GenerateOperatorCode();
    callStackBase[procCallDepth] = currentStackDepth;
}

/**
 * EnterCaseBlock - Begin CASE statement block
 *
 * Initializes a CASE block scope. CASE blocks are multi-way branch structures
 * with multiple possible execution paths.
 *
 * Called from: GenerateStatementCode() for T2_CASE nodes
 */
void EnterCaseBlock() {
    if (EnterBlk())
        blk[activeGrpCnt].activeGrpCnt = topCase;
}

/**
 * ExitCaseBlock - Finalize CASE statement block
 *
 * Generates case label table and cleanup for CASE block.
 * Ensures at least one case was defined in the block.
 *
 * Algorithm:
 *   1. Exit block scope
 *   2. Generate case label table:
 *      - For each case from saved index to topCase:
 *         • Emit DW (word) directive
 *         • Record case label index
 *   3. Validate: Ensure at least one case exists
 *      - If topCase == savedTopCase: Error ERR201
 *   4. Restore case index to pre-CASE state
 *
 * Side effects:
 *   - Generates case label table
 *   - Updates codeSize by 2 bytes per case
 *   - May generate error
 *
 * Called from: GenerateStatementCode() for T2_ENDCASE nodes
 */
void ExitCaseBlock() {
    index_t curCase, savedTopCase;
    curCase = savedTopCase = blk[activeGrpCnt].activeGrpCnt;
    if (ExitBlk()) {
        while (curCase < topCase) {
            iCodeArgs[0] = IR_CASELAB;
            iCodeArgs[1] = casetab[curCase];
            EncodeFragData(CF_DW);
            codeSize += 2;
            curCase++;
        }
        if (topCase == savedTopCase) {
            Tx2SyntaxError(ERR201); /*  Invalid CASE block, at least one case required */
            EmitTopItem();
        }
        topCase = savedTopCase;
    }
}

/**
 * ExitProcedure - Finalize procedure code generation
 *
 * Handles procedure exit including:
 * - Return code generation
 * - Procedure metadata recording
 * - Block structure cleanup
 * - State restoration
 *
 * Algorithm:
 *   1. Exit procedure block
 *   2. Generate return code if needed:
 *      - If !returnGenerated: Generate return sequence + RET
 *   3. Update procedure metadata:
 *      - dim = codeSize (procedure size)
 *      - baseVal = stackUsage + localVariableSize
 *      - For F_INTERRUPT: Add 8 to stackUsage (register saves)
 *   4. Restore block context:
 *      - Move to next block (enclosing scope)
 *      - Reset code size to that block's size
 *      - Clear fragment tracking
 *   5. Write procedure end marker:
 *      - Emit 0xA4 marker
 *      - Write procedure info index
 *      - Write procedure code size
 *   6. Restore state variables:
 *      - currentStackDepth
 *      - stackUsage
 *      - localVariableSize
 *      - curExtProcId
 *
 * Side effects:
 *   - Generates return code
 *   - Updates codeSize
 *   - Updates procedure metadata
 *   - Writes procedure markers
 *   - Restores compilation state
 *
 * Called from: GenerateStatementCode() for T2_ENDPROC nodes
 */
void ExitProcedure() {
    if (ExitBlk()) {
        info = blk[blkId].info;
        if (!returnGenerated) {
            GenerateReturnSequence();
            EncodeFragData(CF_RET);
            codeSize++;
        }
        if ((info->flag & F_INTERRUPT))
            stackUsage += 8;

        info->dim     = codeSize;
        info->baseVal = stackUsage + localVariableSize;
        codeSize      = blk[blkId = blk[blkId].next].codeSize;
        fragLen       = 0;
        PutTx1Byte(0xa4);
        PutTx1Word(ToIdx(blk[blkId].info));
        PutTx1Word(codeSize);
        WrFragData();
        currentStackDepth = blk[blkId].wB4B0;
        stackUsage        = blk[blkId].stackSize;
        localVariableSize = 0;
        curExtProcId      = blk[blkId].extProcId;
    }
}

/**
 * GenerateLengthBuiltin - Generate LENGTH or LAST builtin function code
 *
 * Generates code for LENGTH (array dimension) and LAST (last valid index)
 * builtin functions.
 *
 * @param adjust - 0 for LENGTH (full dimension), 1 for LAST (dimension - 1)
 *
 * Algorithm:
 *   1. Get array symbol info
 *   2. Calculate result: dim - adjust
 *   3. Create constant node with result
 *   4. Determine attribute: BYTE_A or ADDRESS_A based on size
 *
 * Side effects:
 *   - Generates code via CreateConstantOrIdNode
 *   - Updates codeSize
 *
 * Called from: GenerateStatementCode() for T2_LENGTH, T2_LAST nodes
 */
void GenerateLengthBuiltin(uint8_t adjust) {
    uint16_t resultValue;
    info        = FromIdx(tx2[tx2qp].left);
    resultValue = info->dim - adjust;
    CreateConstantOrIdNode(resultValue, NULL, resultValue < 0x100 ? BYTE_A : ADDRESS_A, LOC_REG);
}

/**
 * GenerateSizeBuiltin - Generate SIZE builtin function code
 *
 * Generates code for SIZE (element size) builtin function.
 *
 * Algorithm:
 *   1. Get array/struct symbol info
 *   2. Calculate element size via GetElementSize
 *   3. Create constant node with size
 *   4. Determine attribute: BYTE_A or ADDRESS_A based on size
 *
 * Side effects:
 *   - Generates code via CreateConstantOrIdNode
 *   - Updates codeSize
 *
 * Called from: GenerateStatementCode() for T2_SIZE nodes
 */
void GenerateSizeBuiltin() {
    uint16_t resultValue;
    resultValue = GetElementSize(FromIdx(tx2[tx2qp].left));
    CreateConstantOrIdNode(resultValue, NULL, resultValue < 0x100 ? BYTE_A : ADDRESS_A, LOC_REG);
}

/**
 * BeginProcedureCall - Initialize procedure call sequence
 *
 * Manages procedure call nesting by incrementing call depth and
 * initializing state for the call.
 *
 * Algorithm:
 *   1. Increment procCallDepth
 *   2. If depth <= 10:
 *      a. Invalidate all registers (may be modified by call)
 *      b. Save current stack depth
 *   3. If depth == 11:
 *      - Generate error ERR203 (nesting limit exceeded)
 *
 * Error handling:
 *   - Maximum 10 levels of nested procedure calls
 *   - Prevents unbounded recursion
 *   - Compiler resource exhaustion
 *
 * Side effects:
 *   - Increments procCallDepth
 *   - Invalidates registers
 *   - Saves stack tracking state
 *   - May generate error
 *
 * Called from: GenerateStatementCode() for T2_BEGCALL nodes
 */
void BeginProcedureCall() {
    procCallDepth++;
    if (procCallDepth <= 10) {
        InvalidateRegistersByMask(0xf);
        callStackDepth[procCallDepth] = currentStackDepth;
        callStackBase[procCallDepth]  = currentStackDepth;
    } else if (procCallDepth == 11) {
        Tx2SyntaxError(ERR203); /*  LIMIT EXCEEDED: NESTING OF TYPED procedure CALLS */
        EmitTopItem();
    }
}

/**
 * ResetCodeGenState - Initialize code generation state for new statement
 *
 * Clears all transient code generation state before processing a new statement.
 * Ensures clean state and prevents state leakage between statements.
 *
 * State reset:
 *   - nextReturnState = false (clear pending return)
 *   - registerState[0-3].dataType = 0xC (unknown, force re-analysis)
 *   - registerState[0-3].contents = 0 (no values in registers)
 *   - registerState[0-3].isDirect = false (no valid directness info)
 *
 * Why 0xC for dataType?
 *   - Not BYTE_A (0), ADDRESS_A (3, 6), or other valid values
 *   - Indicates "uninitialized/unknown"
 *   - Forces re-analysis before register usage
 *   - Prevents using stale type information
 *
 * Side effects:
 *   - Clears registerState array
 *   - Resets nextReturnState
 *
 * Called from: GenerateStatementCode() at start of statement processing
 */
static void ResetCodeGenState() {
    uint8_t regIdx;

    nextReturnState = false;
    for (regIdx = 0; regIdx <= 3; regIdx++) {
        registerState[regIdx].dataType = 0xc;   // unknown - force re-analysis
        registerState[regIdx].contents = 0;     // no values in registers
        registerState[regIdx].isDirect = false; // no directness info
    }
}

/**
 * GenerateStatementCode - Main statement code generation dispatcher
 *
 * Central entry point for statement-level code generation. Processes all
 * TX2 nodes in the current statement and routes each to the appropriate handler.
 *
 * Algorithm:
 *   1. Reset code generation state
 *   2. For each TX2 node (4 to tx2qNxt-1):
 *      a. Get node type and control flags
 *      b. Classify by nodeControlMap >> 6:
 *         - Category 0: Operators → route by type
 *         - Category 1: Builtins → call builtin handler
 *         - Category 2: Procedures/statements → call handler
 *         - Category 3: Control flow → call handler
 *      c. Clear node's extra field
 *   3. Final cleanup: AdjustStackOnReturn(0)
 *   4. Update return state: returnGenerated = nextReturnState
 *
 * Node Categories (nodeControlMap >> 6):
 *
 *   Category 0 (Binary/Unary Operators):
 *     ├─ T2_CALL → GenerateCallCode()
 *     ├─ T2_CALLVAR → GenerateIndirectCallCode()
 *     ├─ T2_BEGMOVE → BeginMoveOperation()
 *     └─ Other → GenerateOperatorCode()
 *
 *   Category 1 (Builtin Functions):
 *     ├─ T2_LENGTH → GenerateLengthBuiltin(0)
 *     ├─ T2_LAST → GenerateLengthBuiltin(1)
 *     └─ T2_SIZE → GenerateSizeBuiltin()
 *
 *   Category 2 (Procedures/Statements):
 *     ├─ T2_PROCEDURE → c_procedure()
 *     └─ Other → ProcessSpecialNodes()
 *
 *   Category 3 (Control Flow):
 *     ├─ T2_CASE → EnterCaseBlock()
 *     ├─ T2_ENDCASE → ExitCaseBlock()
 *     ├─ T2_ENDPROC → ExitProcedure()
 *     └─ T2_BEGCALL → BeginProcedureCall()
 *
 * State Management:
 *   - Statement starts: Clean register state via ResetCodeGenState()
 *   - During: Register state updated by handlers
 *   - End: Stack adjusted, return state finalized
 *
 * Side effects:
 *   - Generates complete code for statement
 *   - Updates codeSize, currentStackDepth, stackUsage
 *   - Modifies registerState and block structures
 *   - Updates returnGenerated flag
 *
 * Called from: main2.c::Start2() main compilation loop
 * Postcondition: Code generated for all nodes in current statement
 */
void GenerateStatementCode() {
    ResetCodeGenState();
    for (tx2qp = 4; tx2qp <= tx2qNxt - 1; tx2qp++) {
        curNodeType      = tx2[tx2qp].type;
        nodeControlFlags = nodeControlMap[curNodeType];
        switch (nodeControlFlags >> 6) { // node category
        case 0:                          // Binary/Unary operators & special operations
            if (curNodeType == T2_CALL)
                GenerateCallCode();
            else if (curNodeType == T2_CALLVAR)
                GenerateIndirectCallCode();
            else if (curNodeType == T2_BEGMOVE)
                BeginMoveOperation();
            else
                GenerateOperatorCode();
            break;
        case 1: // builtin functions (LENGTH, LAST, SIZE)
            if (curNodeType == T2_LENGTH)
                GenerateLengthBuiltin(0);
            else if (curNodeType == T2_LAST)
                GenerateLengthBuiltin(1);
            else if (curNodeType == T2_SIZE)
                GenerateSizeBuiltin();
            break;
        case 2: // procedure/statement nodes
            if (curNodeType == T2_PROCEDURE)
                c_procedure();
            else
                ProcessSpecialNodes();
            break;
        case 3: // control flow structures (CASE, ENDCASE, ENDPROC, BEGCALL)
            if (curNodeType == T2_CASE)
                EnterCaseBlock();
            else if (curNodeType == T2_ENDCASE)
                ExitCaseBlock();
            else if (curNodeType == T2_ENDPROC)
                ExitProcedure();
            else if (curNodeType == T2_BEGCALL)
                BeginProcedureCall();
            break;
        }

        tx2[tx2qp].extra = 0;
    }
    AdjustStackOnReturn(0);            // final code generation cleanup
    returnGenerated = nextReturnState; // update code generation state flag
}