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
//  File:  ioctl.c
//
//  File implements OEMIoControl function.
//
#include <windows.h>
#include <oal.h>

//------------------------------------------------------------------------------
//
//  Global:  g_ioctlState;
//
//  This state variable contains critical section used to serialize IOCTL
//  calls.
//
static struct {
    BOOL postInit;
    CRITICAL_SECTION cs;
} g_ioctlState = { FALSE };


//------------------------------------------------------------------------------
//
//  Function:  OEMIoControl
//
//  The function is called by kernel a device driver or application calls 
//  KernelIoControl. The system is fully preemtible when this function is 
//  called. The kernel does no processing of this API. It is provided to 
//  allow an OEM device driver to communicate with kernel mode code.
//
BOOL OEMIoControl(
    DWORD code, VOID *pInBuffer, DWORD inSize, VOID *pOutBuffer, DWORD outSize,
    DWORD *pOutSize
) {
    BOOL rc = FALSE;
    UINT32 i;

    OALMSG(OAL_IOCTL&&OAL_FUNC, (
        L"+OEMIoControl(0x%x, 0x%x, %d, 0x%x, %d, 0x%x)\r\n", 
        code, pInBuffer, inSize, pOutBuffer, outSize, pOutSize
    ));

    //Initialize g_ioctlState.cs when IOCTL_HAL_POSTINIT is called. By this time, 
    //the kernel is up and ready to handle the critical section initialization.
    if (!g_ioctlState.postInit && code == IOCTL_HAL_POSTINIT) {
        // Initialize critical section
        InitializeCriticalSection(&g_ioctlState.cs);
        g_ioctlState.postInit = TRUE;
    } 

    // Search the IOCTL table for the requested code.
    for (i = 0; g_oalIoCtlTable[i].pfnHandler != NULL; i++) {
        if (g_oalIoCtlTable[i].code == code) break;
    }

    // Indicate unsupported code
    if (g_oalIoCtlTable[i].pfnHandler == NULL) {
        NKSetLastError(ERROR_NOT_SUPPORTED);
        OALMSG(OAL_IOCTL, (
            L"OEMIoControl: Unsupported Code 0x%x - device 0x%04x func %d\r\n", 
            code, code >> 16, (code >> 2)&0x0FFF
        ));
        goto cleanUp;
    }        

    // Take critical section if required (after postinit & no flag)
    if (
        g_ioctlState.postInit && 
        (g_oalIoCtlTable[i].flags & OAL_IOCTL_FLAG_NOCS) == 0
    ) {
        // Take critical section            
        EnterCriticalSection(&g_ioctlState.cs);
    }

    // Execute the handler
    rc = g_oalIoCtlTable[i].pfnHandler(
        code, pInBuffer, inSize, pOutBuffer, outSize, pOutSize
    );

    // Release critical section if it was taken above
    if (
        g_ioctlState.postInit && 
        (g_oalIoCtlTable[i].flags & OAL_IOCTL_FLAG_NOCS) == 0
    ) {
        // Release critical section            
        LeaveCriticalSection(&g_ioctlState.cs);
    }

cleanUp:
    OALMSG(OAL_IOCTL&&OAL_FUNC, (L"-OEMIoControl(rc = %d)\r\n", rc ));
    return rc;
}

//------------------------------------------------------------------------------
