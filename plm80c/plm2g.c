/****************************************************************************
 *  plm2g.c: Procedure Entry Generation & Parameter Handling
 *
 *  PART OF: Intel ISIS-II PL/M-80 Compiler (C Port)
 *  Original: Copyright Intel
 *  Re-engineered to C by Mark Ogden <mark.pm.ogden@btinternet.com>
 *
 *  Released for academic interest and personal use only
 *
 *  PURPOSE:
 *  --------
 *  This module implements procedure entry code generation, handling:
 *
 *  1. Parameter passing strategy selection (reentrant vs. non-reentrant)
 *  2. Parameter location calculation and tracking
 *  3. Local variable allocation on stack
 *  4. Stack frame setup for procedures
 *  5. Special statement processing (labels, jumps, input operations)
 *  6. Procedure prologue code generation
 *
 *  The module bridges the gap between high-level procedure definitions
 *  (from symbol table) and low-level 8080 assembly code generation,
 *  implementing the calling convention for the PL/M language compiler.
 *
 *  CALLING CONVENTIONS:
 *  --------------------
 *
 *  version 3.0 onwards of PL/M-80 implements a standard parameter passing convention
 *  but has two ways of handling these parameters dependent on whether the procedure
 *  if non-reentrant (default) or reentrant:
 * 
 * Parameter passing:
 *  if there is a single parameter this is passed in BC/C
 *  if there are two parameters, the first is passed in BC/C and the second in DE/E
 *  for three parameters, all but the last two are passed on the stack in order
 *  stack saves always occupy 2 bytes, although the top byte is unused for byte parameters
 *  the last two are passed in BC/C and DE/E
 *
 *
 *  Parameter handling:
 *  1. NON-REENTRANT (Default):
 *     - Parameters stored in fixed memory locations
 *     - No stack frame created per call
 *     - Faster (no frame setup overhead)
 *     - Cannot be called recursively
 *     - HL is set to the last location of the parameter storage as
 *       parameters are stored in decreasing memory location so final parameter
 *       layout reflects the parameter calling order
 *     - DE/E is saved if there are more than 2 parameters
 *     - BC/C is saved if there is at least 1 parameter
 *     - while parameters remain they are popped of the stack and saved
 *     - Note the parameter type determines the number of bytes stored
 *
 *
 *  2. REENTRANT:
 *     - Stack frame created for each call
 *     - Parameters on procedure stack
 *     - Local variables below parameters
 *     - Can be called recursively
 *     - Parameters pushed before call
 *
 *     Example: procedure foo(a, b) reentrant;
 *       load BC = a, DE = b; Call foo;
         Entry: PUSH D; PUSH B; allocate locals on stack
 *       Access: Parameters via stack pointer
 *       Return: Deallocate frame, return
 *
 *  COMPILATION FLOW:
 *  -----------------
 *  Start2() (main2.c)
 *    ├─ FillTx2Queue() - Build expression tree
 *    ├─ DeRelStmt() - Convert relative references
 *    ├─ OptimiseStmtNodes() - Optimize tree
 *    ├─ GenerateStatementCode() (plm2h.c)
 *    │   ├─ T2_PROCEDURE node encountered
 *    │   └─ c_procedure() from (plm2f.c)
 *    │       └─ GenerateProcedureEntry() ← THIS MODULE
 *    │           ├─ Generate interrupt save code
 *    │           ├─ Allocate local variables
 *    │           ├─ Set up parameter locations
 *    │           └─ Generate parameter handling code
 *    └─ Next statement
 *
 *  STACK LAYOUT - REENTRANT PROCEDURE:
 *  ------------------------------------
 *
 *  Higher Addresses:
 *    ┌─────────────────────────────┐
 *    │      Caller's Variables     │
 *    ├─────────────────────────────┤
 *    │     Return Address (SP)     │  ← SP on entry
 *    ├─────────────────────────────┤
 *    │   Parameter N (if any)      │
 *    │   ...                       │
 *    │   Parameter 1               │
 *    ├─────────────────────────────┤
 *    │   Local Variables           │
 *    │   ...                       │
 *    └─────────────────────────────┘  ← New SP after frame setup
 *  Lower Addresses:
 *
 *  STACK LAYOUT - NON-REENTRANT PROCEDURE:
 *  ----------------------------------------
 *
 *  No stack frame. Parameters in fixed memory:
 *
 *    Fixed Memory:
 *      Param 1: foo_params
 *      Param 2: foo_params+2
 *      Param N: foo_params+...
 *
 *  ARCHITECTURE:
 *  ---------------
 *
 *  The module is organized around the concept of parameter handling
 *  modes based on three key factors:
 *
 *  1. PARAMETER COUNT:
 *     - 0: No parameters (simple stack setup if reentrant)
 *     - 1: Single parameter (special handling, register B)
 *     - 2: Two parameters (registers B and D)
 *     - 3: Three parameters (B, D, and stack pop)
 *     - 4+: All on stack, handled in loop
 *
 *  2. PARAMETER TYPE:
 *     - BYTE_T: 8-bit value (half of register pair)
 *     - ADDRESS_T: 16-bit pointer/address (full register pair)
 *     - Affects how much of register is used
 *     - Affects stack space needed
 *
 *  3. PROCEDURE ATTRIBUTES:
 *     - F_REENTRANT: Stack-based parameter passing
 *     - F_INTERRUPT: Must save all registers on entry
 *     - Combination affects entry sequence
 *
 *  KEY CONCEPTS:
 *  ---------------
 *
 *  1. PARAMETER STACK FRAME (Reentrant):
 *     - Parameters pushed in order
 *     - Accessed as positive offset from stack base
 *     - localVariableSize = base + extra for 3+ params
 *     - Space = (paramCount > 2) ? (paramCount-2)*2 : 0
 *
 *  2. EFFECTIVE ADDRESS CALCULATION:
 *     - firstParamStackOffset = totalSize - linkVal - 1
 *     - Adjusted for ADDRESS_T (-1 more byte)
 *     - Loaded into HL via CF_SA2HL fragment
 *     - Used as base for parameter copying
 *
 *  3. PARAMETER COPYING STRATEGIES:
 *     - 1 param: Load into B, copy once
 *     - 2 params: Load into B and D, copy in sequence
 *     - 3 params: Pop D and B from stack, then copy
 *     - 4+ params: Loop with CF_MOVRPM (load D from memory via HL)
 *
 *  4. LOCAL VARIABLE ALLOCATION:
 *     - Small (≤10 bytes): Use PUSH H loop + DCX SP for odd byte
 *     - Large (>10 bytes): Use SPHL after loading HL with offset
 *     - Reduces code size for large frames
 *
 *  DATA FLOW - PARAMETER HANDLING:
 *  --------------------------------
 *
 *  Non-Reentrant Example (2 parameters: a BYTE, b ADDRESS):
 *
 *    Symbol Table: a (BYTE, offset 0), b (ADDRESS, offset 1)
 *
 *    Generated Code:
 *      LXI H, top_param_area        (3 bytes)
 *      [Parameter processing]
 *
 *    Parameter Processing:
 *      1. FindParamInfo(2) → get b (ADDRESS)
 *         MOV D,M               (1 byte: load high byte of b)
 *         MOV L,M               (1 byte: load low byte of b)
 *      2. FindParamInfo(1) → get a (BYTE)
 *         DCX H                 (1 byte: move to next location)
 *         MOV H,M               (1 byte: load a into H)
 *      3. PUSH D               (1 byte: save parameters)
 *
 *  Reentrant Example (2 parameters: a BYTE, b ADDRESS):
 *
 *    Calling Convention:

 *      CALL procedure
 *
 *    Entry Code:
 * 
 *      [Optional: save registers if interrupt]
 *      [Optional: push D if 2+ params, push B if 1+ params]
 *      [Allocate locals on stack]
 *      [Load parameters and work with them]
 *
 *  ALGORITHMS:
 *  -----------
 *
 *  FindParamInfo(paramIndex):
 *    - Start with first parameter info from symbol table
 *    - Advance through parameter list by count
 *    - Updates global 'info' to point to parameter info
 *
 *  SaveParameterToMemory(paramNo, irReg):
 *    - If ADDRESS_T: Use CF_MOVMRPR (word store)
 *    - Else BYTE_T: Use CF_MOVMLR (byte store)
 *    - If not last parameter: Generate CF_DCXH (decrement HL)
 *    - Moves HL pointer to next parameter location
 *
 *  MoveParametersToMemory() [Non-Reentrant]:
 *    - Determine initial register: B if 1 param, D if 2+
 *    - For each parameter (in reverse order):
 *      • Locate parameter info
 *      • Handle special parameter sequence:
 *        - Param 1: Use initial register
 *        - Param 2: Switch to B
 *        - Param 3+: Pop from stack first
 *      • Call SaveParameterToMemory
 *    - After processing 3+ params: PUSH D to restore it
 *
 *  PushParametersToStack() [Reentrant]:
 *    - If 3+ parameters: Load effective address into HL first
 *    - For each parameter:
 *      • Locate parameter info
 *      • Special handling by position:
 *        - Param 1-2: Direct register
 *        - Param 3+: Load from memory via HL
 *      • For BYTE_T: Use CF_MOVHRLR to shift register content
 *      • Push parameter to stack via PushParameterValue
 *    - Decrement count for next parameter
 *
 *  GenerateProcedureEntry():
 *    - Get parameter count from symbol table
 *    - If F_INTERRUPT:
 *      • Push PSW, B, D, H (save state)
 *    - If F_REENTRANT:
 *      • Calculate local variable size
 *      • Allocate locals on stack
 *      • If parameters: Calculate first param offset, push to stack
 *      • Update localVariableSize for 3+ parameters
 *    - Else (non-reentrant):
 *      • If parameters: Load HL with parameter address, copy to memory
 *      • Set localVariableSize = 0
 *      • Calculate stack usage for 3+ parameters
 *
 *  ProcessSpecialNodes():
 *    - Routes special statement types to handlers:
 *      • T2_LABELDEF: Mark code position
 *      • T2_LOCALLABEL: Track local label address
 *      • T2_CASELABEL: Track case label
 *      • T2_JMP/JNC/JNZ/GOTO: Stack cleanup on jump
 *      • T2_INPUT: Mark input operation
 *      • T2_STMTCNT: Statement counter marker
 *      • Etc.
 *
 *  SPECIAL CASES:
 *  ---------------
 *
 *  1. INTERRUPT PROCEDURES:
 *     - Must save all registers (PSW, B, D, H)
 *     - Pushed in reverse order (H, D, B, PSW)
 *     - Restored by exit code
 *     - Allows safe operation in interrupt context
 *
 *  2. BYTE PARAMETERS:
 *     - Only half of register pair used
 *     - Other half must be handled separately
 *     - CF_MOVHRLR shifts byte to proper position
 *     - CF_INXSP adjusts for odd-sized parameter
 *
 *  3. ADDRESS PARAMETERS:
 *     - Full 16-bit register pair used
 *     - CF_MOVMRPR (word write) used for storage
 *     - Two bytes read/written per parameter
 *
 *  4. PARAMETER ORDERING:
 *     - PL/M passes parameters left-to-right
 *     - But processing order is optimized based on register constraints
 *     - Non-reentrant: Parameters stored in memory, accessed via HL
 *     - Reentrant: Parameters on stack, accessed via stack frame
 *
 *  5. LOCAL VARIABLE ALLOCATION:
 *     - Small frames: PUSH H loop (faster, smaller code)
 *       • DCX SP for odd byte (to align)
 *       • PUSH H repeated for 2-byte chunks
 *     - Large frames: SPHL method (more compact)
 *       • Calculate negative offset (-size)
 *       • Load into HL via SA2HL
 *       • Set SP = HL via SPHL
 *
 *  REGISTER USAGE:
 *  ----------------
 *
 *  During procedure entry:
 *    - HL: Parameter address (non-reentrant) or stack base (reentrant)
 *    - B: Parameter 1 or temporary storage
 *    - D: Parameter 2 or temporary storage
 *    - PSW: Saved if interrupt procedure
 *
 *  After entry:
 *    - HL: May hold parameter value or available
 *    - B: Holds parameter 1 (if 1+ params)
 *    - D: Holds parameter 2 (if 2+ params)
 *    - Stack: Contains parameters and locals
 *
 *  CODE SIZE ACCOUNTING:
 *  ----------------------
 *
 *  Each code generation step updates codeSize:
 *    - CF_PUSH: +1 byte
 *    - CF_DCXH: +1 byte
 *    - CF_MOVLRM: +1 byte
 *    - CF_MOVHRM: +1 byte
 *    - CF_SA2HL: +4 bytes
 *    - CF_LXI: +3 bytes
 *    - CF_SPHL: +1 byte
 *    - Etc.
 *
 *  Total entry code typically 5-20 bytes depending on:
 *    - Parameter count
 *    - Parameter types
 *    - Interrupt procedure flag
 *    - Local variable size
 *
 *  INTEGRATION POINTS:
 *  --------------------
 *  - Called from: plm2f.c::c_procedure() after EnterBlk()
 *  - Calls: EncodeFragData() - Emit code fragments
 *  - Calls: FindParamInfo() - Get parameter metadata
 *  - Uses: info (global) - Symbol table entry
 *  - Uses: blk[] - Block/procedure structures
 *  - Uses: FromIdx(), ToIdx() - Symbol indexing
 *  - Updates: codeSize - Track code generation
 *  - Updates: localVariableSize - Stack frame size
 *  - Updates: stackUsage - Maximum stack usage
 *
 *  GLOBAL STATE MODIFIED:
 *  ----------------------
 *  - codeSize: Incremented by code generated
 *  - localVariableSize: Set to allocated local size
 *  - stackUsage: Updated for 3+ parameters
 *  - label tracking (localLabels[], procIds[])
 *  - registerState[] (for special input operations)
 *  - curExprLoc[], exprLoc[] (for special operations)
 *
 *  DEBUGGING NOTES:
 *  ----------------
 *  - Add tracing in GenerateProcedureEntry() to see entry sequence
 *  - Check codeSize updates to verify code generation counts
 *  - Monitor HL changes during parameter processing
 *  - Verify stack layout for reentrant procedures
 *  - Check parameter offset calculations
 *
 *  PERFORMANCE CONSIDERATIONS:
 *  ---------------------------
 *  1. Non-reentrant procedures: Faster (no frame setup)
 *     - Trade-off: Can't be recursive
 *     - Suitable for most procedures
 *
 *  2. Reentrant procedures: Slower but flexible
 *     - Required for recursive/ISR procedures
 *     - Stack-based parameter passing
 *
 *  3. Parameter allocation strategies optimized for:
 *     - Minimal code size
 *     - Efficient register usage
 *     - Fast parameter access
 *
 *  FUTURE IMPROVEMENTS:
 *  ---------------------
 *  1. Parameter splitting strategies:
 *     - More efficient handling of byte parameters
 *     - Combine multiple byte params into registers
 *
 *  2. Tail call optimization:
 *     - Reuse frame for tail calls
 *     - Reduce stack growth in recursive calls
 *
 *  3. Parameter inline expansion:
 *     - Avoid parameter copying for leaf procedures
 *     - Direct parameter use from registers
 *
 *  VERSION HISTORY:
 *  ----------------
 *  - Original: Intel ISIS-II PLM-80 compiler
 *  - C port: Mark Ogden
 *  - Current: Fully renamed and documented
 *
 ****************************************************************************/
#include "int.h"
#include "plm.h"

static uint8_t curParamCnt;
static uint8_t remainingParamCount;
static uint16_t firstParamStackOffset;

/**
 * FindParamInfo - Locate parameter symbol table entry
 *
 * Advances through parameter list to find the specified parameter's
 * symbol table entry. The 'info' global is updated to point to the
 * requested parameter.
 *
 * @param paramIndex - Parameter number (1-based) to locate
 *
 * Side effects:
 *   - Updates global 'info' to point to requested parameter
 *
 * Called from: Parameter handling functions
 */
void FindParamInfo(uint8_t paramIndex) {
    info = blk[activeGrpCnt].info;
    while (paramIndex-- != 0)
        AdvNxtInfo();
}


/**
 * MoveParametersToMemory - Non-reentrant parameter initialization
 *
 * Moves procedure parameters from registers/stack to fixed memory locations.
 * Used for non-reentrant procedures where parameters are accessed from fixed memory
 * rather than on the stack frame.
 *
 * Algorithm:
 *   1. For each parameter (in reverse order):
 *      a. Locate parameter info
 *      b. Handle special parameter sequences:
 *         - Param 1-2: Already in register
 *         - Param 3: Pop D (return address)
 *         - Param 3+: Pop B for each parameter from stack
 *      c. Store to memory via HL (which points to storage location)
 *         handles case where IR_D is pushed first (2+ parameters and 1st param)
 *      d. Decrement HL to next parameter location (except last)
 *   3. After 3+ params: PUSH D return address to restore it
 *
 * Side effects:
 *   - Generates code for parameter copying
 *   - Updates codeSize
 *   - Updates HL via DCX H
 *
 * Called from: GenerateProcedureEntry() for non-reentrant procedures
 */
void MoveParametersToMemory() {
    uint8_t paramIndex = curParamCnt;
   
    for (uint8_t paramNo = 1; paramNo <= curParamCnt; paramNo++, paramIndex--) {
        FindParamInfo(paramIndex); // popping stack in reverse parameter order
        if (paramNo > 2) {
            if (paramNo == 3) {
                iCodeArgs[0] = IR_D; /*  pop d return address */
                iCodeArgs[1] = LOC_REG;
                EncodeFragData(CF_POP);
                codeSize++;
            }
            iCodeArgs[0] = IR_B; /*  pop b parameter */
            iCodeArgs[1] = LOC_REG;
            EncodeFragData(CF_POP);
            codeSize++;
        }
        iCodeArgs[0] = paramNo == 1 && curParamCnt > 1 ? IR_D : IR_B;
        if (info->type == ADDRESS_T) {
            EncodeFragData(CF_MOVMRPR);
            codeSize += 3;
        } else {
            EncodeFragData(CF_MOVMLR);
            codeSize++;
        }
        if (paramNo != curParamCnt) {
            EncodeFragData(CF_DCXH);
            codeSize++;
        }
    }
    if (curParamCnt > 2) {
        iCodeArgs[0] = IR_D; /*  push d return address */
        iCodeArgs[1] = LOC_REG;
        EncodeFragData(CF_PUSH);
        codeSize++;
    }
}

/**
 * LoadEffectiveAddressToHL - Calculate and load stack-relative address
 *
 * Generates code to load the effective address (stack pointer + offset)
 * into HL register using CF_SA2HL fragment.
 *
 * @param stackOffset - Signed offset from stack pointer (typically negative)
 *
 * Side effects:
 *   - Generates CF_SA2HL fragment (+4 bytes)
 *   - Updates codeSize
 *   - HL contains effective address after execution
 *
 * Called from: GenerateProcedureEntry(), PushParametersToStack()
 */
void LoadEffectiveAddressToHL(uint16_t stackOffset) {
    iCodeArgs[0] = IR_SR;       // get effective address into HL
    iCodeArgs[1] = stackOffset; // -ve offset from stack
    EncodeFragData(CF_SA2HL);
    codeSize += 4;
}

/**
 * AllocateLocalVariables - Reserve stack space for local variables
 *
 * Generates code to allocate space for local variables on the stack.
 * Uses two strategies based on allocation size:
 *
 * Small allocation (≤10 bytes):
 *   - Use PUSH H loop for efficiency
 *   - Each PUSH H allocates 2 bytes
 *   - DCX SP for odd-sized allocation
 *   - Faster execution, smaller code overhead
 *
 * Large allocation (>10 bytes):
 *   - Load effective address into HL
 *   - Use SPHL to set new stack pointer
 *   - More compact code, comparable performance
 *
 * @param localSize - Size of local variables in bytes to allocate
 *
 * Side effects:
 *   - Generates PUSH and/or SPHL instructions
 *   - Updates codeSize
 *   - SP adjusted to reserve space
 *
 * Called from: GenerateProcedureEntry() for reentrant procedures
 */
void AllocateLocalVariables(uint16_t localSize) {
    if (localSize <= 10) {
        if (localSize & 1) {
            EncodeFragData(CF_DCXSP);
            codeSize++;
        }
        while (localSize > 1) {
            iCodeArgs[0] = IR_H; /*  push h */
            iCodeArgs[1] = LOC_REG;
            EncodeFragData(CF_PUSH);
            codeSize++;
            localSize -= 2;
        }
    } else {
        LoadEffectiveAddressToHL(-localSize);
        EncodeFragData(CF_SPHL);
        codeSize++;
    }
}

/**
 * IncrementHL - Generate INX H instruction
 *
 * Emits code to increment HL register by 1, typically used to advance
 * through parameter or local variable storage.
 *
 * Side effects:
 *   - Generates CF_INX fragment (+1 byte)
 *   - Updates codeSize
 *   - HL incremented by 1
 *
 * Called from: CopyParameterFromHL()
 */
void IncrementHL() {
    iCodeArgs[0] = IR_H;
    EncodeFragData(CF_INX);
    codeSize++;
}

/**
 * EmitRegisterB - Generate code fragment using register B
 *
 * Generic helper to emit a code fragment that operates on register B.
 *
 * @param fragmentCode - Code fragment ID to emit
 *
 * Side effects:
 *   - Generates code fragment (+1 byte typically)
 *   - Updates codeSize
 *
 * Called from: CopyParameterFromHL()
 */
void EmitRegisterB(uint8_t fragmentCode) {
    iCodeArgs[0] = IR_B;
    EncodeFragData(fragmentCode);
    codeSize++;
}

/**
 * EmitRegisterD - Generate code fragment using register D
 *
 * Generic helper to emit a code fragment that operates on register D.
 *
 * @param fragmentCode - Code fragment ID to emit
 *
 * Side effects:
 *   - Generates code fragment (+1 byte typically)
 *   - Updates codeSize
 *
 * Called from: CopyParameterFromHL(), PushParametersToStack()
 */
void EmitRegisterD(uint8_t fragmentCode) {
    iCodeArgs[0] = IR_D;
    EncodeFragData(fragmentCode);
    codeSize++;
}

/**
 * CopyParameterFromHL - Load parameter from memory via HL pointer
 *
 * Generates code to load a parameter from memory (pointed to by HL) into
 * registers. Handles both BYTE_T and ADDRESS_T types with appropriate
 * load sequences.
 *
 * Algorithm:
 *   1. Increment HL to next parameter location
 *   2. For ADDRESS_T: Load 16-bit value across two bytes
 *      - First load low byte via MOV B,M
 *      - Increment HL
 *      - Load high byte via MOV H,M
 *   3. For BYTE_T: Load 8-bit value
 *      - Load via MOV H,M
 *      - Increment HL
 *   4. Special case for last parameter (remainingParamCount==1):
 *      - Also load into D register via MOV D,M
 *
 * Side effects:
 *   - Generates multiple code fragments
 *   - Updates codeSize
 *   - Loads parameters into B/D/H registers
 *
 * Called from: PushParametersToStack() for params 3+
 */
void CopyParameterFromHL() {
    IncrementHL();
    if (info->type == ADDRESS_T) {
        EmitRegisterB(CF_MOVLRM);
        if (remainingParamCount == 1)
            EmitRegisterD(CF_MOVMLR);

        IncrementHL();
        EmitRegisterB(CF_MOVHRM);
    } else {
        EmitRegisterB(CF_MOVHRM);
        if (remainingParamCount == 1)
            EmitRegisterD(CF_MOVMLR);
        IncrementHL();
    }
    if (remainingParamCount == 1)
        EmitRegisterD(CF_MOVMHR);
}

/**
 * LoadDEFromMemory - Generate MOV D,E,M instruction sequence
 *
 * Emits code to load D register from memory location pointed to by HL.
 * This is a specialized fragment that efficiently loads a 16-bit value.
 *
 * Side effects:
 *   - Generates CF_MOVRPM fragment (+2 bytes)
 *   - Updates codeSize
 *   - Loads into D register
 *
 * Called from: PushParametersToStack() for param 3
 */
void LoadDEFromMemory() {
    EmitRegisterD(CF_MOVRPM);
    codeSize += 2;
}

/**
 * PushParameterValue - Push parameter value to stack
 *
 * Generates code to push a parameter (in irReg) onto the procedure's
 * stack frame. For BYTE_T parameters, also generates INX SP to adjust
 * for the odd byte.
 *
 * @param irReg - Register containing parameter value to push
 *
 * Side effects:
 *   - Generates PUSH instruction (+1 byte)
 *   - For BYTE_T: Also generates INX SP (+1 byte)
 *   - Updates codeSize
 *
 * Called from: PushParametersToStack()
 */
static void PushParameterValue(uint8_t irReg) {
    iCodeArgs[0] = irReg;
    iCodeArgs[1] = LOC_REG;
    EncodeFragData(CF_PUSH);
    codeSize++;
    if (info->type == BYTE_T) {
        EncodeFragData(CF_INXSP);
        codeSize++;
    }
}

/**
 * PushParametersToStack - Reentrant parameter initialization
 *
 * Copies procedure parameters from registers to the procedure's stack frame.
 * Used for reentrant procedures where parameters must be accessible via the
 * stack frame rather than fixed memory locations.
 *
 * Algorithm:
 *   1. If 3+ parameters: Load effective address into HL
 *      - HL points to first parameter storage on stack
 *   2. Initialize currentReg (B if 1 param, D if 2+)
 *   3. For each parameter:
 *      a. Locate parameter info
 *      b. Param 1-2: Already in register
 *         Param 3: Load from memory via HL
 *         Param 4+: Load from memory via HL
 *      c. For BYTE_T: Use CF_MOVHRLR to shift byte properly
 *      d. Push to stack via PushParameterValue
 *   4. Switch to B for subsequent parameters
 *
 * Side effects:
 *   - Generates code for parameter loading
 *   - Updates codeSize
 *   - Stack contains parameters after execution
 *
 * Called from: GenerateProcedureEntry() for reentrant procedures
 */
void PushParametersToStack() {
    if ((remainingParamCount = curParamCnt) > 2)
        LoadEffectiveAddressToHL(firstParamStackOffset);

    for (uint8_t paramNo = 1; paramNo <= curParamCnt; paramNo++, remainingParamCount--) {
        FindParamInfo(remainingParamCount);
        uint8_t irReg = paramNo == 1 && curParamCnt > 1 ? IR_D : IR_B;

        if (paramNo > 3)
            CopyParameterFromHL();
        else if (paramNo == 3) {
            LoadDEFromMemory();
            CopyParameterFromHL();
        } else if (info->type == BYTE_T) {
            iCodeArgs[0] = irReg;
            EncodeFragData(CF_MOVHRLR);
            codeSize++;
        }
        PushParameterValue(irReg);
    }
}

/**
 * GenerateProcedureEntry - Main procedure entry code generator
 *
 * Generates the complete procedure entry sequence including:
 * - Register saves for interrupt procedures
 * - Stack frame setup for reentrant procedures
 * - Parameter handling (memory copy or stack setup)
 * - Local variable allocation
 *
 * Algorithm based on procedure attributes:
 *
 * 1. Save registers if F_INTERRUPT:
 *    - Push PSW, B, D, H in that order (for restore sequence)
 *
 * 2. Setup reentrant frame (if F_REENTRANT):
 *    a. Calculate local variable size
 *    b. Allocate locals on stack
 *    c. If parameters:
 *       - Calculate first param offset
 *       - Push parameters to stack frame
 *       - Update localVariableSize for 3+ params
 *    d. Set stackUsage = 0 (reentrant tracks frame separately)
 *
 * 3. Setup non-reentrant (else):
 *    a. If parameters:
 *       - Initialize HL with parameter storage address
 *       - Copy parameters to memory
 *    b. Set localVariableSize = 0
 *    c. Calculate stack usage for 3+ parameters
 *
 * Side effects:
 *   - Generates entry code sequences
 *   - Updates codeSize counter
 *   - Updates localVariableSize
 *   - Updates stackUsage
 *
 * Called from: plm2f.c::c_procedure()
 */
void GenerateProcedureEntry() {
    curParamCnt = info->paramCnt;
    if ((info->flag & F_INTERRUPT)) {
        /*  push h, push d, push b, push psw */
        for (int8_t irReg = IR_H; irReg >= IR_PSW; irReg--) {
            iCodeArgs[0] = irReg;
            iCodeArgs[1] = LOC_REG; 
            EncodeFragData(CF_PUSH);
            codeSize++;
        }
    }
    if ((info->flag & F_REENTRANT)) {
        localVariableSize = info->totalSize;
        if (curParamCnt > 0) {
            FindParamInfo(curParamCnt);
            firstParamStackOffset = localVariableSize - info->linkVal - 1;
            if (info->type == ADDRESS_T)
                firstParamStackOffset--;
            AllocateLocalVariables(firstParamStackOffset);
            PushParametersToStack();
        } else
            AllocateLocalVariables(localVariableSize);

        if (curParamCnt > 2)
            localVariableSize += (curParamCnt - 2) * 2;

        stackUsage = 0;
    } else {
        if (curParamCnt > 0) {
            FindParamInfo(curParamCnt); /*  locate info for last param */
            iCodeArgs[0] = IR_H;
            iCodeArgs[1] = LOC_VAR;
            iCodeArgs[2] = info->type == ADDRESS_T ? 1 : 0; // offset
            iCodeArgs[3] = ToIdx(info);                     /*  info for last param */
            EncodeFragData(CF_LXI);
            MoveParametersToMemory();
            codeSize += 3;
        }
        localVariableSize = 0;
        stackUsage        = curParamCnt > 2 ? (curParamCnt - 2) * 2 : 0;
    }
}

/**
 * ProcessSpecialNodes - Handle special statement node types
 *
 * Routes various special statement nodes to appropriate handlers.
 * Manages label tracking, jump processing, input operations, and
 * statement count markers.
 *
 * Node Types Handled:
 *   - T2_LABELDEF: Mark label definition position
 *   - T2_LOCALLABEL: Track local label address
 *   - T2_CASELABEL: Track case statement label
 *   - T2_JMP/JNC/JNZ/GOTO: Handle jumps
 *   - T2_INPUT: Handle input operation
 *   - T2_STMTCNT: Statement counter marker
 *   - (Others): Process via EmitTopItem
 *
 * Side effects:
 *   - Updates label tracking arrays
 *   - Updates register state (for input)
 *   - Generates code fragments
 *   - Updates codeSize
 *
 * Called from: GenerateStatementCode() (plm2h.c)
 */
void ProcessSpecialNodes() {
    uint8_t type;

    if (curNodeType == T2_LABELDEF) {
        returnGenerated = false;
        info            = FromIdx(tx2[tx2qp].left);
        info->linkVal   = codeSize;
    } else if (curNodeType == T2_LOCALLABEL) {
        returnGenerated              = false;
        localLabels[tx2[tx2qp].left] = codeSize;
        procIds[tx2[tx2qp].left]     = curExtProcId;
    } else if (curNodeType == T2_CASELABEL) {
        localLabels[tx2[tx2qp].left] = codeSize;
        procIds[tx2[tx2qp].left]     = curExtProcId;
        newCase(tx2[tx2qp].left);
    } else if (curNodeType == T2_JMP || curNodeType == T2_JNC || curNodeType == T2_JNZ ||
               curNodeType == T2_GOTO) {
        type = tx2[tx2qp - 1].type;
        if (type == T2_RETURN || type == T2_RETURNBYTE || type == T2_RETURNWORD || type == T2_GOTO)
            return;
        AdjustStackOnReturn(0);
    } else if (curNodeType == T2_INPUT || (T2_SIGN <= curNodeType && curNodeType <= T2_CARRY)) {
        curExprLoc[0] = 0;
        curExprLoc[1] = 0;
        exprLoc[0]    = 8;
        exprLoc[1]    = 8;
        AnalyzeRegisterUsage();
        SaveOrRedirectRegister(0);
        registerState[0].dataType = 0;
        registerState[0].contents = tx2qp;
        registerState[0].isDirect = false;
        registerState[0].offset   = 0;
        tx2[tx2qp].exprAttr       = BYTE_A;
        tx2[tx2qp].exprLoc        = LOC_SPECIAL;
    } else if (curNodeType == T2_STMTCNT) {
        bool found = false;
        for (int j = tx2qp + 1; j < 255 && tx2[j].type != T2_STMTCNT && tx2[j].type != T2_EOF;
             j++) {
            if ((nodeControlMap[tx2[j].type] & 0x20) == 0 || tx2[j].type == T2_MODULE) {
                found = true;
                break;
            }
        }
        if (!found) {
            curNodeType     = CF_134;
            tx2[tx2qp].type = CF_134;
        }
    }
    EmitTopItem();
    codeSize += (fragmentCodeLength[curNodeType] & 0x1f);
}