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
//  File:  device.c
//
#include <windows.h>
#include <nkintr.h>
#include <oal.h>
#include <oal_kitl.h>

//------------------------------------------------------------------------------
//
//  Function:  OALKitlPCIInfo
#ifdef KITL_PCI
static VOID OALKitlPCIInfo(
    DEVICE_LOCATION *pDevLoc, UINT32 *pId,  UINT64 *pAddress, UINT32 *pIrqPin
) {
    PCI_COMMON_CONFIG config;
    OAL_DDK_PARAMS params;

    memset(&config, 0xFF, sizeof(config));
    *pId = 0xFFFFFFFF;
    *pAddress = 0;
    *pIrqPin = 0;
    
    params.function = IOCTL_OAL_READBUSDATA;
    params.rc = 0;
    params.busData.devLoc = *pDevLoc;
    params.busData.offset = 0;
    params.busData.length = sizeof(config);
    params.busData.pBuffer = &config;

    if (!OEMIoControl(
        IOCTL_HAL_DDK_CALL, &params, sizeof(params), NULL, 0, NULL
    ) || !params.rc) goto cleanUp;

    *pId = config.VendorID | (config.DeviceID << 16);
    *pAddress = config.u.type0.BaseAddresses[0];
    *pIrqPin = config.u.type0.InterruptPin;
    
cleanUp:    
    return;
}
#endif

//------------------------------------------------------------------------------
//
//  Function:  OALKitlFindDevice
//
//  This function find KITL device at specified location in devices table.
//  Depending on bus type it read device address and interrupt pin and translate
//  it to system address space and then to kernel virtual address.
//
OAL_KITL_DEVICE* OALKitlFindDevice(
    DEVICE_LOCATION *pDevLoc, OAL_KITL_DEVICE *pDevice
) {
    BOOL rc = FALSE;
    UINT32 space, id = 0xFFFFFFFF;
    UINT64 address;
#ifndef BUILDING_BOOTLOADER // Not building boot loader
    OAL_DDK_PARAMS params;
#endif
    
    KITL_RETAILMSG(ZONE_KITL_OAL, (
        "+OALKitlFindDevice(%d/%d/%08x, 0x%08x)\r\n",
        pDevLoc->IfcType, pDevLoc->BusNumber, pDevLoc->LogicalLoc, pDevice
    ));

    // Look for driver in list
    while (pDevice->name != NULL && !rc) {

        // Is it our device? Then move
        if (pDevLoc->IfcType != pDevice->ifcType) {
            // Move to next driver
            pDevice++;
            continue;
        }            

        // Identify device and read its address/interrupt depends on bus type
        switch (pDevLoc->IfcType) {
        case InterfaceTypeUndefined:
            if (pDevLoc->LogicalLoc != pDevice->id) break;
            address = pDevLoc->LogicalLoc;
            rc = TRUE;
            break;
        case Internal:
            if (pDevLoc->LogicalLoc != pDevice->id) break;
            address = pDevLoc->LogicalLoc;
            rc = TRUE;
            break;
#ifdef KITL_PCI            
        case PCIBus:
            OALKitlPCIInfo(pDevLoc, &id, &address, &pDevLoc->Pin);
            if (id != pDevice->id) break;
            rc = TRUE;
            break;
#endif
        }

        // If we don't identify device skip it
        if (!rc) {
            pDevice++;
            continue;
        }

        // When interface type is undefined physical address
        // is equal to logical, so break loop
        if (pDevLoc->IfcType == InterfaceTypeUndefined) {
            pDevLoc->PhysicalLoc = (VOID*)address;
            break;
        }

        // Translate bus address, if it fails skip device
        // Are we in IO space (1) or memory space (0)?
        space = (UINT32)address & 0x1;

        // Mask off the lowest bit; it just indicates which space we're in.  This isn't
        // actually part of the address
        address &= ~0x1;

#ifdef BUILDING_BOOTLOADER
        if (!OALIoTransBusAddress(
            pDevLoc->IfcType, pDevLoc->BusNumber, address, &space, &address
        )) {
            rc = FALSE;
            pDevice++;
            continue;
        }

        // If address has address above 32bit address space skip device
        if ((address >> 32) != 0) {
            rc = FALSE;
            pDevice++;
            continue;
        }


        if (space == 0) {
            // Do mapping to virtual address for memory space
            pDevLoc->PhysicalLoc = OALPAtoVA((UINT32)address, FALSE);
        }
        else
        {
            // We're in IO space, no mapping necessary
            UINT8* pAddress = (UINT8*)address;
            pDevLoc->PhysicalLoc = (VOID*)address;
        }
#else // Not building boot loader
        params.function = IOCTL_OAL_TRANSBUSADDRESS;
        params.transAddress.ifcType = pDevLoc->IfcType;
        params.transAddress.busNumber = pDevLoc->BusNumber;
        params.transAddress.space = space;
        params.transAddress.address = address;
        if (!OEMIoControl(
            IOCTL_HAL_DDK_CALL, &params, sizeof(params), NULL, 0, NULL
        ) || !params.rc) {
            rc = FALSE;
            pDevice++;
            continue;
        }
        address = params.transAddress.address;
        space = params.transAddress.space;

        // If address has address above 32bit address space skip device
        if ((address >> 32) != 0) {
            rc = FALSE;
            pDevice++;
            continue;
        }

        // Do mapping to virtual address for memory space
        if (space == 0) {
            UINT32 offset;
            UINT8 *pAddress;
            UINT32 pa = (UINT32)address;
            
            offset = pa & (VM_PAGE_SIZE - 1);
            pa &= ~(VM_PAGE_SIZE - 1);
            pAddress = NKPhysToVirt(pa >> 8, FALSE);
            pAddress += offset;
            
            pDevLoc->PhysicalLoc = pAddress;
        }
        else
        {
            // We're in IO space, no mapping necessary
            UINT8* pAddress = (UINT8*)address;
            pDevLoc->PhysicalLoc = (VOID*)address;
        }
#endif
        // We get all we need
        break;
    }

    // Return NULL if driver wasn't found
    if (!rc) pDevice = NULL;

    KITL_RETAILMSG(ZONE_KITL_OAL, (
        "-OALKitlFindDevice(pDevice = 0x%08x(%s), PhysicalLoc = 0x%08x)\r\n",
        pDevice, (pDevice != NULL) ? pDevice->name : L"", pDevLoc->PhysicalLoc
    ));
    return pDevice;
}


//------------------------------------------------------------------------------
//
//  Function:  OALKitlDeviceName
//
LPCWSTR OALKitlDeviceName(
    DEVICE_LOCATION *pDevLoc, OAL_KITL_DEVICE *pDevice
) {
    BOOL rc = FALSE;
    static WCHAR name[64];
    LPCWSTR pName = NULL;
    
    KITL_RETAILMSG(ZONE_KITL_OAL, (
        "+OALKitlDeviceName(%d/%d/%08x, 0x%08x)\r\n",
        pDevLoc->IfcType, pDevLoc->BusNumber, pDevLoc->LogicalLoc, pDevice
    ));

    // Look for driver in list
    while (pDevice->name != NULL && !rc) {

        // Is it our device? Then move
        if (pDevLoc->IfcType != pDevice->ifcType) {
            // Move to next driver
            pDevice++;
            continue;
        }            

        // Identify device and read its address/interrupt depends on bus type
        switch (pDevLoc->IfcType) {
        case Internal:
            if (pDevLoc->LogicalLoc != pDevice->id) break;
            pName = pDevice->name;
            rc = TRUE;
            break;
#ifdef KITL_PCI            
        case PCIBus:
            {
                UINT32 id;
                OAL_PCI_LOCATION pciLoc;

                pciLoc = *(OAL_PCI_LOCATION*)&pDevLoc->LogicalLoc;
                id = OALPCIGetId(pDevLoc->BusNumber, pciLoc);
                if (id != pDevice->id) break;
                OALLogPrintf(
                    name, sizeof(name)/sizeof(WCHAR),
                    L"%s @ id %d bus %d dev %d fnc %d",
                    pDevice->name, pDevLoc->BusNumber, pciLoc.bus, pciLoc.dev,
                    pciLoc.fnc
                );
                pName = name;
                rc = TRUE;
            }                
            break;
#endif
        }

        // If we don't identify device skip it
        if (!rc) {
            pDevice++;
            continue;
        }
            
        // We get all we need
        break;
    }


    KITL_RETAILMSG(ZONE_KITL_OAL, (
        "-OALKitlDeviceName(name = 0x%08x('%s')\r\n", pName, pName
    ));
    return pName;
}


//------------------------------------------------------------------------------
//
//  Function:  OALKitlDeviceType
//
OAL_KITL_TYPE OALKitlDeviceType(
        DEVICE_LOCATION *pDevLoc, OAL_KITL_DEVICE *pDevice
) {
    BOOL rc = FALSE;
    OAL_KITL_TYPE type = OAL_KITL_TYPE_NONE;
    
    KITL_RETAILMSG(ZONE_KITL_OAL, (
        "+OALKitlDeviceType(%d/%d/%08x, 0x%08x)\r\n",
        pDevLoc->IfcType, pDevLoc->BusNumber, pDevLoc->LogicalLoc, pDevice
    ));

    // Look for driver in list
    while (pDevice->name != NULL && !rc) {

        // Is it our device? Then move
        if (pDevLoc->IfcType != pDevice->ifcType) {
            // Move to next driver
            pDevice++;
            continue;
        }            

        // Identify device and read its address/interrupt depends on bus type
        switch (pDevLoc->IfcType) {
        case Internal:
            if (pDevLoc->LogicalLoc != pDevice->id) break;
            type = pDevice->type;
            rc = TRUE;
            break;
#ifdef KITL_PCI            
        case PCIBus:
            {
                UINT32 id;
                id = OALPCIGetId(
                    pDevLoc->BusNumber, *(OAL_PCI_LOCATION*)&pDevLoc->LogicalLoc
                );
                if (id != pDevice->id) break;
                type = pDevice->type;
                rc = TRUE;
            }                
            break;
#endif
        }

        // If we don't identify device skip it
        if (!rc) {
            pDevice++;
            continue;
        }
            
        // We get all we need
        break;
    }


    KITL_RETAILMSG(ZONE_KITL_OAL, ("-OALKitlDeviceType(type = %d)\r\n", type));
    return type;
}

//------------------------------------------------------------------------------
