/****************************************************************************
 *  plm1a.c: part of the C port of Intel's ISIS-II plm80                    *
 *  The original ISIS-II application is Copyright Intel                     *
 *                                                                          *
 *  Re-engineered to C by Mark Ogden <mark.pm.ogden@btinternet.com>         *
 *                                                                          *
 *  It is released for academic interest and personal use only              *
 ****************************************************************************/
#include "plm.h"

/* lex item to icode */
// clang-format off
static byte tx1ToIMap[] = {
    0,           0,      0,       0,      0,            0,        0,             0,          //LINEINFO, SYNTAXERROR, TOKENERROR, LIST, NOLIST, CODE, NOCODE, EJECT
    0,           0,      0,       0,      0,            0,        0,             0,          //INCLUDE, STMTCNT, LABELDEF, LOCALLABEL, JMP, JMPFALSE, PROCEDURE, SCOPE
    0,           0,      0,       0,      0,            0,        0,             0,          //END, DO, DOLOOP, WHILE, CASE, CASELABEL, IF, STATEMENT
    0,           0,      0,       0,      0,            0,        0,             0,          //CALL, RETURN, GO, GOTO, SEMICOLON, ENABLE, DISABLE, HALT
    0,           0,      0,       0,      I_IDENTIFIER, I_NUMBER, 0,             I_PLUSSIGN, //EOF, AT, INITIAL, DATA, IDENTIFIER, NUMBER, STRING, PLUSSIGN
    I_MINUSSIGN, I_PLUS, I_MINUS, I_STAR, I_SLASH,      I_MOD,    I_COLONEQUALS, I_AND,
    I_OR,        I_XOR,  I_NOT,   I_LT,   I_LE,         I_EQ,     I_NE,          I_GE,
    I_GT,        0,      0,       0,      0,            0,        0,             0,          //GT, COMMA, LPAREN, RPAREN, PERIOD, TO, BY, INVALID
    0,           0,      0,       0                                                          //MODULE, XREFUSE, XREFDEF, EXTERNAL
};

byte tx1ToTx2Map[] = {
    T2_LINEINFO,   T2_SYNTAXERROR, T2_TOKENERROR,  T2_LIST,
    T2_NOLIST,     T2_CODE,        T2_NOCODE,      T2_EJECT,
    T2_INCLUDE,    T2_STMTCNT,     T2_LABELDEF,    T2_LOCALLABEL,
    T2_JMP,        T2_JMPFALSE,    T2_PROCEDURE,   0/*T1_SCOPE*/,
    0/*END*/,      0/*DO*/,        0/*DOLOOP*/,    0/*WHILE*/,
    T2_CASE,       T2_CASELABEL,   0/*IF*/,        0/*STATEMENT*/,
    0/*CALL*/,     T2_RETURN,      T2_GOTO/*GO*/,  T2_GOTO,
    T2_SEMICOLON,  T2_ENABLE,      T2_DISABLE,     T2_HALT,
    0/*EOF*/,      0/*AT*/,        0/*INITIAL*/,   0/*DATA*/,
    T2_IDENTIFIER, 0/*NUMBER*/,    0/*T1_STRING*/,  T2_PLUSSIGN,
    T2_MINUSSIGN,  T2_PLUS,        T2_MINUS,       T2_STAR,
    T2_SLASH,      T2_MOD,         T2_COLONEQUALS, T2_AND,
    T2_OR,         T2_XOR,         T2_NOT,         T2_LT,
    T2_LE,         T2_EQ,          T2_NE,          T2_GE,
    T2_GT,         0/*COMMA*/,     0/*LPAREN*/,    0/*RPAREN*/,
    0/*PERIOD*/,   0/*TO*/,        0/*BY*/,        0/*INVALID*/,
    T2_MODULE,     0/*XREFUSE*/,   0/*XREFDEF*/,   0/*EXTERNAL*/
};


/* 0x80	- Expression item */
/* 0x40 - binary operator */
/* 0x20 - pass through */
/* 0x10 - procedure, at, data, initial or external */

byte tx1AttrMap[] = {
    0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, //LINEINFO, SYNTAXERROR, TOKENERROR, LIST, NOLIST, CODE, NOCODE, EJECT
    0x20, 0,    0,    0,    0,    0,    0x10, 0,    //INCLUDE, STMTCNT, LABELDEF, LOCALLABEL, JMP, JMPFALSE, PROCEDURE, SCOPE
    0,    0,    0,    0,    0,    0,    0,    0,    //END, DO, DOLOOP, WHILE, CASE, CASELABEL, IF, STATEMENT
    0,    0,    0,    0,    0,    0,    0,    0,    //CALL, RETURN, GO, GOTO, SEMICOLON, ENABLE, DISABLE, HALT
    0,    0x10, 0x10, 0x10, 0x80, 0x80, 0x80, 0xC0, //EOF, AT, INITIAL, DATA, IDENTIFIER, NUMBER, STRING, PLUSSIGN
    0xC0, 0xC0, 0xC0, 0xC0, 0xC0, 0xC0, 0x80, 0xC0, //MINUSSIGN, PLUS, MINUS, STAR, SLASH, MOD, COLONEQUALS, AND
    0xC0, 0xC0, 0x80, 0xC0, 0xC0, 0xC0, 0xC0, 0xC0, //OR, XOR, NOT, LT, LE, EQ, NE, GE
    0xC0, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0,    //GT, COMMA, LPAREN, RPAREN, PERIOD, TO, BY, INVALID
    0x20, 0, 0, 0x10                                //MODULE, XREFUSE, XREFDEF, EXTERNAL
};


byte tx1Lengths[] = {
    /* 0    1    2    3    4    5    6    7    8    9    A    B    C    D    E    F */
       6,   2,   4,   0,   0,   0,   0,   0,   2,   2,   2,   2,   2,   2,   2,   2,
       0,   0,   0,   0,   0,   2,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
       0,   2,   2,   2,   2,   2,   255, 0,   0,   0,   0,   0,   0,   0,   0,   0,
       0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
       0,   2,   2,   2
};

byte iToTx2Map[] = {
    0,         T2_IDENTIFIER, T2_NUMBER,    T2_PLUSSIGN,   T2_MINUSSIGN, T2_PLUS,
    T2_MINUS,  T2_STAR,       T2_SLASH,     T2_MOD,        T2_AND,       T2_OR,
    T2_XOR,    T2_NOT,        T2_LT,        T2_LE,         T2_EQ,        T2_NE,
    T2_GE,     T2_GT,         T2_ADDRESSOF, T2_UNARYMINUS, T2_STACKPTR,  T2_INPUT,
    T2_OUTPUT, T2_CALL,       T2_CALLVAR,   T2_BYTEINDEX,  T2_WORDINDEX, T2_COLONEQUALS,
    T2_MEMBER, T2_BASED,      T2_CARRY,     T2_DEC,        T2_DOUBLE,    T2_HIGH,
    T2_LAST,   T2_LENGTH,     T2_LOW,       T2_MOVE,       T2_PARITY,    T2_ROL,
    T2_ROR,    T2_SCL,        T2_SCR,       T2_SHL,        T2_SHR,       T2_SIGN,
    T2_SIZE,   T2_TIME,       T2_ZERO,
};

byte precedence[] = {
    /*I_BOTTOM, I_IDENTIFIER, I_NUMBER,  I_PLUSSIGN,  I_MINUSSIGN, I_PLUS,        I_MINUS,    I_STAR*/
    10,         0,            0,         60,          60,          60,            60,         70,
    /*I_SLASH,  I_MOD,        I_AND,     I_OR,        I_XOR,       I_NOT,         I_LT,       I_LE*/
    70,         70,           30,        20,          20,          40,            50,         50,
    /*I_EQ,     I_NE,         I_GE,      I_GT,        _ADDRESSOF,  I_UNARYMINUS,  I_STACKPTR, I_INPUT*/
    50,         50,           50,        50,          0,           80,            0,          0,
    /*I_OUTPUT, I_CALL,       I_CALLVAR, I_BYTEINDEX, I_WORDINDEX, I_COLONEQUALS, I_MEMBER,   I_BASED*/
    0,          0,            0,         0,           0,           0,             0,          0,
    /*I_CARRY,  I_DEC,        I_DOUBLE,  I_HIGH,      I_LAST,      I_LENGTH,      I_LOW,      I_MOVE*/
    0,          0,            0,         0,           0,           0,             0,          0,
    /*I_PARITY, I_ROL,        I_ROR,     I_SCL,       I_SCR,       I_SHL,         I_SHR,      I_SIGN*/
    0,          0,            0,         0,           0,           0,             0,          0,
    /*I_SIZE,   I_TIME,       I_ZERO*/
    0,          0,
};
// clang-format on
byte builtinsMap[] = { I_CARRY, I_DEC,    I_DOUBLE, I_HIGH,     I_INPUT, I_LAST, I_LENGTH, I_LOW,
                       I_MOVE,  I_OUTPUT, I_PARITY, I_ROL,      I_ROR,   I_SCL,  I_SCR,    I_SHL,
                       I_SHR,   I_SIGN,   I_SIZE,   I_STACKPTR, I_TIME,  I_ZERO };

word parseSP;
word parseStack[100];
word operatorSP;
word operatorStack[50];
word operandSP;
operand_t operandStack[EXPRSTACKSIZE];
word stSP;
operand_t tree[300];

void fatal_ov1(byte err) {
    hasErrors      = true;
    fatalCode = err;
    longjmp(exception, -1);
}

void OptWrXrf() {
    if (!XREF)
        return;
    info->xref = newXref(info->xref, curStmtNum); /* add reference */
}

void Wr2Buf(void *buf, word len) {
    vfWbuf(&utf2, buf, len);
}

void Wr2Byte(uint8_t v) {
    Wr2Buf(&v, sizeof(v));
} /* WrByte() */

void Wr2Word(uint16_t v) {
    Wr2Buf(&v, sizeof(v));
}

void Rd2Buf(void *buf, uint16_t len) {
    vfRbuf(&utf2, buf, len);
}

uint8_t Rd2Byte() {
    uint8_t v;
    Rd2Buf(&v, sizeof(v));
    return v;
}

uint16_t Rd2Word() {
    uint16_t v;
    Rd2Buf(&v, sizeof(v));
    return v;
}

/*
    Write the T2 item unless we have errors, in which case just write
    line info and errors
*/
word Wr2Item(uint8_t type, void *buf, uint16_t len) {
    if (!hasErrors || (T2_STMTCNT <= linfo.type && linfo.type <= T2_ERROR)) {
        Wr2Byte(type); // split to avoid potential word alignment
        if (len)
            Wr2Buf(buf, len);
    }
    return ++stmtT2Cnt;
}

/*
    write the item, but if T2_STMTCNT flush any pending line info

*/
static word Wr2SyncedItem(uint8_t type, void *buf, uint16_t len) {
    if (tx2LinfoPending && type == T2_STMTCNT) {
        Wr2Item(linfo.type, &linfo.lineCnt, sizeof(struct _linfo));
        tx2LinfoPending = false;
        if (tx1Item.dataw[0] == 0) // no  new lines all done
            return stmtT2Cnt;
    }
    return Wr2Item(type, buf, len);
}

word Wr2Simple(byte type) {
    return Wr2SyncedItem(type, NULL, 0);
}

word Wr2Leaf(byte type, word val) {
    return Wr2SyncedItem(type, &val, sizeof(val));
}

word Wr2Node(byte type, word lhsRel, word rhsRel) {
    word args[2] = { lhsRel, rhsRel };
    return Wr2SyncedItem(type, &args, sizeof(args));
}

word Wr2ArgNode(byte type, word relBC, word relDE, word extRel) {
    uint16_t args[3] = { relBC, relDE, extRel };
    return Wr2SyncedItem(type, &args, sizeof(args));
}

word CvtToRel(word nodeIdx) {
    return (stmtT2Cnt + 1 - nodeIdx);
}

/* note normally string would require special processing but strings will either be
   in the initialisation stream or converted to numbers for 1 or 2 char
*/
void MapLToT2(void) {
    byte arglen  = tx1Lengths[tx1Item.type];    // length of T1 args
    tx1Item.type = tx1ToTx2Map[tx1Item.type];   // update to T2 code
    Wr2SyncedItem(tx1Item.type, tx1Item.dataw, arglen);
}

void Wr2Error(word errCode) {
    hasErrors = true;
    Wr2SyncedItem(T2_SYNTAXERROR, &errCode, sizeof(errCode));
}

void Wr2TokError(word errCode) {
    if (info) {
        word args[2] = { errCode, ToIdx(info) };
        hasErrors    = true;
        Wr2SyncedItem(T2_TOKENERROR, args, sizeof(args));
    } else
        Wr2Error(errCode);
}

void UngetTx1Item(void) {
    regetTx1Item = true;
}

void RdTx1Item(void) {
    word tx1ItemLen;
    tx1Item.type = Rd1Byte();
    tx1ItemLen   = tx1Lengths[tx1Item.type]; // get the size supplementary information
    if (tx1ItemLen) {
        if (tx1ItemLen != 255)                        /* i.e. not a string */
            vfRbuf(&utf1, tx1Item.dataw, tx1ItemLen); // read the words of data
        else {
            tx1Item.len = Rd1Word();                 // read the length of string
            vfRbuf(&utf1, tx1Item.str, tx1Item.len); // read the string itself
        }
    }
    tx1ICode = tx1ToIMap[tx1Item.type];
    tx1Attr  = tx1AttrMap[tx1Item.type];
}
