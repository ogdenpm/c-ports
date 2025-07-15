#include "_appinfo.h"
#include "_version.h"
#include "verInfo.h"

// common defaults if not specified
#ifndef APP_PRODUCT
#define APP_PRODUCT APP_NAME
#endif
#ifndef APP_OWNER
#define APP_OWNER "Mark Ogden"
#endif
#ifdef CPORT
#ifndef APP_MODS
#define APP_MODS "port Mark Ogden"
#endif
#ifndef APP_DESCRIPTION
#define APP_DESCRIPTION APP_PRODUCT " re-engineered to modern C"
#endif
#endif
#ifndef APP_EMAIL
#define APP_EMAIL "support@mark-ogden.uk"
#endif



appInfo_t appInfo = { APP_NAME,
                      APP_PRODUCT,
                      APP_OWNER,
#ifdef APP_MODS
                      .mods = APP_MODS,
#endif
#ifdef APP_DESCRIPTION
                      .description = APP_DESCRIPTION,
#endif
#ifdef APP_CONTRIBUTOR
                      .contributors = APP_CONTRIBUTOR,
#endif
                      .email   = APP_EMAIL,
                      .version = GIT_VERSION,
                      .build   = __DATE__ " " __TIME__ };
