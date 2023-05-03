/****************************************************************************
 *  lib4.c: part of the C port of Intel's ISIS-II lib             *
 *  The original ISIS-II application is Copyright Intel                     *
 *																			*
 *  Re-engineered to C by Mark Ogden <mark.pm.ogden@btinternet.com> 	    *
 *                                                                          *
 *  It is released for hobbyist use and for academic interest			    *
 *                                                                          *
 ****************************************************************************/

#include "lib.h"

static byte hexdigits[] = "0123456789ABCDEF";

void Binasc(word num, byte base, byte padch, pointer chaP, byte width)
{
	byte i;

	for (i = 1; i <= width; i++) {
		chaP[width - i] = hexdigits[num % base];
		num /= base;
	}
	i = 0;
	while (chaP[i] == '0' && i < width - 1) {
		chaP[i] = padch;
		i = i + 1;
	}
} /* Binasc */


