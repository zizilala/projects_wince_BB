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
//  File: data.c
//
//  This file implements generic OALIoReadBusData/OALIoWriteBusData functions.
//  It is compiled in several versions depending on supported bus (none, PCI
//  currently). Another buses with configuration space (like PCMCIA) can
//  be added when required.
//
#include <windows.h>
#include "oal_log.h"
#include "oal_io.h"
#include "oal_pci.h"

//------------------------------------------------------------------------------
//
//  Function: OALIoReadBusData
//
//  This function reads data from device configuration space. It returns
//  number of bytes read. Implementation simply dispatch call to bus
//  specific function depending on device location.
//
UINT32 OALIoReadBusData(
    DEVICE_LOCATION *pDevLoc, UINT32 address, UINT32 size, VOID *pData
) {
    UINT32 rc = 0;
    
    switch (pDevLoc->IfcType) {
#ifdef OAL_IO_PCI        
    case PCIBus:
        rc = OALPCICfgRead(
            pDevLoc->BusNumber, *(OAL_PCI_LOCATION*)&pDevLoc->LogicalLoc,
            address, size, pData
        );
        // Workaround PCIBus enumeration issue
        if (rc == 0) {
            memset(pData, 0xFF, size);
            rc = size;
        }
        break;
#endif        
    }
    return rc;
}

//------------------------------------------------------------------------------
//
//  Function: OALIoWriteBusData
//
//  This function writes data from device configuration space. It returns
//  number of bytes written. Implementation simply dispatch call to bus
//  specific function depending on device location.
//
UINT32 OALIoWriteBusData(
    DEVICE_LOCATION *pDevLoc, UINT32 address, UINT32 size, VOID *pData
) {
    UINT32 rc = 0;
    
    switch (pDevLoc->IfcType) {
#ifdef OAL_IO_PCI
    case PCIBus:
        rc = OALPCICfgWrite(
            pDevLoc->BusNumber, *(OAL_PCI_LOCATION*)&pDevLoc->LogicalLoc, 
            address, size, pData
        );
        break;
#endif        
    }
    return rc;
}

//------------------------------------------------------------------------------
