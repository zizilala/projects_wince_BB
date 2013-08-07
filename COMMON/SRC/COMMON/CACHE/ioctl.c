//
// Copyright (c) Microsoft Corporation.  All rights reserved.
//
//
// Use of this source code is subject to the terms of the Microsoft end-user
// license agreement (EULA) under which you licensed this SOFTWARE PRODUCT.
// If you did not accept the terms of the EULA, you are not authorized to use
// this source code. For a copy of the EULA, please see the LICENSE.RTF on your
// install media.
//
//------------------------------------------------------------------------------
//
//  File: ioctl.c
//
//  This file implements OALIoCtlHalGetCacheInfo function called from
//  OEMIoControl handler for IOCTL_HAL_GETCACHEINFO code.
//
#include <windows.h>
#include <pkfuncs.h>
#include <oal.h>


//------------------------------------------------------------------------------
//
//  Function:  OALIoctlHalGetCacheInfo
//
//  Implements the IOCTL_HAL_GET_CACHE_INFO. This function fills in a 
//  CacheInfo structure.
//
BOOL OALIoCtlHalGetCacheInfo( 
    UINT32 code, VOID *pInpBuffer, UINT32 inpSize, VOID *pOutBuffer, 
    UINT32 outSize, UINT32 *pOutSize
) {
    BOOL rc = FALSE;
    CacheInfo *pInfo = (CacheInfo*)pOutBuffer;   

    OALMSG(OAL_FUNC, (L"+OALIoctlHalGetCacheInfo(...)\r\n"));

    if (pOutSize) {
        *pOutSize = sizeof(CacheInfo);
    }

    if (pOutBuffer == NULL && outSize > 0) {
        NKSetLastError(ERROR_INVALID_PARAMETER);
        OALMSG(OAL_WARN, (
            L"WARN: OALIoctlHalGetCacheInfo: Invalid output buffer passed\r\n"
        ));
        goto cleanUp;
    }      

    if (pOutBuffer == NULL || outSize < sizeof(CacheInfo)) {
        NKSetLastError(ERROR_INSUFFICIENT_BUFFER);
        OALMSG(OAL_WARN, (
            L"WARN: OALIoctlHalGetCacheInfo: Buffer too small\r\n"
        ));
        goto cleanUp;
    }      

    memset(pInfo, 0, sizeof(CacheInfo));
    
    pInfo->dwL1Flags = g_oalCacheInfo.L1Flags;
    pInfo->dwL1ICacheLineSize = g_oalCacheInfo.L1ILineSize;
    pInfo->dwL1ICacheNumWays = g_oalCacheInfo.L1INumWays;
    pInfo->dwL1ICacheSize = g_oalCacheInfo.L1ISize;
    pInfo->dwL1DCacheLineSize = g_oalCacheInfo.L1DLineSize;
    pInfo->dwL1DCacheNumWays = g_oalCacheInfo.L1DNumWays;
    pInfo->dwL1DCacheSize = g_oalCacheInfo.L1DSize;

    pInfo->dwL2Flags = g_oalCacheInfo.L2Flags;
    pInfo->dwL2ICacheLineSize = g_oalCacheInfo.L2ILineSize;
    pInfo->dwL2ICacheNumWays = g_oalCacheInfo.L2INumWays;
    pInfo->dwL2ICacheSize = g_oalCacheInfo.L2ISize;
    pInfo->dwL2DCacheLineSize = g_oalCacheInfo.L2DLineSize;
    pInfo->dwL2DCacheNumWays = g_oalCacheInfo.L2DNumWays;
    pInfo->dwL2DCacheSize = g_oalCacheInfo.L2DSize;
    
    rc = TRUE;
   
cleanUp:
    OALMSG(OAL_FUNC, (L"-OALIoctlHalGetCacheInfo(rc = %d)\r\n", rc));
    return rc;      
}

//------------------------------------------------------------------------------
