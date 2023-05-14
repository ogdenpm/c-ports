/****************************************************************************
 *  asm80.h: part of the C port of Intel's ISIS-II asm80             *
 *  The original ISIS-II application is Copyright Intel                     *
 *																			*
 *  Re-engineered to C by Mark Ogden <mark.pm.ogden@btinternet.com> 	    *
 *                                                                          *
 *  It is released for hobbyist use and for academic interest			    *
 *                                                                          *
 ****************************************************************************/

#pragma once
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <memory.h>
#include <stdbool.h>

#ifdef _MSC_VER
#define NORETURN(func)      __declspec(noreturn) void func
#define unlink	_unlink
#elif __GNUC__
#define NORETURN(func)     __attribute__((noreturn)) void func 
#else
#define NORETURN(func)     void func
#endif

#include "plm80types.h"
#include "literals.h"
#include "data.h"
#include "procs.h"
#include "error.h"


