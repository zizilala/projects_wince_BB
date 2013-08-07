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
#include <ceddk.h>
#include <oal.h>

//------------------------------------------------------------------------------
//
//  Function:  OALIoCtlHalDdkCall
//
//  Implements the IOCTL_HAL_DDK_CALL
//
BOOL OALIoCtlHalDdkCall(
    UINT32 code, VOID *pInpBuffer, UINT32 inpSize, VOID *pOutBuffer, 
    UINT32 outSize, UINT32 *pOutSize
) {
    BOOL rc = FALSE;
    BUSDATA_PARMS *pBusData;
    PCI_SLOT_NUMBER slot;
    DEVICE_LOCATION devLoc;
    OAL_DDK_PARAMS *pParams;
    

    OALMSG(OAL_IO&&OAL_FUNC, (L"+OALIoCtlHalDdkCall(...)\r\n"));

    if (pOutSize) {
        *pOutSize = 0;
    }

    if (pInpBuffer == NULL || inpSize < sizeof(UINT32)) {
        NKSetLastError(ERROR_INVALID_PARAMETER);
        OALMSG(OAL_ERROR, (
            L"ERROR:OALIoctlHalDdkCall: INVALID PARAMETER\r\n"
        ));
        goto cleanUp;
    }        

    switch (*(UINT32*)pInpBuffer) {

    case IOCTL_HAL_SETBUSDATA:
        pBusData = (BUSDATA_PARMS*)pInpBuffer;
        switch (pBusData->BusDataType) {
        case PCIConfiguration:
            devLoc.IfcType = PCIBus;
            slot.u.AsULONG = pBusData->SlotNumber;
            devLoc.BusNumber = pBusData->BusNumber >> 8;
            devLoc.LogicalLoc = (pBusData->BusNumber & 0xFF) << 16;
            devLoc.LogicalLoc |= slot.u.bits.DeviceNumber << 8;
            devLoc.LogicalLoc |= slot.u.bits.FunctionNumber;
            pBusData->ReturnCode = OALIoWriteBusData(
                &devLoc, pBusData->Offset, pBusData->Length, pBusData->Buffer
            );
            rc = TRUE;
            break;
        default:
            OALMSG(OAL_ERROR, (
                L"ERROR: OALIoctlHalDdkCall: Unsupported bus type\r\n"
            ));
        }
        break;
    case IOCTL_HAL_GETBUSDATA:
        pBusData = (BUSDATA_PARMS*)pInpBuffer;
        switch (pBusData->BusDataType) {
        case PCIConfiguration:
            devLoc.IfcType = PCIBus;
            slot.u.AsULONG = pBusData->SlotNumber;
            devLoc.BusNumber = pBusData->BusNumber >> 8;
            devLoc.LogicalLoc = (pBusData->BusNumber & 0xFF) << 16;
            devLoc.LogicalLoc |= slot.u.bits.DeviceNumber << 8;
            devLoc.LogicalLoc |= slot.u.bits.FunctionNumber;
            pBusData->ReturnCode = OALIoReadBusData(
                &devLoc, pBusData->Offset, pBusData->Length, pBusData->Buffer
            );
            rc = TRUE;
            break;
        default:
            OALMSG(OAL_ERROR, (
                L"ERROR: OALIoctlHalDdkCall: Unsupported bus type\r\n"
            ));
        }
        break;

    case IOCTL_OAL_READBUSDATA:
        pParams = (OAL_DDK_PARAMS*)pInpBuffer;
        pParams->rc = OALIoReadBusData(
            &pParams->busData.devLoc, pParams->busData.offset, 
            pParams->busData.length , pParams->busData.pBuffer
        );
        rc = TRUE;
        break;
    case IOCTL_OAL_WRITEBUSDATA:
        pParams = (OAL_DDK_PARAMS*)pInpBuffer;
        pParams->rc = OALIoWriteBusData(
            &pParams->busData.devLoc, pParams->busData.offset, 
            pParams->busData.length , pParams->busData.pBuffer
        );
        rc = TRUE;
        break;
    case IOCTL_OAL_TRANSBUSADDRESS:
        pParams = (OAL_DDK_PARAMS*)pInpBuffer;
        pParams->rc = OALIoTransBusAddress(
            pParams->transAddress.ifcType, pParams->transAddress.busNumber, 
            pParams->transAddress.address, &pParams->transAddress.space, 
            &pParams->transAddress.address
        );
        rc = TRUE;
        break;
    case IOCTL_OAL_TRANSSYSADDRESS:
        pParams = (OAL_DDK_PARAMS*)pInpBuffer;
        pParams->rc = OALIoTransSystemAddress(
            pParams->transAddress.ifcType, pParams->transAddress.busNumber,
            pParams->transAddress.address, &pParams->transAddress.address
        );
        rc = TRUE;
        break;
    case IOCTL_OAL_BUSPOWEROFF:
        pParams = (OAL_DDK_PARAMS*)pInpBuffer;
        pParams->rc = OALIoBusPowerOff(
            pParams->busPower.ifcType, pParams->busPower.busNumber
        );            
        rc = TRUE;
        break;
    case IOCTL_OAL_BUSPOWERON:
        pParams = (OAL_DDK_PARAMS*)pInpBuffer;
        pParams->rc = OALIoBusPowerOn(
            pParams->busPower.ifcType, pParams->busPower.busNumber
        );            
        rc = TRUE;
        break;

    default:
        NKSetLastError(ERROR_INVALID_PARAMETER);
        OALMSG(OAL_ERROR, (
            L"ERROR: OALIoctlHalDdkCall: Unsupported function code 0x%08x\r\n",
            *(UINT32*)pInpBuffer
        ));
    }

cleanUp:
    OALMSG(OAL_IO&&OAL_FUNC, (L"-OALIoCtlHalDdkCall(rc = %d)\r\n", rc));
    return rc;    
}

//------------------------------------------------------------------------------
