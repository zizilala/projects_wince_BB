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
//  Header: oal_pcmcia.h
//
//  This header define OAL PCMCIA module.
//
#ifndef __OAL_PCMCIA_H
#define __OAL_PCMCIA_H

#if __cplusplus
extern "C" {
#endif

//------------------------------------------------------------------------------
//
//  Function: OALPCMCIATransBusAddress 
//
//  This function translates PCMCIA bus relative address to processor address
//  space.
//
BOOL OALPCMCIATransBusAddress(
    UINT32 busId, UINT64 busAddress, UINT32 *pAddressSpace, 
    UINT64 *pSystemAddress
);

VOID OALPCMCIAPowerOn(UINT32 busId);
VOID OALPCMCIAPowerOff(UINT32 busId);

//------------------------------------------------------------------------------

#if __cplusplus
    }
#endif

#endif

