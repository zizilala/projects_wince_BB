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
//  File:  power.c
//
#include <windows.h>
#include <oal.h>


//------------------------------------------------------------------------------
//
//  Function:  OALIoBusPowerOff
//
BOOL OALIoBusPowerOff(INTERFACE_TYPE ifcType, UINT32 busNumber)
{
    BOOL rc = FALSE;

    OALMSG(OAL_IO&&OAL_FUNC, (
        L"+OALIoBusPowerOff(%d, %d)\r\n", ifcType, busNumber
    ));
    
    switch (ifcType) {
#ifdef OAL_IO_PCI
    case PCIBus:
        OALPCIPowerOff(busNumber);
        rc = TRUE;
        break;
#endif
#ifdef OAL_IO_PCMCIA
    case PCMCIABus:
        OALPCMCIAPowerOff(busNumber);
        rc = TRUE;
        break;
#endif
    }

    OALMSG(OAL_IO&&OAL_FUNC, (L"-OALIoBusPowerOff(rc = %d)\r\n", rc));
    return rc;
}


//------------------------------------------------------------------------------
//
//  Function:  OALIoBusPowerOn
//
BOOL OALIoBusPowerOn(INTERFACE_TYPE ifcType, UINT32 busNumber)
{
    BOOL rc = FALSE;

    OALMSG(OAL_IO&&OAL_FUNC, (
        L"+OALIoBusPowerOn(%d, %d)\r\n", ifcType, busNumber
    ));
    
    switch (ifcType) {
#ifdef OAL_IO_PCI
    case PCIBus:
        OALPCIPowerOn(busNumber);
        rc = TRUE;
        break;
#endif
#ifdef OAL_IO_PCMCIA
    case PCMCIABus:
        OALPCMCIAPowerOn(busNumber);
        rc = TRUE;
        break;
#endif
    }

    OALMSG(OAL_IO&&OAL_FUNC, (L"-OALIoBusPowerOn(rc = %d)\r\n", rc));
    return rc;
}

//------------------------------------------------------------------------------
