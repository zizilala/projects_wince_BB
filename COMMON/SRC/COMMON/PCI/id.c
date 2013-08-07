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
//  File:  id.c
//
//  This file implements helper PCI bus functions.
//
#include <windows.h>
#include <ceddk.h>
#include "oal_log.h"
#include "oal_pci.h"

//------------------------------------------------------------------------------

UINT32 OALPCIGetId(UINT32 busId, OAL_PCI_LOCATION pciLoc)
{
    UINT32 id = 0xFFFFFFFF, offset;

    OALMSG(OAL_PCI&&OAL_FUNC, (
        L"+OALPCIGetDeviceId(%d, %d/%d/%d)\r\n", busId, pciLoc.bus, pciLoc.dev,
        pciLoc.fnc
    ));
    
    offset = FIELD_OFFSET(PCI_COMMON_CONFIG, VendorID);
    OALPCICfgRead(busId, pciLoc, offset, sizeof(id), &id);

    OALMSG(OAL_PCI&&OAL_FUNC, (L"-OALPCIGetDeviceId(id = 0x%08x)\r\n", id));
    return id;
}

//------------------------------------------------------------------------------

UINT32 OALPCIGetMBAR(
    UINT32 busId, OAL_PCI_LOCATION pciLoc, UINT32 index
) {
    UINT32 address = 0xFFFFFFFF, offset;
    
    OALMSG(OAL_PCI&&OAL_FUNC, (
        L"+OALPCIGetDeviceMBAR(%d, %d/%d/%d, %d)\r\n", busId, pciLoc.bus,
        pciLoc.dev, pciLoc.fnc, index
    ));

    // Read BAR register
    offset = FIELD_OFFSET(PCI_COMMON_CONFIG, u.type0.BaseAddresses);
    offset += index * sizeof(UINT32);
    OALPCICfgRead(busId, pciLoc, offset, sizeof(address), &address);

    OALMSG(OAL_PCI&&OAL_FUNC, (
        L"-OALPCIGetDeviceMBAR(address = 0x%08x)\r\n", address
    ));
    return address;
}

//------------------------------------------------------------------------------

UINT32 OALPCIGetInterruptPin(UINT32 busId, OAL_PCI_LOCATION pciLoc)
{
    UINT8 pin = 1;
    UINT32 offset;

    OALMSG(OAL_PCI&&OAL_FUNC, (
        L"+OALPCIGetDeviceInterruptPin(%d, %d/%d/%d, %d)\r\n", busId, 
        pciLoc.bus, pciLoc.dev, pciLoc.fnc
    ));

    offset = FIELD_OFFSET(PCI_COMMON_CONFIG, u.type0.InterruptPin);
    OALPCICfgRead(busId, pciLoc, offset, sizeof(pin), &pin);

    OALMSG(OAL_PCI&&OAL_FUNC, (
        L"-OALPCIGetDeviceInterruptPin(pin = %d)\r\n", pin
    ));
    return pin;
}

//------------------------------------------------------------------------------

UINT8 OALPCIGetHeaderType(UINT32 busId, OAL_PCI_LOCATION pciLoc)
{
    UINT8 rc;

    OALMSG(OAL_PCI&&OAL_FUNC, (
        L"+OALPCIGetHeaderType(%d, %d/%d/%d, %d)\r\n", busId, 
        pciLoc.bus, pciLoc.dev, pciLoc.fnc
    ));
    
    // Read header
    if (OALPCICfgRead(
        busId, pciLoc, FIELD_OFFSET(PCI_COMMON_CONFIG, HeaderType),
        sizeof(rc), &rc
    ) != sizeof(rc)) rc = 0xFF;
    
    OALMSG(OAL_PCI&&OAL_FUNC, (
        L"-OALPCIGetHeaderType(type = %02x)\r\n", rc
    ));
    return rc;
}

//------------------------------------------------------------------------------

UINT8 OALPCIGetSecBusNum(UINT32 busId, OAL_PCI_LOCATION pciLoc)
{
    UINT8 rc;
    
    OALMSG(OAL_PCI&&OAL_FUNC, (
        L"+OALPCIGetSucBusNum(%d, %d/%d/%d, %d)\r\n", busId, 
        pciLoc.bus, pciLoc.dev, pciLoc.fnc
    ));

    // Read header
    if (OALPCICfgRead(
        busId, pciLoc, FIELD_OFFSET(
            PCI_COMMON_CONFIG, u.type1.SecondaryBusNumber
        ), sizeof(rc), &rc
    ) != sizeof(rc)) rc = 0xFF;

    OALMSG(OAL_PCI&&OAL_FUNC, (
        L"-OALPCIGetSucBusNum(num = %d)\r\n", rc
    ));
    return rc;
}

//------------------------------------------------------------------------------

UINT8 OALPCIGetSubBusNum(UINT32 busId, OAL_PCI_LOCATION pciLoc)
{
    UINT8 rc;
    
    OALMSG(OAL_PCI&&OAL_FUNC, (
        L"+OALPCIGetSubBusNum(%d, %d/%d/%d, %d)\r\n", busId, 
        pciLoc.bus, pciLoc.dev, pciLoc.fnc
    ));

    // Read header
    if (OALPCICfgRead(
        busId, pciLoc, FIELD_OFFSET(
            PCI_COMMON_CONFIG, u.type1.SubordinateBusNumber
        ), sizeof(rc), &rc
    ) != sizeof(rc)) rc = 0xFF;

    OALMSG(OAL_PCI&&OAL_FUNC, (
        L"-OALPCIGetSubBusNum(num = %d)\r\n", rc
    ));
    return rc;
}

//------------------------------------------------------------------------------

