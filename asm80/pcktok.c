/****************************************************************************
 *  pcktok.c: part of the C port of Intel's ISIS-II asm80             *
 *  The original ISIS-II application is Copyright Intel                     *
 *																			*
 *  Re-engineered to C by Mark Ogden <mark.pm.ogden@btinternet.com> 	    *
 *                                                                          *
 *  It is released for hobbyist use and for academic interest			    *
 *                                                                          *
 ****************************************************************************/

#include "asm80.h"

/* packToken - packs the token pointed by token.start, with length token.size into 4 bytes
 packed version replaces original and token.size set to 4 bytes
*/
static byte Pack1(byte i)
{
	return  i >= token[0].size ? 0 : tokPtr[i] < 0x3f ? tokPtr[i] - 0x2f : tokPtr[i] - 0x34;
}

void PackToken(void) {
	*(wpointer)tokPtr = (Pack1(0) * 40 + Pack1(1)) * 40 + Pack1(2);
	*(wpointer)(tokPtr + 2) = (Pack1(3) * 40 + Pack1(4)) * 40 + Pack1(5);
	token[0].size = 4;
}
