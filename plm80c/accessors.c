/****************************************************************************
 *  accessors.c: part of the C port of Intel's ISIS-II plm80c             *
 *  The original ISIS-II application is Copyright Intel                     *
 *																			*
 *  Re-engineered to C by Mark Ogden <mark.pm.ogden@btinternet.com> 	    *
 *                                                                          *
 *  It is released for hobbyist use and for academic interest			    *
 *                                                                          *
 ****************************************************************************/

#include "plm.h"






byte GetDataType() {
    if (info->type == PROC_T)
        return info->dtype;
    else
        return info->dataType;
}

void SetDataType(byte dtype) {
    if (info->type == PROC_T)
        info->dtype = dtype;
    else
        info->dataType = dtype;
}



byte GetParamCnt() {
    if (info->type == PROC_T)
        return info->pcnt;
    else
        return info->paramCnt;
}

void SetParamCnt(byte cnt) {
    if (info->type == PROC_T)
        info->pcnt = cnt;
    else
        info->paramCnt = cnt;
}




