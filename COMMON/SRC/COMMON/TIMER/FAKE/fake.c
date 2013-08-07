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
//  File:  fake.c
//
//  This file contains fake/busy loop implementation if OALCPUIdle function. 
//  It is assumed to be used in development only.
//
#include <windows.h>
#include <nkintr.h>
#include <oal.h>
 
//------------------------------------------------------------------------------
//
//  Global:  g_oalLastSysIntr
//
//  This global variable is set by fake version of interrupt/timer handler
//  to last SYSINTR value.
//
volatile UINT32 g_oalLastSysIntr;


//------------------------------------------------------------------------------
//
//  Function:   OALCPUIdle
//
//  This Idle function implements a busy idle. It is intend to be used only
//  in development (when CPU doesn't support idle mode it is better to stub
//  OEMIdle function instead use this busy loop). The busy wait is cleared by
//  an interrupt from interrupt handler setting the g_oalLastSysIntr.
//

VOID OALCPUIdle()
{
    // Clear last SYSINTR global value
    g_oalLastSysIntr = SYSINTR_NOP;

    INTERRUPTS_ON();
    // Wait until interrupt handler set interrupt flag
    while (g_oalLastSysIntr == SYSINTR_NOP);
    INTERRUPTS_OFF();
}

//------------------------------------------------------------------------------
