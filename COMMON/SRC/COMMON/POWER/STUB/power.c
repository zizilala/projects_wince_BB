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
//  File:  power.c
//

#include <windows.h>
#include <oal.h>

//------------------------------------------------------------------------------
//
//  Function:  OEMPowerOff
//
//  Called when the system is to transition to it's lowest power mode (off)
//
VOID OEMPowerOff()
{
}

//------------------------------------------------------------------------------
//
//  Function:  OALIoCtlHalPresuspend
//
BOOL OALIoCtlHalPresuspend(
    UINT32 code, VOID* pInpBuffer, UINT32 inpSize, VOID* pOutBuffer, 
    UINT32 outSize, UINT32 *pOutSize
) {

    NKSetLastError( ERROR_NOT_SUPPORTED );
    return FALSE;
}

//------------------------------------------------------------------------------
//
//  Function:  OALIoCtlHalEnableWake
//
BOOL OALIoCtlHalEnableWake(
    UINT32 code, VOID* pInpBuffer, UINT32 inpSize, VOID* pOutBuffer, 
    UINT32 outSize, UINT32 *pOutSize
) {
    NKSetLastError( ERROR_NOT_SUPPORTED );
    return FALSE;
}

//------------------------------------------------------------------------------
//
//  Function:  OALIoCtlHalDisableWake
//
BOOL OALIoCtlHalDisableWake(
    UINT32 code, VOID* pInpBuffer, UINT32 inpSize, VOID* pOutBuffer, 
    UINT32 outSize, UINT32 *pOutSize
) {
    NKSetLastError( ERROR_NOT_SUPPORTED );
    return FALSE;
}

//------------------------------------------------------------------------------
//
//  Function:  OALIoCtlHalDisableWake
//
BOOL OALIoCtlHalGetWakeSource(
    UINT32 code, VOID* pInpBuffer, UINT32 inpSize, VOID* pOutBuffer, 
    UINT32 outSize, UINT32 *pOutSize
) {
    NKSetLastError( ERROR_NOT_SUPPORTED );
    return FALSE;
}

//------------------------------------------------------------------------------
