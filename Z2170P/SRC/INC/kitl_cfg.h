// All rights reserved ADENEO EMBEDDED 2010
// Copyright (c) 2007, 2008 BSQUARE Corporation. All rights reserved.

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
//  File:  kitl_cfg.h
//
#ifndef __KITL_CFG_H
#define __KITL_CFG_H

//------------------------------------------------------------------------------
// Prototypes for SMSC LAN911x

BOOL   LAN911XInit(UINT8 *pAddress, UINT32 offset, UINT16 mac[3]);
UINT16 LAN911XSendFrame(UINT8 *pBuffer, UINT32 length);
UINT16 LAN911XGetFrame(UINT8 *pBuffer, UINT16 *pLength);
VOID   LAN911XEnableInts();
VOID   LAN911XDisableInts();

VOID LAN911XCurrentPacketFilter(UINT32 filter);
BOOL LAN911XMulticastList(UINT8 *pAddresses, UINT32 count);

#define BSP_ETHDRV_LAN911X   { \
    LAN911XInit, NULL, NULL, LAN911XSendFrame, LAN911XGetFrame, \
    LAN911XEnableInts, LAN911XDisableInts, \
    NULL, NULL,  LAN911XCurrentPacketFilter, LAN911XMulticastList \
}

//------------------------------------------------------------------------------

OAL_KITL_ETH_DRIVER g_kitlEthLan911x = BSP_ETHDRV_LAN911X;
OAL_KITL_ETH_DRIVER g_kitlUsbRndis = OAL_ETHDRV_RNDIS;

OAL_KITL_DEVICE g_kitlDevices[] = {
	{ 
        L"LAN9115 MAC", Internal, BSP_LAN9115_REGS_PA, 
        0, OAL_KITL_TYPE_ETH, &g_kitlEthLan911x
    },  
	{
        L"USBFn RNDIS ", Internal, OMAP_USBHS_REGS_PA, 
        0, OAL_KITL_TYPE_ETH, &g_kitlUsbRndis
    },
	{
        NULL, 0, 0, 0, 0, NULL
    }
};    


#endif
