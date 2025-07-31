
#include "plm82.h"
// the builtins are encoded as items, each proceeded by a tag
// if the tag is zero then all done
// if tag is 1-254 then tag number of bytes following are stuffed into memory
// if the tag is 0xff, then the following byte is added
// to the start location and stuffed as a word into memory

// clang-format off

const uint8_t multiply[] = {
//  tag   data
    5,    0x79,             /*        mov   a, c  */
          0x93,             /*        sub   e     */
          0x78,             /*        mov   a, b  */
          0x9A,             /*        sbb   d     */
          0xF2,             /*        jp    L000C */
    0xff, 0x0c,
    20,   0x60,             /*        mov   h, b  */
          0x69,             /*        mov   l, c  */
          0xEB,             /*        xchg        */
          0x44,             /*        mov   b, h  */
          0x4D,             /*        mov   c, l  */
          0x21, 0x00, 0x00, /* L000C: lxi   h, 0  */
          0xEB,             /*        xchg        */
          0x78,             /* L0010: mov   a, b  */
          0xB1,             /*        ora   c     */
          0xC8,             /*        rz          */
          0xEB,             /*        xchg        */
          0x78,             /*        mov   a, b  */
          0x1F,             /*        rar         */
          0x47,             /*        mov   b, a  */
          0x79,             /*        mov   a, c  */
          0x1F,             /*        rar         */
          0x4F,             /*        mov   c, a  */
          0xD2,             /*        jnc   L001E */
    0xff, 0x1e,
    4,    0x19,             /*        dad   d     */
          0xEB,             /* L001E: xchg        */
          0x29,             /*        dad   h     */
          0xC3,             /*        jmp   L1010 */
    0xff, 0x10,
    0
};

const uint8_t divide[] = {
    //  tag   data
    15,   0x7A,             /*        mov  a, d   */
          0x2F,             /*        cma         */
          0x57,             /*        mov  d, a   */
          0x7B,             /*        mov  a, e   */
          0x2F,             /*        cma         */
          0x5F,             /*        mov  e, a   */
          0x13,             /*        inx  d      */
          0x21, 0x00, 0x00, /*        lxi  h, 0   */
          0x3E, 0x11,       /*        mvi  a, 11h */   
          0xE5,             /* L000C: push h      */
          0x19,             /*        dad  d      */
          0xD2,             /*        jnc  L0012  */
    0xff, 0x12,
    18,   0xE3,             /*        xthl        */
          0xE1,             /* L0012: pop  h      */
          0xF5,             /*        push psw    */
          0x79,             /*        mov  a, c   */
          0x17,             /*        ral         */
          0x4F,             /*        mov  c, a   */
          0x78,             /*        mov  a, b   */
          0x17,             /*        ral         */
          0x47,             /*        mov  b, a   */
          0x7D,             /*        mov  a, l   */
          0x17,             /*        ral         */
          0x6F,             /*        mov  l, a   */
          0x7C,             /*        mov  a, h   */
          0x17,             /*        ral         */
          0x67,             /*        mov  h, a   */
          0xF1,             /*        pop  psw    */
          0x3D,             /*        dcr  a      */
          0xC2,             /*        jnz  L000C  */
    0xff, 0x0c,
    8,
          0xB7,             /*        ora  a      */
          0x7C,             /*        mov  a, h   */
          0x1F,             /*        rar         */
          0x57,             /*        mov  d, a   */
          0x7D,             /*        mov  a, l   */
          0x1F,             /*        rar         */
          0x5F,             /*        mov  e, a   */
          0xC9,             /*        ret         */
    0
};

uint16_t biftab[2];

void emitbf(int bf) {

    // modified code to emit a call to the builtin in fuction bf
    // bf - 0->multiply 1->divide
    // emits routine in line if not yet done so.
    // biftab[bf] is zero if code not emitted, else location of routine

    if (!biftab[bf]) {
        emit(JMP, 0, 0);     // jump around the code - back stuffed later
        biftab[bf] = codloc; // update to record location of routine

        for (uint8_t const *codePtr = bf == 0 ? multiply : divide; *codePtr; codePtr++) {
            if (*codePtr == 0xff) { // relocatable address
                int target = biftab[bf] + *++codePtr;
                put(codloc++, LOW(target));
                put(codloc++, HIGH(target));
            } else {
                for (int i = *codePtr; i != 0; i--)
                    put(codloc++, *++codePtr);
            }
        }
        /* backstuff branch around function */
        putword(biftab[bf] - 2, codloc);
    }

    /* emit call on the function */
    emit(CAL, biftab[bf], 0);
    return;
}


/* inst */
// replace original ctran array with pre expanded strings to simplify the logic
// the first byte is the number of additional bytes in ascii
// the original compiler uses SBC vs. SBB and has non standard immediate forms for mov and arith
// if STD is defined these are replaced by standard ones, although currently commas are omitted as
// per the original
#ifdef STD
#define MVI(R) "MVI " #R
#define ADI    "ADI"
#define ACI    "ACI"
#define SUI    "SUI"
#define SBI    "SBI"
#define ANI    "ANI"
#define XRI    "XRI"
#define ORI    "ORI"
#define CPI    "CPI"
#define SBB(R) "SBB" #R
#else
#define MVI(R) "MOV " #R "I"
#define ADI    "ADD I"
#define ACI    "ADC I"
#define SUI    "SUB I"
#define SBI    "SBC I"
#define ANI    "ANA I"
#define XRI    "XRA I"
#define ORI    "ORA I"
#define CPI    "CMP I"
#define SBB(R) "SBC " #R
#endif

// clang-format off
ctran_t const ctran[256] = {
   { 0, "NOP" },    { 2, "LXI B" },  { 0, "STAX B" }, { 0, "INX B" },  { 0, "INR B" },  { 0, "DCR B" },  { 1,  MVI(B) },  { 0, "RLC" },
   { 0, "DB 08H" }, { 0, "DAD B" },  { 0, "LDAX B" }, { 0, "DCX B" },  { 0, "INR C" },  { 0, "DCR C" },  { 1,  MVI(C) },  { 0, "RRC" },
   { 0, "DB 10H" }, { 2, "LXI D" },  { 0, "STAX D" }, { 0, "INX D" },  { 0, "INR D" },  { 0, "DCR D" },  { 1,  MVI(D) },  { 0, "RAL" },
   { 0, "DB 18H" }, { 0, "DAD D" },  { 0, "LDAX D" }, { 0, "DCX D" },  { 0, "INR E" },  { 0, "DCR E" },  { 1,  MVI(E) },  { 0, "RAR" },
   { 0, "RIM" },    { 2, "LXI H" },  { 2, "SHLD" },   { 0, "INX H" },  { 0, "INR H" },  { 0, "DCR H" },  { 1,  MVI(H) },  { 0, "DAA" },
   { 0, "DB 28H" }, { 0, "DAD H" },  { 2, "LHLD" },   { 0, "DCX H" },  { 0, "INR L" },  { 0, "DCR L" },  { 1,  MVI(L) },  { 0, "CMA" },
   { 0, "SIM" },    { 2, "LXI SP" }, { 2, "STA" },    { 0, "INX SP" }, { 0, "INR M" },  { 0, "DCR M" },  { 1,  MVI(M) },  { 0, "STC" },
   { 0, "DB 38H" }, { 0, "DAD SP" }, { 2, "LDA" },    { 0, "DCX SP" }, { 0, "INR A" },  { 0, "DCR A" },  { 1,  MVI(A) },  { 0, "CMC" },
   { 0, "MOV BB" }, { 0, "MOV BC" }, { 0, "MOV BD" }, { 0, "MOV BE" }, { 0, "MOV BH" }, { 0, "MOV BL" }, { 0, "MOV BM" }, { 0, "MOV BA" },
   { 0, "MOV CB" }, { 0, "MOV CC" }, { 0, "MOV CD" }, { 0, "MOV CE" }, { 0, "MOV CH" }, { 0, "MOV CL" }, { 0, "MOV CM" }, { 0, "MOV CA" },
   { 0, "MOV DB" }, { 0, "MOV DC" }, { 0, "MOV DD" }, { 0, "MOV DE" }, { 0, "MOV DH" }, { 0, "MOV DL" }, { 0, "MOV DM" }, { 0, "MOV DA" },
   { 0, "MOV EB" }, { 0, "MOV EC" }, { 0, "MOV ED" }, { 0, "MOV EE" }, { 0, "MOV EH" }, { 0, "MOV EL" }, { 0, "MOV EM" }, { 0, "MOV EA" },
   { 0, "MOV HB" }, { 0, "MOV HC" }, { 0, "MOV HD" }, { 0, "MOV HE" }, { 0, "MOV HH" }, { 0, "MOV HL" }, { 0, "MOV HM" }, { 0, "MOV HA" },
   { 0, "MOV LB" }, { 0, "MOV LC" }, { 0, "MOV LD" }, { 0, "MOV LE" }, { 0, "MOV LH" }, { 0, "MOV LL" }, { 0, "MOV LM" }, { 0, "MOV LA" },
   { 0, "MOV MB" }, { 0, "MOV MC" }, { 0, "MOV MD" }, { 0, "MOV ME" }, { 0, "MOV MH" }, { 0, "MOV ML" }, { 0, "HLT" },    { 0, "MOV MA" },
   { 0, "MOV AB" }, { 0, "MOV AC" }, { 0, "MOV AD" }, { 0, "MOV AE" }, { 0, "MOV AH" }, { 0, "MOV AL" }, { 0, "MOV AM" }, { 0, "MOV AA" },
   { 0, "ADD B" },  { 0, "ADD C" },  { 0, "ADD D" },  { 0, "ADD E" },  { 0, "ADD H" },  { 0, "ADD L" },  { 0, "ADD M" },  { 0, "ADD A" },
   { 0, "ADC B" },  { 0, "ADC C" },  { 0, "ADC D" },  { 0, "ADC E" },  { 0, "ADC H" },  { 0, "ADC L" },  { 0, "ADC M" },  { 0, "ADC A" },
   { 0, "SUB B" },  { 0, "SUB C" },  { 0, "SUB D" },  { 0, "SUB E" },  { 0, "SUB H" },  { 0, "SUB L" },  { 0, "SUB M" },  { 0, "SUB A" },
   { 0,  SBB(B) },  { 0,  SBB(C) },  { 0,  SBB(D) },  { 0,  SBB(E) },  { 0,  SBB(H) },  { 0,  SBB(L) },  { 0,  SBB(M) },  { 0,  SBB(A) },
   { 0, "ANA B" },  { 0, "ANA C" },  { 0, "ANA D" },  { 0, "ANA E" },  { 0, "ANA H" },  { 0, "ANA L" },  { 0, "ANA M" },  { 0, "ANA A" },
   { 0, "XRA B" },  { 0, "XRA C" },  { 0, "XRA D" },  { 0, "XRA E" },  { 0, "XRA H" },  { 0, "XRA L" },  { 0, "XRA M" },  { 0, "XRA A" },
   { 0, "ORA B" },  { 0, "ORA C" },  { 0, "ORA D" },  { 0, "ORA E" },  { 0, "ORA H" },  { 0, "ORA L" },  { 0, "ORA M" },  { 0, "ORA A" },
   { 0, "CMP B" },  { 0, "CMP C" },  { 0, "CMP D" },  { 0, "CMP E" },  { 0, "CMP H" },  { 0, "CMP L" },  { 0, "CMP M" },  { 0, "CMP A" },
   { 0, "RNZ" },    { 0, "POP B" },  { 2, "JNZ" },    { 2, "JMP" },    { 2, "CNZ" },    { 0, "PUSH B" }, { 1,  ADI },     { 0, "RST 0" },
   { 0, "RZ" },     { 0, "RET" },    { 2, "JZ" },     { 0, "DB CBH" }, { 2, "CZ" },     { 2, "CALL" },   { 1,  ACI },     { 0, "RST 1" },
   { 0, "RNC" },    { 0, "POP D" },  { 2, "JNC" },    { 1, "OUT" },    { 2, "CNC" },    { 0, "PUSH D" }, { 1,  SUI },     { 0, "RST 2" },
   { 0, "RC" },     { 0, "DB D9H" }, { 2, "JC" },     { 1, "IN" },     { 2, "CC" },     { 0, "DB DDH" }, { 1,  SBI },     { 0, "RST 3" },
   { 0, "RPO" },    { 0, "POP H" },  { 2, "JPO" },    { 0, "XTHL" },   { 2, "CPO" },    { 0, "PUSH H" }, { 1,  ANI },     { 0, "RST 4" },
   { 0, "RPE" },    { 0, "PCHL" },   { 2, "JPE" },    { 0, "XCHG" },   { 2, "CPE" },    { 0, "DB EDH" }, { 1,  XRI },     { 0, "RST 5" },
   { 0, "RP" },     { 0, "POP A" },  { 2, "JP" },     { 0, "DI" },     { 2, "CP" },     { 0, "PUSH A" }, { 1,  ORI },     { 0, "RST 6" },
   { 0, "RM" },     { 0, "SPHL" },   { 2, "JM" },     { 0, "EI" },     { 2, "CM" },     { 0, "DB FDH" }, { 1,  CPI },     { 0, "RST 7"}
};