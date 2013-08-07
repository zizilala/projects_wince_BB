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
//  File:  register.c
//
#include <windows.h>
#include <ceddk.h>
#include <pcireg.h>
#include <oal.h>

//------------------------------------------------------------------------------
//
//  Function:  OALPCIRegisterAsUsed
//
BOOL OALPCIRegisterAsUsed(UINT32 busId, OAL_PCI_LOCATION pciLoc)
{
    BOOL rc = FALSE;
    PCI_COMMON_CONFIG pciCfg;
    PCI_REG_INFO pciInfo;

    OALMSG(OAL_KITL&&OAL_FUNC, (
        L"+OALPCIRegisterAsUsed(%d, %d/%d/%d)\r\n", busId, pciLoc.bus,
        pciLoc.dev, pciLoc.fnc
    ));

    // First read all device configuration space
    if (OALPCICfgRead(
        busId, pciLoc, 0, sizeof(pciCfg), &pciCfg) != sizeof(pciCfg)
    ) {
        OALMSG(OAL_ERROR, (
            L"ERROR:OALPCIRegisterAsUsed: Device config space read failed\r\n"
        ));            
        goto cleanUp;
    }  

    // Fill info structure
    PCIInitInfo(
       L"Drivers\\BuiltIn\\PCI\\Instance\\KITL", pciLoc.bus, pciLoc.dev, 
       pciLoc.fnc, pciCfg.u.type0.InterruptLine, &pciCfg, &pciInfo
    );

    // And write it to registry
    PCIReg(&pciInfo);

    // We are done
    rc = TRUE;

cleanUp:
    OALMSG(OAL_KITL&&OAL_FUNC, (L"-OALPCIRegisterAsUsed(rc = %d)\r\n", rc));
    return rc;
}

//------------------------------------------------------------------------------
