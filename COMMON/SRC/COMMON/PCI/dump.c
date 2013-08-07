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
//  File:  dump.c
//
//  This file implements PCI device configuration dump functions. It is
//  implemented in  separate file to avoid linking when not referenced.
//
#include <windows.h>
#include <ceddk.h>
#include <oal_log.h>
#include <oal_pci.h>

//------------------------------------------------------------------------------
//
//  Function:  OALPCIDumpConfig
//
VOID OALPCIDumpConfig(UINT32 busId, OAL_PCI_LOCATION pciLoc)
{
    PCI_COMMON_CONFIG cfg;

    memset(&cfg, 0, sizeof(cfg));
    OALPCICfgRead(busId, pciLoc, 0, sizeof(cfg), &cfg);

    OALLog(L"---\r\n");
    OALLog(
        L"PCI %d/%d/%d - VendorID 0x%04x  DeviceID 0x%04x\r\n", 
        pciLoc.bus, pciLoc.dev, pciLoc.fnc, cfg.VendorID, cfg.DeviceID
    );        
    OALLog(
        L"PCI %d/%d/%d - Command 0x%04x  Status 0x%04x\r\n", 
        pciLoc.bus, pciLoc.dev, pciLoc.fnc, cfg.Command, cfg.Status
    );        
    OALLog(
        L"PCI %d/%d/%d - Rev 0x%02x  ProgIf 0x%02x  SubClass 0x%02x  BaseClass 0x%02x\r\n", 
        pciLoc.bus, pciLoc.dev, pciLoc.fnc, cfg.RevisionID, cfg.ProgIf,
        cfg.SubClass, cfg.BaseClass
    );        
    OALLog(
        L"PCI %d/%d/%d - CacheLine 0x%02x  Latency 0x%02x  Header 0x%02x  BIST 0x%02x\r\n", 
        pciLoc.bus, pciLoc.dev, pciLoc.fnc, cfg.CacheLineSize, cfg.LatencyTimer,
        cfg.HeaderType, cfg.BIST
    );        

    // Depending on device type do configuration
    switch (cfg.HeaderType & ~PCI_MULTIFUNCTION) {
    case PCI_DEVICE_TYPE:
        OALLog(
            L"PCI %d/%d/%d - Addresses 0x%08x 0x%08x 0x%08x 0x%08x 0x%08x 0x%08x\r\n", 
            pciLoc.bus, pciLoc.dev, pciLoc.fnc, cfg.u.type0.BaseAddresses[0],
            cfg.u.type0.BaseAddresses[1], cfg.u.type0.BaseAddresses[2], 
            cfg.u.type0.BaseAddresses[3], cfg.u.type0.BaseAddresses[4], 
            cfg.u.type0.BaseAddresses[5]
        );
        OALLog(
            L"PCI %d/%d/%d - CIS 0x%08x  SubVendorID 0x%04x  SubSystemID 0x%04x\r\n", 
            pciLoc.bus, pciLoc.dev, pciLoc.fnc, cfg.u.type0.CIS, 
            cfg.u.type0.SubVendorID, cfg.u.type0.SubSystemID
        );
        OALLog(
            L"PCI %d/%d/%d - ROM 0x%08x  IntrLine 0x%02x  IntrPin 0x%02x\r\n", 
            pciLoc.bus, pciLoc.dev, pciLoc.fnc, cfg.u.type0.ROMBaseAddress,
            cfg.u.type0.InterruptLine, cfg.u.type0.InterruptPin
        );            
        OALLog(
            L"PCI %d/%d/%d - MinGrant 0x%02x  MaxLatency 0x%02x\r\n", 
            pciLoc.bus, pciLoc.dev, pciLoc.fnc, cfg.u.type0.MinimumGrant,
            cfg.u.type0.MaximumLatency
        );
        break;
    case PCI_BRIDGE_TYPE:
        OALLog(
            L"PCI %d/%d/%d - Addresses 0x%08x 0x%08x\r\n", 
            pciLoc.bus, pciLoc.dev, pciLoc.fnc, cfg.u.type1.BaseAddresses[0],
            cfg.u.type1.BaseAddresses[1]
        );
        OALLog(
            L"PCI %d/%d/%d - Bus Primary %d  Secondary %d  Subordinate %d\r\n", 
            pciLoc.bus, pciLoc.dev, pciLoc.fnc, cfg.u.type1.PrimaryBusNumber,
            cfg.u.type1.SecondaryBusNumber, cfg.u.type1.SubordinateBusNumber
        );
        OALLog(
            L"PCI %d/%d/%d - IOBase 0x%02x  IOLimit 0x%02x  IOBaseUpper 0x%04x  IOLimitUpper 0x%04x\r\n", 
            pciLoc.bus, pciLoc.dev, pciLoc.fnc, cfg.u.type1.IOBase, 
            cfg.u.type1.IOLimit, cfg.u.type1.IOBaseUpper, 
            cfg.u.type1.IOLimitUpper
        );            
        OALLog(
            L"PCI %d/%d/%d - MemBase 0x%04x  MemLimit 0x%04x\r\n", 
            pciLoc.bus, pciLoc.dev, pciLoc.fnc, cfg.u.type1.MemoryBase, 
            cfg.u.type1.MemoryLimit
        );
        OALLog(
            L"PCI %d/%d/%d - Prefetch MemBase 0x%04x  MemLimit 0x%04x\r\n",
            pciLoc.bus, pciLoc.dev, pciLoc.fnc, 
            cfg.u.type1.PrefetchableMemoryBase, 
            cfg.u.type1.PrefetchableMemoryLimit
        );
        OALLog(
            L"PCI %d/%d/%d - MemBaseUpper32 0x%08x  MemLimitUpper32 0x%08x\r\n", 
            pciLoc.bus, pciLoc.dev, pciLoc.fnc, 
            cfg.u.type1.PrefetchableMemoryBaseUpper32, 
            cfg.u.type1.PrefetchableMemoryLimitUpper32
        );            
        OALLog(
            L"PCI %d/%d/%d - SecLatency 0x%02x  BridgeControl 0x%04x\r\n",
            pciLoc.bus, pciLoc.dev, pciLoc.fnc, 
            cfg.u.type1.SecondaryLatencyTimer,
            cfg.u.type1.BridgeControl
        );            
        OALLog(
            L"PCI %d/%d/%d - ROM 0x%08x  IntrLine 0x%02x  IntrPin 0x%02x\r\n", 
            pciLoc.bus, pciLoc.dev, pciLoc.fnc, cfg.u.type1.ExpansionROMBase,
            cfg.u.type1.InterruptLine, cfg.u.type1.InterruptPin
        );            
        break;
    }
}

//------------------------------------------------------------------------------
//
//  Function:  OALPCIDump
//
VOID OALPCIDump(UINT32 busId, BOOL full)
{
    OAL_PCI_LOCATION pciLoc;
    UINT8 maxBus, subBus, header;
    UINT32 id;
    
    // First find number of buses
    maxBus = 0;
    pciLoc.logicalLoc = 0;
    while (pciLoc.bus <= maxBus) {
        pciLoc.dev = 0;
        while (pciLoc.dev < PCI_MAX_DEVICES) {
            pciLoc.fnc = 0;
            while (pciLoc.fnc < PCI_MAX_FUNCTION) {
                // Get device id
                id = OALPCIGetId(busId, pciLoc);
                // If it is invalid (-1) let try next device
                if (LOWORD(id) == PCI_INVALID_VENDORID ||
                    HIWORD(id) == PCI_INVALID_DEVICEID
                ) break;
                // Dump device
                if (full) {
                    OALPCIDumpConfig(busId, pciLoc);
                } else {
                    OALLog(
                        L"PCI %d/%d/%d: VendorId %04x DeviceId %04x\r\n",
                        pciLoc.bus, pciLoc.dev, pciLoc.fnc, LOWORD(id),
                        HIWORD(id)
                    );
                }
                // Read header
                header = OALPCIGetHeaderType(busId, pciLoc);
                if (header == 0xFF) break;
                // We are on bus 0 and device is bridge read subordinate
                // bus number -- this give us maximal bus number on PCI bus
                if (pciLoc.bus == 0) {
                    switch (header & ~PCI_MULTIFUNCTION) {
                    case PCI_BRIDGE_TYPE:
                    case PCI_CARDBUS_TYPE:
                        subBus = OALPCIGetSubBusNum(busId, pciLoc);
                        if (subBus != 0xFF && subBus > maxBus) maxBus = subBus;
                        break;
                    }
                }
                // Break loop if device isn't multifunction
                if (pciLoc.fnc == 0 && (header & PCI_MULTIFUNCTION) == 0) break;
                // Move to next function
                pciLoc.fnc++;
            }
            pciLoc.dev++;
        }
        pciLoc.bus++;
    }

}

//------------------------------------------------------------------------------

