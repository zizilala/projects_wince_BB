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
//  File: cntcmp.c
//  
//  This file contains timer functions implementation for count/compare
//  counters to be used with variable system tick.
//
#include <windows.h>
#include <oal_log.h>
#include <oal_timer.h>

//------------------------------------------------------------------------------
//
//  Static:  g_timer
//
//  This structure contains timer internal variable. The period is value used
//  to timer recharge, margin value define maximal latency for timer recharge
//  without shift, base value is actual period base and compare is actual
//  period end. The period isn't changed by timer reduce/extend function.
//
static struct {
    UINT32 base;
    UINT32 compare;
} g_timer;

//------------------------------------------------------------------------------
//
//  Function:  OALTimerInitCount
//
//  This function initialize count/compare timer.
//
VOID OALTimerInitCount(UINT32 period)
{
    g_timer.base = OALTimerGetCount();
    g_timer.compare = g_timer.base + period;
    OALTimerSetCompare(g_timer.compare); 
}

//------------------------------------------------------------------------------
//
//  Function:  OALTimerRecharge
//
//  This function recharge count/compare timer. In case that we are late more
//  than margin value we will use count as new counter base. Under
//  normal conditions previous compare value stored in global variable is used.
//  Using global variable instead reading compare register allows compensate
//  offset when we reduce system tick to value which can cause hazard.
//
VOID OALTimerRecharge(UINT32 period, UINT32 margin)
{
    UINT32 count;

    count = OALTimerGetCount();
    // Check if recharge was called in required margin
    if ((INT32)(count - OALTimerGetCompare() - margin) < 0) {
        // Yes, base new period on previous (no shift)
        g_timer.base = g_timer.compare;
        g_timer.compare += period;
    } else {
        // No, base new period on actual counter value (shift)
        g_timer.base = count;
        g_timer.compare = g_timer.base + period;
    }
    OALTimerSetCompare(g_timer.compare); 
}

//------------------------------------------------------------------------------
//
//  Function:  OALTimerCountsSinceSysTick
//
INT32 OALTimerCountsSinceSysTick()
{
    return OALTimerGetCount() - g_timer.base;
}

//------------------------------------------------------------------------------
//
//  Function: OALTimerUpdate
//
//  This function is called to change length of actual system timer period.
//  When end of new period is closer to actual time than margin period end
//  is shifted by margin (but next period should fix this shift - this is
//  reason why OALTimerRecharge doesn't read back compare register and it
//  uses saved value instead).
//
UINT32 OALTimerUpdate(UINT32 period, UINT32 margin)
{
    UINT32 edge;

    // New compare value is base plus period
    g_timer.compare = g_timer.base + period;
    // Are we more than margin before new compare?
    edge = OALTimerGetCount() + margin;
    if ((INT32)(g_timer.compare - edge) > 0) {
       // Yes, then use calculated value
       OALTimerSetCompare(g_timer.compare);
    } else {
       // No, shift real compare value
       OALTimerSetCompare(edge);
    }            
    return 0;
}

//------------------------------------------------------------------------------
