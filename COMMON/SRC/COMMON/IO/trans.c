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
//  File:  trans.c
//
#include <windows.h>
#include <oal.h>


//------------------------------------------------------------------------------
//
//  Function:  OALIoTransBusAddress
//
BOOL OALIoTransBusAddress(
    INTERFACE_TYPE ifcType, UINT32 busNumber, UINT64 busAddress, 
    UINT32 *pAddressSpace, UINT64 *pSystemAddress
) {
    BOOL rc = FALSE;

    OALMSG(OAL_IO&&OAL_FUNC, (
        L"+OALIoTranslateBusAddress(%d, %d, 0x%08x%08x, %d)\r\n",
        ifcType, busNumber, (UINT32)(busAddress >> 32), (UINT32)busAddress, 
        *pAddressSpace
    ));

    if (pAddressSpace == NULL || pSystemAddress == NULL) goto cleanUp;
    
    switch (ifcType) {
    case Internal:
        *pSystemAddress = busAddress;
        *pAddressSpace = 0;
        rc = TRUE;
        break;
#ifdef OAL_IO_PCI
    case Isa:
    case PCIBus:
        rc = OALPCITransBusAddress(
            busNumber, busAddress, pAddressSpace, pSystemAddress
        );
        break;
#endif
#ifdef OAL_IO_PCMCIA
    case PCMCIABus: 
        rc = OALPCMCIATransBusAddress(
            busNumber, busAddress, pAddressSpace, pSystemAddress
        );
        break;
#endif            
    }

cleanUp:
    OALMSG(OAL_IO&&OAL_FUNC, (
        L"-OALTranslateBusAddress(addressSpace = %d, "
        L"systemAddress = 0x%08x%08x, rc = %d)\r\n", *pAddressSpace, 
        (UINT32)(*pSystemAddress >> 32), (UINT32)*pSystemAddress, rc
    ));      
    return rc;    
}


//------------------------------------------------------------------------------
//
//  Function:  OALIoTransSystemAddress
//
BOOL OALIoTransSystemAddress(
    INTERFACE_TYPE ifcType, UINT32 busNumber, UINT64 systemAddress, 
    UINT64 *pBusAddress
) {
    OALMSG(OAL_IO&&OAL_FUNC, (
        L"+OALTranslateSystemAddress(%d, %d, 0x%08x%08x)\r\n", 
        ifcType, busNumber, (UINT32)(systemAddress >> 32), (UINT32)systemAddress
    ));

    // Check parameters
    if (pBusAddress == NULL) return FALSE;

    // All buses can access system memory without offset
    *pBusAddress = systemAddress;
    
    OALMSG(OAL_IO&&OAL_FUNC, (
        L"-OALTranslateSystemAddress(busAddress = 0x%08x%08x, rc = 1)\r\n",
        (UINT32)(*pBusAddress >> 32), (UINT32)*pBusAddress
    ));
    return TRUE;
}

//------------------------------------------------------------------------------
