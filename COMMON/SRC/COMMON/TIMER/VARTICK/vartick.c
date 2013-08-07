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
#include <windows.h>
#include <nkintr.h>
#include <pkfuncs.h>
#include <oal.h>

//------------------------------------------------------------------------------
//
//  Function:  OALTimerUpdateRescheduleTime
//
//  This function is called by kernel to set next reschedule time.
//
VOID OALTimerUpdateRescheduleTime(DWORD time)
{
    UINT32 baseMSec, diffMSec, diffCounts;
    INT32 counts;

    // Get current system timer counter
    baseMSec = CurMSec;

    // Return if we are already setup correctly
    if (time == (baseMSec + g_oalTimer.actualMSecPerSysTick)) goto cleanUp;

    // How far we are from next tick
    counts = g_oalTimer.actualCountsPerSysTick - OALTimerCountsSinceSysTick();

    // If timer interrupts occurs, or we are within 1 ms of the scheduled
    // interrupt, just return - timer ISR will take care of it.
    if (baseMSec != CurMSec || counts < (INT32)g_oalTimer.countsPerMSec) {
        goto cleanUp;
    }        

    // Calculate the distance between the new time and the last timer interrupt
    diffMSec = time - baseMSec;

    // Trying to set reschedule time prior or equal to CurMSec - this could
    // happen if a thread is on its way to sleep while preempted before
    // getting into the Sleep Queue
    if ((INT32)diffMSec < 0) diffMSec = 0;

    // Account for limitation (we are using msecPerSysTick instead
    // maxPeriodMSec - this allows little more ways how to modify timer
    // behavior, but in most cases those values will be same)
    if (diffMSec > g_oalTimer.msecPerSysTick) {
        diffMSec = g_oalTimer.msecPerSysTick;
    }        

    // Calculate count difference
    diffCounts = diffMSec * g_oalTimer.countsPerMSec;

    // Actual values to be used by interrupt handler
    g_oalTimer.actualMSecPerSysTick = diffMSec;
    g_oalTimer.actualCountsPerSysTick = diffCounts;

    // Reduct actual timer period (implementation must shift interrupt time
    // if we are too close to new tick time)
    OALTimerUpdate(diffCounts, g_oalTimer.countsMargin);

cleanUp:
    return;    
}

//------------------------------------------------------------------------------
