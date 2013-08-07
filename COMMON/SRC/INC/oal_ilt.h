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
//  Header: oal_ilt.h
//
//  This header define OAL interrupt latency timing module. Module code
//  implements functionality related to interrupt latency timing functionality.
//  It is used usually in development process.
//
//  Interaction with kernel/OS:
//      * IOCTL_HAL_ILTIMING
//
//  Interaction with other OAL modules
//      * g_oalILT  (interrupt handlers)
//      * OALTimerHiResTickSinceSysTick (timer)
//
#ifndef __OAL_ILT_H
#define __OAL_ILT_H

#if __cplusplus
extern "C" {
#endif

//------------------------------------------------------------------------------
//
//  Type:   OAL_ILT_STATE
//
//  Timer module component control block - it encapsulate all OAL internal
//  module variables which can be used by other modules.
//
typedef struct {
    BOOL active;                     // Is IL timing active?
    UINT16 interrupts;               // IL interrupt counter
    UINT32 isrTime1;                 // IL start of ISR
    UINT32 isrTime2;                 // IL end of ISR
    UINT32 savedPC;                  // IL saved PC
    UINT32 counter;                  // IL counter
    UINT32 counterSet;               // IL counter set value
} OAL_ILT_STATE, *POAL_ILT_STATE;

//------------------------------------------------------------------------------
//
//  Extern:     g_oalILT
//
//  Exports the OAL ILT control block. This is global instance of internal
//  ILT module variables.
//
extern OAL_ILT_STATE g_oalILT;

//------------------------------------------------------------------------------
//
//  Function:   OALIoCtlHalILTiming
//
//  This function is called by OEMIoControl for IOCTL_HAL_ILTIMING code.
//
BOOL OALIoCtlHalILTiming(UINT32, VOID*, UINT32, VOID*, UINT32, UINT32*);

//------------------------------------------------------------------------------

#if __cplusplus
}
#endif

#endif
