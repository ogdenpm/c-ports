/****************************************************************************
 *  appinfo.h: part of the C port of Intel's ISIS-II plm80             *
 *  The original ISIS-II application is Copyright Intel                     *
 *																			*
 *  Re-engineered to C by Mark Ogden <mark.pm.ogden@btinternet.com> 	    *
 *                                                                          *
 *  It is released for hobbyist use and for academic interest			    *
 *                                                                          *
 ****************************************************************************/

// static version information
// overrides are in _appinfo.h in the source directory
#ifndef _APPINFO_H_
#define _APPINFO_H_
#include <_appinfo.h>
#ifndef APP_PRODUCT
#define APP_PRODUCT APP_NAME
#endif
#ifndef APP_OWNER
#define APP_OWNER		"Intel"
#endif
#ifndef APP_MODS
#define APP_MODS		"port Mark Ogden"
#endif
#ifndef APP_DESCRIPTION
#define APP_DESCRIPTION APP_PRODUCT " reverse engineered to modern C"
#endif
#define APP_EMAIL       "support@mark-ogden.uk"
#endif
