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
//  Header: oal_pci.h
//
//  The OAL PCI module implements PCI bus support. The module must be
//  implemented on hardware which support PCI bus and it uses standard PCI
//  bus driver (which calls OAL/HAL to read/write PCI bus configuration space.
//  Module also implements simple PCI bus configration which is intented to
//  be used in boot loader/KITL implementation.
//
#ifndef __OAL_PCI_H
#define __OAL_PCI_H

#if __cplusplus
extern "C" {
#endif

//------------------------------------------------------------------------------
//
//  Type:  OAL_PCI_LOCATION
//
//  This type defines the PCI Bus/Device/Function device location.
//
typedef union {
    struct {
        UINT8 fnc;                    // function
        UINT8 dev;                    // device
        UINT8 bus;                    // bus
    };
    UINT32 logicalLoc;
} OAL_PCI_LOCATION, *POAL_PCI_LOCATION;

//------------------------------------------------------------------------------
//
//  Function:  OALPCIInit
//
//  This function should initialize PCI module. It is called from OEMInit
//  to initialize PCI bus. In most cases it will be implemented in SoC library
//  or it will be platform specific.
//
BOOL OALPCIInit();

//------------------------------------------------------------------------------
//
//  Function:  OALPCICfgRead
//
//  This function reads PCI configuration space at location specified by pciLoc.
//  This routine is hardware dependend and in most cases it will be implemented
//  in SoC library or it will platform specific.
//
UINT32 OALPCICfgRead(
   UINT32 busId, OAL_PCI_LOCATION pciLoc, UINT32 offset, UINT32 size,
   VOID *pData
);

//------------------------------------------------------------------------------
//
//  Function:  OALPCICfgWrite
//
//  This function write PCI configuration space at location specified by pciLoc.
//  This routine is hardware dependend and in most cases it will be implemented
//  in SoC library or it will be platform specific.
//
UINT32 OALPCICfgWrite(
    UINT32 busId, OAL_PCI_LOCATION pciLoc, UINT32 offset, UINT32 size,
    VOID *pData
);

//------------------------------------------------------------------------------
//
//  Function:  OALPCIPowerOff
//
//  This function is called from power module to prepare PCI hardware for
//  power-off/suspend (PCI bus configuration is/should be saved by PCI bus
//  driver). This function only should power off PCI clock and other hardware
//  related to PCI support.
//
VOID OALPCIPowerOn(UINT32 busId);

//------------------------------------------------------------------------------
//
//  Function:  OALPCIPowerOn
//
//  This function is called from power module to restore PCI hardware after
//  power-off/suspend (PCI bus configuration is/should be restored by PCI bus
//  driver). This function should power on PCI hardware, restore PCI clock and
//  issue PCI bus reset.
//
VOID OALPCIPowerOff(UINT32 busId);

//------------------------------------------------------------------------------
//
//  Function:  OALPCITransBusAddress
//
//  This function translates PCI bus relative address to CPU address space.
//
BOOL OALPCITransBusAddress(
    UINT32 busId, UINT64 busAddress, UINT32 *pAddressSpace,
    UINT64 *pSystemAddress
);

//------------------------------------------------------------------------------
//
//  Function:  OALPCIConfig
//
//  This function provide simple PCI bus configuration. It configures PCI bus
//  device resources. By default function configure all bridges and devices
//  specified in location list (pciLocCount, aPciLoc). When pciLocCount is
//  set to -1 all devices are configured. In most cases only device used by
//  KITL should be configured (configuration doesn't optimize resource use).
//
BOOL OALPCIConfig(
    UINT32 busId, UINT32 memBase, UINT32 memSize, UINT32 ioBase, UINT32 ioSize,
    UINT32 pciLocCount, OAL_PCI_LOCATION *aPciLoc
);

#define OAL_PCI_CONFIG_ALL      (-1)

//------------------------------------------------------------------------------
//
//  Function:  OALPCIRegisterAsUsed
//
//  This fuction register KITL device in WinCE OS registry as used.
//
BOOL OALPCIRegisterAsUsed(UINT32 busId, OAL_PCI_LOCATION pciLoc);

//------------------------------------------------------------------------------
//
//  Functions:  OALPCIGetXxx
//
//  Helper functions used in configuration/KITL to read specific configuration
//  registers.
//
UINT32 OALPCIGetId(UINT32 busId, OAL_PCI_LOCATION pciLoc);
UINT32 OALPCIGetMBAR(UINT32 busId, OAL_PCI_LOCATION pciLoc, UINT32 index);
UINT32 OALPCIGetInterruptPin(UINT32 busId, OAL_PCI_LOCATION pciLoc);
UINT8  OALPCIGetHeaderType(UINT32 busId, OAL_PCI_LOCATION pciLoc);
UINT8  OALPCIGetSecBusNum(UINT32 busId, OAL_PCI_LOCATION pciLoc);
UINT8  OALPCIGetSubBusNum(UINT32 busId, OAL_PCI_LOCATION pciLoc);

//------------------------------------------------------------------------------
//
//  Functions:  OALPCIFindNextId
//
//  This function searches the PCI bus for cards that match the Vendor/Device
//  Id. Search starts from position next to actual position on *pPciLoc. When
//  device with given Id is found function return TRUE. Device location is in
//  *pPciLoc.
//  
BOOL OALPCIFindNextId(
    UINT32 busId, UINT32 deviceId, OAL_PCI_LOCATION *pPciLoc
);

//------------------------------------------------------------------------------
//
//  Functions:  OALPCIDumpXxx
//
//  Helper dump functions. They dump device/bus configuration space.
//
VOID OALPCIDumpConfig(UINT32 busId, OAL_PCI_LOCATION pciLoc);
VOID OALPCIDump(UINT32 busId, BOOL full);

//------------------------------------------------------------------------------

#if __cplusplus
}
#endif

#endif // __OAL_PCI_H
