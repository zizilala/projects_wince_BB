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
//  counters.
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
//  If end of actual period is closer than margin period isn't changed (so
//  original period elapse). Function returns time which already expires
//  in new period length units.
//
UINT32 OALTimerUpdate(UINT32 period, UINT32 margin)
{
    UINT32 rc, edge;

    // Get actual count value
    edge = OALTimerGetCount() + margin;
    // Are we at least edge before actual period end?
    if ((INT32)(g_timer.compare - edge) > 0) {
        // How many new period already passed from system tick
        rc = (edge - g_timer.base)/period;
        // Shift base and new compare
        g_timer.base += rc * period;         
        g_timer.compare = g_timer.base + period;
        // Program hardware
        OALTimerSetCompare(g_timer.compare);
    } else {
        // No, we are too close - stay with original period
        rc = (g_timer.compare - g_timer.base)/period - 1;
    }    
    return rc;
}

//------------------------------------------------------------------------------

