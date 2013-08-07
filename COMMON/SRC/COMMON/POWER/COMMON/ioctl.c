//
// Copyright (c) Microsoft Corporation.  All rights reserved.
//
//
// Use of this sample source code is subject to the terms of the Microsoft
// license agreement under which you licensed this sample source code. If
// you did not accept the terms of the license agreement, you are not
// authorized to use this sample source code. For the terms of the license,
// please see the license agreement between you and Microsoft or, if applicable,
// see the LICENSE.RTF on your install media or the root of your tools installation.
// THE SAMPLE SOURCE CODE IS PROVIDED "AS IS", WITH NO WARRANTIES.
//
//------------------------------------------------------------------------------
//
//  File:  ioctl.c
//
#include <windows.h>
#include <nkintr.h>
#include <oal.h>

//------------------------------------------------------------------------------

volatile UINT32 g_oalWakeSource = SYSWAKE_UNKNOWN;
static UINT32 g_oalWakeMask[(SYSINTR_MAXIMUM + 31)/32];

//------------------------------------------------------------------------------
//
//  Function:  OALPowerWakeSource
//
//  The function return TRUE when SYSINTR is wake source.
//
BOOL OALPowerWakeSource(UINT32 sysIntr)
{
    return (g_oalWakeMask[sysIntr >> 5] & (1 << (sysIntr & 0x1F))) != 0;
}

//------------------------------------------------------------------------------
//
//  Function:  OALIoCtlHalEnableWake
//
BOOL OALIoCtlHalEnableWake(
    UINT32 code, VOID* pInpBuffer, UINT32 inpSize, VOID* pOutBuffer, 
    UINT32 outSize, UINT32 *pOutSize
) {
    BOOL rc = FALSE;
    UINT32 sysIntr;

    OALMSG(OAL_POWER&&OAL_FUNC, (
        L"+OALIoCtlHalEnableWake(sysIntr = %d)\r\n", *(UINT32*)pInpBuffer
    ));
    
    if (pOutSize) {
        *pOutSize = 0;
    }

    if (pInpBuffer == NULL || inpSize < sizeof(UINT32)) {
        NKSetLastError(ERROR_INVALID_PARAMETER);
        goto cleanUp;
    }

    sysIntr = *(UINT32*)pInpBuffer;
    if (sysIntr < SYSINTR_DEVICES || sysIntr >= SYSINTR_MAXIMUM) {
        OALMSG(OAL_ERROR, (L"ERROR: OALIoCtlHalEnableWake: ",
            L"Invalid SYSINTR value %d\r\n", sysIntr
        ));
        NKSetLastError(ERROR_INVALID_PARAMETER);
        goto cleanUp;
    }                    

    // Set mask
    g_oalWakeMask[sysIntr >> 5] |= 1 << (sysIntr & 0x1F);
    rc = TRUE;
    
cleanUp:
    OALMSG(OAL_POWER&&OAL_FUNC, (L"+OALIoCtlHalEnableWake(rc = %d)\r\n", rc));
    return rc;
}

//------------------------------------------------------------------------------
//
//  Function:  OALIoCtlHalDisableWake
//
BOOL OALIoCtlHalDisableWake(
    UINT32 code, VOID* pInpBuffer, UINT32 inpSize, VOID* pOutBuffer, 
    UINT32 outSize, UINT32 *pOutSize
) {
    BOOL rc = FALSE;
    UINT32 sysIntr;

    OALMSG(OAL_POWER&&OAL_FUNC, (
        L"+OALIoCtlHalDisableWake(sysIntr = %d)\r\n", *(UINT32*)pInpBuffer
    ));
    
    if (pOutSize) {
        *pOutSize = 0;
    }

    if (pInpBuffer == NULL || inpSize < sizeof(UINT32)) {
        NKSetLastError(ERROR_INVALID_PARAMETER);
        goto cleanUp;
    }

    sysIntr = *(UINT32*)pInpBuffer;
    if (sysIntr < SYSINTR_DEVICES || sysIntr >= SYSINTR_MAXIMUM) {
        OALMSG(OAL_ERROR, (L"ERROR: OALIoCtlHalDisableWake: ",
            L"Invalid SYSINTR value %d\r\n", sysIntr
        ));
        NKSetLastError(ERROR_INVALID_PARAMETER);
        goto cleanUp;
    }                    

    // Set mask
    g_oalWakeMask[sysIntr >> 5] &= ~(1 << (sysIntr & 0x1F));
    rc = TRUE;
    
cleanUp:
    OALMSG(OAL_POWER&&OAL_FUNC, (L"+OALIoCtlHalDisableWake(rc = %d)\r\n", rc));
    return rc;
}

//------------------------------------------------------------------------------
//
//  Function:  OALIoCtlGetWakeSource
//
BOOL OALIoCtlHalGetWakeSource(
    UINT32 code, VOID* pInpBuffer, UINT32 inpSize, VOID* pOutBuffer, 
    UINT32 outSize, UINT32 *pOutSize
) {
    BOOL rc = FALSE;
    
    OALMSG(OAL_POWER&&OAL_FUNC, (L"+OALIoCtlHalGetWakeSource\r\n"));

    if (pOutSize) {
        *pOutSize = sizeof(UINT32);
    }

    if (pOutBuffer == NULL && outSize > 0) {
        NKSetLastError(ERROR_INVALID_PARAMETER);
        goto cleanUp;
    }

    if (pOutBuffer == NULL || outSize < sizeof(UINT32)) {
        NKSetLastError(ERROR_INSUFFICIENT_BUFFER);
        goto cleanUp;
    }

    *(UINT32*)pOutBuffer = g_oalWakeSource;

    rc = TRUE;
    
cleanUp:
    OALMSG(OAL_POWER&&OAL_FUNC, (
        L"+OALIoCtlHalGetWakeSource(rc = %d, sysIntr = %d)\r\n", 
        rc, *(UINT32*)pOutBuffer
    ));
    return rc;
}

//------------------------------------------------------------------------------
