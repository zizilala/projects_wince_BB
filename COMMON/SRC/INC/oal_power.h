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
//  Header: oal_power.h
//
#ifndef __OAL_POWER_H
#define __OAL_POWER_H

#if __cplusplus
extern "C" {
#endif

//------------------------------------------------------------------------------

extern volatile UINT32 g_oalWakeSource;

//------------------------------------------------------------------------------

VOID OEMPowerOff();
BOOL OALPowerWakeSource(UINT32 sysIntr);

BOOL OALIoCtlHalPresuspend(UINT32, VOID*, UINT32, VOID*, UINT32, UINT32*);
BOOL OALIoCtlHalEnableWake(UINT32, VOID*, UINT32, VOID*, UINT32, UINT32*);
BOOL OALIoCtlHalDisableWake(UINT32, VOID*, UINT32, VOID*, UINT32, UINT32*);
BOOL OALIoCtlHalGetWakeSource(UINT32, VOID*, UINT32, VOID*, UINT32, UINT32*);

//------------------------------------------------------------------------------

VOID OALCPUPowerOff();
VOID OALCPUPowerOn();

VOID BSPPowerOff();
VOID BSPPowerOn();

//------------------------------------------------------------------------------

#if __cplusplus
}
#endif

#endif
