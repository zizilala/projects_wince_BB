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
//  File:  uuid.c
//
//  This file implements the IOCTL_HAL_GET_UUID handler.
//
#include <windows.h>
#include <oal.h>

//------------------------------------------------------------------------------
//
//  Function:  OALIoCtlHalGetUUID
//
//  Implements the IOCTL_HAL_GET_UUID handler. This function fills in a 
//  GUID structure.
//
BOOL OALIoCtlHalGetUUID (
    UINT32 code, VOID *pInpBuffer, UINT32 inpSize, VOID *pOutBuffer,
    UINT32 outSize, UINT32 *pOutSize
) {
    BOOL rc = FALSE;
    static const GUID guidPattern = {0x600cc7d0, 0xde3a, 0x4713, {
        0xa5, 0xb0, 0x56, 0xe, 0x6c, 0x36, 0x4e, 0xde 
    }};
    GUID *pUuid;
    LPCSTR pDeviceId;
    UINT32 length;

    RETAILMSG(1, (TEXT("Warning: you are calling IOCTL_HAL_GET_UUID, which has been deprecated.  Use IOCTL_HAL_GET_DEVICE_INFO instead.\r\n")));

    // Check buffer size
    if (pOutSize) {
        *pOutSize = sizeof(GUID);
    }

    if (pOutBuffer == NULL && outSize > 0 ) {
        NKSetLastError(ERROR_INVALID_PARAMETER);
        OALMSG(OAL_WARN, (L"WARN: OALIoCtlHalGetUUID: Invalid output buffer\r\n"));
        goto cleanUp;
    }

    if (pOutBuffer == NULL || outSize < sizeof(GUID)) {
        NKSetLastError(ERROR_INSUFFICIENT_BUFFER);
        OALMSG(OAL_WARN, (L"WARN: OALIoCtlHalGetUUID: Buffer too small\r\n"));
        goto cleanUp;
    }

    // Does BSP specific UUID exist?
    pUuid = OALArgsQuery(OAL_ARGS_QUERY_UUID);
    if (pUuid != NULL) {

        // It does, copy it as return value
        memcpy(pOutBuffer, pUuid, sizeof(GUID));

    } else {

        // Try to generate GUID from device id...
        pDeviceId = OALArgsQuery(OAL_ARGS_QUERY_DEVID);
        if (pDeviceId == NULL) {
            NKSetLastError(ERROR_NOT_SUPPORTED);
            OALMSG(OAL_ERROR, (
                L"ERROR: OALIoCtlHalGetUUID: Device doesn't support UUID\r\n"
            ));
            goto cleanUp;
        }

        RETAILMSG(1, (TEXT("Warning: IOCTL_HAL_GET_UUID is using IOCTL_HAL_GET_DEVICEID and returning potentially nonunique data.\r\n")));

        // How many chars/bytes will be used from device id
        length = strlen(pDeviceId);
        if (length < sizeof(GUID)) {
            // Use pattern
            memcpy(pOutBuffer, &guidPattern, sizeof(GUID) - length);
            (UINT8*)pOutBuffer += sizeof(GUID) - length;
        }
        // Copy deviceId as rest of GUID
        memcpy(pOutBuffer, pDeviceId, length);
        
    }

    // Done
    rc = TRUE;

cleanUp:
    return rc;
}

//------------------------------------------------------------------------------
