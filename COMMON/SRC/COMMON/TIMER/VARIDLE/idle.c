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
//  Function:     OEMIdle
//
//  This function is called by the kernel when there are no threads ready to 
//  run. The CPU should be put into a reduced power mode if possible and halted. 
//  It is important to be able to resume execution quickly upon receiving an 
//  interrupt.
//
//  Interrupts are disabled when OEMIdle is called and when it returns.
//
//  This implementation doesn't change system tick. It is intend to be used
//  with variable tick implementation. However it should work with fixed
//  variable tick implementation also (with lower efficiency because maximal
//  idle time is 1 ms).
//
VOID OEMIdle(DWORD idleParam)
{
    UINT32 baseMSec;
    INT32 usedCounts, idleCounts;
    ULARGE_INTEGER idle;

    // Get current system timer counter
    baseMSec = CurMSec;
    
    // Find how many hi-res ticks was already used
    usedCounts = OALTimerCountsSinceSysTick();

    // We should wait this time
    idleCounts = g_oalTimer.actualCountsPerSysTick;
    
    // Move SoC/CPU to idle mode
    OALCPUIdle();

    // When there wasn't timer interrupt modify idle time
    if (CurMSec == baseMSec) {
        idleCounts = OALTimerCountsSinceSysTick();
    }

    // Get real idle value. If result is negative we didn't idle at all.
    idleCounts -= usedCounts;
    if (idleCounts < 0) idleCounts = 0;

    // Update idle counters
    idle.LowPart = curridlelow;
    idle.HighPart = curridlehigh;
    idle.QuadPart += idleCounts;
    curridlelow  = idle.LowPart;
    curridlehigh = idle.HighPart;
}

//------------------------------------------------------------------------------
