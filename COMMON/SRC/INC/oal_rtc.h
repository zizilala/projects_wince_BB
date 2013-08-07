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
//  Header: oal_rtc.h
//
#ifndef __OAL_RTC_H
#define __OAL_RTC_H

#if __cplusplus
extern "C" {
#endif

//------------------------------------------------------------------------------
//
// File:        oal_rtc.h
//
// This header file defines functions, constant and macros used in RTC module.
//
// The module exports for Windows CE OS kernel:
//      * OEMIoControl/IOCTL_HAL_INIT_RTC
//      * OEMGetRealTime
//      * OEMSetRealTime
//      * OEMSetAlarmTime
//
//  The module export for OAL libraries:
//      -- N/A --
//
//  The module OAL dependencies:
//      * There should be static interrupt mapping for SYSINTR_RTC_ALARM set
//        for IRQ used for RTC alarm.
//
//------------------------------------------------------------------------------

BOOL OALIoCtlHalInitRTC(UINT32, VOID*, UINT32, VOID*, UINT32, UINT32*);

//------------------------------------------------------------------------------

#if __cplusplus
}
#endif

#endif
