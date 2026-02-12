/****************************************************************************
 *  plm2a.c: Code Generation Data Tables & Register Management Utilities
 *
 *  PART OF: Intel ISIS-II PL/M-80 Compiler (C Port)
 *  Original: Copyright Intel
 *  Re-engineered to C by Mark Ogden <mark.pm.ogden@btinternet.com>
 *
 *  Released for academic interest and personal use only
 *
 *  PURPOSE:
 *  --------
 *  This module contains the critical lookup tables and encoding information
 *  that drive the 8080 code generation system. It serves as the "configuration
 *  database" for the compiler's ability to generate efficient assembly code.
 *
 *  The tables encode:
 *
 *  1. CODE FRAGMENT ENCODING - How to emit 8080 instructions
 *  2. HELPER FUNCTION MAPPING - Runtime library linkage
 *  3. TYPE CONVERSION RULES - Data type handling and conversions
 *  4. OPERATION ENCODING - Instruction selection and optimization
 *  5. OPERAND ENCODING - How operands map to registers and memory
 *  6. REGISTER MANAGEMENT - Register state tracking and allocation
 *
 *  These tables are used during the code generation phase (plm2f.c, plm2h.c)
 *  to make decisions about:
 *  - Which instruction sequence to use for an operation
 *  - How to load operands into registers
 *  - How to handle type conversions
 *  - Which helper functions to link in
 *  - How to optimize register usage
 *
 *  DATA TABLE ORGANIZATION:
 *  -------------------------
 *
 *  The tables are organized by purpose:
 *
 *  FRAGMENT ENCODING (Code Generation):
 *    - fragControlBits[] - Control bit encoding for fragments
 *    - fragmentCodeLength[] - Size of each fragment in bytes
 *    - fragmentOpcodeTable[] - Opcode encoding for fragments
 *    - fragmentOpcodeIndex[] - Fragment index/precedence
 *    - fragmentArgTypes[] - Fragment argument types
 *    - loadOpAttributes[] - Operand load strategy attributes
 *    - loadOpTargetReg[] - Target register selection
 *    - loadOpFragment[] - Fragment selection for loads
 *
 *  HELPER FUNCTION MAPPING (Runtime Library):
 *    - helperIndexMap[] - Helper index mapping
 *    - fragmentHelperIndex[] - Fragment-to-helper index
 *    - helperMap[][] - Helper function ID mapping
 *    - helperGroup[] - Helper group classification
 *    - noteTypeToHelperGroup[] - Node type to helper group
 *
 *  OPERAND AND REGISTER ENCODING:
 *    - opcodeEncodingMap[] - Opcode to encoding mapping
 *    - exprComplexity[] - Expression complexity costs
 *    - exprRegisterCost[] - Register usage cost
 *    - typeConversionTable[][] - Type conversion costs
 *    - nodeExprCategory[] - Node expression categorization
 *    - lookupResultAttr[] - Result attribute lookup
 *    - lookupResultLoc[] - Result location lookup
 *    - attrLocLookupTable[][] - Attribute-location mapping
 *
 *  OPERATOR SEMANTICS:
 *    - operatorPrecedence[] - Operator precedence rules
 *    - operatorTypeRules[] - Operator type rules
 *    - operatorResultType[] - Operator result type encoding
 *
 *  OPTIMIZATION TABLES:
 *    - optimisationStepTable[] - Optimization step encoding
 *    - optimisationStep2Map[] - Secondary optimization mapping
 *    - optimisationStep1Map[] - Primary optimization mapping
 *    - step2ActionTable[] - Optimization action table
 *    - peepholePatterns[] - Peephole optimization patterns
 *    - constFoldRules[] - Constant folding rules
 *
 *  COMPILATION FLOW:
 *  -----------------
 *  Start2() (main2.c)
 *    └─ Main compilation loop for each statement:
 *       1. FillTx2Queue() - Build TX2 tree
 *       2. DeRelStmt() (plm2b.c) - Convert relative references
 *       3. OptimiseStmtNodes() (plm2c.c) - Optimize tree
 *       4. GenerateStatementCode() (plm2h.c)
 *          └─ GenerateExpressionCode() (plm2f.c)
 *             ├─ SelectOptimalFragment() ← Uses tables
 *             └─ GenerateOperandCode() (plm2e.c) ← Uses tables
 *                └─ EncodeFragData() ← Uses tables
 *
 *  KEY TABLE CATEGORIES:
 *  ----------------------
 *
 *  1. FRAGMENT CONTROL BITS (fragControlBits[]):
 *     Encoded as: aaa bbb ccc 0
 *     - aaa (bits 7-5): Node type specifier
 *     - bbb (bits 4-2): Source operand encoding
 *     - ccc (bits 1-0): Dest operand encoding
 *     Used to determine how to encode fragment arguments
 *
 *  2. FRAGMENT CODE LENGTH (fragmentCodeLength[]):
 *     Format: xxx nnnnn
 *     - xxx (bits 7-5): Fragment category
 *     - nnnnn (bits 4-0): Code size in bytes
 *     Used to calculate total code generation cost
 *
 *  3. TYPE CONVERSION TABLE (typeConversionTable[][]):
 *     2D table: [source_type][target_type]
 *     - Entries: 0 (free), 1-n (cost), 0x30+ (special encoding)
 *     Used to find cheapest conversion between types
 *     Lower cost = preferred conversion method
 *
 *  4. OPERAND CATEGORY (nodeExprCategory[]):
 *     Maps node types to operand categories (0-0x14)
 *     Used to determine:
 *     - How operand is encoded
 *     - Which registers can hold it
 *     - What conversions are needed
 *
 *  5. HELPER FUNCTION MAPPING:
 *     fragmentHelperIndex[] → helperIndexMap[] → helperMap[group][]
 *     Allows quick lookup of runtime library helper IDs
 *     Used to link required helper functions
 *
 *  FUNCTION ORGANIZATION:
 *  ----------------------
 *
 *  The module also contains critical utility functions:
 *
 *  CODE EMISSION:
 *    - WrFragData() - Write fragment to output
 *    - PutTx1Byte() - Put byte to fragment buffer
 *    - PutTx1Word() - Put word to fragment buffer
 *    - EncodeFragData() - Encode and emit code fragment
 *    - EncodeFragArgs() - Encode fragment arguments
 *    - EncodeFragmentArgument() - Encode single argument
 *    - EmitTopItem() - Emit TX2 node
 *
 *  REGISTER MANAGEMENT:
 *    - AnalyzeRegisterUsage() - Determine register preservation
 *    - SaveRegisterToStack() - Save register to stack
 *    - RestoreRegisterFromStack() - Restore register from stack
 *    - SaveOrRedirectRegister() - Save or redirect register
 *    - SpillNonActiveRegisters() - Spill non-active registers
 *    - InvalidateRegistersByMask() - Invalidate register set
 *    - PushRegisterToStack() - Push register with tracking
 *    - PopRegisterFromStack() - Pop register with tracking
 *    - AdjustRegisterOffset() - Normalize register offsets
 *
 *  OPERAND ENCODING:
 *    - EncodeOperandInfo() - Encode operand information
 *    - EncodeExpressionOperand() - Encode expression operand
 *    - GetOperandValue() - Extract operand value
 *    - GetFragmentOperandType() - Get fragment operand type
 *
 *  EXPRESSION MANAGEMENT:
 *    - DecrementExprRefs() - Decrement expression references
 *    - UpdateExpressionLookup() - Update expression lookup code
 *    - CreateConstantOrIdNode() - Create constant node
 *    - IndirectAddr() - Calculate indirect address mode
 *    - MoveTx2() - Move TX2 node
 *
 *  STACK MANAGEMENT:
 *    - AdjustStackOnReturn() - Adjust stack on procedure return
 *    - GenerateReturnSequence() - Generate return code
 *
 *  BLOCK MANAGEMENT:
 *    - EnterBlk() - Enter new block scope
 *    - ExitBlk() - Exit block scope
 *    - HandleFatalError() - Handle fatal compiler error
 *
 *  TX2 PROCESSING:
 *    - FillTx2Queue() - Fill TX2 expression queue
 *    - SetFirstStatementEnd() - Mark statement end
 *    - GetTx2Item() - Read TX2 item
 *    - MergeRedundantNodes() - Merge redundant TX2 nodes
 *    - SkipStmt() - Skip to next/previous statement
 *
 *  ERROR HANDLING:
 *    - Tx2SyntaxError() - Mark syntax error
 *
 *  CRITICAL LOOKUP PATTERNS:
 *  --------------------------
 *
 *  Pattern 1: Fragment Selection
 *    operatorResultType[curNodeType] → range of fragments
 *    For each fragment in range:
 *      typeConversionTable[leftType][fragmentType] → cost
 *      + fragmentCodeLength[fragment] & 0x1f
 *    Select fragment with minimum total cost
 *
 *  Pattern 2: Operand Encoding
 *    exprAttr[side] → operandCategory lookup
 *    operandCategory[side] → loadOpTargetReg/loadOpAttributes
 *    loadOpAttributes >> 4 → state update strategy
 *    loadOpAttributes & 0xf → attribute assignment strategy
 *
 *  Pattern 3: Helper Function Linkage
 *    fragmentHelperIndex[cfrag] → helperIndexMap index
 *    helperIndexMap[index] → helper index
 *    noteTypeToHelperGroup[curNodeType] → group
 *    helperGroup[group] → primary group index
 *    helperMap[group][index] → helper ID
 *    → Mark helper as used via helperAddr[ID] = 1
 *
 *  Pattern 4: Type Conversion
 *    typeConversionTable[sourceType][targetType]
 *    0 = no conversion needed
 *    1-127 = cost of conversion
 *    128+ = special encoding or error
 *
 *  DATA FLOW EXAMPLE - Simple Addition:
 *  -------------------------------------
 *
 *  TX2 Node: PLUS (operands: left=INT, right=INT)
 *
 *  1. Get operation fragments:
 *     operatorResultType[T2_PLUS] = 3
 *     first = nodeTypeToAttribute[T2_PLUS]
 *     last = first + 3 - 1
 *
 *  2. Analyze operands:
 *     operandCategory[Left] = INT_CATEGORY
 *     operandCategory[Right] = INT_CATEGORY
 *
 *  3. Select optimal fragment:
 *     For each fragment in range:
 *       leftCost = exprComplexity[INT_CATEGORY]
 *       rightCost = exprComplexity[INT_CATEGORY]
 *       fragSize = fragmentCodeLength[fragment] & 0x1f
 *       totalCost = leftCost + rightCost + fragSize
 *       if totalCost < minCost: select this fragment
 *
 *  4. Generate code:
 *     cfrag1 = opcodeEncodingMap[selectedFragment]
 *     EncodeFragData(cfrag1) → emits instruction sequence
 *
 *  5. Link helpers if needed:
 *     if cfrag1 > CF_171:
 *       i = helperIndexMap[fragmentHelperIndex[cfrag1-CF_174]]
 *       grp = helperGroup[noteTypeToHelperGroup[T2_PLUS]]
 *       helperID = helperMap[grp][i]
 *       helperAddr[helperID] = 1
 *
 *  OPTIMIZATION TABLES:
 *  ---------------------
 *
 *  optimisationStepTable[] maps node types to optimization patterns
 *  Values:
 *    0xFF = no optimization
 *    Other values = optimization action code
 *
 *  peepholePatterns[] encodes instruction pattern replacements
 *  Used to optimize generated code sequences
 *
 *  constFoldRules[] determine if expressions can be folded at compile time
 *  Format: (left_rules << 6) | (right_rules << 3) | result_rules
 *
 *  REGISTER STATE TRACKING:
 *  -------------------------
 *
 *  Register tracking is maintained across code generation:
 *
 *  registerState[].contents: TX2 node index in register
 *  registerState[].dataType: Expression attribute/type
 *  registerState[].isDirect: Contains direct value (not stack-relative)
 *  registerState[].offset: Offset adjustment for address
 *  registerState[].stackOffset: Stack offset if spilled
 *  registerState[].storageClass: Storage type flags
 *
 *  registerNeedsSave[]: Register value needed later (must preserve)
 *  registerInExpression[]: Register used in current expression
 *  registerHasValue[]: Register contains a value
 *  registerWasSaved[]: Register was saved to stack
 *
 *  STACK FRAME LAYOUT:
 *  --------------------
 *
 *  Stack tracking uses:
 *  - currentStackDepth: Current depth (0-127)
 *  - stackNodeContents[]: TX2 node at each stack level
 *  - stackRegisterAttrs[]: Register attribute/offset packed
 *
 *  Format of stackRegisterAttrs[level]:
 *    bits 7-4: Register dataType (attribute)
 *    bits 3-0: Register offset (sign-extended)
 *
 *  INTEGRATION POINTS:
 *  --------------------
 *  - Called from: All code generation modules (plm2e.c, plm2f.c, plm2h.c)
 *  - Uses: TX2 expression tree, symbol table, register state
 *  - Updates: Code fragments, register state, stack tracking
 *  - Produces: 8080 assembly code fragments
 *
 *  PERFORMANCE CHARACTERISTICS:
 *  ----------------------------
 *
 *  Table lookups: O(1) - direct array indexing
 *  Register scanning: O(4) - 4 registers (fixed)
 *  Fragment selection: O(F) where F = fragment count (typically 2-5)
 *  Code generation: O(1) per operation
 *
 *  Overall: Single-pass compilation with table-driven code generation
 *
 *  VERSION HISTORY:
 *  ----------------
 *  - Original: Intel ISIS-II PLM-80 compiler
 *  - C port: Mark Ogden
 *  - Current: Fully documented data module
 *
 ****************************************************************************/
#include "os.h"
#include "plm.h"
#include <stdlib.h>

// clang-format off
// ===== CODE FRAGMENT ENCODING TABLES =====

/**
 * fragControlBits - Fragment argument encoding control bits
 *
 * Format: aaa bbb ccc 0
 *   aaa (bits 7-5): Node type specifier (whether to include curNodeType)
 *   bbb (bits 4-2): Source operand encoding format
 *   ccc (bits 1-0): Destination operand encoding format
 *
 * Used by EncodeFragArgs() to determine how to encode fragment arguments
 * into the instruction stream.
 *
 * Indexed by: cfCode (fragment/code ID)
 */
 // 0 at entry 0, patterns increasing through entries
uint8_t fragControlBits[] = {
    0,    0,    0,    0,    0x26, 0x30, 0x30, 0x26, 0x30, 0x20, 0x30, 0x12, 0x12, 0x12, 0,    0x10,
    0x10, 0x10, 0x10, 0x10, 0x10, 0x60, 0,    0x26, 0x20, 0x20, 0,    0,    0,    0,    0,    0,
    0x10, 0x80, 0x80, 0x80, 0x90, 0x90, 0x40, 0xA0, 0xA0, 0xA0, 0x80, 0xB0, 0x90, 0x80, 0xB0, 0x90,
    0x80, 0xB0, 0x90, 0x80, 0xB0, 0x90, 0x80, 0xB0, 0x90, 0x20, 0x30, 0x30, 0x30, 0x10, 0x10, 0x70,
    0x70, 0x30, 0x30, 0x30, 0x30, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0x20, 0x20, 0,    0,    0x20, 0,    0,    0x2C,
    0x40, 0,    0x10, 0x10, 0x60, 0x20, 0,    0,    0xA0, 0xA0, 0xA0, 0xA0, 0x32, 0x16, 0x10, 0x20,
    0,    0x10, 0x10, 0x10, 0x10, 0x10, 0x60, 0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0x70, 0x60, 0x60, 0x70, 0x50, 0x70, 0x60, 0x60, 0xE0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0,    0,    0x80, 0x80,
    0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
    0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
    0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
    0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
    0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80
};

/**
 * helperIndexMap - Maps fragment helper indices to helper lookup indices
 *
 * Provides indirect indexing for helper function lookup
 * Used by GenerateFragmentCode() to find runtime library helpers
 *
 * Indexed by: fragmentHelperIndex value
 */
uint8_t helperIndexMap[]      = { 0, 0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7, 8, 9, 0xA };

/**
 * fragmentHelperIndex - Fragment to helper index mapping
 *
 * Maps code fragments to their corresponding helper function indices
 * Repeated pattern for different operand count scenarios
 *
 * Used by: GenerateFragmentCode() -> helperIndexMap
 */
uint8_t fragmentHelperIndex[] = { 0,   2,    4,    6,   8,    0xA,  0xC,  0x10, 0x11, 0xE, 0x12,
                                  0,   2,    4,    6,   8,    0xA,  0xC,  0x10, 0x11, 0xE, 0x12,
                                  0,   2,    4,    6,   8,    0xA,  0xC,  0x10, 0x11, 0xE, 0x12,
                                  0,   2,    4,    6,   8,    0xA,  0xC,  0x10, 0x11, 0xE, 0x12,
                                  0,   2,    4,    6,   8,    0xA,  0xC,  0x10, 0x11, 0xE, 0x12,
                                  0xC, 0x11, 0x12, 0xC, 0x11, 0x12, 2,    6,    0,    4,   6,
                                  8,   0xA,  0xC,  0xE, 0x10, 0x11, 0x12, 0xD,  0xF,  7,   9,
                                  0xB, 1,    5,    3 };

/**
 * helperMap - Runtime library helper function ID mapping table
 *
 * 2D table: helperMap[group][index]
 *   group: Helper group (0-18) determined by operation type
 *   index: Helper index within group (0-10)
 *   value: Helper ID (0-116) for linking
 *
 * Each group contains helper functions for related operations:
 *   Group 0: Primary arithmetic (helpers 94-104)
 *   Group 1: Bit operations (helpers 73-83)
 *   Group 8: Basic operations (helpers 11-17)
 *   Group 9: Core helpers (helpers 0-10)
 *   Etc.
 *
 * Used by GenerateFragmentCode() to link required runtime helpers
 */
uint8_t helperMap[][11] = {
    /* 11 uint8_t entries */
    { 94, 95, 96, 97, 98, 99, 100, 101, 103, 104, 102 },
    { 73, 74, 75, 76, 77, 78, 79, 80, 82, 83, 81 },
    { 0, 0, 0, 0, 0, 0, 69, 70, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 71, 72, 0, 0, 0 },
    { 0, 61, 0, 0, 62, 63, 59, 60, 0, 0, 0 },
    { 0, 66, 0, 0, 67, 68, 64, 65, 0, 0, 0 },
    { 0, 86, 0, 0, 87, 88, 84, 85, 0, 0, 0 },
    { 0, 91, 0, 0, 92, 93, 89, 90, 0, 0, 0 },
    { 0, 0, 0, 0, 11, 12, 13, 14, 15, 16, 17 },
    { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 },
    { 0, 34, 0, 35, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 29, 0, 30, 0, 0, 0, 0, 0, 0, 0 },
    { 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28 },
    { 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58 },
    { 106, 107, 108, 109, 110, 111, 112, 113, 114, 115, 116 },
    { 36, 38, 37, 39, 40, 41, 0, 0, 0, 0, 0 },
    { 42, 43, 44, 45, 46, 47, 0, 0, 0, 0, 0 },
    { 0, 31, 0, 0, 32, 33, 0, 0, 0, 0, 0 },
    { 105, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
};

/**
 * helperGroup - Helper group classification for node types
 *
 * Maps operation types to helper groups
 * Indexed by: helperGroupIndex (derived from operation type)
 *
 * Used by GenerateFragmentCode() to find appropriate helper group
 */
uint8_t helperGroup[]           = { 8, 0x1C, 0xC, 0, 0xB, 0x11, 1, 0xA, 0xF,  0x10, 0xD,
                                    9, 2,    3,   4, 5,   6,    7, 0,   0x1E, 0x12, 0xE };

/**
 * noteTypeToHelperGroup - Maps TX2 node types to helper groups
 *
 * Direct mapping from node type to helper group index
 * Indexed by: curNodeType (T2_* node types)
 *
 * Used by GenerateFragmentCode() to determine which helper group to use
 * for a particular operation type
 */
uint8_t noteTypeToHelperGroup[] = {
    3, 3, 3, 3, 3, 3, 0xC, 0xD, 0xE, 0xF, 0x10, 0x11, 0, // was also b457C in
                                                         // plm4a.c
    0, 0, 0, 0, 0, 0, 0, 0x12, 7, 4, 4, 2, 0xA, 0x15, 0, 0, 0, 0, 8, 9, 0, 0, 0, 0xB, 6, 0x12, 0x12,
    0x12, 0x12, 0x12, 0x12, 5, 1, 1, 1, 0x13, 0x13, 0x13, 1, 1, 1, 0x13, 0x13, 0x13, 0x14, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0xC, 0xC, 0xC, 0xC, 0xC, 0xC, 0xC, 0xC, 0xD,
    0xD, 0xD, 0xD, 0xD, 0xD, 0xD, 0xD, 0, 0xE, 0xE, 0xE, 0xE, 0xF, 0xF, 0xF, 0xF, 0, 0xF, 0xF, 0xF,
    0xF, 0xE, 0xE, 0xE, 0xE, 0, 0x10, 0x10, 0x10, 0x10, 0x10, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1,
    0, 0, 0, 0
};

// ===== OPERAND AND REGISTER ENCODING =====

/**
 * fragmentArgTypes - Fragment argument type specification
 *
 * Encodes which arguments each fragment expects
 * Format: high_nibble = first arg type, low_nibble = second arg type
 *
 * Indexed by: cfrag (fragment code)
 */
uint8_t fragmentArgTypes[] = {
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0x10, 0,    0,    0,    0,    0,    0,
    0,    0,    0x10, 0,    0x20, 0,    0,    0x13, 0,    0,    0,    0,    0,    0,    0,    0,
    0x50, 0,    0,    0,    0x10, 0x10, 0x20, 0x10, 0x10, 0x10, 0,    0x50, 0x50, 0,    0x50, 0x50,
    0,    0x50, 0x50, 0,    0x50, 0x50, 0,    0x50, 0x50, 0x40, 0,    0x10, 0x10, 0x20, 0x20, 0x60,
    0x60, 0x60, 0x60, 0x60, 0x60, 0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0x40, 0x40, 0,    0,    0x10, 0,    0,    0x21,
    0x10, 0,    0x10, 0x10, 0x10, 0x40, 0,    0,    0x10, 0x10, 0x10, 0x10, 0x12, 0,    0,    0x40,
    0,    0,    0,    0,    0,    0,    0x10, 0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0
};

/**
 * fragmentCodeLength - Size of each code fragment in bytes
 *
 * Format: xxx nnnnn
 *   xxx (bits 7-5): Fragment category/flags
 *   nnnnn (bits 4-0): Code size in bytes
 *
 * Used to calculate total code generation cost
 * Indexed by: curOp (current operation/fragment code)
 */
uint8_t fragmentCodeLength[] = {
/*  0     1     2     3     4     5     6     7     8     9     A     B     C     D     E     F */
    0,    0x20, 0x40, 0x60, 0x81, 0x81, 0x84, 0x83, 0x83, 1,    0x83, 0x82, 0x83, 0x81, 0x81, 0x82,
    0x83, 0x83, 1,    0x83, 0x81, 0x82, 0x81, 0x81, 0x81, 0x81, 0x81, 0x81, 0x21, 0x22, 0x23, 0x24,
    0x82, 0x81, 0x82, 0x83, 1,    2,    2,    0x21, 0x22, 0x23, 1,    2,    1,    2,    3,    2,
    4,    5,    4,    4,    5,    4,    3,    4,    3,    0x21, 0x85, 0x83, 0x83, 0x84, 0x83, 3,
    0x23, 0x88, 0x89, 0x8D, 0x91, 1,    2,    3,    4,    5,    4,    3,    2,    5,    4,    3,
    2,    4,    5,    6,    6,    5,    4,    3,    0x62, 0x41, 1,    2,    2,    2,    1,    0x8A,
    0x82, 0x83, 1,    1,    0x84, 0x61, 0xC,  0xE,  0x41, 0x42, 0x61, 0x62, 3,    2,    1,    0x62,
    0x61, 0,    0,    0,    0,    0,    0x84, 0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0x20, 0x20, 0x20, 0x20, 0x20, 0x22, 0x23, 0x23, 0x23, 0x23,
    6,    6,    6,    1,    1,    1,    2,    0x20, 0x60, 3,    0x20, 0x40, 0,    0,    0,    0,
    0,    0,    0x60, 0x60, 0x40, 0,    0x80, 0x84, 7,    7,    4,    5,    0,    0,    0x84, 0x84,
    0x84, 0x84, 0x84, 0x84, 0x84, 0x84, 0x84, 0x84, 0x84, 7,    7,    7,    7,    7,    7,    7,
    7,    7,    7,    7,    7,    7,    7,    7,    7,    7,    7,    7,    7,    7,    7,    4,
    4,    4,    4,    4,    4,    4,    4,    4,    4,    4,    5,    5,    5,    5,    5,    5,
    5,    5,    5,    5,    5,    0x43, 0x43, 0x43, 0x63, 0x63, 0x63, 0x23, 0x23, 0x23, 0x23, 0x23,
    0x23, 0x23, 0x23, 0x23, 0x23, 0x23, 0x23, 3,    3,    0x23, 0x23, 0x23, 0x23, 0x23, 0x23
};

/**
 * fragmentOpcodeIndex - Fragment precedence and indexing
 *
 * Provides precedence and index information for fragments
 * Used to select optimal instruction sequences
 *
 * Indexed by: currentFragmentCode
 */
uint8_t fragmentOpcodeTable[] = {
/*  0     1     2     3     4     5     6     7     8     9     A     B     C     D     E     F */
    0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x50, 0x5C, 0x5D, 0x5E, 5,    0x50, 0x5C, 0x5D, 0xED, 0xCD,
    0xDE, 0xDC, 0xDE, 0xDC, 0xDE, 0xDC, 0x85, 0x85, 0x85, 5,    0x15, 0x25, 0x35, 0xC5, 0xD5, 0xE5,
    5,    0x15, 0x25, 0x35, 0xC5, 0xD5, 0xE5, 0xC5, 0xD5, 0xE5, 0xFE, 0x85, 0x40, 0xB0, 0xB1, 0xB2,
    0xBC, 0xBD, 0x80, 0x81, 0x82, 0x8C, 0x8D, 0x5E, 0x15, 0x25, 0x35, 0xC5, 0xD5, 0xE5, 0xC5, 0xD5,
    0xE5, 5,    5,    0xE5, 5,    0xE5, 5,    0xE5, 5,    0xE5, 5,    5,    5,    5,    5,    5,
    5,    5,    5,    5,    5,    5,    5,    5,    0xB1, 0xE1, 0xD1, 0xB1, 0xE1, 0xD1, 0x81, 1,
    0x81, 1,    0x81, 1,    0xB1, 0xE1, 0xD1, 5,    5,    0xC5, 0xD5, 0xE5, 0x95, 0xB5, 0xA5, 5,
    0xC5, 0xD5, 0xE5, 0x95, 0xA5, 0xB5, 0xC5, 0xD5, 0xE5, 0x95, 0xA5, 0xB5, 5,    0x15, 0x25, 0x35,
    5,    0x15, 0x25, 0x35, 0xC5, 0xD5, 0xE5, 0x65, 0x75, 0x85, 0x95, 0xA5, 0xB5, 0xC5, 0xD5, 0xE5,
    0x65, 0x75, 0x85, 0x95, 0xA5, 0xB5, 0xBC, 0xBD, 0x9E, 0xAE, 0x8C, 0x8D, 0x6E, 0x7E, 0x6A, 0x7B,
    0x8A, 0x86, 0x87, 0x68, 0x78, 0x89, 0x8A, 0x6B, 0x7B, 0xE6, 0xE7, 0xC8, 0xD8, 0xE9, 0xEA, 0xCB,
    0xDB, 0xA6, 0xB7, 0xA8, 0xA9, 0xBA, 0xAB, 0xC5, 0xD5, 0xE5, 0xC5, 0xD5, 0xE5, 0xC5, 0xD5, 0xE5,
    0xC5, 0xD5, 0xE5, 0xC5, 0xD5, 0xE5, 0xC5, 0xD5, 0xE5, 5,    0xC5, 0xD5, 0xE5, 0xE5, 0x55, 0xED,
    0xCE, 0xDE, 0xEC, 0xA0, 0xB0, 0xBD, 0xAE, 0xA,  0xB,  0xDB, 0xEA, 0x80, 0x50, 0x10, 0x20, 0x30,
    0x9A, 0xBA, 0xCA, 8,    5,    1,    2,    3,    0xA9, 0xAB, 0xAC, 0xA0, 0xB0, 0xBD, 0xAE, 0xA,
    0xB,  0xDB, 0xEA, 0x80, 0x50, 0x10, 0x20, 0x30, 0x9A, 0xBA, 0xCA, 8,    5,    1,    2,    3,
    0xA9, 0xAB, 0xAC, 0xE0, 0xED, 0xD,  0xCD, 0xE,  0xDE, 0xD0, 0xDC, 0xE,  0xDE, 0xD0, 0xDC, 8,
    5,    1,    2,    3,    0xA9, 0xAB, 0xAC, 0xB,  0xDB, 0xA0, 0xAE, 0xA9, 0xAB, 0xAC, 0xB,  0xDB,
    0xA0, 0xAE, 0xE,  0xDE, 0xD0, 0xDC, 8,    5,    1,    2,    3,    8,    5,    1,    2,    3,
    0xA9, 0xAB, 0xAC, 0xB,  0xDB, 0xA0, 0xAE, 0xE,  0xDE, 0xD0, 0xDC, 0x9A, 0xBA, 0xCA, 0xB0, 0xBD,
    0xA,  0xEA, 0xE0, 0xED, 0xD,  0xCD, 0x80, 0x50, 0x10, 0x20, 0x30, 0x80, 0x50, 0x10, 0x20, 0x30,
    0x9A, 0xBA, 0xCA, 0xB0, 0xBD, 0xA,  0xEA, 0xE0, 0xED, 0xD,  0xCD, 8,    5,    1,    2,    3,
    0xA9, 0xAB, 0xAC, 0xB,  0xDB, 0xA0, 0xAE, 0xE,  0xDE, 0xD0, 0xDC, 0x80, 0x50, 0x10, 0x20, 0x30,
    0x9A, 0xBA, 0xCA, 0xB0, 0xBD, 0xA,  0xEA, 0xE0, 0xED, 0xD,  0xCD, 8,    5,    1,    2,    3,
    0xA9, 0xAB, 0xAC, 0xB,  0xDB, 0xA0, 0xAE, 0xE,  0xDE, 0xD0, 0xDC, 0x80, 0x50, 0x10, 0x20, 0x30,
    0x9A, 0xBA, 0xCA, 0xB0, 0xBD, 0xA,  0xEA, 0xE0, 0xED, 0xD,  0xCD, 8,    5,    1,    2,    3,
    0xA9, 0xAB, 0xAC, 0xB,  0xDB, 0xA0, 0xAE, 0xE,  0xDE, 0xD0, 0xDC, 0x80, 0x50, 0x10, 0x20, 0x30,
    0x9A, 0xBA, 0xCA, 0xB0, 0xBD, 0xA,  0xEA, 0xE0, 0xED, 0xD,  0xCD, 8,    5,    1,    2,    3,
    0xA9, 0xAB, 0xAC, 0xB,  0xDB, 0xA0, 0xAE, 0xE,  0xDE, 0xD0, 0xDC, 0x80, 0x50, 0x10, 0x20, 0x30,
    0x9A, 0xBA, 0xCA, 0xB0, 0xBD, 0xA,  0xEA, 0xE0, 0xED, 0xD,  0xCD, 0xCD, 0x35, 0xE5, 0x15, 0x25,
    0x35, 0xC5, 0xD5, 0xE5, 0xC5, 0xD5, 0xE5, 0xE5, 0x65, 0x75, 0x85, 0x95, 0xA5, 0xB5, 0x65, 0x75,
    0x85, 0x65, 0x75, 0x85
};

/**
 * fragmentOpcodeIndex - Fragment precedence and indexing
 *
 * Provides precedence and index information for fragments
 * Used to select optimal instruction sequences
 *
 * Indexed by: currentFragmentCode
 */
uint8_t fragmentOpcodeIndex[] = {
/*  0     1     2     3     4     5     6     7     8     9     A     B     C     D     E     F */
    0x18, 0x26, 0x18, 0x18, 0x18, 0x18, 0,    0x12, 0x12, 0x12, 0x2E, 0x27, 0x27, 0x27, 0x2D, 0x2A,
    0x2D, 0x2A, 0x2B, 0x28, 0x2C, 0x29, 0,    0,    0,    0x12, 7,    5,    2,    7,    5,    2,
    0x12, 7,    5,    2,    7,    5,    2,    7,    5,    2,    0,    0,    0,    1,    1,    1,
    1,    1,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0x12, 0x12, 2,    0x12, 2,    0x12, 2,    0x12, 2,    0x12, 0x12, 0x12, 0x12, 0x12, 0x12,
    0x12, 0x12, 0x12, 0x12, 0x12, 0x12, 0x12, 0x12, 0xA,  9,    0xB,  0xA,  9,    0xB,  0x17, 0x17,
    0x17, 0x17, 0x17, 0x17, 0x23, 0x22, 0x24, 0x12, 0x12, 0x1E, 0x1E, 0x1D, 0x1F, 0x20, 0x1C, 0x12,
    0x12, 0x12, 0x12, 0x12, 0x12, 2,    0x12, 0x12, 0x12, 0xD,  0x10, 0x13, 0x12, 7,    5,    2,
    0x12, 7,    5,    2,    7,    5,    2,    7,    5,    2,    7,    5,    2,    7,    5,    2,
    7,    5,    2,    7,    5,    2,    2,    2,    2,    2,    2,    2,    2,    2,    0x1C, 0x20,
    0x1C, 2,    2,    2,    2,    2,    2,    2,    2,    2,    2,    2,    2,    2,    2,    2,
    2,    0x1C, 0x20, 0x1C, 0x1C, 0x20, 0x1C, 7,    5,    2,    7,    5,    2,    7,    5,    2,
    7,    5,    2,    7,    5,    2,    7,    5,    2,    0,    0,    0,    0,    0,    0,    2,
    2,    2,    2,    0x1C, 0x20, 0x20, 0x1C, 0x1C, 0x20, 0x20, 0x1C, 0x12, 0x12, 0x12, 0x12, 0x12,
    0x1A, 0x21, 0x1C, 0x12, 0x12, 0x12, 0x12, 0x12, 0x1A, 0x21, 0x1C, 0x1C, 0x20, 0x20, 0x1C, 0x1C,
    0x20, 0x20, 0x1C, 0x12, 0x12, 0x12, 0x12, 0x12, 0x1A, 0x21, 0x1C, 0x12, 0x12, 0x12, 0x12, 0x12,
    0x1A, 0x21, 0x1C, 0x1B, 0x1D, 0x19, 0x1D, 0x1B, 0x1D, 0x19, 0x1D, 0x1B, 0x1D, 0x19, 0x1D, 0x12,
    0x12, 0x12, 0x12, 0x12, 0x1A, 0x21, 0x1C, 0x20, 0x20, 0x1C, 0x1C, 0x1A, 0x21, 0x1C, 0x20, 0x20,
    0x1C, 0x1C, 0x1B, 0x1D, 0x19, 0x1D, 0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0x1A, 0x21, 0x1C, 0x20, 0x20, 0x1C, 0x1C, 0x1B, 0x1D, 0x19, 0x1D, 0x1A, 0x21, 0x1C, 0x20, 0x20,
    0x1C, 0x1C, 0x1B, 0x1D, 0x19, 0x1D, 0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0x1A, 0x21, 0x1C, 0x20, 0x20, 0x1C, 0x1C, 0x1B, 0x1D, 0x19, 0x1D, 0x12, 0x12, 0x12, 0x12, 0x12,
    0xE,  0x16, 0x11, 0x15, 0x15, 0x11, 0x11, 0xF,  0x14, 0xC,  0x14, 0x12, 0x12, 0x12, 0x12, 0x12,
    0xE,  0x16, 0x11, 0x15, 0x15, 0x11, 0x11, 0xF,  0x14, 0xC,  0x14, 0x12, 0x12, 0x12, 0x12, 0x12,
    0xE,  0x16, 0x11, 0x15, 0x15, 0x11, 0x11, 0xF,  0x14, 0xC,  0x14, 0x12, 0x12, 0x12, 0x12, 0x12,
    0xE,  0x16, 0x11, 0x15, 0x15, 0x11, 0x11, 0xF,  0x14, 0xC,  0x14, 0x12, 0x12, 0x12, 0x12, 0x12,
    0xE,  0x16, 0x11, 0x15, 0x15, 0x11, 0x11, 0xF,  0x14, 0xC,  0x14, 0x12, 0x12, 0x12, 0x12, 0x12,
    0xE,  0x16, 0x11, 0x15, 0x15, 0x11, 0x11, 0xF,  0x14, 0xC,  0x14, 0x12, 0x12, 0x12, 0x12, 0x12,
    0xE,  0x16, 0x11, 0x15, 0x15, 0x11, 0x11, 0xF,  0x14, 0xC,  0x14, 0x12, 0x12, 0x12, 0x12, 0x12,
    0xE,  0x16, 0x11, 0x15, 0x15, 0x11, 0x11, 0xF,  0x14, 0xC,  0x14, 0x25, 0x26, 0x26, 0,    0,
    0,    0,    0,    0,    7,    5,    2,    2,    7,    5,    2,    7,    5,    2,    7,    5,
    2,    7,    5,    2
};

/**
 * operatorPrecedence - Operator precedence encoding
 *
 * 16-bit values encoding register assignment precedence
 * Format: four 4-bit fields specifying register handling
 *   bits 15-12: Register 3 (HL) precedence
 *   bits 11-8:  Register 2 (DE) precedence
 *   bits 7-4:   Register 1 (BC) precedence
 *   bits 3-0:   Register 0 (PSW) precedence
 *
 * Indexed by: selectedOperatorIdx (operator encoding index)
 */

uint16_t operatorPrecedence[] = { 0,      1,      0x1000, 0x1000, 0x2000, 0x2000, 0x2000, 0x4000,
                                  0x4000, 0x5000, 0x6008, 0x6000, 0x5080, 0x8040, 0xD088, 0x3010,
                                  0x8008, 0x9008, 0x8000, 0x8001, 0x9000, 0xA008, 0xD088, 0xC000,
                                  0xF000, 0x5080, 0xD088, 0x3010, 0x9008, 0x9000, 0xA000, 0xB008,
                                  0xA008, 0xD088, 0xD000, 0xE008, 0xE000, 0xE000, 0xF000, 0xF000,
                                  0xB000, 0xB000, 0xB000, 0xE000, 0xE000, 0xE000, 0xC000 };

/**
 * operatorTypeRules - Operator type conversion rules
 *
 * 16-bit values encoding type rules for operands
 * Format: four 4-bit fields specifying conversion requirements
 *
 * Indexed by: selectedOperatorIdx (operator encoding index)
 */
uint16_t operatorTypeRules[]  = { 0x123,  0x123,  0x124,  0x126,  0x134,  0x143,  0x163,  0x423,
                                  0x623,  0x624,  0x634,  0x634,  0x4026, 0x4123, 0x4126, 0x4106,
                                  0x4123, 0x4126, 0x4123, 0x4123, 0x4126, 0x4136, 0x4326, 0x4623,
                                  0x4666, 0x6024, 0x6124, 0x6104, 0x6124, 0x6124, 0x6134, 0x6114,
                                  0x6134, 0x6324, 0x6624, 0x6634, 0x6634, 0x6663, 0x6664, 0x6666,
                                  0x5146, 0x5164, 0x5154, 0x5346, 0x5364, 0x5354, 0x5623 };

/**
 * operatorResultType - Operator result type and fragment count
 *
 * Encodes result type and number of valid fragments for each operation
 * Format: nibble values specifying result type codes
 *
 * Indexed by: curNodeType (current operation)
 */
uint8_t operatorResultType[]  = {
/*  0     1     2     3     4     5     6     7     8     9     A     B     C     D     E     F */
    0x10, 0x10, 0x20, 0x20, 0x10, 0x10, 2,    2,    5,    5,    5,    5,    1,    3,    1,    6,
    3,    3,    3,    0x1C, 0x10, 4,    2,    2,    0x20, 0x20, 0x20, 0xE,  7,    4,    8,    7,
    7,    6,    6,    6,    0x20, 0x10, 0x10, 0x10, 0x20, 0x20, 0x10, 0x10, 3,    7,    7,    3,
    7,    7,    3,    1,    1,    1,    1,    1,    1,    1,    6,    3,    1,    0xE,  4,    2,
    6,    0xD,  1,    1,    1,    1,    1,    4,    1,    1,    3,    3,    3,    1,    1,    1,
    1,    1,    1,    1,    1,    1,    1,    1,    1,    1,    1,    1,    1,    4,    1,    1,
    1,    1,    1,    1,    1,    1,    4,    1,    1,    1,    1,    1,    1,    1,    1,    1,
    2,    2,    2,    2,    1,    1,    1,    1,    1,    1,    1,    1,    1,    1,    1,    3,
    3,    3,    0x1C, 2,    1,    1
};

/**
 * opcodeEncodingMap - Maps operation types to 8080 instruction encodings
 *
 * Provides the primary lookup from PLM operation to 8080 opcode
 * Multiple entries may map to same opcode (instruction selection)
 *
 * Indexed by: operation category or node type
 */
uint8_t opcodeEncodingMap[] = {
/*  0     1     2     3     4     5     6     7     8     9     A     B     C     D     E     F */
    0x3F, 0x40, 0x44, 0x43, 0x41, 0x42, 0x60, 0x6C, 0x6C, 0x6C, 0x61, 0x64, 0x5F, 0x5F, 0xEB, 0xEF,
    0xEB, 0xEF, 0xEB, 0xEF, 0xEB, 0xEF, 0x21, 0x22, 0x23, 0x24, 0x24, 0x24, 0x24, 0x27, 0x27, 0x27,
    0x25, 0x25, 0x25, 0x25, 0x28, 0x28, 0x28, 0x29, 0x29, 0x29, 0x3B, 0x26, 0x3C, 0x3D, 0x3D, 0x3D,
    0x3E, 0x3E, 0x14, 0x14, 0x14, 0x14, 0x14, 0x16, 0x17, 0x17, 0x17, 0x17, 0x17, 0x17, 0x17, 0x17,
    0x17, 0x4C, 0x45, 0x1C, 0x46, 0x1D, 0x47, 0x1E, 0x48, 0x1F, 0x49, 0x4A, 0x4B, 0x4D, 0x4E, 0x4F,
    0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57, 0xFE, 0xFB, 0xFA, 0xFE, 0xFB, 0xFA, 0xF7, 0xF8,
    0xF7, 0xF8, 0xF7, 0xF8, 0xFE, 0xFB, 0xFA, 0x5E, 0x5A, 0xF9, 0xFA, 0xFB, 0xFC, 0xFE, 0xFD, 0x5B,
    0x62, 0x62, 0x62, 9,    9,    0x12, 0x63, 0x63, 0x63, 0x5C, 0x5C, 0x5D, 0,    0,    0,    0,
    0,    0,    0,    0,    1,    1,    1,    2,    2,    2,    3,    3,    3,    1,    1,    1,
    1,    1,    1,    1,    1,    1,    0x58, 0x58, 0x6F, 0x6F, 0x59, 0x59, 0x59, 0x59, 0xE5, 0xE6,
    0xE7, 0x59, 0x59, 0x59, 0x59, 0x65, 0x65, 0x65, 0x65, 0x59, 0x59, 0x59, 0x59, 0x65, 0x65, 0x65,
    0x65, 0xE5, 0xE6, 0xE7, 0xE8, 0xE9, 0xEA, 2,    2,    2,    0x68, 0x68, 0x68, 0x69, 0x69, 0x69,
    3,    3,    3,    0x6A, 0x6A, 0x6A, 0x6B, 0x6B, 0x6B, 0x1B, 0x20, 0x20, 0x20, 0x1B, 0x1B, 0x39,
    0x39, 0x39, 0x39, 0xF3, 0xF4, 0xF5, 0xF6, 0xF3, 0xF4, 0xF5, 0xF6, 0x2A, 0x2B, 0x2C, 0x2C, 0x2C,
    0xF0, 0xF1, 0xF2, 0x2A, 0x2B, 0x2C, 0x2C, 0x2C, 0xF0, 0xF1, 0xF2, 0xF3, 0xF4, 0xF5, 0xF6, 0xF3,
    0xF4, 0xF5, 0xF6, 0x2A, 0x2B, 0x2C, 0x2C, 0x2C, 0xF0, 0xF1, 0xF2, 0x2A, 0x2B, 0x2C, 0x2C, 0x2C,
    0xF0, 0xF1, 0xF2, 0xED, 0xEB, 0xEE, 0xEF, 0xED, 0xEB, 0xEE, 0xEF, 0xED, 0xEB, 0xEE, 0xEF, 0x2A,
    0x2B, 0x2C, 0x2C, 0x2C, 0xF0, 0xF1, 0xF2, 0xF4, 0xF5, 0xF3, 0xF6, 0xF0, 0xF1, 0xF2, 0xF4, 0xF5,
    0xF3, 0xF6, 0xED, 0xEB, 0xEE, 0xEF, 0x2A, 0x2B, 0x2C, 0x2C, 0x2C, 0x2A, 0x2B, 0x2C, 0x2C, 0x2C,
    0xB2, 0xB3, 0xB4, 0xB5, 0xB6, 0xB7, 0xB8, 0xAE, 0xAF, 0xB0, 0xB1, 0xB2, 0xB3, 0xB4, 0xB5, 0xB6,
    0xB7, 0xB8, 0xAE, 0xAF, 0xB0, 0xB1, 0x2A, 0x2B, 0x2C, 0x2C, 0x2C, 0x2A, 0x2B, 0x2C, 0x2C, 0x2C,
    0xF0, 0xF1, 0xF2, 0xF4, 0xF5, 0xF3, 0xF6, 0xED, 0xEB, 0xEE, 0xEF, 0x2D, 0x2E, 0x2F, 0x2F, 0x2F,
    0xD3, 0xD4, 0xD5, 0xD6, 0xD7, 0xD8, 0xD9, 0xCF, 0xD0, 0xD1, 0xD2, 0x2D, 0x2E, 0x2F, 0x2F, 0x2F,
    0xD3, 0xD4, 0xD5, 0xD6, 0xD7, 0xD8, 0xD9, 0xCF, 0xD0, 0xD1, 0xD2, 0x36, 0x37, 0x38, 0x38, 0x38,
    0xDE, 0xDF, 0xE0, 0xE1, 0xE2, 0xE3, 0xE4, 0xDA, 0xDB, 0xDC, 0xDD, 0x36, 0x37, 0x38, 0x38, 0x38,
    0xDE, 0xDF, 0xE0, 0xE1, 0xE2, 0xE3, 0xE4, 0xDA, 0xDB, 0xDC, 0xDD, 0x30, 0x31, 0x32, 0x32, 0x32,
    0xBD, 0xBE, 0xBF, 0xC0, 0xC1, 0xC2, 0xC3, 0xB9, 0xBA, 0xBB, 0xBC, 0x30, 0x31, 0x32, 0x32, 0x32,
    0xBD, 0xBE, 0xBF, 0xC0, 0xC1, 0xC2, 0xC3, 0xB9, 0xBA, 0xBB, 0xBC, 0x33, 0x34, 0x35, 0x35, 0x35,
    0xC8, 0xC9, 0xCA, 0xCB, 0xCC, 0xCD, 0xCE, 0xC4, 0xC5, 0xC6, 0xC7, 0x33, 0x34, 0x35, 0x35, 0x35,
    0xC8, 0xC9, 0xCA, 0xCB, 0xCC, 0xCD, 0xCE, 0xC4, 0xC5, 0xC6, 0xC7, 0x66, 0,    1,    0x17, 0x17,
    0x17, 0x17, 0x17, 0x17, 2,    2,    2,    0x70, 2,    2,    2,    3,    3,    3,    2,    2,
    2,    3,    3,    3
};

// b4C15
/**
 * loadOpAttributes - Operand load strategy attributes
 *
 * Format: (state << 4) | (attr)
 *   state (bits 7-4): Register state update strategy (0-7)
 *   attr (bits 3-0):  Attribute assignment strategy (0-7)
 *
 * Indexed by: operationCode (0-23)
 */
uint8_t loadOpAttributes[] = { 0x21, 0x42, 0x42, 0x43, 0x41, 0x24, 0x34, 0x43,
                               0x42, 0x42, 0x42, 0x43, 0x45, 0x45, 0x52, 0x53,
                               0x50, 0x50, 0,    0,    0x60, 0x10, 0x70, 0x70 };

/**
 * loadOpTargetReg - Target register selection for operand loads
 *
 * Encodes target register selection strategy
 * Format: target_reg | (arg1 << 1) | (arg2 << 3) | flags
 *
 * Indexed by: operationCode (0-23)
 */
uint8_t loadOpTargetReg[]  = { 0x79, 1,    0x61, 0x61, 0x61, 0x61, 0x89, 0x88,
                               0x89, 1,    0x89, 0x89, 0x49, 0x89, 0x89, 0x89,
                               0x69, 0x89, 0x8F, 0x6F, 0x80, 0x60, 0x80, 0x80 };


// ===== EXPRESSION ATTRIBUTE AND LOCATION ENCODING =====

/**
 * exprComplexity - Expression complexity cost for type conversions
 *
 * Costs (lower = preferred):
 *   0x38-0x39: Low complexity
 *   0x3C-0x3F: Medium complexity
 *   0x40+:     High complexity
 *
 * Used by SelectOptimalFragment() to determine conversion costs
 * Indexed by: operand type code
 */
uint8_t exprComplexity[]   = {
/*  0     1     2     3     4     5     6     7     8     9     A     B     C     D     E     F */
    0x63, 0,    0x37, 0x46, 4,    5,    6,    0x3B, 0x3C, 3,    0x3A, 3,    4,    4,    6,    0x3A,
    0x3B, 0x3B, 5,    6,    3,    4,    5,    4,    5,    6,    5,    5,    7,    7,    8,    8,
    9,    9,    0xB,  0xB,  0xC,  0x3B, 0x3C, 0x3C, 0x3C, 0x3D, 0x3E, 0x3E, 0x3F, 0x3F, 0x40, 0x40,
    3,    2,    2,    0x39, 1,    2,    4,    4,    0x38, 0x39, 1,    0x38, 3,    4,    4,    5,
    7,    8,    0x3B, 0x3C, 3,    3,    0x3A, 1,    0x38, 3,    5,    6,    7,    9,    0xA,  0x3C,
    0x3D, 0x3E, 2,    0x39, 1,    2,    3,    4,    0x38, 0x39, 2,    2,    4,    4,    5,    0x39,
    0x39, 1,    2,    4,    2,    4,    5,    6,    8,    9,    0x39, 0x3B, 0x3C, 0x3D, 0x38
};

/**
 * exprRegisterCost - Register usage cost for expressions
 *
 * Costs representing difficulty of getting value into register
 *   Low: 0x00, 0x01, 0x02 (in register already)
 *   Mid: 0x0B, 0x0D (simple access)
 *   High: 0x2D-0x2F (complex access)
 *
 * Used by SetOperandCosts() to classify operand loading requirements
 * Indexed by: operand type code
 */

uint8_t exprRegisterCost[] = {
/*  0     1     2     3     4     5     6     7     8     9     A     B     C     D     E     F */
    0x63, 0,    1,    0x32, 1,    2,    2,    1,    2,    1,    1,    1,    0xB,  0xD,  0xD,  1,
    0xB,  0xD,  1,    0xB,  1,    0xB,  0xD,  1,    0xB,  0xD,  0x11, 0x13, 0x11, 0x13, 0x11, 0x1B,
    0x19, 0x1B, 0x19, 0x1B, 0x19, 1,    0xB,  0x11, 0x13, 0xD,  0x11, 0x13, 0x11, 0x1B, 0x19, 0x1B,
    1,    2,    1,    1,    1,    2,    1,    2,    1,    2,    0x2F, 0x2F, 0x2F, 0x2D, 0x2F, 0x2D,
    0x2D, 0x2D, 0x2D, 0x2D, 0x2D, 0x2F, 0x2D, 3,    3,    3,    0x29, 0x29, 0x29, 0x29, 0x29, 0x29,
    0x29, 0x29, 3,    3,    1,    1,    1,    1,    1,    1,    0x1D, 0x1F, 0x1D, 0x1F, 0x1D, 0x1D,
    0x1F, 3,    3,    3,    0x2B, 0x2B, 0x2B, 0x2B, 0x2B, 0x2B, 0x2B, 0x2B, 0x2B, 0x2B, 3
};


/**
 * typeConversionTable - Type conversion cost matrix
 *
 * 2D table: typeConversionTable[sourceType][targetType]
 *   Entry value 0: No conversion needed (compatible types)
 *   Entry value 1-127: Conversion cost (lower = preferred)
 *   Entry value 128+: Special encoding or error code
 *
 * Used by SelectOptimalFragment() to find cheapest conversion
 * for each operand pair
 */

uint8_t typeConversionTable[][16] = { 
/*    0     1     2     3     4     5     6     7     8     9     A     B     C     D     E     F */
    { 1,    0x47, 0x47, 0x47, 0,    0,    0,    0,    0,    0,    0,    0,    0x49, 0x49, 0x49, 0 },
    { 0x47, 1,    0x47, 0x47, 0,    0,    0,    0,    0,    0,    0,    0,    0x31, 0x49, 0x49, 0 },
    { 0x47, 0x47, 1,    0x61, 0,    0,    0,    0,    0,    0,    0,    0,    0x49, 0x31, 0x63, 0 },
    { 0x47, 0x47, 0x61, 1,    0,    0,    0,    0,    0,    0,    0,    0,    0x49, 0x63, 0x31, 0 },
    { 9,    0xD, 0xC, 0xB,    1,    0,    0x30, 0x30, 0x30, 0,    0,    0,    0xE,  0x13, 0x12, 0 },
    { 0x32, 0x32, 0x32, 0x32, 0,    1,    0,    0,    0,    0,    0,    0,    0x30, 0x30, 0x30, 0 },
    { 0x34, 0x35, 0x35, 0x35, 0,    0,    1,    0x52, 0x52, 0,    0,    0,    0x36, 0x37, 0x37, 0 },
    { 0x34, 0x64, 0x64, 0x64, 0,    0,    0x52, 1,    0x61, 0,    0,    0,    0x65, 0x65, 0x65, 0 },
    { 0x3A, 0x3A, 0x3A, 0x3A, 0,    0,    0x52, 0x61, 1,    0,    0,    0,    0x3C, 0x3C, 0x3C, 0 },
    { 0x38, 0x39, 0x39, 0x39, 0,    0,    2,    0x53, 0x53, 1,    0x52, 0x52, 0x4A, 0x4A, 0x4B, 0 },
    { 0x38, 0x6A, 0x6A, 0x6A, 0,    0,    0x53, 2,    0x6E, 0x52, 1,    0x61, 0x65, 0x65, 0x66, 0 },
    { 0x3B, 0x3B, 0x3B, 0x3B, 0,    0,    0x53, 0x6E, 2,    0x52, 0x61, 1,    0x45, 0x45, 0x3E, 0 },
    { 0x48, 2,    0x48, 0x48, 0,    0,    0,    0,    0,    0,    0,    0,    1,    0x52, 0x52, 0 },
    { 0x48, 0x48, 2,    0x6E, 0,    0,    0,    0,    0,    0,    0,    0,    0x52, 1,    0x61, 0 },
    { 0x48, 0x48, 0x6E, 2,    0,    0,    0,    0,    0,    0,    0,    0,    0x52, 0x61, 1,    0 },
    { 0xA,  0x11, 0x10, 0xF,  0,    0,    0,    0,    0,    0x30, 0x30, 0x30, 0x16, 0x15, 0x14, 1 },
    { 0x33, 0x33, 0x33, 0x33, 0,    0,    0,    0,    0,    0,    0,    0,    0x30, 0x30, 0x30, 0 },
    { 0,    0,    0,    0,    0,    0,    0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0,    0,    0,    0 },
    { 0x4B, 0x4C, 0x4C, 0x4C, 0,    0,    0x4A, 0x4A, 0x4B, 0,    0,    0,    0x4D, 0x4D, 0x4D, 0 },
    { 0x66, 0x67, 0x67, 0x67, 0,    0,    0x65, 0x65, 0x66, 0,    0,    0,    0x68, 0x68, 0x68, 0 },
    { 0x3D, 0x3F, 0x3F, 0x3F, 0,    0,    0x44, 0x44, 0x3D, 0,    0,    0,    0x40, 0x40, 0x40, 0 },
    { 0x50, 0x51, 0x51, 0x51, 0,    0,    0x4F, 0x4F, 0x50, 0x4A, 0x4A, 0x4B, 0x4D, 0x4D, 0x4E, 0 },
    { 0x6C, 0x6D, 0x6D, 0x6D, 0,    0,    0x6B, 0x6B, 0x6C, 0x65, 0x65, 0x66, 0x68, 0x68, 0x69, 0 },
    { 0x42, 0x43, 0x43, 0x43, 0,    0,    0x46, 0x46, 0x42, 0x44, 0x44, 0x3D, 0x40, 0x40, 0x41, 0 },
    { 0x47, 1,    0x47, 0x47, 0,    0,    0,    0,    0,    0,    0,    0,    1,    0x52, 0x52, 0 },
    { 0x47, 0x47, 1,    0x61, 0,    0,    0,    0,    0,    0,    0,    0,    0x52, 1,    0x61, 0 },
    { 0x47, 0x47, 0x61, 1,    0,    0,    0,    0,    0,    0,    0,    0,    0x52, 0x61, 1,    0 },
    { 8,    8,    8,    7,    0,    3,    0,    0,    0,    0,    0,    0,    6,    5,    4,    0 },
    { 0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0 },
    { 0,    0,    0,    0,    0,    0,    0x19, 0x18, 0x17, 0x19, 0x18, 0x17, 0,    0,    0,    0 },
    { 0x2D, 0x2F, 0x2E, 0x2E, 0,    0,    0x2B, 0x2A, 0x2C, 0x1D, 0x1C, 0x1E, 0x23, 0x22, 0x24, 0 },
    { 0x59, 0x60, 0x5F, 0x5F, 0,    0,    0x58, 0x58, 0x58, 0x54, 0x54, 0x54, 0x5D, 0x5C, 0x5E, 0 },
    { 0x28, 0x28, 0x27, 0x27, 0,    0,    0x29, 0x26, 0x25, 0x19, 0x18, 0x17, 0x1D, 0x1C, 0x1E, 0 },
    { 0x59, 0x58, 0x58, 0x58, 0,    0,    0,    0,    0,    0,    0,    0,    0x54, 0x54, 0x54, 0 },
    { 0x55, 0x54, 0x54, 0x54, 0,    0,    0,    0,    0,    0,    0,    0,    0x54, 0x54, 0x54, 0 },
    { 0x1F, 0x21, 0x20, 0x20, 0,    0,    0x1D, 0x1C, 0x1E, 0,    0,    0,    0x23, 0x22, 0x22, 0 },
    { 0x55, 0x5B, 0x5A, 0x5A, 0,    0,    0x54, 0x54, 0x54, 0,    0,    0,    0x5D, 0x5C, 0x5C, 0 },
    { 0x1B, 0x1B, 0x1A, 0x1A, 0,    0,    0x19, 0x18, 0x17, 0,    0,    0,    0x1D, 0x1C, 0x1C, 0 },
    { 0x55, 0x54, 0x54, 0x54, 0,    0,    0,    0,    0,    0,    0,    0,    0x56, 0x56, 0x56, 0 },
    { 0x54, 0x55, 0x55, 0x55, 0,    0,    0,    0,    0,    0,    0,    0,    0x57, 0x57, 0x57, 0 }
};


/**
 * nodeExprCategory - Maps node types to expression categories
 *
 * Categories determine:
 *   - How operand is loaded into register
 *   - Which registers can hold it
 *   - What conversions are available
 *
 * Values: 0x00-0x17 (0-23 categories)
 * Indexed by: curNodeType (TX2 node type)
 */
uint8_t nodeExprCategory[] = {
/*  0     1     2     3     4     5     6     7     8     9     A     B     C     D     E     F */
    0x17, 0x16, 0x16, 0x16, 0,    0,    0,    0,    0,    1,    1,    2,    2,    2,    2,    2,
    2,    2,    3,    3,    4,    4,    4,    5,    5,    5,    5,    5,    5,    5,    5,    5,
    5,    5,    5,    5,    5,    5,    5,    5,    5,    5,    5,    5,    5,    5,    5,    5,
    6,    7,    8,    8,    9,    9,    9,    9,    9,    9,    0xA,  0xA,  0xB,  0xC,  0xC,  0xC,
    0xC,  0xC,  0xC,  0xC,  0xD,  0xD,  0xD,  0xE,  0xE,  0xF,  0x10, 0x10, 0x10, 0x10, 0x10, 0x10,
    0x10, 0x10, 0x11, 0x11, 0x12, 0x12, 0x12, 0x12, 0x12, 0x12, 0x13, 0x13, 0x13, 0x13, 0x13, 0x13,
    0x13, 0x14, 0x14, 0x14, 0x14, 0x14, 0x14, 0x14, 0x14, 0x14, 0x14, 0x14, 0x14, 0x14, 0x14
};


/**
 * loadOpFragment - Fragment selection for operand loads
 *
 * Maps load operation codes to code fragments
 * Indexed by: operationCode (0-23)
 */
uint8_t loadOpFragment[]   = { 6,    8,    0xA, 0x3A, 0xA, 6,   7, 0xF, 0x6D, 9, 0x12, 0x11,
                               0x10, 0x10, 0xD, 0xC,  0xB, 0xB, 4, 4,   0xE,  5, 0,    0 };


/**
 * commonConstants - Frequently used constant values
 *
 * Pre-encoded constants available for rapid code generation
 * Values: 0, 1, 2, 3, 4, 8, 9, 10, 16, -3, -2, -1, -3, -2, -1
 */
uint16_t commonConstants[] = { 0,    1,    2,    3,    4,      8,      9,     0xA,
                               0x10, 0xFD, 0xFE, 0xFF, 0xFFFD, 0xFFFE, 0xFFFF };

// ===== OPTIMIZATION TABLES =====

/**
 * peepholePatterns - Peephole optimization instruction patterns
 *
 * Encodes instruction patterns that can be optimized
 * Used to detect and replace inefficient sequences
 */
uint8_t peepholePatterns[] = {
/*  0     1     2     3     4     5     6     7     8     9     A     B     C     D     E     F */
    0,    0x2A, 0,    0x2A, 0,    0x32, 0,    0x32, 0,    0x2A, 0x2A, 0,    0x2A, 0,    0x5A, 2,
    0xA,  0,    2,    0,    9,    9,    0x11, 0x11, 0x19, 0x71, 0x59, 0x69, 0x51, 0x61, 1,    1,
    1,    0,    1,    1,    0,    0xA,  0xA,  0x12, 0x12, 0x1A, 0x72, 0x5A, 0x6A, 0x52, 0x62, 2,
    2,    2,    0,    0,    0x11, 0x21, 0x29, 0x39, 0x41, 9,    9,    0,    9,    9,    0,    8,
    0,    0x59, 0x59, 0x71, 0x71, 0,    0,    0,    0,    1,    1,    1,    0x5B, 0x73, 0,    1,
    1,    1,    0x59, 0x71, 0,    2,    0xA,  0x12, 2,    0xA,  0x12, 0,    1,    0,    1,    0,
    1,    0,    1,    1,    0
};

/**
 * optimisationStepTable - Optimization step encoding
 *
 * Maps node types to optimization actions
 * 0xFF = no optimization
 * Other = optimization action code
 */

uint8_t optimisationStepTable[] = {
/*       0     1     2     3     4     5     6     7     8     9     A     B     C     D     E     F */
/* 00 */ 0xFF, 0x22, 0xFF, 0x22, 0xFF, 0x22, 0xFF, 0x22, 0xFF, 0x22, 0x62, 0xFF, 0x22, 0xFF, 4,    4,
/* 10 */ 4,    0xFF, 0x50, 0xFF, 0x64, 0x85, 0x64, 0x85, 0x64, 0x64, 0xA5, 0x64, 0xA5, 0x64, 0x64, 5,
/* 20 */ 4,    0xFF, 0x64, 5,    0xFF, 0x64, 0x85, 0x64, 0x85, 0x64, 0x64, 0xA5, 0x64, 0xA5, 0x64, 0x64,
/* 30 */ 5,    4,    0xFF, 4,    4,    4,    4,    4,    4,    0x64, 4,    0xFF, 0x64, 4,    0xFF, 4,
/* 40 */ 0xFF, 0x25, 0x24, 0x64, 4,    4,    0xFF, 4,    0xFF, 0x64, 5,    4,    0x24, 4,    0xFF, 0x64,
/* 50 */ 5,    4,    0x25, 0x64, 0xFF, 0x14, 0x14, 0x14, 4,    4,    4,    0xFF, 4,    0xFF, 4,    0xFF,
/* 50 */ 4,    0xFF, 0x14, 4,    0xFF };

// indexed by arith op
// DOUBLE, PLUSSIGN, MINUSSIGN, STAR, SLASH, MOD, AND, OR, XOR
// BASED, BYTEINDEX, WORDINDEX, MEMBER, UNARYMINUS, NOT, LOW, HIGH, ADDRESSOF

/**
 * constFoldRules - Constant folding optimization rules
 *
 * Format: (left_rules << 6) | (right_rules << 3) | (result_rules)
 *   left_rules: Constraints on left operand (0-3)
 *   right_rules: Constraints on right operand (0-7)
 *   result_rules: Result type rules (0-7)
 *
 * Used by FoldConstantExpr() to determine which expressions
 * can be evaluated at compile time
 *
 * Indexed by: node type (arithmetic operations)
 */
uint8_t constFoldRules[] = { 0x5A, 0x61, 0x69, 0x92, 0x92, 0x92, 0x91, 0x91, 0x91,
                             0x5D, 0xD3, 0xD3, 0xDC, 0x9B, 0x9B, 0x98, 0x98, 0x1A };

/* clang-format off */
// note  INCLUDE (162) item changed to have only 1 uint16_t
// indexed by T2 opcode
// xxxxxxnn
// nn -> length of t2 item in words
// other bits as follows
// 80: SETSTMTNO PROCEDURE LOCALLABEL CASELABEL LABELDEF INPUT GOTO JMP JNC JNZ SIGN ZERO PARITY CARRY DISABLE ENABLE
//     HALT STMTCNT LINEINFO MODULE SYNTAXERROR TOKENERROR EOF LIST NOLIST CODE NOCODE EJECT INCLUDE ERROR STACKPTR
//     SEMICOLON OPTBACKREF CASE ENDCASE ENDPROC BEGCALL
// 40: IDENTIFIER NUMBER BIGNUMBER 175 STACKPTR SEMICOLON OPTBACKREF CASE ENDCASE ENDPROC LENGTH LAST SIZE BEGCALL
// 20: meta node STMTCNT LINEINFO MODULE SYNTAXERROR TOKENERROR EOF LIST NOLIST CODE NOCODE EJECT INCLUDE ERROR
// 10: PROCEDURE LOCALLABEL CASELABEL LABELDEF HALT EOF ENDPROC
// 08: control flow boundary JMPFALSE RETURNBYTE RETURNWORD RETURN BEGMOVE CALL CALLVAR PROCEDURE GOTO JMP JNC JNZ ENDPROC
// 04: JMPFALSE CASEBLOCK
// 
// 
              /* 0     1     2     3     4     5     6     7     8     9     A     B     C     D     E     F */
uint8_t nodeControlMap[] = {
                 2,    2,    2,    2,    2,    2,    2,    2,    2,    2,    2,    2,    0xE,  1,    1,    1,    // 00
                 1,    1,    1,    2,    2,    2,    2,    2,    2,    2,    2,    2,    3,    3,    2,    1,    // 10
                 1,    1,    1,    1,    2,    2,    2,    2,    2,    2,    2,    2,    0,    0,    0,    0,    // 20
                 0,    0,    0,    0,    0,    0,    0,    0,    0,    1,    1,    1,    1,    2,    2,    6,    // 30
                 1,    0,    0,    0,    0,    3,    0,    9,    9,    8,    1,    1,    1,    0,    0,    0,    // 40
                 0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    // 50
                 0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    // 60
                 0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    1,    // 70
                 1,    1,    2,    9,    0xB,  0xB,  0x81, 0x99, 0x91, 0x91, 0x91, 0x81, 0x89, 0x89, 0x89, 0x8A, // 80
                 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x90, 0xA1, 0xA3, 0xA0, 0xA1, 0xA2, 0xB0, 0xA0, 0xA0, 0xA0, // 90
                 0xA0, 0xA0, 0xA1, 0xA3, 0,    0,    0,    0,    0,    0,    0,    0,    0x41, 0x41, 0x41, 0x40, // a0
                 0,    0,    0,    0,    0,    0xC0, 0xC0, 0xC0, 0xC0, 0xC0, 0xD8, 0x41, 0x41, 0x41, 0xC0 };     // b0


/**
 * optimisationStep2Map - Secondary optimization mapping
 *
 * Additional optimization rules for specific node types
 * Indexed by: curNodeType (arithmetic operators)
 */
uint8_t optimisationStep2Map[] = 
                 /* 0     1     2     3     4     5     6     7     8     9 */
                  { 0,    0,    0,    0,    0,    0,    0,    0,    0,    0,      // LT  LE  NE  EQ GE GT ROL ROR SCL SCR
                    0,    0,    0,    0,    0,    0,    0,    0,    0x12, 0x14,   // SHL SHR JMPFALSE ..... DOUBLE PLUSSIGN
                    0x22, 0x33, 0x47, 0x47, 0x41, 0x49, 0x4F, 0,    0x5C, 0x5E,   // MINUSSIGN STAR SLASH MOD AND OR XOR BASED BYTEINDEX WORDINDEX
                    0x60 };                                                       // MEMBER

/**
 * optimisationStep1Map - Primary optimization mapping
 *
 * Primary optimization rules
 * Indexed by: operation type
 */
uint8_t optimisationStep1Map[] = {
                 /* 0     1     2     3     4     5     6     7     8     9 */
                    0,    0,    0,    0,    0,    0,    1,    3,    5,    7,
                    9,    0xC,  0xE,  0,    0,    0,    0,    0,    0,    0x14,
                    0x25, 0x33, 0x3C, 0x3F, 0x41, 0x49, 0x4F, 0x55, 0,    0,
                    0x62 };

/**
 * step2ActionTable - Optimization action codes
 *
 * Lookup table for optimization actions
 * Maps action codes to specific transformation rules
 */
uint8_t step2ActionTable[] = {
                 0,    0x4D, 0,    0x55, 0,    0x5D, 0,    0x66, 0,    0x6F, 0x6F, 0,    0x77,
                 0,    0xB6, 0x8D, 0xB6, 0,    0x41, 0,    0x2D, 0x2D, 0x2E, 0x2E, 0x2F, 0x30,
                 0x30, 0x31, 0x31, 0x32, 0x41, 0x41, 0x12, 0,    0x1F, 0x1F, 0,    0x30, 0x30,
                 0x31, 0x31, 0x32, 0x2D, 0x2D, 0x2E, 0x2E, 0x2F, 0x41, 0x41, 0x41, 0,    0xAD,
                 0x42, 0x43, 0x44, 0x2C, 0x46, 0x41, 0x12, 0,    0x41, 0x12, 0,    0xAD, 0,
                 0x41, 0x12, 0x41, 0x12, 0xAD, 0,    0xAD, 0,    0x41, 0x41, 0x12, 0xAD, 0xAD,
                 0,    0x41, 0x41, 0x12, 0x20, 0x20, 0,    0x4A, 0x4B, 0x4C, 0x7F, 0x80, 0x81,
                 0,    0xD,  0,    0xE,  0,    0xF,  0,    0x10, 0x11, 0 };


/**
 * lookupResultAttr - Result attribute lookup table
 *
 * Maps lookup code to expression attribute
 * Values represent: BYTE, ADDRESS, STRUCT, POINTER, INDIRECT, etc.
 *
 * Indexed by: lookup code (0-39)
 */
uint8_t lookupResultAttr[] = {
                 0, 0, 0, 0, 0, 0, 2, 2,   2, 3, 3, 3, 1, 1, 1, 1, 1, 8, 4, 4,
                 4, 5, 5, 5, 6, 6, 6, 0xA, 9, 8, 3, 3, 1, 1, 6, 2, 2, 0, 0, 0xB };

/**
 * lookupResultLoc - Result location lookup table
 *
 * Maps lookup code to expression location
 * Values represent: REGISTER (0-3), MEMORY (8), STACK (9), STACK_PTR (0xA)
 *
 * Indexed by: lookup code (0-39)
 */
uint8_t lookupResultLoc[] = {
                 0, 1, 2, 3, 4, 8, 1, 2, 3, 1,   2,   3, 1,   2, 3, 4,   8, 4,   1, 2,
                 3, 1, 2, 3, 1, 2, 3, 8, 8, 0xA, 0xA, 9, 0xA, 9, 9, 0xA, 9, 0xA, 9, 9 };


/**
 * attrLocLookupTable - Attribute-location to lookup code mapping
 *
 * 2D table: attrLocLookupTable[attr][loc]
 *   attr: Expression attribute (0-11)
 *   loc:  Expression location (0-10)
 *   value: Lookup code for use with lookupResultAttr/Loc
 *
 * Reverse mapping for expression tracking
 */
uint8_t attrLocLookupTable[][11] = {
               { 0, 1, 2, 3, 4, 0, 0, 0, 5, 0x26, 0x25 },
               { 0, 0xC, 0xD, 0xE, 0xF, 0, 0, 0, 0x10, 0x21, 0x20 },
               { 0, 6, 7, 8, 0, 0, 0, 0, 0, 0x24, 0x23 },
               { 0, 9, 0xA, 0xB, 0, 0, 0, 0, 0, 0x1F, 0x1E },
               { 0, 0x12, 0x13, 0x14, 0, 0, 0, 0, 0, 0, 0 },
               { 0, 0x15, 0x16, 0x17, 0, 0, 0, 0, 0, 0, 0 },
               { 0, 0x18, 0x19, 0x1A, 0, 0, 0, 0, 0, 0x22, 0 },
               { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
               { 0, 0, 0, 0, 0x11, 0, 0, 0, 0, 0, 0x1D },
               { 0, 0, 0, 0, 0, 0, 0, 0, 0x1C, 0, 0 },
               { 0, 0, 0, 0, 0, 0, 0, 0, 0x1B, 0, 0 },
               { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x27, 0 } };

// clang-format on

// ===== UTILITY FUNCTIONS =====

/**
 * WrFragData - Write generated code fragment to output
 *
 * Emits the code fragment buffer (fragment[0..fragLen-1]) to the output
 * file (utf1). Also handles error tracking for syntax/token errors.
 *
 * Side effects:
 *   - Writes to utf1 output file
 *   - Increments programErrCnt on errors
 *   - Clears fragment buffer
 */
void WrFragData() {
    if (!(PRINT || OBJECT)) {
        if (fragment[0] == T2_SYNTAXERROR || fragment[0] == T2_TOKENERROR ||
            fragment[0] == T2_ERROR)
            programErrCnt++;
        return;
    }
    vfWbuf(&utf1, fragment, fragLen);
}

/**
 * ===== CODE FRAGMENT EMISSION UTILITIES =====
 *
 * This group of functions handles the low-level emission of code fragments
 * to the output buffer and the encoding of TX2 nodes into intermediate code.
 */

/**
 * PutTx1Byte - Add byte to code fragment buffer
 *
 * Simple buffer management function that appends a single byte to the
 * fragment[] buffer and increments fragLen to track buffer position.
 *
 * Usage: Building up fragment data byte-by-byte before emitting
 *
 * @param val - Byte value to add
 *
 * Side effects:
 *   - Increments fragLen (buffer position counter)
 *   - Modifies fragment[] at index [fragLen-1]
 *
 * Called from: PutTx1Word(), EncodeFragmentArgument()
 */
void PutTx1Byte(uint8_t val) {
    fragment[fragLen++] = val;
}

/**
 * PutTx1Word - Add 16-bit word to code fragment buffer
 *
 * Adds a 16-bit value to the fragment buffer using little-endian format
 * (low byte first, then high byte).
 *
 * Usage: Encoding 16-bit parameters or addresses in fragments
 *
 * @param val - 16-bit word value to add
 *
 * Side effects:
 *   - Calls PutTx1Byte() twice (increments fragLen by 2)
 *   - Modifies fragment[] at positions [fragLen] and [fragLen+1]
 *
 * Called from: EncodeFragmentArgument(), EmitTopItem()
 */
void PutTx1Word(uint16_t val) {
    PutTx1Byte(Low(val));  // Low byte first
    PutTx1Byte(High(val)); // High byte second
}

/**
 * EncodeFragmentArgument - Encode single code fragment argument
 *
 * Encodes a single argument into the fragment buffer, handling the encoding
 * strategy based on argument type code. Different argument types require
 * different amounts of data (0, 1, or 2 words).
 *
 * Argument Type Codes (argCode):
 *   0-7:     Small codes, encoded directly
 *   8:       Byte immediate (1 byte data)
 *   9:       Word immediate (1 word data)
 *   10:      Two-word immediate (2 words data)
 *   11:      Three-word immediate (3 words data)
 *   12:      Four-word immediate (4 words data)
 *   13:      Byte register (1 byte data)
 *   Other:   Word data (1 word)
 *
 * When iCodeArgs[0] <= 12, codes are packed into nibbles (4-bit fields)
 * in a single byte. Otherwise, codes are separate.
 *
 * @param iArg - Current argument index in iCodeArgs[]
 * @param start - Starting position in fragment buffer for potential merge
 *
 * @return Updated argument index for next argument
 *
 * Side effects:
 *   - Modifies fragment[] buffer
 *   - Increments fragLen by encoded data size
 *   - May merge codes into nibbles if iCodeArgs[0] <= 12
 *
 * Called from: EncodeFragArgs()
 */
static uint8_t EncodeFragmentArgument(uint8_t iArg, uint8_t start) {
    uint8_t argCode = (uint8_t)iCodeArgs[iArg++];

    // Step 1: Handle code encoding
    if (iCodeArgs[0] <= 12) {
        // Codes are packed into nibbles
        if (fragLen == start)
            PutTx1Byte(argCode); // First argument goes directly
        else
            fragment[start] = (fragment[start] << 4) | argCode; // Merge with previous
    }

    // Step 2: Encode argument data based on type
    if (argCode > 7) {
        // Large codes have associated data
        if (argCode == 8 || argCode == 13 || argCode == 10)
            PutTx1Byte((uint8_t)iCodeArgs[iArg++]); // Byte data
        else
            PutTx1Word(iCodeArgs[iArg++]); // Word data

        // Step 3: Extended data for multi-word arguments
        if (argCode >= 10 && argCode <= 12)
            PutTx1Word(iCodeArgs[iArg++]); // Second word
        if (argCode == 12)
            PutTx1Word(iCodeArgs[iArg++]); // Third word
    }

    return iArg; // Return updated index
}


/**
 * EncodeFragArgs - Encode all arguments for a code fragment
 *
 * Processes all arguments for a fragment by checking fragControlBits
 * to determine how many arguments exist and calling EncodeFragmentArgument()
 * for each.
 *
 * The fragControlBits specify argument encoding strategy (bits 4-2).
 *
 * @param frag - Fragment code
 *
 * Side effects:
 *   - Modifies fragment[] buffer
 *   - Updates fragLen
 *   - Calls EncodeFragmentArgument() once or twice
 *
 * Called from: EncodeFragData()
 */
static void EncodeFragArgs(uint8_t frag) {
    uint8_t iArg  = 0;
    uint8_t start = fragLen;

    // Check if fragment has arguments (bits 6-4 of fragControlBits)
    if (fragControlBits[frag] & (7 << 4)) {
        // Encode first argument
        iArg = EncodeFragmentArgument(iArg, start);

        // Encode second argument if present
        if (iCodeArgs[iArg] != 0 || iCodeArgs[0] <= 12)
            EncodeFragmentArgument(iArg, start);
    }
}

/**
 * EncodeFragData - Main code fragment encoding and emission function
 *
 * The primary function that generates intermediate code by:
 * 1. Initializing fragment buffer
 * 2. Writing fragment opcode
 * 3. Optionally writing node type (if flagged in fragControlBits)
 * 4. Encoding any arguments
 * 5. Emitting the complete fragment to output
 *
 * This is called for every code generation operation (arithmetic, memory
 * access, control flow, etc.)
 *
 * Algorithm:
 *   1. Clear buffer and write fragment code
 *   2. If fragControlBits & 0x80: Write curNodeType after fragment code
 *   3. Call EncodeFragArgs() to encode arguments
 *   4. Clear iCodeArgs array for next operation
 *   5. Call WrFragData() to emit to output file
 *
 * @param frag - Fragment code to encode and emit
 *
 * Side effects:
 *   - Resets fragLen to 0
 *   - Fills fragment[] buffer
 *   - Clears iCodeArgs[]
 *   - Calls WrFragData() to write to output
 *   - Increments codeSize (via WrFragData)
 *
 * Called from: All code generation functions throughout the compiler
 *
 * Example usage:
 * ```
 * iCodeArgs[0] = 2;     // Register D
 * iCodeArgs[1] = 0x100; // Immediate value
 * EncodeFragData(CF_MOV);  // Generate MOV instruction
 * ```
 */
void EncodeFragData(uint8_t frag) {
    fragLen = 0;
    PutTx1Byte(frag); // Step 1: Write fragment code

    if (fragControlBits[frag] & 0x80) // Step 2: Optional node type
        PutTx1Byte(curNodeType);

    EncodeFragArgs(frag);                    // Step 3: Encode arguments
    memset(iCodeArgs, 0, sizeof(iCodeArgs)); // Step 4: Clear for next use
    WrFragData();                            // Step 5: Emit to output
}

/**
 * EmitTopItem - Emit TX2 node to output
 *
 * Outputs a TX2 node directly to the TX1 output file, bypassing code
 * generation. Used for meta-information nodes, line numbers, syntax errors, etc.
 *
 * Node Structure Encoding:
 *   - Type: Always written first
 *   - Extra fields based on nodeControlMap[type] & 3:
 *     • 0: No extra fields
 *     • 1: Write left field (word)
 *     • 2: Write left and right fields (words)
 *     • 3: Write left, right, and extra fields (words)
 *   - Special case: If fragControlBits[type] & 0x80: Write right, then left
 *
 * @side effects:
 *   - Resets fragLen to 0
 *   - Writes to TX1 output file via WrFragData()
 *   - Skips LINEINFO and INCLUDE nodes unless PRINT mode
 *
 * Called from: Syntax error handling, meta-information emission
 */
void EmitTopItem() {
    fragLen = 0;

    // Skip line info and include nodes unless printing
    if (!PRINT)
        if (tx2[tx2qp].type == T2_LINEINFO || tx2[tx2qp].type == T2_INCLUDE)
            return;

    PutTx1Byte(tx2[tx2qp].type);

    // Special handling for meta-type nodes
    if (fragControlBits[tx2[tx2qp].type] & 0x80) {
        PutTx1Byte((uint8_t)tx2[tx2qp].right);
        PutTx1Word(tx2[tx2qp].left);
    } else {
        // Standard node field encoding based on control map
        switch (nodeControlMap[tx2[tx2qp].type] & 3) {
        case 0:
            break; // No fields
        case 1:
            PutTx1Word(tx2[tx2qp].left);
            break; // Left field only
        case 2:
            PutTx1Word(tx2[tx2qp].left);
            PutTx1Word(tx2[tx2qp].right);
            break; // Left and right
        case 3:
            PutTx1Word(tx2[tx2qp].left);
            PutTx1Word(tx2[tx2qp].right);
            PutTx1Word(tx2[tx2qp].extra);
            break; // All three fields
        }
    }

    WrFragData();
}

// ===== TX2 NODE MANIPULATION =====

/**
 * Tx2SyntaxError - Mark current TX2 node as syntax error
 *
 * Converts the current TX2 node into a syntax error node, storing the
 * error code for later reporting.
 *
 * @param arg1b - Error code (e.g., ERR200, ERR203, etc.)
 *
 * Side effects:
 *   - Modifies tx2[tx2qp].type to T2_SYNTAXERROR
 *   - Stores error code in tx2[tx2qp].left
 *
 * Called from: Error handling in code generation
 */
void Tx2SyntaxError(uint8_t arg1b) {
    tx2[tx2qp].type = T2_SYNTAXERROR;
    tx2[tx2qp].left = arg1b;
}

/**
 * GetFragmentOperandType - Extract operand type from fragment opcode table
 *
 * Retrieves the type encoding for left or right operand from the fragment
 * opcode table entry. Used during code generation to determine operand
 * handling strategy.
 *
 * Fragment Opcode Format:
 *   High nibble (bits 7-4): Left operand type
 *   Low nibble (bits 3-0):  Right operand type
 *
 * @param lrIdx - Operand side (Left=0, Right=1)
 *
 * @return Operand type code (0-15)
 *
 * Called from: Operand encoding, expression generation
 */
uint8_t GetFragmentOperandType(uint8_t lrIdx) {
    return lrIdx == Left ? fragmentOpcodeTable[currentFragmentCode] >> 4
                         : fragmentOpcodeTable[currentFragmentCode] & 0xf;
}

/**
 * MoveTx2 - Copy TX2 node from one location to another
 *
 * Simple node copying operation. Used during expression optimization
 * and tree manipulation.
 *
 * @param src - Source node index
 * @param dst - Destination node index
 *
 * Side effects:
 *   - Overwrites tx2[dst] with complete copy of tx2[src]
 *
 * Called from: TryInlineExpression(), tree optimization
 */
void MoveTx2(uint8_t src, uint8_t dst) {
    tx2[dst] = tx2[src];
}

/**
 * IndirectAddr - Calculate indirect addressing mode for attribute
 *
 * Converts an expression attribute to an indirect addressing mode.
 * Used to determine how to load values that are themselves addresses.
 *
 * Conversion:
 *   STRUCT_A (struct type) → 2
 *   Other types → attribute + 2
 *
 * @param attr - Expression attribute (BYTE_A, ADDRESS_A, etc.)
 *
 * @return Indirect addressing mode code
 *
 * Called from: Address calculation, pointer operations
 */
uint8_t IndirectAddr(uint8_t attr) {
    return attr == STRUCT_A ? 2 : attr + 2;
}

// ===== STACK AND BLOCK MANAGEMENT =====

/**
 * AdjustStackOnReturn - Generate code to deallocate stack space on return
 *
 * Generates 8080 code to restore the stack pointer when exiting a procedure.
 * Two strategies based on offset size:
 *
 * 1. LARGE OFFSET (> 7 words):
 *    Uses SPHL sequence for efficiency:
 *    - Optional XCHG (if HL in use)
 *    - Load HL with offset via SA2HL (stack address to HL)
 *    - SPHL (set SP from HL)
 *    - Optional XCHG restore (5-7 bytes total)
 *
 * 2. SMALL OFFSET (≤ 7 words):
 *    Uses POP loop:
 *    - Optional INX SP for odd byte
 *    - Sequence of POP H or POP D (1 byte per word)
 *
 * Algorithm:
 *   1. Calculate offset = stackOffset + currentStackDepth * 2
 *   2. Convert to word count: q = ((offset + 1) >> 1) + 2
 *   3. Special handling for T2_RETURNWORD (reduce by 2 words)
 *   4. Route to large or small offset handler
 *   5. Update currentStackDepth for tracking
 *
 * @param stackOffset - Signed offset to adjust (typically negative for deallocate)
 *
 * Side effects:
 *   - Generates code fragments (SPHL, POP, XCHG, INX SP)
 *   - Updates codeSize (5-7 for large, 1-n for small)
 *   - Updates currentStackDepth
 *
 * Called from: GenerateReturnSequence(), exit code generation
 */
void AdjustStackOnReturn(uint16_t stackOffset) {
    uint16_t offset, q;

    offset = stackOffset + currentStackDepth * 2;
    q      = ((offset + 1) >> 1) + 2; // Convert to words
    if (curNodeType == T2_RETURNWORD)
        q -= 2; // Special case adjustment

    if (q > 7) {
        // Large offset: use SPHL method (more efficient)
        if (exprLoc[0] == IR_H)
            EncodeFragData(CF_XCHG); // Save HL if in use

        iCodeArgs[0] = OP_IM16;
        iCodeArgs[1] = offset;
        EncodeFragData(CF_SA2HL); // Load HL with offset
        EncodeFragData(CF_SPHL);  // Set SP from HL

        if (exprLoc[0] == IR_H) {
            EncodeFragData(CF_XCHG); // Restore HL
            codeSize += 7;
        } else
            codeSize += 5;
    } else {
        // Small offset: use POP loop
        if (offset & 1) {
            EncodeFragData(CF_INXSP);
            codeSize++;
        }

        while (offset > 1) {
            if (exprLoc[0] == IR_H)  // HL is used for return value
                iCodeArgs[0] = IR_D; // Pop D instead
            else
                iCodeArgs[0] = IR_H; // Pop H
            iCodeArgs[1] = LOC_REG;
            EncodeFragData(CF_POP);
            codeSize++;
            offset -= 2;
        }
    }

    // Update stack depth tracking
    if (stackOffset > 0xff00)
        currentStackDepth = (uint16_t)(-stackOffset) >> 1;
    else
        currentStackDepth = 0;
}

/**
 * EnterBlk - Enter new block/scope
 *
 * Increments block nesting level, up to maximum of 20.
 * Generates error if limit exceeded.
 *
 * @return true if block entered successfully, false if limit exceeded
 *
 * Side effects:
 *   - Increments activeGrpCnt (nesting level)
 *   - May generate syntax error and emit
 *   - Increments blkOverCnt (overflow counter)
 *
 * Called from: EnterCaseBlock(), procedure entry
 */
bool EnterBlk() {
    if (activeGrpCnt < 20) {
        activeGrpCnt++;
        return true;
    }

    if (blkOverCnt == 0) {
        Tx2SyntaxError(ERR204); // LIMIT EXCEEDED: NUMBER OF ACTIVE PROCEDURES
        EmitTopItem();
    }

    blkOverCnt++;
    return false;
}

/**
 * ExitBlk - Exit current block/scope
 *
 * Decrements block nesting level. Handles overflow cases.
 * Returns false if there was an overflow condition.
 *
 * @return true if block exited successfully, false if unbalanced
 *
 * Side effects:
 *   - Decrements activeGrpCnt or blkOverCnt
 *   - May generate syntax error
 *
 * Called from: ExitCaseBlock(), procedure exit
 */
bool ExitBlk() {
    if (blkOverCnt > 0) {
        blkOverCnt--;
        return false;
    } else if (activeGrpCnt > 0) {
        activeGrpCnt--;
        return true;
    } else {
        Tx2SyntaxError(ERR205); // ILLEGAL NESTING OF BLOCKS, ENDS not BALANCED
        EmitTopItem();
        return false;
    }
}

/**
 * HandleFatalError - Handle fatal compiler error and unwind state
 *
 * Terminates compilation on fatal error by:
 * 1. Recording error code
 * 2. Emitting error marker to output
 * 3. Unwinding all active blocks
 * 4. Saving procedure metadata
 * 5. Long-jumping to error handler
 *
 * @param err - Fatal error code
 *
 * Side effects:
 *   - Emits error marker to output
 *   - Modifies procedure metadata
 *   - Performs non-local jump to exception handler
 *   - Never returns (long jump)
 *
 * Called from: Resource limit checks, unrecoverable errors
 */
void HandleFatalError(uint16_t err) {
    fatalCode = err;

    // Emit error marker
    fragment[0] = T2_SYNTAXERROR;
    fragment[1] = (uint8_t)fatalCode;
    fragment[2] = 0;
    fragLen     = 3;
    WrFragData();

    // Unwind all active blocks
    while (activeGrpCnt > 0) {
        if (ExitBlk()) {
            // Save procedure metadata before exiting
            if (blkId > activeGrpCnt) {
                info             = blk[blkId].info;
                info->codeSize   = codeSize;
                info->stackUsage = stackUsage;
                blkId            = blk[blkId].next;
                codeSize         = blk[blkId].codeSize;
                stackUsage       = blk[blkId].stackSize;
            }
        }
    }

    longjmp(exception, -1); // Non-local jump to error handler
}

/**
 * AnalyzeRegisterUsage - Analyze register usage and preservation requirements
 *
 * PURPOSE:
 * --------
 * Determines the state and requirements for all 4 registers (PSW, BC, DE, HL).
 * Performs sophisticated analysis to determine which registers need preservation
 * when values are spilled to stack. This is critical for correct register allocation
 * and spilling decisions during code generation.
 *
 * This function is called frequently during code generation to maintain accurate
 * register state information before operand loading and register allocation decisions.
 *
 * REGISTER MODEL (8080):
 * ----------------------
 * The 8080 has 4 main register pairs:
 *   Index 0: PSW (Processor Status Word) - flags register
 *   Index 1: BC  - General purpose 16-bit register pair
 *   Index 2: DE  - General purpose 16-bit register pair
 *   Index 3: HL  - General purpose 16-bit register pair
 *
 * ALGORITHM - TWO PASS ANALYSIS:
 * ================================
 *
 * PASS 1: Individual Register Analysis (lines: for i = 0 to 3)
 * ============================================================
 * For each register (0-3), determine:
 *
 * Step 1: Initialize flags
 *   - registerInExpression[i] = false (assume not in active expression)
 *   - registerNeedsSave[i] = false (assume doesn't need preservation)
 *   - registerWasSaved[i] = false (assume hasn't been saved to stack)
 *
 * Step 2: Get register contents
 *   - k = registerState[i].contents (TX2 node index stored in this register)
 *   - n = nodeControlMap[tx2[k].type] & 0xc0 (node type category)
 *
 * Step 3: Analyze register value
 *   IF register contains a valid TX2 node (k != 0):
 *   {
 *     registerHasValue[i] = true (register has content)
 *
 *     Step 3a: Check if register is in active expression
 *     - IF (exprLoc[0] == i OR exprLoc[1] == i):
 *       • registerInExpression[i] = true (this register is being used)
 *       • IF i > 0: exprRegisterCount++ (count active non-PSW registers)
 *
 *     Step 3b: Determine if value needs preservation
 *     - IF (n == 0 OR n == 0x80): // Binary/unary or statement-level operator
 *       • Values that are operators or statements might need preservation
 *       • IF tx2[k].extra == 0: // Not on stack already
 *         » Check attribute match:
 *           IF (tx2[k].exprAttr == registerState[i].dataType) OR
 *              (tx2[k].exprAttr == BYTE_A AND registerState[i].dataType == 6):
 *           {
 *             • Attribute types match, value is valid
 *             • Check reference count and usage:
 *               IF (tx2[k].cnt > 1): // Referenced multiple times
 *                 » registerNeedsSave[i] = true (need to preserve for other uses)
 *               ELSE IF (registerInExpression[i]): // In active expression
 *                 » registerNeedsSave[i] = true (need to preserve before overwrite)
 *               ELSE IF (curExprLoc[Left] != k AND k != curExprLoc[Right]):
 *                 » registerNeedsSave[i] = true (need to preserve, not current operand)
 *           }
 *   }
 *   ELSE (k == 0): // Register empty
 *   {
 *     registerHasValue[i] = registerState[i].isDirect
 *       • Register is considered as having value only if isDirect flag set
 *       • isDirect = register holds direct value (not stack-relative)
 *   }
 *
 * PASS 1 RESULTS:
 * ---------------
 * For each register:
 * - registerHasValue[i]: true if register contains a value
 * - registerInExpression[i]: true if value is in active expression (operand)
 * - registerNeedsSave[i]: true if value must be preserved (still needed later)
 * - exprRegisterCount: count of non-PSW registers in active expression
 *
 *
 * PASS 2: Duplicate Detection and Save Counting (lines: for i = IR_PSW to IR_H)
 * ===============================================================================
 * Handles special case: Multiple registers holding the SAME value.
 * When duplicates exist, only one needs to be saved.
 *
 * For each register i with registerNeedsSave[i] = true:
 * {
 *   registerWasSaved[i] = true (mark as a candidate for saving)
 *   m = registerInExpression[i] (remember if this register is in expression)
 *
 *   Step 1: Search for duplicate registers with same value
 *   - Loop j from IR_H (3) down to i+1
 *     FOR each register j > i:
 *     {
 *       IF registerNeedsSave[j]:
 *       {
 *         IF registerState[j].contents == registerState[i].contents:
 *         {
 *           // Found duplicate! Both registers hold same TX2 node
 *           registerNeedsSave[j] = false // Don't need to save duplicate
 *
 *           IF i != 0: // Not PSW register
 *           {
 *             registerNeedsSave[i] = false // Keep one copy by not saving either?
 *             m |= registerInExpression[j] // Remember if either is in expression
 *           }
 *           // This optimization: if one is in expression, keep it
 *           // Don't save the duplicate if the original isn't in expression
 *         }
 *       }
 *     }
 *   }
 *
 *   Step 2: Count registers that will actually be saved
 *   - IF i != 0 AND NOT m:
 *     • Register is not PSW AND value is not in expression
 *     • savedRegisterCount++ (this register will be pushed to stack)
 * }
 *
 * PASS 2 RESULTS:
 * ---------------
 * - registerWasSaved[i]: true if register marked for saving
 * - registerNeedsSave[i]: updated if duplicates found
 * - savedRegisterCount: total count of registers that will actually be saved
 *
 *
 * KEY REGISTER STATE FLAGS:
 * ========================
 *
 * registerState[i].contents:
 *   - TX2 node index stored in this register
 *   - 0 = register empty
 *   - 1-254 = index of TX2 node currently in register
 *
 * registerState[i].dataType:
 *   - Expression attribute of value in register
 *   - BYTE_A, ADDRESS_A, STRUCT_A, etc.
 *   - Used to verify value matches when checking if value can be reused
 *
 * registerState[i].isDirect:
 *   - true = register holds direct value (not address pointing to stack)
 *   - false = register holds indirect/computed value
 *   - Used for empty registers to indicate they may have state
 *
 * exprLoc[Left (0)]:
 *   - Register index (0-3) or memory location code for left operand
 *   - Used to identify which register holds left operand of current operation
 *
 * exprLoc[Right (1)]:
 *   - Register index (0-3) or memory location code for right operand
 *   - Used to identify which register holds right operand of current operation
 *
 * curExprLoc[Left (0)]:
 *   - TX2 node index for left operand's value (different from exprLoc which is location)
 *   - Used to determine which value is being used
 *
 * curExprLoc[Right (1)]:
 *   - TX2 node index for right operand's value
 *
 *
 * OUTPUT VARIABLES SET:
 * ====================
 *
 * Global Arrays (indexed by register 0-3):
 * ----------------------------------------
 * registerInExpression[i]:
 *   - true if register value is operand in active expression
 *   - Used by: SaveOrRedirectRegister, SpillNonActiveRegisters
 *   - Meaning: Don't invalidate this register without saving first
 *
 * registerNeedsSave[i]:
 *   - true if register value is needed later (must not lose it)
 *   - Used by: SaveOrRedirectRegister, InvalidateRegistersByMask
 *   - Meaning: Must push to stack before register can be reused
 *
 * registerWasSaved[i]:
 *   - true if register was marked for saving in pass 2
 *   - Candidate flag; not all marked registers are actually saved
 *   - Used for tracking which registers were candidates
 *
 * registerHasValue[i]:
 *   - true if register contains meaningful value
 *   - Used to distinguish empty registers from those with content
 *   - Helps determine register allocation priorities
 *
 * Global Counters:
 * ----------------
 * exprRegisterCount:
 *   - Count of non-PSW registers (1-3) in active expression
 *   - Value: 0-3 (can't exceed 3, only BC, DE, HL)
 *   - Used by: operand loading to track expression register usage
 *
 * savedRegisterCount:
 *   - Count of registers that will be PUSHED to stack
 *   - Calculation: Registers where registerNeedsSave=true but not in expression
 *   - Used by: code generation to estimate stack usage
 *
 *
 * DUPLICATE DETECTION LOGIC (Pass 2):
 * ====================================
 *
 * Why detect duplicates?
 * - Same value might be in multiple registers (optimization by earlier code)
 * - Example: Both D and H hold variable 'a' (loaded once, copied to multiple registers)
 * - Benefit: If multiple registers hold same value, only save ONE copy
 * - Avoids redundant stack writes
 *
 * Duplicate Detection Algorithm:
 * 1. For each register i marked for saving:
 *    2. Search registers j where j > i for same contents
 *    3. If found: mark j's save flag false (keep i's)
 *    4. If neither is in expression: increment saved count
 *
 * Example Scenario:
 * ```
 * Before:
 *   registerState[2].contents = 42 (D holds TX2 node 42)
 *   registerState[3].contents = 42 (H holds same TX2 node 42)
 *   registerNeedsSave[2] = true
 *   registerNeedsSave[3] = true
 *   registerInExpression[2] = false
 *   registerInExpression[3] = true
 *
 * After Pass 2:
 *   registerNeedsSave[2] = false (D no longer needs saving - duplicate)
 *   registerNeedsSave[3] = true (H still needs saving - in expression)
 *   m = true (because H is in expression)
 *   savedRegisterCount not incremented (because m is true for D)
 * ```
 *
 *
 * ATTRIBUTE MATCHING (Pass 1, Step 3b):
 * ======================================
 *
 * The condition checks:
 * ```
 * (tx2[k].exprAttr == registerState[i].dataType) OR
 * (tx2[k].exprAttr == BYTE_A AND registerState[i].dataType == 6)
 * ```
 *
 * Why this check?
 * - Register value must match the attribute expected for the TX2 node
 * - BYTE_A (1 byte value) vs ADDRESS_A (2 byte value) mismatch = register invalid
 * - Special case: Code 6 is treated as BYTE_A for compatibility
 *
 * If attributes match:
 * - Value in register is valid representation of TX2 node
 * - Register can be trusted for preservation decisions
 *
 * If attributes don't match:
 * - Register content is stale or misrepresented
 * - Don't mark for saving (value is invalid)
 *
 *
 * REFERENCE COUNT LOGIC (Pass 1, Step 3b):
 * =========================================
 *
 * tx2[k].cnt: Reference count for the TX2 node
 *   - How many times this value is used in the current expression/statement
 *   - 1 = used once (safe to lose after use)
 *   - 2+ = used multiple times (must preserve for other uses)
 *
 * Decision tree:
 * 1. IF cnt > 1: → registerNeedsSave[i] = true
 *    Reason: Value used multiple places, need it later
 *
 * 2. ELSE IF in expression: → registerNeedsSave[i] = true
 *    Reason: Currently being used, don't lose it during operation
 *
 * 3. ELSE IF not in current operands: → registerNeedsSave[i] = true
 *    Reason: Value exists but not being used right now, still needed later
 *
 * 4. ELSE: → registerNeedsSave[i] = false
 *    Reason: Used once, currently being processed, not needed after
 *
 *
 * WHEN THIS FUNCTION IS CALLED:
 * =============================
 *
 * Called from:
 * - SaveRegisterToStack() - Before saving register to stack
 * - SaveOrRedirectRegister() - To analyze register preservation needs
 * - InvalidateRegistersByMask() - To determine which registers to save during invalidation
 * - AnalyzeRegisterUsage() itself - Called recursively in pass 2 analysis
 *
 * Preconditions:
 * - registerState[] must be populated with current register contents
 * - exprLoc[] must point to current operands (if any)
 * - curExprLoc[] must point to current operand values (if any)
 * - tx2[] tree must be valid and populated
 *
 * Postconditions:
 * - All register flags accurately reflect current state
 * - Safe to make register allocation/spilling decisions
 * - Counters exprRegisterCount and savedRegisterCount are accurate
 *
 *
 * PERFORMANCE CHARACTERISTICS:
 * ============================
 *
 * Time complexity: O(1) per register
 * - Pass 1: 4 registers × O(1) operations = O(4) = O(1)
 * - Pass 2: 4 registers × 4 registers max inner loop = O(16) = O(1)
 * - Overall: Called frequently but very fast (constant time)
 *
 * Space complexity: O(1)
 * - Local variables: i, j, k, n, m (5 bytes)
 * - No dynamic allocation
 *
 *
 * EDGE CASES & SPECIAL HANDLING:
 * ==============================
 *
 * 1. PSW Register (i == 0):
 *    - Treated specially in pass 2
 *    - NOT counted in savedRegisterCount even if saved
 *    - Reason: PSW is status register, different handling rules
 *
 * 2. Empty Registers (k == 0):
 *    - No TX2 node content
 *    - Use isDirect flag to determine if still valid
 *    - Don't mark for saving (nothing to save)
 *
 * 3. Node on Stack (tx2[k].extra != 0):
 *    - Value already saved to stack at level tx2[k].extra
 *    - Don't mark for saving again (already preserved)
 *    - Skip preservation analysis
 *
 * 4. Duplicate with PSW Register:
 *    - If PSW and another register have same contents
 *    - Special PSW handling: if (i != 0)
 *    - Don't apply full duplicate logic to PSW
 *
 * 5. Reference Count = 1:
 *    - Value not used multiple times
 *    - Still might need saving if in expression or not current operand
 *
 *
 * INTEGRATION WITH OTHER FUNCTIONS:
 * ==================================
 *
 * SaveOrRedirectRegister():
 *   - Uses: registerNeedsSave[], registerInExpression[]
 *   - Makes decision to save or redirect based on flags
 *
 * SpillNonActiveRegisters():
 *   - Uses: registerNeedsSave[], registerInExpression[]
 *   - Saves only registers NOT in current expression
 *
 * InvalidateRegistersByMask():
 *   - Uses: registerNeedsSave[]
 *   - Calls AnalyzeRegisterUsage() to update flags
 *   - Then saves registers marked as needing preservation
 *
 * AllocateRegister():
 *   - Uses: registerInExpression[], registerNeedsSave[]
 *   - Chooses least-important register to allocate
 *
 *
 * DEBUG INFORMATION:
 * ==================
 *
 * To trace register analysis:
 * 1. Check registerHasValue[] to see which registers have values
 * 2. Check registerInExpression[] to see active registers
 * 3. Check registerNeedsSave[] to see preservation requirements
 * 4. Compare exprRegisterCount with actual registers in exprLoc[]
 * 5. Verify savedRegisterCount matches actual save candidates
 *
 * Common issues:
 * - registerNeedsSave[i] = false when should be true: Orphaned value
 * - registerNeedsSave[i] = true when should be false: Unnecessary spills
 * - exprRegisterCount mismatch: exprLoc[] pointing to invalid registers
 * - savedRegisterCount = 0: May lose values during code generation
 */
void AnalyzeRegisterUsage() {
    uint8_t i, j, k, n;
    bool m;

    exprRegisterCount  = 0;
    savedRegisterCount = 0;

    // ===== PASS 1: Individual Register Analysis =====
    // For each register (PSW=0, BC=1, DE=2, HL=3)
    for (i = 0; i <= 3; i++) {
        // Initialize flags for this register
        registerInExpression[i] = false;
        registerNeedsSave[i]    = false;
        registerWasSaved[i]     = false;

        // Get the TX2 node index stored in this register
        k = registerState[i].contents;

        // Get the node type category for this TX2 node
        n = nodeControlMap[tx2[k].type] & 0xc0;

        // Check if register contains a valid TX2 node
        if (k != 0) {
            // Register has content - mark it
            registerHasValue[i] = true;

            // Check if this register is an operand in the active expression
            if (exprLoc[0] == i || exprLoc[1] == i) {
                registerInExpression[i] = true; // This register is being used
                if (i > 0)
                    exprRegisterCount++; // Count non-PSW registers in expression
            }

            // Determine if the value needs to be preserved for later use
            // Only analyze if node is binary/unary operator or statement-level node
            if (n == 0 || n == 0x80) {
                // Only consider if value is not already on stack
                if (tx2[k].extra == 0) {
                    // Check that register's attribute matches the value's attribute
                    if (tx2[k].exprAttr == registerState[i].dataType ||
                        (tx2[k].exprAttr == BYTE_A && registerState[i].dataType == 6)) {
                        // Attributes match - check if preservation is needed

                        // NEED SAVE IF:
                        // 1. Value is referenced multiple times (cnt > 1)
                        // 2. OR value is currently in active expression
                        // 3. OR value is not the current operand being processed
                        if (tx2[k].cnt > 1 || registerInExpression[i] ||
                            (curExprLoc[Left] != k && k != curExprLoc[Right])) {
                            registerNeedsSave[i] = true;
                        }
                    }
                }
            }
        } else {
            // Register is empty - check isDirect flag for validity
            registerHasValue[i] = registerState[i].isDirect;
        }
    }

    // ===== PASS 2: Duplicate Detection and Save Counting =====
    // Identify duplicate registers (same value in multiple registers)
    // and count registers that will actually be saved
    for (i = IR_PSW; i <= IR_H; i++) {
        if (registerNeedsSave[i]) {
            registerWasSaved[i] = true;
            m                   = registerInExpression[i]; // Remember if in expression

            // Search for higher-indexed registers with the same contents
            j = IR_H; // Start from highest register (HL)
            while (j > i) {
                if (registerNeedsSave[j]) {
                    // Check if register j contains the same TX2 node
                    if (registerState[j].contents == registerState[i].contents) {
                        // Found duplicate! Don't save the duplicate
                        registerNeedsSave[j] = false;

                        // Special handling: if not PSW, adjust flags
                        if (i != 0) {
                            registerNeedsSave[i] = false; // Already have a copy
                            m |= registerInExpression[j]; // OR in j's in-expression flag
                        }
                    }
                }
                j--;
            }

            // Count registers that will actually be saved
            // Only count non-PSW registers that aren't in active expression
            if (i != 0 && !m) {
                savedRegisterCount++;
            }
        }
    }
}

void SaveRegisterToStack(uint8_t irReg) {
    uint8_t i;
    AnalyzeRegisterUsage();
    i = stackNodeContents[currentStackDepth] = registerState[irReg].contents;
    if (registerNeedsSave[irReg])
        tx2[i].extra = currentStackDepth;

    if (irReg != 0)
        stackRegisterAttrs[currentStackDepth] =
            (registerState[irReg].dataType << 4) | (registerState[irReg].offset & 0xf);
    else
        stackRegisterAttrs[currentStackDepth] = 0xB0;
}

/**
 * RestoreRegisterFromStack - Restore register contents from stack frame
 *
 * Retrieves a register's value and associated metadata from the stack frame,
 * updating the register tracking arrays to reflect the restored state. This is
 * the inverse operation of Sub_5C1D() which saves register state to stack.
 *
 * The function performs three main tasks:
 *
 * 1. Restore Register Association:
 *    - Retrieves the TX2 node index from stack: registerState[irReg].contents =
 * bC140[currentStackDepth]
 *    - Links register back to the expression node it was holding
 *    - If this stack level matches the node's saved position (tx2[side].extra ==
 * currentStackDepth): • Clears the node's stack reference: tx2[side].extra = 0 • Indicates the
 * value is now in register, not on stack
 *
 * 2. Clear Direct Register Flag:
 *    - Sets registerState[irReg].isDirect = false
 *    - Indicates register now contains a direct value (not stack-relative)
 *    - Marks register as "clean" for code generation
 *
 * 3. Restore Register Attributes:
 *    - Extracts attribute from stack metadata: registerState[irReg].dataType =
 * bC0C3[currentStackDepth] >> 4 • Upper 4 bits contain data type/addressing mode • Values:
 * 0=uint8_t, 1=uint16_t, 2-8=various addressing modes
 *
 *    - Extracts offset from stack metadata: bC0A8[irReg] = bC0C3[currentStackDepth] & 0xf
 *      • Lower 4 bits contain register offset adjustment
 *      • Sign-extends if negative (bit 3 set):
 *        » If offset > 7: OR with 0xf0 to create sign-extended byte
 *        » Creates proper signed value for offset calculations
 *
 * Stack Metadata Format (bC0C3[currentStackDepth]):
 *
 *   Bits 7-4: Register attribute (registerState[].dataType)
 *   Bits 3-0: Register offset (bC0A8[], sign-extended)
 *
 *   Example: 0xB3 = attribute 0xB, offset -13 (0x3 → 0xF3)
 *            0x25 = attribute 0x2, offset +5
 *
 * Usage Context:
 *
 * This function is called when:
 * - Popping a register from stack (PopRegisterFromStack/Sub_6416)
 * - Restoring register state after spill (UpdateRegisterState case 0)
 * - Exchanging HL with stack top (UpdateRegisterState case 1)
 *
 * Relationship to Sub_5C1D:
 * - Sub_5C1D: Saves register → stack (PUSH operation)
 * - Sub_5C97: Restores stack → register (POP operation)
 *
 * Register State Arrays Updated:
 * - registerState[].contents: TX2 node index (what value is in register)
 * - registerState[].dataType: Data type/addressing mode attribute
 * - bC0A8[]: Register offset adjustment
 * - registerState[].isDirect: Direct value flag (cleared to false)
 *
 * Stack Tracking Arrays Read:
 * - bC140[]: TX2 node index saved at stack level
 * - bC0C3[]: Packed attribute and offset metadata
 *
 * @param irReg - Register index to restore (0=B/BC, 2=D/DE, 3=H/HL)
 *
 * @global currentStackDepth - Current stack depth (stack level to restore from)
 * @global bC140[] - Stack-to-TX2 node mapping
 * @global bC0C3[] - Stack metadata (attribute and offset)
 * @global registerState[].contents - Register-to-TX2 node mapping
 * @global registerState[].dataType - Register attributes
 * @global bC0A8[] - Register offsets
 * @global registerState[].isDirect - Direct value flags
 * @global tx2[] - Expression tree nodes
 *
 * Side effects:
 * - Updates register tracking for irReg
 * - May clear tx2[].extra if register was saved at this stack level
 * - Does NOT modify currentStackDepth (caller must decrement stack depth)
 *
 * Called from:
 * - PopRegisterFromStack() - After POP instruction
 * - UpdateRegisterState() - During register state management
 * - ExchangeHLStack() - During XTHL operation
 *
 * Example:
 * ```
 * // Register H was saved to stack level 5 with attribute 0x6, offset -2
 * // bC140[5] = 42 (TX2 node), bC0C3[5] = 0x6E (attr=6, offset=14/-2)
 * // currentStackDepth = 5
 *
 * Sub_5C97(3);  // Restore register H (irReg=3)
 *
 * // Results:
 * // registerState[3].contents = 42        (H now holds node 42)
 * // registerState[3].dataType = 6         (attribute 6)
 * // bC0A8[3] = 0xFE      (offset -2, sign-extended from 0xE)
 * // registerState[3].isDirect = 0        (direct register)
 * // tx2[42].extra = 0    (value no longer on stack)
 * ```
 */
void RestoreRegisterFromStack(uint8_t irReg) {
    uint8_t i;

    // Step 1: Restore register-to-TX2 node association
    i = registerState[irReg].contents = stackNodeContents[currentStackDepth];

    // If this stack level matches where the value was saved, clear stack reference
    if (currentStackDepth == tx2[i].extra)
        tx2[i].extra = 0;

    // Step 2: Mark register as containing direct value
    registerState[irReg].isDirect = false;

    // Step 3: Restore register attribute and offset from packed stack metadata
    registerState[irReg].dataType =
        stackRegisterAttrs[currentStackDepth] >> 4; // Upper 4 bits: attribute
    registerState[irReg].offset =
        stackRegisterAttrs[currentStackDepth] & 0xf; // Lower 4 bits: offset

    // Sign-extend offset if negative (bit 3 set)
    if (registerState[irReg].offset > 7)
        registerState[irReg].offset =
            registerState[irReg].offset | 0xf0; // OR with 0xF0 to sign-extend
}

void PushRegisterToStack(uint8_t irReg) {
    if (stackUsage < ++currentStackDepth * 2)
        stackUsage = currentStackDepth * 2;
    SaveRegisterToStack(irReg);
    iCodeArgs[0] = irReg;
    iCodeArgs[1] = LOC_STACK;
    iCodeArgs[2] = currentStackDepth;
    EncodeFragData(CF_PUSH);
    codeSize++;
}

/**
 * SpillNonActiveRegisters - Save non-active registers to stack before invalidating operand
 *
 * Saves all registers that need preservation (registerNeedsSave[]) but are not currently in use
 * by the active expression (registerInExpression[]), then marks the specified operand as being on
 * the stack. This function is called before operations that will invalidate a register
 * containing a needed value.
 *
 * The function performs two main tasks:
 *
 * 1. Save Preserved Registers Not In Active Expression:
 *    - Loops through all registers (PSW=0, BC=1, DE=2, HL=3)
 *    - For each register where:
 *      • registerNeedsSave[side] = true (register needs to be preserved)
 *      • registerInExpression[side] = false (register NOT in current expression)
 *    - Calls PushRegisterToStack(side) to push register to stack
 *    - This prevents losing values needed later when we invalidate regMask
 *
 * 2. Mark Operand As Stack-Based:
 *    - Determines which expression side (Left or Right) uses the register
 *    - Updates that side's location to 9 (stack-based)
 *    - If exprLoc[0] == regMask: Set exprLoc[0] = 9
 *    - If exprLoc[1] == regMask: Set exprLoc[1] = 9
 *    - This tells subsequent code that the operand is now on the stack
 *
 * Usage Scenarios:
 *
 * This function is called from Sub_5D6B() in two cases:
 *
 * Case 1: Register needs preservation AND is in active expression
 * - Before: registerNeedsSave[reg]=true, registerInExpression[reg]=true
 * - Action: Save other non-active registers, then save this one
 * - After: All preserved values are on stack, reg can be reused
 *
 * Case 2: Register is in active expression but doesn't need preservation
 * - But: No duplicate register found with same value
 * - Action: Save other non-active registers, mark as stack, then save
 * - After: All preserved values safe, reg marked as stack-based
 *
 * Example Flow (Sub_5D6B calling Sub_5E16):
 *
 * ```
 * // Setup: Register D holds operand for Left side
 * // exprLoc[0] = 2 (Left in D), exprLoc[1] = 3 (Right in H)
 * // registerNeedsSave[0]=true, registerInExpression[0]=false (B needs save, not active)
 * // registerNeedsSave[2]=false, registerInExpression[2]=true (D active, no save needed)
 * // registerNeedsSave[3]=true, registerInExpression[3]=true  (H active and needs save)
 *
 * Sub_5D6B(2);  // Called to save/handle register D
 *
 * // Sub_5D6B determines: registerInExpression[2]=true (D is in expression)
 * // But no duplicate register found
 * // So calls: Sub_5E16(2)
 *
 * Sub_5E16(2):
 *   // Loop side=0: registerNeedsSave[0]=true, registerInExpression[0]=false
 *   //   -> Save B to stack via PushRegisterToStack(0)
 *   // Loop side=2: Skip (arg=2)
 *   // Loop side=3: registerNeedsSave[3]=true, registerInExpression[3]=true
 *   //   -> Skip (in expression)
 *
 *   // exprLoc[0] == 2, so set exprLoc[0] = 9
 *
 * // Results:
 * // - Register B saved to stack
 * // - Left operand marked as stack-based (exprLoc[0]=9)
 * // - Register D can now be safely saved/reused
 * ```
 *
 * Register State Flags:
 * - registerNeedsSave[side]: Register needs preservation (value used later)
 * - registerInExpression[side]: Register in current active expression
 * - Combinations:
 *   • registerNeedsSave=false, registerInExpression=false: Empty/unused register
 *   • registerNeedsSave=false, registerInExpression=true:  Active operand, temporary value
 *   • registerNeedsSave=true,  registerInExpression=false: Saved value, not currently used -> SAVE
 IT • registerNeedsSave=true,  registerInExpression=true:  Active operand, needs preservation
 *
 * Why This Function Exists:
 *
 * When we need to save or invalidate a register that's in the active expression,
 * we must first ensure that OTHER registers holding values we'll need later are
 * safely preserved. This function identifies and saves those "bystander" registers
 * before we proceed with the operation on the target register.
 *
 * Without this step, we could lose important intermediate values when spilling
 * the active register to stack.
 *
 * @param regMask - Register index being saved/invalidated (0=BC, 2=DE, 3=HL)
 *
 * @global registerNeedsSave[] - Register preservation flags
 * @global registerInExpression[] - Register in-expression flags
 * @global exprLoc[] - Expression location tracking (Left and Right)
 *
 * Side effects:
 * - Pushes non-active preserved registers to stack via PushRegisterToStack()
 * - Updates expression location for operand using regMask
 * - Increases stack depth (currentStackDepth) for each saved register
 * - Generates PUSH instructions for saved registers
 * - Updates code size counter
 *
 * Called from:
 * - Sub_5D6B() - When saving a register in active expression
 *
 * Calls:
 * - PushRegisterToStack() - To push registers to stack
 *
 * Related Functions:
 * - Sub_5D6B(): Main register spilling logic (calls this)
 * - PushRegisterToStack(): Actually pushes register to stack
 * - AnalyzeRegisterUsage(): Sets registerNeedsSave[] and registerInExpression[] flags
 */
static void SpillNonActiveRegisters(uint8_t regIdx) {
    // Step 1: Save all preserved registers not in current expression
    for (uint8_t i = IR_PSW; i <= IR_H; i++)
        if (registerNeedsSave[i] && !registerInExpression[i])
            PushRegisterToStack(i); // Push register to stack

    // Step 2: Mark the specified operand as now being on the stack
    if (exprLoc[0] == regIdx)
        exprLoc[0] = 9; // Left operand now on stack
    else
        exprLoc[1] = 9; // Right operand now on stack
}

/**
 * SaveOrRedirectRegister - Handle register spilling or redirection for operand reuse
 *
 * Manages register conflicts when a register needs to be saved or when an operand
 * needs to be redirected to another register. This is a central function in the
 * register spilling mechanism that determines whether to save a register to stack
 * or simply redirect expression tracking to a duplicate register.
 *
 * The function implements a sophisticated strategy based on two key register flags:
 * - registerNeedsSave[reg]: Register value needs to be preserved (used later in code)
 * - registerInExpression[reg]: Register is currently in active expression (Left or Right operand)
 *
 * Decision Tree:
 *
 * 1. IF registerNeedsSave[regMask] = true (Register needs preservation):
 *
 *    a) IF registerInExpression[regMask] = true (AND register in active expression):
 *       • Call SpillNonActiveRegisters(arg1b)
 *         » Saves all OTHER preserved registers not in expression
 *         » Marks this operand as stack-based (exprLoc = 9)
 *       • Call PushRegisterToStack(arg1b)
 *         » Pushes this register to stack (PUSH instruction)
 *         » Updates stack tracking metadata
 *
 *       Example: Register holds operand AND needs to be saved for later
 *       → Save bystander registers, mark as stack, then PUSH
 *
 *    b) ELSE (register NOT in active expression):
 *       • Call PushRegisterToStack(regMask) directly
 *         » Just push the register to stack
 *         » No need to save other registers (not in expression)
 *
 *       Example: Register not currently used but needs preservation
 *       → Simple PUSH operation
 *
 * 2. ELSE IF registerInExpression[regMask] = true (Register in expression but doesn't need save):
 *
 *    First, attempt to find a duplicate register:
 *    • Loop through all registers (side = 0 to 3)
 *    • Look for register where:
 *      » registerState[side].contents == registerState[regMask].contents (holds same TX2 node)
 *      » side != regMask (different register)
 *      » registerState[side].dataType == registerState[regMask].dataType (same attribute)
 *
 *    IF duplicate found:
 *       • Redirect expression to use the duplicate register
 *       • If exprLoc[0] == regMask: exprLoc[0] = i
 *       • If exprLoc[1] == regMask: exprLoc[1] = i
 *       • Return immediately (no save needed)
 *
 *       Example: Both D and H hold the same value
 *       → Redirect operand from D to H, freeing up D
 *
 *    IF no duplicate found:
 *       • Call SpillNonActiveRegisters(arg1b)
 *         » Saves other preserved registers
 *         » Marks operand as stack-based
 *       • Call PushRegisterToStack(arg1b)
 *         » Pushes register to stack
 *
 *       Example: Register in expression, no duplicate available
 *       → Must save to stack like case 1a
 *
 * 3. ELSE (Register neither needs preservation nor in expression):
 *    • Function does nothing
 *    • Register can be freely overwritten
 *
 * Register State Combinations:
 *
 * registerNeedsSave  registerInExpression  Action
 * ------  ------  ------
 * false   false   Nothing (register free to use)
 * false   true    Try duplicate redirect, else save (case 2)
 * true    false   Simple PUSH to stack (case 1b)
 * true    true    Spill bystanders + PUSH (case 1a)
 *
 * Usage Scenarios:
 *
 * Scenario 1: Commutative Operation Optimization
 * ```
 * // Expression: a + a (both operands same variable)
 * // Both D and H contain value of 'a'
 * // exprLoc[0] = 2 (Left in D), exprLoc[1] = 3 (Right in H)
 *
 * Sub_5D6B(2);  // Need to free up D
 *
 * // Finds: registerState[3].contents == registerState[2].contents (H has same value as D)
 * // Action: exprLoc[0] = 3 (redirect Left to H)
 * // Result: Both operands now in H, D freed without save
 * ```
 *
 * Scenario 2: Complex Expression with Dependencies
 * ```
 * // Expression: (b + c) * d
 * // B holds 'b', D holds 'c', H holds intermediate (b+c)
 * // Need to load 'd' into D, but 'c' in D is still needed
 * // registerNeedsSave[2]=true, registerInExpression[2]=false
 *
 * Sub_5D6B(2);  // Save register D
 *
 * // Case 1b: registerNeedsSave[2]=true, registerInExpression[2]=false
 * // Action: PushRegisterToStack(2) → PUSH D
 * // Result: D saved to stack, can be reused for 'd'
 * ```
 *
 * Scenario 3: Active Expression with No Duplicate
 * ```
 * // Expression: a + b
 * // D holds 'a' (Left), H holds 'b' (Right)
 * // Need to save D for later use, but it's in active expression
 * // registerNeedsSave[2]=true, registerInExpression[2]=true
 * // No other register holds 'a'
 *
 * Sub_5D6B(2);  // Save active register D
 *
 * // Case 1a: registerNeedsSave[2]=true, registerInExpression[2]=true
 * // Action 1: SpillNonActiveRegisters(2)
 * //   - Saves any other preserved registers (e.g., B if needed)
 * //   - Sets exprLoc[0] = 9 (mark Left as stack)
 * // Action 2: PushRegisterToStack(2) → PUSH D
 * // Result: All preserved values safe, Left operand on stack
 * ```
 *
 * Why Duplicate Detection Matters:
 *
 * When a register is in the active expression but doesn't need preservation,
 * checking for duplicates avoids unnecessary stack operations. This is common
 * in scenarios like:
 * - Commutative operations (a + a)
 * - Common subexpressions loaded into multiple registers
 * - Register allocation with multiple live copies
 *
 * By redirecting to a duplicate, we:
 * - Avoid PUSH/POP overhead
 * - Keep values in registers (faster access)
 * - Reduce stack usage
 * - Simplify code generation
 *
 * @param regMask - Register index to save/redirect (0=BC, 2=DE, 3=HL)
 *
 * @global registerNeedsSave[] - Register preservation flags (needs save)
 * @global registerInExpression[] - Register in-expression flags (currently used)
 * @global registerState[].contents - Register contents (TX2 node index)
 * @global registerState[].dataType - Register attributes (data type/addressing mode)
 * @global exprLoc[] - Expression locations (Left and Right operands)
 *
 * Side effects:
 * - May push register(s) to stack via PushRegisterToStack()
 * - May save bystander registers via SpillNonActiveRegisters()
 * - May redirect expression tracking to duplicate register
 * - Updates stack depth (currentStackDepth) if saving
 * - Generates PUSH instructions if saving
 * - Updates code size counter if saving
 *
 * Called from:
 * - GenerateOperandCode() - Before loading operand
 * - SaveConflictingRegisters() - During register conflict resolution
 * - Other register allocation points
 *
 * Calls:
 * - SpillNonActiveRegisters() - Saves bystander registers
 * - PushRegisterToStack() - Pushes register to stack
 *
 * Related Functions:
 * - AnalyzeRegisterUsage() - Sets registerNeedsSave[] and registerInExpression[] flags
 * - SpillNonActiveRegisters() - Saves non-active preserved registers
 * - PushRegisterToStack() - Actually pushes register to stack with tracking
 * - AllocateRegister() - Main register allocation logic
 */
void SaveOrRedirectRegister(uint8_t regIdx) {

    if (registerNeedsSave[regIdx]) {
        // Case 1: Register needs preservation

        if (registerInExpression[regIdx]) {
            // Case 1a: Register in active expression
            // Save bystander registers first, then save this one
            SpillNonActiveRegisters(regIdx);
        }
        // Case 1b: Register not in expression, or case 1a continues
        // Push register to stack
        PushRegisterToStack(regIdx);

    } else if (registerInExpression[regIdx]) {
        // Case 2: Register in expression but doesn't need preservation

        // Try to find a duplicate register with same value
        for (uint8_t i = IR_PSW; i <= IR_H; i++) {
            if (registerState[i].contents == registerState[regIdx].contents && i != regIdx &&
                registerState[i].dataType == registerState[regIdx].dataType) {
                // Found duplicate - redirect expression to use it instead
                if (exprLoc[Left] == regIdx)
                    exprLoc[Left] = i; // Redirect Left operand
                else
                    exprLoc[Right] = i; // Redirect Right operand
                return;                 // Done - no save needed
            }
        }

        // No duplicate found - must save to stack
        SpillNonActiveRegisters(regIdx);
        PushRegisterToStack(regIdx);
    }

    // Case 3: Register neither needs save nor in expression
    // Do nothing - register is free to overwrite
}

/**
 * InvalidateRegistersByMask - Selectively invalidate and save registers based on bitmask
 *
 * Invalidates a set of registers specified by a bitmask, optionally saving them to stack
 * if they contain values that need preservation. This function is used to clear registers
 * before operations that will overwrite them, while preserving the current expression
 * operand locations.
 *
 * The function performs a sophisticated register invalidation process:
 *
 * 1. Preserve Current Expression Context:
 *    - Saves curExprLoc[0] and curExprLoc[1] to local variables (locLeft, locRight)
 *    - Clears curExprLoc[] to prevent interference during register analysis
 *    - Ensures expression operands aren't treated as "in use" during invalidation
 *
 * 2. Bit mask transformation removed and replaced by bit AND
 *    bits are stored as xxxxPBDH where P=PSW, B=BC, D=DE, H=HL
 *
 * 3. Analyze Register State:
 *    - Calls AnalyzeRegisterUsage() to determine which registers need preservation
 *    - Sets registerNeedsSave[] flags (registers that need to be saved)
 *    - Since curExprLoc[] is cleared, no registers marked as "in expression"
 *
 * 4. Process Each Register (Loop side = IR_PSW to IR_H):
 *    For each register position:
 *
 *    a) Check if register should be invalidated (regMask & 0x8):
 *       IF bit is set:
 *
 *       side) Save if needed:
 *          • IF registerNeedsSave[side] = true (register needs preservation):
 *            » Call PushRegisterToStack(side) to push register to stack
 *            » Saves the value before invalidation
 *
 *       ii) Clear register tracking:
 *           • Set registerState[side].isDirect = false (clear direct value flag)
 *           • Set registerState[side].contents = 0 (clear TX2 node association)
 *           • Marks register as empty/available
 *
 *       iii) Re-analyze:
 *            • Call AnalyzeRegisterUsage() again
 *            • Updates register state after invalidation
 *            • Ensures subsequent iterations see current state
 *
 *    b) Shift bitmask for next iteration:
 *       • regMask <<= 1
 *
 * 5. Restore Expression Context:
 *    - Restores curExprLoc[0] = locLeft (original Left operand location)
 *    - Restores curExprLoc[1] = locRight (original Right operand location)
 *    - Expression operands remain unchanged by the invalidation
 *
 * Register Invalidation Behavior:
 *
 * - Bit set, registerNeedsSave=true:  Save register to stack, then invalidate
 * - Bit set, registerNeedsSave=false: Just invalidate (no save needed)
 * - Bit clear:             Leave register unchanged
 *
 *
 * Usage Scenarios:
 *
 * Scenario 1: Clear Multiple Registers for Function Call
 * ```
 * // Before calling a function, need to free up B, D, H
 * // Current expression uses H (exprLoc[0] = 3)
 * // B contains a value needed later (registerNeedsSave[0] = true)
 *
 * // Bitmask: 0x0B (bits for B, D, H after rotation)
 * Sub_5E66(0x0B);
 *
 * // Process:
 * // - Save curExprLoc[0]=3, curExprLoc[1]=?
 * // - Clear curExprLoc[] temporarily
 * // - Rotate bitmask to align with registers
 * // - Loop side=0: Bit set, registerNeedsSave[0]=true → PUSH B, invalidate
 * // - Loop side=1: Skip (no register 1)
 * // - Loop side=2: Bit set, registerNeedsSave[2]=false → Just invalidate D
 * // - Loop side=3: Bit set, registerNeedsSave[3]=? → Save if needed, invalidate H
 * // - Restore curExprLoc[0]=3 (H still in expression, but invalidated)
 * ```
 *
 * Scenario 2: Clear Specific Register Set
 * ```
 * // Need to clear D and H, preserve B
 * // Bitmask: 0x0A (bits for D, H after rotation)
 *
 * Sub_5E66(0x0A);
 *
 * // - Saves/invalidates D if needed
 * // - Saves/invalidates H if needed
 * // - Leaves B unchanged
 * ```
 *
 * Why Clear curExprLoc[] During Processing:
 *
 * By temporarily clearing curExprLoc[], the function ensures that:
 * - AnalyzeRegisterUsage() doesn't mark registers as "in expression" (registerInExpression)
 * - registerNeedsSave[] flags are set purely based on reference counts and usage
 * - Register invalidation proceeds without expression interference
 * - Expression operands remain valid after invalidation
 *
 * This is crucial when invalidating registers that might contain
 * the active expression operands - we want to invalidate the register
 * state without losing track of where the operands are.
 *
 * Register State Arrays Modified:
 * - registerState[].isDirect: Direct value flags (cleared for invalidated registers)
 * - registerState[].contents: Register contents (cleared for invalidated registers)
 * - Stack depth (currentStackDepth): Incremented for each saved register
 * - bC140[]: Stack-to-node mapping (if registers saved)
 * - bC0C3[]: Stack metadata (if registers saved)
 *
 * @param regMask - Bitmask specifying which registers to invalidate
 *                (bit encoding is rotated before use)
 *
 * @global curExprLoc[] - Expression operand locations (saved/restored)
 * @global registerNeedsSave[] - Register preservation flags (set by AnalyzeRegisterUsage)
 * @global registerState[].isDirect - Direct value flags (cleared for invalidated registers)
 * @global registerState[].contents - Register contents (cleared for invalidated registers)
 *
 * Side effects:
 * - Temporarily modifies curExprLoc[] (saved/restored)
 * - May push registers to stack via PushRegisterToStack()
 * - Clears register tracking for invalidated registers
 * - Calls AnalyzeRegisterUsage() multiple times
 * - Generates PUSH instructions for saved registers
 * - Updates stack depth and code size
 *
 * Called from:
 * - Code generation when multiple registers need clearing
 * - Before function calls or complex operations
 * - When register set must be reset to known state
 *
 * Calls:
 * - AnalyzeRegisterUsage() - To determine which registers need saving
 * - PushRegisterToStack() - To push registers to stack
 *
 * Related Functions:
 * - SaveOrRedirectRegister() - Handles single register save/redirect
 * - AnalyzeRegisterUsage() - Determines register preservation needs
 * - PushRegisterToStack() - Actually pushes register to stack
 */
void InvalidateRegistersByMask(uint8_t regMask) {
    // Step 1: Save current expression operand locations
    uint8_t locLeft  = curExprLoc[Left];
    uint8_t locRight = curExprLoc[Right];

    // Step 2: Clear expression locations during processing
    // This prevents interference with register usage analysis
    curExprLoc[Left] = curExprLoc[Right] = 0;

    // Step 3: Initial register usage analysis
    AnalyzeRegisterUsage();

    // Step 4: Process each register based on bitmask
    for (uint8_t i = IR_PSW; i <= IR_H; i++, regMask <<= 1) {
        // Check if this register should be invalidated
        if (regMask & 0x8) { // Save register if it needs preservation
            if (registerNeedsSave[i])
                PushRegisterToStack(i); // Push to stack

            // Clear register tracking - mark as empty
            registerState[i].isDirect = false; // Clear direct value flag
            registerState[i].contents = 0;     // Clear TX2 node association

            // Re-analyze register state after invalidation
            AnalyzeRegisterUsage();
        }
    }

    // Step 6: Restore original expression operand locations
    curExprLoc[Left]  = locLeft;
    curExprLoc[Right] = locRight;
}

void GenerateReturnSequence() {
    AdjustStackOnReturn(localVariableSize);
    info = blk[blkId].info;
    if (info && (info->flag & F_INTERRUPT)) {
        for (int i = IR_PSW; i <= IR_H; i++) {
            iCodeArgs[0] = i; /*  pop psw, pop b, pop d, pop h */
            iCodeArgs[1] = LOC_REG;
            EncodeFragData(CF_POP);
        }
        EncodeFragData(CF_EI);
        codeSize += 5;
    }
}

void CreateConstantOrIdNode(uint16_t val, info_t *pInfo, uint8_t exprAttr, uint8_t exprLoc) {
    tx2[tx2qp].right    = val;
    tx2[tx2qp].left     = ToIdx(pInfo);
    tx2[tx2qp].exprAttr = exprAttr;
    tx2[tx2qp].exprLoc  = exprLoc;
    tx2[tx2qp].type     = exprLoc == LOC_REG ? T2_NUMBER : T2_IDENTIFIER;
}

/*
Purpose
Retrieves the value and associated metadata from a TX2 node, determining both the numeric value and
information about how it's stored/accessed. Used during constant folding optimization to extract
operand values.
Parameters
*	Slot - Index of the TX2 node to examine
*	pAcc - Pointer to store the extracted value
*	pAccFlag - Pointer to store flags indicating the value's storage class and attributes
Functionality
The function examines the TX2 node type and extracts different information
based on whether it's an identifier or a number:
For T2_IDENTIFIER or T2_NUMBER nodes:
1.	Extracts the value:
    *pAcc = tx2[slot].right - The value is stored in the right field
2.	Determines the info pointer:
    info = FromIdx(tx2[slot].left) - Retrieves associated symbol information
3.	Sets the flag based on symbol attributes:
Condition	                                Flag Value	            Meaning
No info OR F_MEMBER | F_BASED | F_ABSOLUTE	0	                    Absolute value / simple
constant F_AUTOMATIC	                    0x100	                Automatic (stack) variable
F_EXTERNAL	                                0x400 | info->extId	    External symbol with ID
F_MEMORY	                                0x800	                MEMORY variable
type == PROC_T	                            0x1000 | info->procId	Procedure with ID
F_DATA	                                    0x200	                Initialized data
Otherwise	                                0x2000	                Uninitialized data
For other node types:
*	*pAcc = 0
*	*pAccFlag = slot ? 0x4000 : 0 - Uses slot value to distinguish between left/right operands

Usage Context
Called from:
*	FoldConstantExpr() - During constant folding optimization to get operand values
*	Other optimization passes that need to determine if an expression can be evaluated at compile
    time The flag values encode both the storage class and additional metadata (like external ID or
    procedure ID) in a single uint16_t, allowing the constant folding logic to determine:
*	Whether the value can be folded (flag == 0 for simple constants)
*	How to generate code if folding isn't possible
*	What fixup information is needed for external/procedure references
*/
void GetOperandValue(uint8_t slot, wpointer pAcc, wpointer pAccFlag) {
    if (tx2[slot].type == T2_IDENTIFIER || tx2[slot].type == T2_NUMBER) {
        *pAcc = tx2[slot].right;
        info  = FromIdx(tx2[slot].left);
        if (!info || (info->flag & (F_MEMBER | F_BASED | F_ABSOLUTE)))
            *pAccFlag = 0;
        else if ((info->flag & F_AUTOMATIC))
            *pAccFlag = 0x100;
        else if ((info->flag & F_EXTERNAL))
            *pAccFlag = 0x400 | info->extId;
        else if ((info->flag & F_MEMORY))
            *pAccFlag = 0x800;
        else if (info->type == PROC_T)
            *pAccFlag = 0x1000 | info->procId;
        else if ((info->flag & F_DATA))
            *pAccFlag = 0x200;
        else
            *pAccFlag = 0x2000;
    } else {
        *pAcc     = 0;
        *pAccFlag = slot ? 0x4000 : 0;
    }
}

void DecrementExprRefs() {
    for (uint8_t side = Left; side <= Right; side++) {
        uint8_t j = curExprLoc[side];
        if (j && --tx2[j].cnt == 0) {
            for (uint8_t k = IR_PSW; k <= IR_H; k++) {
                if (registerState[k].contents == j)
                    registerState[k].contents = 0;
            }
            stackNodeContents[tx2[j].extra] = 0;
        }
    }
}

void UpdateExpressionLookup(uint8_t side) {
    iCodeArgsIndex[side] = attrLocLookupTable[exprAttr[side]][exprLoc[side]];
}

/**
 * EncodeOperandInfo - Encode operand information into intermediate code arguments
 *
 * Analyzes a TX2 node and encodes its operand representation into the iCodeArgs[] array.
 * The encoding format depends on the operand type:
 *
 * 1. Stack-based operand (nodeControlMap & 0xC0 == 0):
 *    - Code: 0xA (stack reference)
 *    - Args: stack level, uint8_t offset from current stack pointer
 *
 * 2. Variable with symbol info:
 *    - Code: 0xC (automatic) or 0xB (non-automatic)
 *    - Args: offset from base, symbol index
 *    - For automatic: additional stack-relative offset
 *
 * 3. Immediate constant:
 *    - Code: 8 (byte) or 9 (address)
 *    - Args: immediate value
 *
 * The iCodeArgsIdx  index is advanced as arguments are added, preparing data for
 * subsequent code generation via EncodeFragData().
 *
 * @param regMask - Index of TX2 node to encode
 */

void EncodeOperandInfo(uint8_t idx) {
    if ((nodeControlMap[tx2[idx].type] & 0xc0) == 0) {
        // Stack-based operand
        iCodeArgs[iCodeArgsIdx++] = OP_STK;
        iCodeArgs[iCodeArgsIdx++] = tx2[idx].extra;
        iCodeArgs[iCodeArgsIdx++] = (currentStackDepth - tx2[idx].extra) * 2;
    } else if (tx2[idx].left) {
        // Variable with symbol
        info                      = FromIdx(tx2[idx].left);
        iCodeArgs[iCodeArgsIdx++] = info->flag & F_AUTOMATIC ? OP_AUTO : OP_VAR;
        iCodeArgs[iCodeArgsIdx++] = tx2[idx].right - info->linkVal;
        iCodeArgs[iCodeArgsIdx++] = ToIdx(info);
        if ((info->flag & F_AUTOMATIC))
            iCodeArgs[iCodeArgsIdx++] = tx2[idx].right + currentStackDepth * 2;
    } else {
        // Immediate constant
        iCodeArgs[iCodeArgsIdx++] = tx2[idx].right < 0x100 ? OP_IM8 : OP_IM16;
        iCodeArgs[iCodeArgsIdx++] = tx2[idx].right;
    }
}

void EncodeExpressionOperand(uint8_t side) {
    if (exprLoc[side] <= IR_H)
        iCodeArgs[iCodeArgsIdx++] = exprLoc[side];
    else
        EncodeOperandInfo(curExprLoc[side]);
}

void AdjustRegisterOffset(uint8_t irReg) {
    if (irReg > IR_H)
        return;
    if (registerState[irReg].dataType <= 6) {
        while (registerState[irReg].offset != 0) {
            if (registerState[iCodeArgs[0] = irReg].offset > 0x7f) {
                registerState[irReg].offset++;
                EncodeFragData(CF_INX);
            } else {
                registerState[irReg].offset--;
                EncodeFragData(CF_DCX);
            }
            codeSize++;
        }
    }
}

void PopRegisterFromStack(uint8_t irReg) {
    iCodeArgs[0] = irReg;
    iCodeArgs[1] = LOC_STACK;
    iCodeArgs[2] = currentStackDepth;
    EncodeFragData(CF_POP);
    codeSize++;
    RestoreRegisterFromStack(irReg);
    currentStackDepth--;
}


// ===== TX2 QUEUE MANAGEMENT =====


/**
 * GetTx2Item - Read single TX2 node from input stream
 *
 * Reads a TX2 node type and associated fields from input, then populates
 * tx2[tx2qp] structure with the data.
 *
 * Field encoding based on nodeControlMap[type] & 3:
 *   0: Type only (0 fields)
 *   1: Type + left field (1 word)
 *   2: Type + left + right (2 words)
 *   3: Type + left + right + extra (3 words)
 *
 * @side effects:
 *   - Reads from TX2 input stream
 *   - Populates tx2[tx2qp]
 *   - Sets eofSeen flag if T2_EOF encountered
 *
 * Called from: FillTx2Queue()
 */
void GetTx2Item() {
    tx2[tx2qp].type = Rd2Byte(); // Read node type

    // Read fields based on control map encoding
    switch (3 & nodeControlMap[tx2[tx2qp].type]) {
    case 0:
        if (tx2[tx2qp].type == T2_EOF)
            eofSeen = true;
        break;
    case 1:
        tx2[tx2qp].left = Rd2Word();
        break;
    case 2:
        tx2[tx2qp].left  = Rd2Word();
        tx2[tx2qp].right = Rd2Word();
        break;
    case 3:
        tx2[tx2qp].left  = Rd2Word();
        tx2[tx2qp].right = Rd2Word();
        tx2[tx2qp].extra = Rd2Word();
        break;
    }
}


/**
 * MergeRedundantNodes - Remove redundant TX2 node markers
 *
 * Optimization to reduce TX2 queue size by merging redundant nodes:
 *
 * 1. T2_MODULE nodes that immediately follow private LABELDEF are removed
 *    (module marker not needed for private labels)
 *
 * 2. Consecutive T2_LINEINFO nodes with no intervening statements are merged
 *    (keep only latest line number)
 *
 * @side effects:
 *   - May decrement tx2qp to remove current node
 *   - Modifies tx2 nodes
 *
 * Called from: FillTx2Queue()
 */
void MergeRedundantNodes() {
    // Case 1: Remove redundant MODULE markers
    if (curNodeType == T2_MODULE) {
        if (tx2[tx2qp - 1].type == T2_LABELDEF &&
            !(FromIdx(tx2[tx2qp - 1].left)->flag & (F_MODGOTO | F_PUBLIC)))
            tx2qp--; // Ignore this module marker
    } else
        // Case 2: Merge consecutive empty LINEINFO nodes
        if (curNodeType == T2_LINEINFO && tx2[tx2qp].right == 0 &&
            tx2[tx2qp - 1].type == T2_LINEINFO && tx2[tx2qp - 1].right == 0) {
            tx2[tx2qp - 1].extra = tx2[tx2qp].left; // Update to latest line number
            tx2qp--;                                // Skip this one
        }
}



/**
 * FillTx2Queue - Read and buffer TX2 nodes from input stream
 *
 * Reads TX2 nodes from the input file and buffers them into the tx2[] array.
 * Implements basic block detection and statement size validation.
 *
 * Algorithm:
 *   1. Remove previously processed nodes from buffer
 *   2. Read TX2 items until buffer full or EOF
 *   3. Merge redundant nodes (MODULE after LABELDEF, consecutive LINEINFO)
 *   4. Detect statement boundaries (STMTCNT, LOCALLABEL, EOF)
 *   5. Detect basic block ends (CALL, ENDPROC, etc.)
 *   6. Validate statement doesn't exceed size limit
 *
 * @side effects:
 *   - Reads from input TX2 stream
 *   - Modifies tx2[] buffer (positions 4+)
 *   - Updates tx2qp, tx2qEnd position pointers
 *   - Sets eofSeen flag when EOF encountered
 *   - May emit syntax errors for size violations
 *
 * Called from: main2.c main compilation loop
 */
void FillTx2Queue() {
    tx2qp = 4; // Leave positions 0-3 reserved

    // Step 1: Remove already-processed items by moving remaining down
    if (tx2qEnd > tx2qNxt) {
        uint8_t k = tx2qEnd - tx2qNxt;
        memmove(&tx2[tx2qp], &tx2[tx2qNxt], k * sizeof(tx2_t));
        tx2qp += k;
    }

    // Step 2: Main loop - read until buffer full or EOF
    bool exceeded = false; // Statement size error reported flag
    bool endblk   = false; // Basic block terminator seen flag

    while (tx2qp < 255 && !eofSeen) {
        GetTx2Item();
        curNodeType = tx2[tx2qp].type;
        MergeRedundantNodes();

        // Step 3: Handle first node of statement
        if (tx2qp == 4) {
            if (curNodeType == T2_STMTCNT || curNodeType == T2_LOCALLABEL || curNodeType == T2_EOF)
                tx2qp++;                                   // Skip marker nodes
            else if ((nodeControlMap[curNodeType] & 0x20)) // Meta node
                EmitTopItem();
            else if (!exceeded) {
                exceeded = true;
                Tx2SyntaxError(ERR200); // LIMIT EXCEEDED: STATEMENT SIZE
                EmitTopItem();
            }
        } else
            tx2qp++;

        // Step 4: Detect basic block terminator
        if (nodeControlMap[curNodeType] & 8)
            endblk = true;

        // Step 5: Stop at statement boundary after basic block end
        if (endblk &&
            (curNodeType == T2_STMTCNT || curNodeType == T2_EOF || curNodeType == T2_LOCALLABEL))
            break;
    }

    tx2qEnd = tx2qp; // Mark end of buffer
}


/**
 * SkipStmt - Skip to next or previous statement boundary
 *
 * Advances or retreats through TX2 buffer to find the next/previous
 * statement boundary marker (STMTCNT, EOF, or LOCALLABEL).
 *
 * @param startIdx - Starting TX2 index
 * @param direction - +1 to advance, -1 to retreat
 *
 * Side effects:
 *   - Updates tx2qNxt to point to boundary marker
 *
 * Called from: SetFirstStatementEnd()
 */
static void SkipStmt(uint8_t startIdx, int8_t direction) {
    tx2qNxt = startIdx + direction;

    while (tx2[tx2qNxt].type != T2_STMTCNT && tx2[tx2qNxt].type != T2_EOF &&
           tx2[tx2qNxt].type != T2_LOCALLABEL)
        tx2qNxt += direction;
}

/**
 * SetFirstStatementEnd - Mark the end of first processable statement
 *
 * Locates the boundary between the first statement and remaining statements
 * in the TX2 queue. Complex logic to handle:
 * - Partial statements at queue boundaries
 * - Label definitions at statement start
 * - Basic block boundaries (CALL, JMP, RETURN, etc.)
 *
 * Result stored in tx2qNxt for use by main loop.
 *
 * @side effects:
 *   - Updates tx2qNxt to mark statement end
 *   - Uses SkipStmt() to find boundaries
 *
 * Called from: main2.c main loop after FillTx2Queue()
 */
void SetFirstStatementEnd() {
    SkipStmt(tx2qEnd, -1); // Find start of last statement

    if (tx2qNxt == 4 && !eofSeen)
        tx2qNxt = tx2qEnd; // Assume boundary at end
    else {
        // Scan through statements looking for code-generating items
        for (uint8_t i = tx2qp = 4; tx2qp < tx2qNxt; tx2qp++) {
            nodeControlFlags = nodeControlMap[tx2[tx2qp].type];

            // Check for label definitions
            if ((nodeControlFlags & 0x10)) { // PROCEDURE, LABELDEF, LOCALLABEL, etc.
                if (i < tx2qp) {             // Have code-generating items before label
                    if (tx2[tx2qp].type == T2_LOCALLABEL)
                        tx2qNxt = tx2qp; // Stop at local label
                    else
                        SkipStmt(tx2qp, -1); // Backup from other labels
                    return;
                }
                i++;
            }

            // Check for basic block boundaries
            if ((nodeControlFlags & 8)) { // CALL, RETURN, JMP, etc.
                SkipStmt(tx2qp, 1);       // Skip to next statement
                return;
            }

            // Skip meta-information nodes
            if ((nodeControlFlags & 0x20)) // STMTCNT, LINEINFO, MODULE, errors
                i++;
        }
    }
}
