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
//  File:  config.c
//
#include <windows.h>
#include <ceddk.h>
#include <oal_log.h>
#include <oal_pci.h>

//------------------------------------------------------------------------------
//
//  Define: OAL_PCI_LATENCY_TIMER
//
//  Defines the value used to set the PCI latency
//
#define OAL_PCI_LATENCY_TIMER       64

//------------------------------------------------------------------------------
//
//  Define: OAL_PCI_CACHE_LINE_SIZE
//
//  Defines the value used to set the PCI cache line size
//
#define OAL_PCI_CACHE_LINE_SIZE     4

//------------------------------------------------------------------------------

static VOID PCIConfigBus(
    UINT32 busId, UINT32 bus, UINT32 *pSubBus, UINT32 *pMem, UINT32 *pIo, 
    UINT32 count, OAL_PCI_LOCATION *pPciLoc
);

static VOID PCIScanLocation(
    UINT32 busId, OAL_PCI_LOCATION pciLoc, UINT32 *pSubBus, UINT32 *pMem, 
    UINT32 *pIo, UINT32 count, OAL_PCI_LOCATION *aPciLoc
);

static VOID PCIConfigDevice(
   UINT32 busId, OAL_PCI_LOCATION pciLoc, UINT32 *pMemBase, UINT32 *pIoBase
);

static VOID PCIConfigBridge(
    UINT32 busId, OAL_PCI_LOCATION pciLoc, UINT32 secBus, UINT32 subBus,
    UINT32 *pMemBase, UINT32 memSize, UINT32 *pIoBase, UINT32 ioSize
);

//------------------------------------------------------------------------------
//
//  Function:  OALPCIConfig
//
//  This function is intend to be used for PCI bus configuration. It allows
//  initialize on PCI bus hiearchy (only bus numbers on bridges are initialized)
//  when it is called with count parameter equal zero. When count value is
//  minus one function configures all devices. For other values devices
//  on position defined by DEVICE_LOCATION structure are configured.
//
//  Note:  Current implementation doesn't check memory/io window size
//

BOOL OALPCIConfig(
    UINT32 busId, UINT32 memBase, UINT32 memSize, UINT32 ioBase,
    UINT32 ioSize, UINT32 pciLocCount, OAL_PCI_LOCATION *aPciLoc
) {
    UINT32 mem, io, subBus;

    OALMSG(OAL_FUNC&&OAL_PCI, (
        L"+OALPCIConfig(%d, 0x%08x, 0x%08x, 0x%08x, 0x%08x, %d, 0x%08x)\r\n",
        busId, memBase, memSize, ioBase, ioSize, pciLocCount, aPciLoc
    ));
    OALMSG(OAL_INFO, (L"INFO: PCI configuration start\r\n"));

    mem = memBase;
    io = ioBase;
    subBus = 0;
    PCIConfigBus(busId, 0, &subBus, &mem, &io, pciLocCount, aPciLoc);

    OALMSG(OAL_INFO, (L"INFO: PCI configuration complete\r\n"));
    OALMSG(OAL_FUNC&&OAL_PCI, (L"-OALPCIConfig(rc = 1)\r\n"));
    return TRUE;
}

//------------------------------------------------------------------------------
//
// Function:     CfgPCIBus
//
// Description:  Simplified PCI bus configuration
//

static VOID PCIConfigBus(
    UINT32 busId, UINT32 bus, UINT32 *pSubBus, UINT32 *pMem, UINT32 *pIo, 
    UINT32 count, OAL_PCI_LOCATION *aPciLoc
) {
    OAL_PCI_LOCATION pciLoc;
    UINT32 id;

    OALMSG(OAL_PCI&&OAL_FUNC, (
        L"+PCIConfigBus(%d, %d, %d, 0x%08x, 0x%08x, %d, 0x%08x)\r\n",
        busId, bus, *pSubBus, *pMem, *pIo, count, aPciLoc
    ));

    // Loop over all possible devices
    pciLoc.bus = bus;
    for (pciLoc.dev = 0; pciLoc.dev < PCI_MAX_DEVICES; pciLoc.dev++) {

        // Try read vendor id, if it fails there isn't device at position
        pciLoc.fnc = 0;
        id = OALPCIGetId(busId, pciLoc);
        if (
            LOWORD(id) == PCI_INVALID_VENDORID ||
            HIWORD(id) == PCI_INVALID_DEVICEID
        ) continue;

        OALMSG(OAL_INFO, (
            L"INFO: Bus %d Device %d: VendorId 0x%x DeviceId 0x%x\r\n",
            pciLoc.bus, pciLoc.dev, LOWORD(id), HIWORD(id)
        ));
        
        // Scan this device
        PCIScanLocation(busId, pciLoc, pSubBus, pMem, pIo, count, aPciLoc);         
    }      

    OALMSG(OAL_PCI&&OAL_FUNC, (
        L"-PCIConfigBus(subBus = %d, mem = 0x%X, io = 0x%X)\r\n", 
        *pSubBus, *pMem, *pIo
    ));
}

//------------------------------------------------------------------------------
//
//  Function:  PCIScanLocation
//
static VOID PCIScanLocation(
    UINT32 busId, OAL_PCI_LOCATION pciLoc, UINT32 *pSubBus, UINT32 *pMem, 
    UINT32 *pIo, UINT32 count, OAL_PCI_LOCATION *aPciLoc
) {
    UINT8 header;
    UINT32 secBus;
    UINT32 ix, mem, io;

    OALMSG(OAL_PCI&&OAL_FUNC, (
        L"+PCIScanLocation(%d, %d/%d/%d, %d, 0x%x, 0x%08x, %d, 0x%x)\r\n", 
        busId, pciLoc.bus, pciLoc.dev, pciLoc.fnc, *pSubBus, *pMem, *pIo,
        count, aPciLoc
    ));        

    // Loop over all possible device functions
    for (pciLoc.fnc = 0; pciLoc.fnc < PCI_MAX_FUNCTION; pciLoc.fnc++) {
    
        // Read header to find device type
        header = OALPCIGetHeaderType(busId, pciLoc);
        if (header == 0xFF) break;

        // Depending on device type do configuration
        switch (header & ~PCI_MULTIFUNCTION) {
        case PCI_DEVICE_TYPE:
            // Depending on mode configure device
            if (count == -1) {
                PCIConfigDevice(busId, pciLoc, pMem, pIo);
            } else {
                for (ix = 0; ix < count; ix++) {
                    if (
                        aPciLoc[ix].bus == pciLoc.bus &&
                        aPciLoc[ix].dev == pciLoc.dev &&
                        aPciLoc[ix].fnc == pciLoc.fnc
                    ) {                        
                        PCIConfigDevice(busId, pciLoc, pMem, pIo);
                        break;
                    }
                }
            }
            break;
        case PCI_BRIDGE_TYPE:
        case PCI_CARDBUS_TYPE:
            secBus = ++(*pSubBus);
            // Set primary, secondary and subordinate bus numbers
            PCIConfigBridge(busId, pciLoc, secBus, 0xFF, pMem, 0, pIo, 0);
            // Call config routing recursively
            mem = *pMem;
            io = *pIo;
            PCIConfigBus(
                busId, *pSubBus, pSubBus, pMem, pIo, count, aPciLoc
            );
            // Set bus numbers and configure bridge if device is behind it
            PCIConfigBridge(
                busId, pciLoc, secBus, *pSubBus, &mem, *pMem - mem, 
                &io, *pIo - io
            );
            break;
        }
    
        // Break loop if device isn't multifunction
        if (pciLoc.fnc == 0 && (header & PCI_MULTIFUNCTION) == 0) break;
        
    }

    OALMSG(OAL_PCI&&OAL_FUNC, (
        L"-PCIScanLocation(memBase = 0x%08x, ioBase = 0x%08x)\r\n", *pMem, *pIo
    ));
}

//------------------------------------------------------------------------------
//
//  Function:  PCIConfigDevice
//
static VOID PCIConfigDevice(
   UINT32 busId, OAL_PCI_LOCATION pciLoc, UINT32 *pMemBase, UINT32 *pIoBase
) {
    UINT32 offset, address, size, ix;
    UINT16 u16;
    UINT8 u8;

    OALMSG(OAL_PCI&&OAL_FUNC, (
        L"+PCIConfigDevice(%d, %d/%d/%d, 0x%08x, 0x%08x\r\n",
        busId, pciLoc.bus, pciLoc.dev, pciLoc.fnc, *pMemBase, *pIoBase
    ));
    
    // Scan all base registers
    offset = FIELD_OFFSET(PCI_COMMON_CONFIG, u.type0.BaseAddresses); 
    for (ix = 0; ix < PCI_TYPE0_ADDRESSES; ix++) {
        // Get required resource type and size
        address = 0xFFFFFFFF;
        OALPCICfgWrite(busId, pciLoc, offset, sizeof(address), &address);
        OALPCICfgRead(busId, pciLoc, offset, sizeof(address), &address);
        if ((address & 1) != 0) {
            // Check size result
            size = ~(address & 0xFFFFFFFC) + 1;
            if ((size & (size - 1)) != 0 || size == 0) continue;
            // Assign io space
            address = (*pIoBase + size - 1) & ~(size - 1);
            *pIoBase = address + size;
            OALMSG(OAL_INFO, (
                L"INFO: PCIConfigDevice: IO BAR[%d] 0x%x Size 0x%x\r\n", 
                ix, address, size
            ));
        } else {
            // Check size result
            size = ~(address & 0xFFFFFFF0) + 1;
            if ((size & (size - 1)) != 0 || size == 0) continue;
            // Check allocation type (let support only 32bit space for now)
            if ((size & 0x00000006) != 0) {
                OALMSG(OAL_WARN, (
                    L"WARNING: PCIConfigDevice: 64 bit space NOT supported\r\n" 
                ));
                continue;
            }
            // Assign memory space (allocate 4KB at least)
            if (size < 4096) size = 4096;
            address = (*pMemBase + size - 1) & ~(size - 1);
            *pMemBase = address + size;
            OALMSG(OAL_INFO, (
                L"INFO: PCIConfigDevice: Mem BAR[%d] 0x%x Size 0x%x\r\n", 
                ix, address, size
            ));
        }
        OALPCICfgWrite(busId, pciLoc, offset, sizeof(address), &address);
        offset += sizeof(UINT32);
    }
   
    // Set latency timer
    u8 = OAL_PCI_LATENCY_TIMER;
    OALPCICfgWrite(
        busId, pciLoc, FIELD_OFFSET(PCI_COMMON_CONFIG, LatencyTimer),
        sizeof(u8), &u8
    );

    // Set cache line size
    u8 = OAL_PCI_CACHE_LINE_SIZE;
    OALPCICfgWrite(
        busId, pciLoc, FIELD_OFFSET(PCI_COMMON_CONFIG, CacheLineSize),
        sizeof(u8), &u8
    );
    
    // Enable device and clear all status
    u16 = PCI_ENABLE_MEMORY_SPACE|PCI_ENABLE_IO_SPACE|PCI_ENABLE_BUS_MASTER;
    OALPCICfgWrite(
        busId, pciLoc, FIELD_OFFSET(PCI_COMMON_CONFIG, Command),
        sizeof(u16), &u16
    );

#ifdef OAL_PCI_DUMP_CONFIG
    OALPCIDumpConfig(busId, pciLoc);
#endif

    OALMSG(OAL_PCI&&OAL_FUNC, (
        L"-PCIConfigDevice(memBase = 0x%08x, ioBase = 0x%08x)\r\n", 
        *pMemBase, *pIoBase
    ));
 }

//------------------------------------------------------------------------------
//
//  Function:  PCIConfigBridge
//
static VOID PCIConfigBridge(
    UINT32 busId, OAL_PCI_LOCATION pciLoc, UINT32 secBus, UINT32 subBus,
    UINT32 *pMemBase, UINT32 memSize, UINT32 *pIoBase, UINT32 ioSize
) {
    UINT32 memLimit, ioLimit, u32;
    UINT16 command, u16;
    UINT8  u8;

    OALMSG(OAL_PCI&&OAL_FUNC, (
        L"+PCIConfigBridge(%d, %d/%d/%d, %d, %d, 0x%08x, 0x%08x, 0x%08x, 0x%08x\r\n",
        busId, pciLoc.bus, pciLoc.dev, pciLoc.fnc, secBus, subBus,
        *pMemBase, memSize, *pIoBase, ioSize
    ));

    // Compute limits
    if (memSize > 0) {
        *pMemBase = (*pMemBase + 0xFFFFF) & ~0xFFFFF;       // 1M boundary
        memSize = (memSize + 0xFFFFF) & ~0xFFFFF;           // 1M chunks
        memLimit = (*pMemBase + memSize - 1) & ~0xFFFFF;    // 1M boundary
    }
    if (ioSize > 0) {
        *pIoBase = (*pIoBase + 0xFFF) & ~0xFFF;             // 4K boundary
        ioSize = (ioSize + 0xFFF) & ~0xFFF;                 // 4K chunks
        ioLimit = (*pIoBase + ioSize - 1) & ~0xFFF;         // 4K boundary
    }

    // Disable Memory and I/O access
    command = 0;
    OALPCICfgWrite(
        busId, pciLoc, FIELD_OFFSET(PCI_COMMON_CONFIG, Command), 
        sizeof(command), &command
    );

    // Set master latency timer
    u8 = OAL_PCI_LATENCY_TIMER;
    OALPCICfgWrite(
        busId, pciLoc, FIELD_OFFSET(PCI_COMMON_CONFIG, LatencyTimer),
        sizeof(u8), &u8
    );

    // Set cache line size
    u8 = OAL_PCI_CACHE_LINE_SIZE;
    OALPCICfgWrite(
        busId, pciLoc, FIELD_OFFSET(PCI_COMMON_CONFIG, CacheLineSize),
        sizeof(u8), &u8
    );

    // Set primary bus number
    u8 = pciLoc.bus;
    OALPCICfgWrite(
        busId, pciLoc, 
        FIELD_OFFSET(PCI_COMMON_CONFIG, u.type1.PrimaryBusNumber), 
        sizeof(u8), &u8
    );

    // Set secondary bus number
    u8 = (UINT8)secBus;
    OALPCICfgWrite(
        busId, pciLoc, 
        FIELD_OFFSET(PCI_COMMON_CONFIG, u.type1.SecondaryBusNumber),
        sizeof(u8), &u8
    );

    // Set subordinate bus number
    u8 = (UINT8)subBus;
    OALPCICfgWrite(
        busId, pciLoc, 
        FIELD_OFFSET(PCI_COMMON_CONFIG, u.type1.SubordinateBusNumber),
        sizeof(u8), &u8
    );

    // Set secondary latency timer
    u8 = OAL_PCI_LATENCY_TIMER;
    OALPCICfgWrite(
        busId, pciLoc, 
        FIELD_OFFSET(PCI_COMMON_CONFIG, u.type1.SecondaryLatencyTimer),
        sizeof(u8), &u8
    );
    
    // Prepare configuration command
    command = PCI_ENABLE_SERR;

    // Set memory range
    if (memSize > 0) {

        // Memory base
        u16 = (UINT16)(*pMemBase >> 16);
        OALPCICfgWrite(
            busId, pciLoc, 
            FIELD_OFFSET(PCI_COMMON_CONFIG, u.type1.MemoryBase),
            sizeof(u16), &u16
        );

        // Memory limit
        u16 = (UINT16)((memLimit >> 16) & 0xFFF0);
        OALPCICfgWrite(
            busId, pciLoc, 
            FIELD_OFFSET(PCI_COMMON_CONFIG, u.type1.MemoryLimit),
            sizeof(u16), &u16
        );

        // Disable prefetch memory - base to 0xFFF00000
        u16 = 0xFFF0;
        OALPCICfgWrite(
            busId, pciLoc,
            FIELD_OFFSET(PCI_COMMON_CONFIG, u.type1.PrefetchableMemoryBase),
            sizeof(u16), &u16
        );

        // Disable prefetch memory - limit to 0x000FFFFF
        u16 = 0x0;
        OALPCICfgWrite(
            busId, pciLoc,
            FIELD_OFFSET(PCI_COMMON_CONFIG, u.type1.PrefetchableMemoryLimit),
            sizeof(u16), &u16
        );
    
        // Disable prefetch memory 
        u32 = 0xFFFFFFFF;
        OALPCICfgWrite(
            busId, pciLoc, FIELD_OFFSET(
                PCI_COMMON_CONFIG, u.type1.PrefetchableMemoryBaseUpper32
            ), sizeof(u32), &u32
        );

        // Disable prefetch memory
        u32 = 0;
        OALPCICfgWrite(
            busId, pciLoc, FIELD_OFFSET(
                PCI_COMMON_CONFIG, u.type1.PrefetchableMemoryLimitUpper32
            ), sizeof(u32), &u32
        );
        
        // Enable memory space
        command |= PCI_ENABLE_MEMORY_SPACE|PCI_ENABLE_BUS_MASTER;
    }

    // Set io range (32-bit address decode)
    if (ioSize > 0) {

        // IO base 
        u8 = (UINT8)((*pIoBase >> 8) & 0xF0);
        OALPCICfgWrite(
            busId, pciLoc, FIELD_OFFSET(PCI_COMMON_CONFIG, u.type1.IOBase),
            sizeof(u8), &u8
        );

        // IO upper base
        u16 = (UINT16)(*pIoBase >> 16);
        OALPCICfgWrite(
            busId, pciLoc, FIELD_OFFSET(
                PCI_COMMON_CONFIG, u.type1.IOBaseUpper
             ), sizeof(u16), &u16
        );

        // IO limit
        u8 = (UINT8)((ioLimit >> 8) & 0xF0);
        OALPCICfgWrite(
            busId, pciLoc, FIELD_OFFSET(PCI_COMMON_CONFIG, u.type1.IOLimit),
            sizeof(u8), &u8
        );

        // IO upper limit
        u16 = (UINT16)(ioLimit >> 16);
        OALPCICfgWrite(
            busId, pciLoc, FIELD_OFFSET(
                PCI_COMMON_CONFIG, u.type1.IOLimitUpper
            ), sizeof(u16), &u16
        );
        
        // Enable io space
        command |= PCI_ENABLE_IO_SPACE|PCI_ENABLE_BUS_MASTER;
    }

    // Enable bridge
    OALPCICfgWrite(
        busId, pciLoc, FIELD_OFFSET(PCI_COMMON_CONFIG, Command), 
        sizeof(command), &command
    );

#ifdef OAL_PCI_DUMP_CONFIG
    OALPCIDumpConfig(busId, pciLoc);
#endif

    OALMSG(OAL_PCI&&OAL_FUNC, (
        L"-PCIConfigBridge(memBase = 0x%08x, ioBase = 0x%08x\r\n", 
        *pMemBase, *pIoBase
    ));
}

//------------------------------------------------------------------------------

