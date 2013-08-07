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
//  File:  deviceid.c
//
//  This file implements the IOCTL_HAL_GET_DEVICE_ID handler.
//
#include <windows.h>
#include <oal.h>

#define PLATFORMNAMESIZE 128

extern BOOL OEMIoControl(DWORD code, VOID *pInBuffer, DWORD inSize, VOID *pOutBuffer, DWORD outSize, DWORD *pOutSize);

//------------------------------------------------------------------------------
//
//  Function:  OALIoCtlHalGetDeviceId
//
//  Implements the IOCTL_HAL_GET_DEVICE_ID handler. This function fills in a 
//  DEVICE_ID structure.
//
BOOL OALIoCtlHalGetDeviceId( 
    UINT32 code, VOID *pInpBuffer, UINT32 inpSize, VOID *pOutBuffer, 
    UINT32 outSize, UINT32 *pOutSize
) {
    BOOL rc = FALSE;
    DEVICE_ID *pId = (DEVICE_ID *)pOutBuffer;
    UINT32 size, length1, length2, offset, i;
    WCHAR platformName[PLATFORMNAMESIZE], bootMeNameW[OAL_KITL_ID_SIZE+1];
    char bootMeName[OAL_KITL_ID_SIZE+1];
    UINT32 platformNameSPICode = SPI_GETPLATFORMNAME, bootMeNameSPICode = SPI_GETBOOTMENAME;

    OALMSG(OAL_IOCTL&&OAL_FUNC, (L"+OALIoctlHalGetDeviceID(...)\r\n"));

    RETAILMSG(1, (TEXT("Warning: you are requesting IOCTL_HAL_GET_DEVICEID, which has been deprecated.  Use IOCTL_HAL_GET_DEVICE_INFO instead.\r\n")));

#if defined ( project_smartfon ) || defined ( project_wpc )  // Smartphone or PocketPC build

    // First, handle the special case where we care called with a buffer size of 16 bytes
    if ( outSize == sizeof(UUID) )
    {
        return OALIoCtlHalGetUUID(IOCTL_HAL_GET_UUID, pInpBuffer, inpSize, pOutBuffer, outSize, pOutSize);
    }
#endif

    // Compute required size (first is unicode, second multibyte!)
    if(!OEMIoControl(IOCTL_HAL_GET_DEVICE_INFO, &platformNameSPICode, sizeof(platformNameSPICode), platformName, sizeof(platformName), 0))
    {
        OALMSG(OAL_WARN, (
            L"WARN: OALIoCtlHalGetDeviceID: Call to IOCTL_HAL_GET_DEVICE_INFO::SPI_GETPLATFORMNAME failed.\r\n"
        ));
        goto cleanUp;
    }
    length1 = (NKwcslen(platformName) + 1) * sizeof(WCHAR);
    if(!OEMIoControl(IOCTL_HAL_GET_DEVICE_INFO, &bootMeNameSPICode, sizeof(bootMeNameSPICode), bootMeNameW, sizeof(bootMeNameW), 0))
    {
        OALMSG(OAL_WARN, (
            L"WARN: OALIoCtlHalGetDeviceID: Call to IOCTL_HAL_GET_DEVICE_INFO::SPI_GETBOOTMENAME failed.\r\n"
        ));
        goto cleanUp;
    }
    // Convert the BOOTME name from Unicode to multibyte for BC
    for(i = 0; (i < OAL_KITL_ID_SIZE) && (bootMeNameW[i] != 0); i++)
    {
        bootMeName[i] = (char)(bootMeNameW[i]);
    }
    bootMeName[i] = '\0';
    length2 = strlen(bootMeName) + 1;
    size = sizeof(DEVICE_ID) + length1 + length2;

    // update size if pOutSize is specified
    if (pOutSize) {
        *pOutSize = size;
    }

    // Validate inputs (do it after we can return required size)
    if (pOutBuffer == NULL || outSize < sizeof(DEVICE_ID)) {
        NKSetLastError(ERROR_INVALID_PARAMETER);
        OALMSG(OAL_WARN, (
            L"WARN: OALIoCtlHalGetDeviceID: Invalid parameter\r\n"
        ));
        goto cleanUp;
    }

    // Set size to DEVICE_ID structure
    pId->dwSize = size;

    // If the size is too small, indicate the correct size 
    if (outSize < size) {
        NKSetLastError(ERROR_INSUFFICIENT_BUFFER);
        goto cleanUp;
    }

    // Fill in the Device ID type
    offset = sizeof(DEVICE_ID);

    // Copy in PlatformType data
    pId->dwPresetIDOffset = offset;
    pId->dwPresetIDBytes = length1;
    memcpy((UINT8*)pId + offset, platformName, length1);
    offset += length1;

    // Copy device id data
    pId->dwPlatformIDOffset = offset;
    pId->dwPlatformIDBytes  = length2;
    memcpy((UINT8*)pId + offset, bootMeName, length2);

    // We are done
    rc = TRUE;
    
cleanUp:
    // Indicate status
    OALMSG(OAL_IOCTL&&OAL_FUNC, (L"-OALIoCtlHalGetDeviceID(rc = %d)\r\n", rc));
    return rc;
}

//------------------------------------------------------------------------------
