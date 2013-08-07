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
#include <windows.h>
#include <nkintr.h>
#include <oal.h>


//------------------------------------------------------------------------------
//
//  Function:  OALIoCtlHalRequestSysIntr
//
//  This function return existing SysIntr for non-shareable IRQs and create
//  new Irq -> SysIntr mapping for shareable.
//
BOOL OALIoCtlHalRequestSysIntr(
    UINT32 code, VOID* pInpBuffer, UINT32 inpSize, VOID* pOutBuffer,
    UINT32 outSize, UINT32 *pOutSize
) {
    BOOL rc;
    UINT32 *pInpData = pInpBuffer, sysIntr;
    UINT32 inpElems = inpSize / sizeof (UINT32);

    OALMSG(OAL_INTR&&OAL_FUNC, (L"+OALIoCtlHalRequestSysIntr\r\n"));

    // We know output size already
    if (pOutSize != NULL) *pOutSize = sizeof(UINT32);

    // Check input parameters
    if ((pInpBuffer == NULL || inpElems < 1 || (inpSize % sizeof(UINT32) != 0)) ||
       (!pOutBuffer && pOutSize > 0)
    ) {
        NKSetLastError(ERROR_INVALID_PARAMETER);
        rc = FALSE;
        OALMSG(OAL_WARN, (
            L"WARN: IOCTL_HAL_REQUEST_SYSINTR invalid parameters\r\n"
        ));
        goto cleanUp;
    }

    if (pOutBuffer == NULL || outSize < sizeof(UINT32)
    ) {
        NKSetLastError(ERROR_INSUFFICIENT_BUFFER);
        rc = FALSE;
        OALMSG(OAL_WARN, (
            L"WARN: IOCTL_HAL_REQUEST_SYSINTR invalid parameters\r\n"
        ));
        goto cleanUp;
    }



    // Find if it is new or old call type
    if (inpElems > 1 && pInpData[0] == -1)
    {
        if (inpElems > 2 &&
            (pInpData[1] == OAL_INTR_TRANSLATE || pInpData[1] == OAL_INTR_DYNAMIC ||
             pInpData[1] == OAL_INTR_STATIC || pInpData[1] == OAL_INTR_FORCE_STATIC ||
             pInpData[1] == 0 /* default behavior is OAL_INTR_DYNAMIC */) 
           ) {
            // Second UINT32 contains flags, third and subsequents IRQs
            sysIntr = OALIntrRequestSysIntr(inpElems - 2, &pInpData[2], pInpData[1]);
        } else {
            NKSetLastError(ERROR_INVALID_PARAMETER);
            rc = FALSE;
            OALMSG(OAL_WARN, (
                  L"WARN: IOCTL_HAL_REQUEST_SYSINTR invalid parameters\r\n"
              ));
            goto cleanUp;
        }
    } else {
        if (inpElems == 1) {

            // This is legacy call, first UINT32 contains IRQ
            sysIntr = OALIntrRequestSysIntr(1, pInpData, 0);
        } else {
            NKSetLastError(ERROR_INVALID_PARAMETER);
            rc = FALSE;
            OALMSG(OAL_WARN, (
                  L"WARN: IOCTL_HAL_REQUEST_SYSINTR invalid parameters\r\n"
              ));
            goto cleanUp;
        }
    }

    // Store obtained SYSINTR
    *(UINT32*)pOutBuffer = sysIntr;
    rc = (sysIntr != SYSINTR_UNDEFINED);

cleanUp:
    OALMSG(OAL_INTR&&OAL_FUNC, (
        L"+OALIoCtlHalRequestSysIntr(rc = %d)\r\n", rc
    ));
    return rc;
}


//------------------------------------------------------------------------------
//
//  Function:  OEMReleaseSysIntr
//
//  This function releases a previously-requested SYSINTR.
//
BOOL OALIoCtlHalReleaseSysIntr(
    UINT32 code, VOID* pInpBuffer, UINT32 inpSize, VOID* pOutBuffer,
    UINT32 outSize, UINT32 *pOutSize
) {
    BOOL rc;

    OALMSG(OAL_INTR&&OAL_FUNC, (L"+OALIoCtlHalRequestSysIntr\r\n"));

    // We know output size
    if (pOutSize != NULL) *pOutSize = 0;

    // Check input parameters
    if (pInpBuffer == NULL || inpSize != sizeof(UINT32)) {
        OALMSG(OAL_WARN, (
            L"WARN: IOCTL_HAL_RELEASE_SYSINTR invalid parameters\r\n"
        ));
        NKSetLastError(ERROR_INVALID_PARAMETER);
        rc = FALSE;
        goto cleanUp;
    }

    // Call function itself
    rc = OALIntrReleaseSysIntr(*(UINT32*)pInpBuffer);

cleanUp:
    OALMSG(OAL_INTR&&OAL_FUNC, (
        L"+OALIoCtlHalRequestSysIntr(rc = %d)\r\n", rc
    ));
    return rc;
}


//------------------------------------------------------------------------------
//
//  Function: OALIoCtlHalRequestIrq
//
//  This function returns IRQ for device on given location.
//
BOOL OALIoCtlHalRequestIrq(
    UINT32 code, VOID* pInpBuffer, UINT32 inpSize, VOID* pOutBuffer,
    UINT32 outSize, UINT32 *pOutSize
) {
    BOOL rc = FALSE;
    DEVICE_LOCATION *pDevLoc, devLoc;
    UINT32 count;

    OALMSG(OAL_INTR&&OAL_FUNC, (L"+OALIoCtlHalRequestIrq\r\n"));

    // Check parameters
    if (
        pInpBuffer == NULL || inpSize != sizeof(DEVICE_LOCATION) ||
        pOutBuffer == NULL || outSize < sizeof(UINT32)
    ) {
        NKSetLastError(ERROR_INVALID_PARAMETER);
        OALMSG(OAL_WARN, (
            L"WARN: IOCTL_HAL_REQUEST_IRQ invalid parameters\r\n"
        ));
        goto cleanUp;
    }

    // Call function itself (we must first fix PCI bus device location...)
    pDevLoc = (DEVICE_LOCATION*)pInpBuffer;
    memcpy(&devLoc, pDevLoc, sizeof(devLoc));
    if (devLoc.IfcType == PCIBus) {
        devLoc.BusNumber >>= 8; // shift BusNumber to represent host-to-pci bridge number in bits 8-15
    }
    count = outSize/sizeof(UINT32);
    rc = OALIntrRequestIrqs(&devLoc, &count, pOutBuffer);
    if (pOutSize != NULL) *pOutSize = count * sizeof(UINT32);

cleanUp:
    OALMSG(OAL_INTR&&OAL_FUNC, (
        L"-OALIoCtlHalRequestSysIntr(rc = %d)\r\n", rc
    ));
    return rc;
}

//------------------------------------------------------------------------------
