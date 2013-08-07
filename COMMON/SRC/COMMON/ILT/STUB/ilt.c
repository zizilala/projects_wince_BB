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
//  File:  ilt.c
//
#include <windows.h>
#include <oal.h>

//------------------------------------------------------------------------------
//
// Global:  g_oalILT
//
OAL_ILT_STATE g_oalILT;


//------------------------------------------------------------------------------
//
//  Function:  OALIoCtlHalILTiming
//
//  This function is called to start/stop IL timing. It is called from
//  OEMIoControl for IOCTL_HAL_ILTIMING code.
//
BOOL OALIoCtlHalILTiming( 
    UINT32 code, VOID *pInpBuffer, UINT32 inpSize, VOID *pOutBuffer, 
    UINT32 outSize, UINT32 *pOutSize
) {
    NKSetLastError(ERROR_NOT_SUPPORTED);
    return FALSE;
}

//------------------------------------------------------------------------------
