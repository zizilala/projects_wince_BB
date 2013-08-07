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
//  File:  registry.c
//
#include <windows.h>
#include <ceddk.h>
#include <oal.h>

#ifdef KITL_PCI
#include <PCIReg.h>

//------------------------------------------------------------------------------
//
//  Function:  PCIReadBARs
//
//  Fill out memory and I/O resource info.
//  The flow is the same as that of CEPC.
//  Make use of abstract functions to read/write PCI configuration.
static BOOL
PCIReadBARs(
    DWORD busNumber,
    PPCI_REG_INFO pInfo
    )
{   
    DWORD NumberOfRegs;
    DWORD Offset;
    DWORD i;
    DWORD BaseAddress;
    DWORD Size;
    DWORD Reg;
    DWORD IoIndex = 0;
    DWORD MemIndex = 0;
    OAL_PCI_LOCATION pciLoc;
    pciLoc.fnc = (UINT8)pInfo->Function;
    pciLoc.dev = (UINT8)pInfo->Device;
    pciLoc.bus = (UINT8)pInfo->Bus;
    busNumber >>= 8; // bits 8-15 are host-to-PCI-bridge bus number

    // Determine number of BARs to examine from header type
    switch (pInfo->Cfg.HeaderType & ~PCI_MULTIFUNCTION) {
    case PCI_DEVICE_TYPE:
        NumberOfRegs = PCI_TYPE0_ADDRESSES;
        break;

    case PCI_BRIDGE_TYPE:
        NumberOfRegs = PCI_TYPE1_ADDRESSES;
        break;

    case PCI_CARDBUS_TYPE:
        NumberOfRegs = PCI_TYPE2_ADDRESSES;
        break;

    default:
        return FALSE;
    }
        
    for (i = 0, Offset = 0x10; i < NumberOfRegs; i++, Offset += 4) {
        // Get base address register value
        Reg = pInfo->Cfg.u.type0.BaseAddresses[i];

        // Get size info
        BaseAddress = 0xFFFFFFFF;
        OALPCICfgWrite(busNumber, pciLoc, Offset, sizeof(BaseAddress), &BaseAddress);
        OALPCICfgRead(busNumber, pciLoc, Offset, sizeof(BaseAddress), &BaseAddress);
        OALPCICfgWrite(busNumber, pciLoc, Offset, sizeof(Reg), &Reg);

        // Re-adjust BaseAddress if upper 16-bits are 0 (this happens on some devices that don't follow
        // the PCI spec, like the Intel UHCI controllers)
        if (((BaseAddress & 0xFFFFFFFC) != 0) && ((BaseAddress & 0xFFFF0000) == 0)) {
            BaseAddress |= 0xFFFF0000;
        }
        
        if (Reg & 1) {
            // IO space
            Size = ~(BaseAddress & 0xFFFFFFFC);

            if ((BaseAddress != 0) && (BaseAddress != 0xFFFFFFFF) && (((Size + 1) & Size) == 0)) {
                // BAR has valid format (consecutive high 1's and consecutive low 0's)
                pInfo->IoLen.Reg[IoIndex] = Size + 1;
                pInfo->IoLen.Num++;
                pInfo->IoBase.Reg[IoIndex++] = Reg & 0xFFFFFFFC;
                pInfo->IoBase.Num++;
            } else {
                // BAR invalid => skip to next one
                continue;
            }
        } else {
            // Memory space
            // TODO: don't properly handle the MEM20 case
            Size = ~(BaseAddress & 0xFFFFFFF0);
            
            if ((BaseAddress != 0) && (BaseAddress != 0xFFFFFFFF) && (((Size + 1) & Size) == 0)) {
                // BAR has valid format (consecutive high 1's and consecutive low 0's)
                pInfo->MemLen.Reg[MemIndex] = Size + 1;
                pInfo->MemLen.Num++;
                pInfo->MemBase.Reg[MemIndex++] = Reg & 0xFFFFFFF0;
                pInfo->MemBase.Num++;
            } else {
                // BAR invalid => skip to next one
                continue;
            }
        }

        //
        // check for 64 bit device - BAR is twice as big
        //
        if ((pInfo->Cfg.u.type0.BaseAddresses[i] & 0x7) == 0x4) {
            // 64 bit device - BAR is twice as wide - zero out high part
            Offset += 4;
            i++;
        }
    }

    return TRUE;
}

//------------------------------------------------------------------------------
//
//  Function:  RegisterKITL
//
//  This function is used to update registry for KITL PCI info.
//  It is almost the same as the function "OALPCIRegisterAsUsed" except calling the function "PCIReadBARs".
//  It will fill up complete info on registry, includes all BAR's address and length.
void RegisterKITL(void)
{

    PCI_REG_INFO KitlPCIInfo;
    DEVICE_LOCATION devLoc;

    PCI_COMMON_CONFIG pciCfg;
    OAL_PCI_LOCATION pciLoc;

    KITL_RETAILMSG(ZONE_KITL_OAL, ("+ RegisterKITL \r\n"));

    OALKitlGetDevLoc(&devLoc);
    pciLoc = *(OAL_PCI_LOCATION*)&devLoc.LogicalLoc;

    // First read all device configuration space
    OALPCICfgRead(devLoc.BusNumber, pciLoc, 0, sizeof(pciCfg), &pciCfg);


    // Fill info structure
    PCIInitInfo(
       L"Drivers\\BuiltIn\\PCI\\Instance\\KITL", pciLoc.bus, pciLoc.dev, 
       pciLoc.fnc, pciCfg.u.type0.InterruptLine, &pciCfg, &KitlPCIInfo
    );

    PCIReadBARs(devLoc.BusNumber, &KitlPCIInfo);
    PCIReg(&KitlPCIInfo);
    KITL_RETAILMSG(ZONE_KITL_OAL, ("- RegisterKITL \r\n"));

}
#endif

//------------------------------------------------------------------------------
//
//  Function:  OALKitlInitRegistry
//
//  This function is called as part of IOCTL_HAL_INITREGISTRY to update
//  registry with information about KITL device. This must be done to avoid
//  loading Windows CE driver in case that it is part of image. On image
//  without KITL is this function replaced with empty stub.
//
//  This implementation works only for PCI KITL devices. It probably should
//  be replaced with platform specific code if KITL device is on internal
//  bus.
//
VOID OALKitlInitRegistry()
{
    DEVICE_LOCATION devLoc;
    
    KITL_RETAILMSG(ZONE_KITL_OAL, ("+OALKitlInitRegistry\r\n"));

    if (!OALKitlGetDevLoc(&devLoc)) goto cleanUp;
    
    switch (devLoc.IfcType) {
#ifdef KITL_PCI        
    case PCIBus:
            RegisterKITL();
        break;
#endif
    }        

cleanUp:
    KITL_RETAILMSG(ZONE_KITL_OAL, ("-OALKitlInitRegistry\r\n"));
}

//------------------------------------------------------------------------------
