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
//  File:  counts.c
//
//  This file contains common implementation for functions exported to
//  kernel (OEMGetTickCount, pQueryPerformanceFrequency and
//  pQueryPerformanceCounter). 
//
#include <windows.h>
#include <nkintr.h>
#include <oal.h>


//------------------------------------------------------------------------------
//
//  Function: OEMGetTickCount
//
//  This returns the number of milliseconds that have elapsed since Windows 
//  CE was started. If the system timer period is 1ms the function simply 
//  returns the value of CurMSec. If the system timer period is greater then
//  1 ms, the HiRes offset is added to the value of CurMSec.
//
UINT32 OEMGetTickCount()
{
    UINT32 count;
    INT32 offset;

    if (g_oalTimer.actualMSecPerSysTick == 1) {
        // Return CurMSec if the system tick is 1 ms.
        count = CurMSec;
    }  else {
        // System timer tick period exceeds 1 ms. 
        //
        // This code adjusts the accuracy of the returned value to the nearest
        // MSec when the system tick exceeds 1 ms. The following code checks if 
        // a system timer interrupt occurred between reading the CurMSec value 
        // and the call to fetch the HiResTicksSinceSysTick. If so, the value of
        // CurMSec and Offset is re-read, with the certainty that a system timer
        // interrupt will not occur again.
        do {
            count = CurMSec;
            offset = OALTimerCountsSinceSysTick();
        } 
        while (count != CurMSec);

        // Adjust the MSec value with the contribution from HiRes counter.
        count += offset/g_oalTimer.countsPerMSec;
    }

    return count;
}



//------------------------------------------------------------------------------
//
//  Function: OALQueryPerformanceFrequency
//
//  This function returns the frequency of the high-resolution 
//  performance counter.
//
BOOL OALTimerQueryPerformanceFrequency(LARGE_INTEGER *pFrequency)
{
    if (!pFrequency) return FALSE;

    pFrequency->HighPart = 0;
    pFrequency->LowPart = 1000 * g_oalTimer.countsPerMSec;
    return TRUE;
}


//------------------------------------------------------------------------------
//
//  Function: OALQueryPerformanceCounter
//
//  This function returns the current value of the high-resolution 
//  performance counter.
//  
BOOL OALTimerQueryPerformanceCounter(LARGE_INTEGER *pCounter)
{
    UINT64 base;
    INT32 offset;

    if (!pCounter) return FALSE;
 
    // Make sure CurTicks is the same before and after read of counter
    // to avoid for possible rollover. Note that this is probably not necessary
    // because TimerTicksSinceBeat will return negative value when it happen.
    // We must be careful about signed/unsigned arithmetic.
    
    do {
       base = g_oalTimer.curCounts;
       offset = OALTimerCountsSinceSysTick();
    } while (base != g_oalTimer.curCounts);

    // Update the counter
    pCounter->QuadPart = (ULONGLONG)((INT64)base + offset);

    return TRUE;
}

//------------------------------------------------------------------------------
