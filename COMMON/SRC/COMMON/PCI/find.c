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
//  File:  find.c
//
//  This file implements PCI find function. It is used to find device with
//  match vendor and device id on PCI bus.
//
#include <windows.h>
#include <ceddk.h>
#include <oal_log.h>
#include <oal_pci.h>

//------------------------------------------------------------------------------
//
//  Function:  OALPCIFindNextId
//
//  Searches the PCI bus for cards that match the Vendor/Device Id. Search 
//  starts from position next to actual position on *pPciLoc. When device
//  with given Id is found function return TRUE. Device location is in
//  *pPciLoc.
//
BOOL OALPCIFindNextId(
    UINT32 busId, UINT32 deviceId, OAL_PCI_LOCATION *pPciLoc
) {
    BOOL rc;
    OAL_PCI_LOCATION pciLoc;
    UINT32 id;
    UINT8 header, subBus, maxBus;

    OALMSG(OAL_PCI&&OAL_FUNC, (
        L"+OALPCIFindNextId(%d, 0x%08x, 0x%08x)\r\n", busId, deviceId, *pPciLoc
    ));

    // First find number of buses
    maxBus = 0;
    pciLoc.logicalLoc = 0;
    while (pciLoc.dev < PCI_MAX_DEVICES) {
        pciLoc.fnc = 0;
        while (pciLoc.fnc < PCI_MAX_FUNCTION) {
            // Get device id
            id = OALPCIGetId(busId, pciLoc);
            // If it is invalid (-1) let try next device
            if (LOWORD(id) == PCI_INVALID_VENDORID ||
                HIWORD(id) == PCI_INVALID_DEVICEID
            ) break;
            // Read header
            header = OALPCIGetHeaderType(busId, pciLoc);
            if (header == 0xFF) break;
            // We are on bus 0 and device is bridge read subordinate
            // bus number -- this give us maximal bus number on PCI bus
            switch (header & ~PCI_MULTIFUNCTION) {
            case PCI_BRIDGE_TYPE:
            case PCI_CARDBUS_TYPE:
                subBus = OALPCIGetSubBusNum(busId, pciLoc);
                if (subBus != 0xFF && subBus > maxBus) maxBus = subBus;
                break;
            }                        
            // Break loop if device isn't multifunction
            if (pciLoc.fnc == 0 && (header & PCI_MULTIFUNCTION) == 0) break;
            // Move to next function
            pciLoc.fnc++;
        }
        pciLoc.dev++;
    }

    // Now try to find device
    do {

        // Read header on actual device
        header = OALPCIGetHeaderType(busId, pciLoc);

        // Move to next PCI device but break if end of PCI bus was reached
        if (pPciLoc->fnc < PCI_MAX_FUNCTION && (
            pPciLoc->fnc > 0 ||
            (header != 0xFF && (header & PCI_MULTIFUNCTION) != 0)
        )) {
            pPciLoc->fnc++;
        } else {
            pPciLoc->fnc = 0;
            if (++pPciLoc->dev >= PCI_MAX_DEVICES) {
                pPciLoc->dev = 0;
                if (++pPciLoc->bus > maxBus) break;
            }
        }        

    } while (OALPCIGetId(busId, *pPciLoc) != deviceId);        

    // Find result    
    rc = (pPciLoc->bus <= maxBus);
    
    OALMSG(OAL_PCI&&OAL_FUNC, (
        L"-OALPCIFindNextId(rc = %d, loc = %08x)\r\n", rc, *pPciLoc
    ));
    return rc;
}

//------------------------------------------------------------------------------

