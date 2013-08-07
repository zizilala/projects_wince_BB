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
//  File:  procinfo.c
//
//  This file implements the IOCTL_PROCESSOR_INFORMATION handler.
//
#include <windows.h>
#include <oal.h>


//------------------------------------------------------------------------------
//
//  Function:  OALIoCtlProcessorInformation
//
//  Implements the IOCTL_PROCESSOR_INFORMATION handler.
//
BOOL OALIoCtlProcessorInfo(
    UINT32 code, VOID *pInpBuffer, UINT32 inpSize, VOID *pOutBuffer, 
    UINT32 outSize, UINT32 *pOutSize
) {
    BOOL rc = FALSE;
    PROCESSOR_INFO *pInfo = (PROCESSOR_INFO*)pOutBuffer;
    UINT32 length1, length2, length3;

    OALMSG(OAL_FUNC, (L"+OALIoCtlProcessorInfo(...)\r\n"));

    // Set required/returned size if pointer isn't NULL
    if (pOutSize != NULL) {
        *pOutSize = sizeof(PROCESSOR_INFO);
    }
    
    // Validate inputs
    if (pOutBuffer == NULL && outSize > 0) {
        NKSetLastError(ERROR_INVALID_PARAMETER);
        OALMSG(OAL_WARN, (
            L"WARN: OALIoCtlProcessorInfo: Invalid output buffer\r\n"
        ));
        goto cleanUp;
    }

    if (pOutBuffer == NULL || outSize < sizeof(PROCESSOR_INFO)) {
        NKSetLastError(ERROR_INSUFFICIENT_BUFFER);
        OALMSG(OAL_WARN, (
            L"WARN: OALIoCtlProcessorInfo: Buffer too small\r\n"
        ));
        goto cleanUp;
    }

    // Verify OAL lengths
    length1 = (NKwcslen(g_oalIoCtlProcessorCore) + 1) * sizeof(WCHAR);
    if (length1 > sizeof(pInfo->szProcessCore)) {
        OALMSG(OAL_ERROR, (
            L"ERROR:OALIoCtlProcessorInfo: Core value too big\r\n"
        ));
        goto cleanUp;
    }

    length2 = (NKwcslen(g_oalIoCtlProcessorName) + 1) * sizeof(WCHAR);
    if (length2 > sizeof(pInfo->szProcessorName)) {
        OALMSG(OAL_ERROR, (
            L"ERROR:OALIoCtlProcessorInfo: Name value too big\r\n"
        ));
        goto cleanUp;
    }

    length3 = (NKwcslen(g_oalIoCtlProcessorVendor) + 1) * sizeof(WCHAR);
    if (length3 > sizeof(pInfo->szVendor)) {
        OALMSG(OAL_ERROR, (
            L"ERROR:OALIoCtlProcessorInfo: Vendor value too big\r\n"
        ));
        goto cleanUp;
    }

    // Copy in processor information    
    memset(pInfo, 0, sizeof(PROCESSOR_INFO));
    pInfo->wVersion = 1;
    memcpy(pInfo->szProcessCore, g_oalIoCtlProcessorCore, length1);
    memcpy(pInfo->szProcessorName, g_oalIoCtlProcessorName, length2);
    memcpy(pInfo->szVendor, g_oalIoCtlProcessorVendor, length3);
    pInfo->dwInstructionSet = g_oalIoCtlInstructionSet;
    pInfo->dwClockSpeed  = g_oalIoCtlClockSpeed;

    // Indicate success
    rc = TRUE;

cleanUp:
    OALMSG(OAL_FUNC, (L"-OALIoCtlProcessorInfo(rc = %d)\r\n", rc));
    return rc;
}

//------------------------------------------------------------------------------
