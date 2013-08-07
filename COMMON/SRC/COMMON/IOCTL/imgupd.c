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
//  File:  imgupd.c
//
//  This file implements the IOCTL_HAL_UPDATE_MODE handler.
//
#include <windows.h>
#include <oal.h>

//------------------------------------------------------------------------------
//
//  Function:  OALIoCtlHalUpdateMode
//
//  Implements the IOCTL_HAL_UPDATE_MODE handler. 
//  This function gets/sets the RAM-based update mode flag.
//
BOOL OALIoCtlHalUpdateMode ( 
    UINT32 code, VOID *pInpBuffer, UINT32 inpSize, VOID *pOutBuffer, 
    UINT32 outSize, UINT32 *pOutSize
) {
    BOOL rc = FALSE;
    BOOL *pfUpdateMode = NULL;
    
    if (pOutSize) {
        *pOutSize = 0;
    }

    // verify the input
    if (!pInpBuffer || inpSize != sizeof(BOOL))
    {
        NKSetLastError(ERROR_INVALID_PARAMETER);
        OALMSG(OAL_WARN, (L"ERROR: OALIoCtlHalUpdateMode: Invalid input buffer\r\n"));
        goto cleanUp;
    }

    // Get Update Mode flag from BSP Args
    pfUpdateMode = (BOOL *) OALArgsQuery(OAL_ARGS_QUERY_UPDATEMODE);

    // if there is no update mode flag, fail
    if (pfUpdateMode == NULL)
    {
        NKSetLastError(ERROR_NOT_SUPPORTED);
        OALMSG(OAL_WARN, (L"ERROR: OALIoCtlHalUpdateMode: Device doesn't support Update Mode\r\n"));
        goto cleanUp;
    }
    
    // update the flag
    *pfUpdateMode = *((BOOL *) pInpBuffer);

    // Done
    rc = TRUE;

cleanUp:    
    return rc;
}

//------------------------------------------------------------------------------
