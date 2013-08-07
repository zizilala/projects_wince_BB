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
//  Header:  oal_ethdrv.h
//
//  This header file contains prototypes for ethernet KITL devices drivers
//  implemented in common library. For function prototypes see oal_kilt.h
//  header file. Device driver for SoC specific silicon are defined in
//  corresponding xxx_ethdrv.h header file in SoC subdirectory.
//
#ifndef __OAL_ETHDRV_H
#define __OAL_ETHDRV_H

#if __cplusplus
extern "C" {
#endif

//------------------------------------------------------------------------------
// Include ethdbg.h until we remove prototypes from it...

#include <halether.h>

#ifndef _HALETHER_H_

//------------------------------------------------------------------------------
// Prototypes for NE2000

BOOL   NE2000Init(UINT8 *pAddress, UINT32 offset, UINT16 mac[3]);
UINT16 NE2000SendFrame(UINT8 *pData, UINT32 length);
UINT16 NE2000GetFrame(UINT8 *pData, UINT16 *pLength);
VOID   NE2000EnableInts();
VOID   NE2000DisableInts();
VOID   Ne2000CurrentPacketFilter(UINT32 filter);
BOOL   NE2000MulticastList(UINT8 *pAddresses, UINT32 count);

#define OAL_ETHDRV_NE2000       { \
    NE2000Init, NULL, NULL, NE2000SendFrame, NE2000GetFrame, \
    NE2000EnableInts, NE2000DisableInts, \
    NULL, NULL, Ne2000CurrentPacketFilter, NE2000MulticastList \
}

//------------------------------------------------------------------------------
// Prototypes for RTL8139

BOOL   RTL8139InitDMABuffer(UINT32 address, UINT32 size);
BOOL   RTL8139Init(UINT8 *pAddress, UINT32 offset, UINT16 mac[3]);
UINT16 RTL8139SendFrame(UINT8 *pData, UINT32 length);
UINT16 RTL8139GetFrame(UINT8 *pData, UINT16 *pLength);
VOID   RTL8139EnableInts();
VOID   RTL8139DisableInts();
VOID   RTL8139CurrentPacketFilter(UINT32 filter);
BOOL   RTL8139MulticastList(UINT8 *pAddresses, UINT32 count);

#define OAL_ETHDRV_RTL8139      { \
    RTL8139Init, RTL8139InitDMABuffer, NULL, RTL8139SendFrame, RTL8139GetFrame, \
    RTL8139EnableInts, RTL8139DisableInts, \
    NULL, NULL, RTL8139CurrentPacketFilter, RTL8139MulticastList \
}

//------------------------------------------------------------------------------
// Prototypes for DP83815

BOOL   DP83815InitDMABuffer(UINT32 address, UINT32 size);
BOOL   DP83815Init(UINT8 *pAddress, UINT32 offset, UINT16 mac[3]);
UINT16 DP83815SendFrame(UINT8 *pData, UINT32 length);
UINT16 DP83815GetFrame(UINT8 *pData, UINT16 *pLength);
VOID   DP83815EnableInts();
VOID   DP83815DisableInts();
VOID   DP83815SetOptions (UINT32 filter);
BOOL   DP83815MulticastList(UINT8 *pAddresses, UINT32 count);

#define OAL_ETHDRV_DP83815     { \
    DP83815Init, DP83815InitDMABuffer, NULL, DP83815SendFrame, \
    DP83815GetFrame, DP83815EnableInts, DP83815DisableInts, \
    NULL, NULL, DP83815SetOptions, DP83815MulticastList \
}

//------------------------------------------------------------------------------
// Prototypes for 3C90X

BOOL   D3C90XInitDMABuffer(UINT32 address, UINT32 size);
BOOL   D3C90XInit(UINT8 *pAddress, UINT32 offset, UINT16 mac[3]);
UINT16 D3C90XSendFrame(UINT8 *pBuffer, UINT32 length);
UINT16 D3C90XGetFrame(UINT8 *pBuffer, UINT16 *pLength);
VOID   D3C90XEnableInts();
VOID   D3C90XDisableInts();
VOID   D3C90XCurrentPacketFilter(UINT32 filter);
BOOL   D3C90XMulticastList(UINT8 *pAddresses, UINT32 count);

#define OAL_ETHDRV_3C90X     { \
    D3C90XInit, D3C90XInitDMABuffer, NULL, D3C90XSendFrame, \
    D3C90XGetFrame, D3C90XEnableInts, D3C90XDisableInts, \
    NULL, NULL, D3C90XCurrentPacketFilter, D3C90XMulticastList \
}

//------------------------------------------------------------------------------
// Prototypes for RNDIS

BOOL   RndisInitDMABuffer(UINT32 address, UINT32 size);
BOOL   HostMiniInit(UINT8 *pAddress, UINT32 offset, UINT16 mac[3]);
UINT16 RndisEDbgSendFrame(UINT8 *pbData, UINT32 length);
UINT16 RndisEDbgGetFrame(UINT8 *pbData, UINT16 *pLength);
VOID   RndisEnableInts();
VOID   RndisDisableInts();
VOID   RndisPowerOff();
VOID   RndisPowerOn();
VOID   RndisCurrentPacketFilter(UINT32 filter);
BOOL   RndisMulticastList(UINT8 *pAddresses, UINT32 count);

#define OAL_ETHDRV_RNDIS    { \
    HostMiniInit, NULL, NULL, RndisEDbgSendFrame, RndisEDbgGetFrame, \
    RndisEnableInts, RndisDisableInts, NULL, NULL, \
    RndisCurrentPacketFilter, RndisMulticastList \
}

#else // _HALETHER_H

#define OAL_ETHDRV_NE2000       { \
    (OAL_KITLETH_INIT)NE2000Init, \
    NULL, \
    NULL, \
    (OAL_KITLETH_SEND_FRAME)NE2000SendFrame, \
    NE2000GetFrame, \
    NE2000EnableInts, \
    NE2000DisableInts, \
    NULL, NULL, \
    (OAL_KITLETH_CURRENT_PACKET_FILTER) Ne2000CurrentPacketFilter, \
    (OAL_KITLETH_MULTICAST_LIST) NE2000MulticastList \
}

#define OAL_ETHDRV_RTL8139      { \
    (OAL_KITLETH_INIT)RTL8139Init, \
    (OAL_KITLETH_INIT_DMABUFFER)RTL8139InitDMABuffer, \
    NULL, \
    (OAL_KITLETH_SEND_FRAME)RTL8139SendFrame, \
    RTL8139GetFrame, \
    RTL8139EnableInts, \
    RTL8139DisableInts, \
    NULL, \
    NULL, \
    (OAL_KITLETH_CURRENT_PACKET_FILTER)RTL8139CurrentPacketFilter, \
    (OAL_KITLETH_MULTICAST_LIST)RTL8139MulticastList \
}

#define OAL_ETHDRV_DP83815     { \
    (OAL_KITLETH_INIT)DP83815Init, \
    (OAL_KITLETH_INIT_DMABUFFER)DP83815InitDMABuffer, \
    NULL, \
    (OAL_KITLETH_SEND_FRAME)DP83815SendFrame, \
    DP83815GetFrame, \
    DP83815EnableInts, \
    DP83815DisableInts, \
    NULL, NULL, \
    (OAL_KITLETH_CURRENT_PACKET_FILTER)DP83815SetOptions, \
    (OAL_KITLETH_MULTICAST_LIST)NULL \
}

#define OAL_ETHDRV_3C90X     { \
    (OAL_KITLETH_INIT)D3C90XInit, \
    (OAL_KITLETH_INIT_DMABUFFER)D3C90XInitDMABuffer, \
    NULL, \
    (OAL_KITLETH_SEND_FRAME)D3C90XSendFrame, \
    D3C90XGetFrame, \
    D3C90XEnableInts, \
    D3C90XDisableInts, \
    NULL, NULL, \
    (OAL_KITLETH_CURRENT_PACKET_FILTER)D3C90XCurrentPacketFilter, \
    (OAL_KITLETH_MULTICAST_LIST)D3C90XMulticastList \
}

#define OAL_ETHDRV_RNDIS    { \
    (OAL_KITLETH_INIT)HostMiniInit, \
    NULL, \
    NULL, \
    (OAL_KITLETH_SEND_FRAME)RndisEDbgSendFrame, \
    RndisEDbgGetFrame, \
    RndisEnableInts, \
    RndisDisableInts, \
    NULL, NULL, \
    (OAL_KITLETH_CURRENT_PACKET_FILTER)RndisCurrentPacketFilter, \
    (OAL_KITLETH_MULTICAST_LIST)RndisMulticastList \
}

#endif // _HALETHER_H

//------------------------------------------------------------------------------
// Prototypes for AM79C973

BOOL   AM79C973InitDMABuffer(UINT32 address, UINT32 size);
BOOL   AM79C973Init(UINT8 *pAddress, UINT32 offset, UINT16 mac[3]);
UINT16 AM79C973SendFrame(UINT8 *pbData, UINT32 length);
UINT16 AM79C973GetFrame(UINT8 *pbData, UINT16 *pLength);
VOID   AM79C973EnableInts();
VOID   AM79C973DisableInts();
VOID   AM79C973PowerOff();
VOID   AM79C973PowerOn();
VOID   AM79C973CurrentPacketFilter(UINT32 filter);
BOOL   AM79C973MulticastList(UINT8 *pAddresses, UINT32 count);

#define OAL_ETHDRV_AM79C973     { \
    AM79C973Init, AM79C973InitDMABuffer, NULL, AM79C973SendFrame, \
    AM79C973GetFrame, AM79C973EnableInts, AM79C973DisableInts, \
    AM79C973PowerOff, AM79C973PowerOn, \
    AM79C973CurrentPacketFilter, AM79C973MulticastList \
}

//------------------------------------------------------------------------------
// Prototypes for CS8900A

BOOL   CS8900AInit(UINT8 *pAddress, UINT32 offset, UINT16 mac[3]);
UINT16 CS8900ASendFrame(UINT8 *pBuffer, UINT32 length);
UINT16 CS8900AGetFrame(UINT8 *pBuffer, UINT16 *pLength);
VOID   CS8900AEnableInts();
VOID   CS8900ADisableInts();
VOID   CS8900ACurrentPacketFilter(UINT32 filter);
BOOL   CS8900AMulticastList(UINT8 *pAddresses, UINT32 count);

#define OAL_ETHDRV_CS8900A   { \
    CS8900AInit, NULL, NULL, CS8900ASendFrame, CS8900AGetFrame, \
    CS8900AEnableInts, CS8900ADisableInts, \
    NULL, NULL,  CS8900ACurrentPacketFilter, CS8900AMulticastList \
}

//------------------------------------------------------------------------------
// Prototypes for SMSC LAN91Cxxx

BOOL   LAN91CInit(UINT8 *pAddress, UINT32 offset, UINT16 mac[3]);
UINT16 LAN91CSendFrame(UINT8 *pBuffer, UINT32 length);
UINT16 LAN91CGetFrame(UINT8 *pBuffer, UINT16 *pLength);
VOID   LAN91CEnableInts();
VOID   LAN91CDisableInts();
VOID   LAN91CCurrentPacketFilter(UINT32 filter);
BOOL   LAN91CMulticastList(UINT8 *pAddresses, UINT32 count);

#define OAL_ETHDRV_LAN91C   { \
    LAN91CInit, NULL, NULL, LAN91CSendFrame, LAN91CGetFrame, \
    LAN91CEnableInts, LAN91CDisableInts, \
    NULL, NULL,  LAN91CCurrentPacketFilter, LAN91CMulticastList \
}

//------------------------------------------------------------------------------

#if __cplusplus
}
#endif

#endif
