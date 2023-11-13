/****************************************************************************
 *  trace.hpp: part of the C port of Intel's ISIS-II plm80             *
 *  The original ISIS-II application is Copyright Intel                     *
 *																			*
 *  Re-engineered to C++ by Mark Ogden <mark.pm.ogden@btinternet.com> 	    *
 *                                                                          *
 *  It is released for hobbyist use and for academic interest			    *
 *                                                                          *
 ****************************************************************************/

// $Id: trace.hpp,v 1.1 2003/10/04 21:08:48 Mark Ogden Exp $
#include "plm.hpp"
extern const char *tx2Names[];
void traceTx1(void *buf, word len);
extern Byte tx1Lengths[];
void traceRdTx1(Byte tx1type, Byte *buf, word len);
void traceTx2(word cnt, Byte *buf, Byte len);

void dumpInfo(info_pt p);
void dumpSym(symbol_pt p);
void dumpTx2Q(int n);
void dumpTx2QDetail(int n);
void dumpMem(char *file, word start, word end);