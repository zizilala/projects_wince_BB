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
//  File: rtc.c
//
//  This file implement RTC functions based on timer interrupt handler. The
//  handler must update g_oalRTCTicks/g_pOALRTCTicks each time when it is
//  invoked. When g_oalRTCAlarm is nonzero handler also should substract
//  elapsed time from its value and in case that result is zero or lower
//  return SYSINTR_ALARM instead SYSINTR_RESCHED. 
//
//  To allow pseudo-persistency between boots platform can allocate UINT64
//  arguments with OAL_ARGS_QUERY_RTC id. In such case timer counter is
//  stored on this location (instead g_oalRTCTicks).
//
#include <windows.h>
#include <ceddk.h>
#include <nkintr.h>
#include <oal.h>

//------------------------------------------------------------------------------
//
//  Global: g_oalRTCTicks/g_pOALRTCTicks
//
//  This variable in increment by one in timer interrupt handler. It contains
//  relative time in miliseconds since January 1, 1601.
//
UINT64 g_oalRTCTicks = 12685896000000;
UINT64 *g_pOALRTCTicks = NULL;

//------------------------------------------------------------------------------
//
//  Global: g_oalRTCAlarm
//
//  This variable contains number of miliseconds till next SYSINTR_ALARM. It
//  is zero when alarm isn't active.
//
UINT64 g_oalRTCAlarm = 0;

//------------------------------------------------------------------------------
//
//  Function:  OALIoCtlHalInitRTC
//
//  This function is called by WinCE OS to initialize the time after boot.
//  Input buffer contains SYSTEMTIME structure with default time value.
//  If hardware has persistent real time clock it will ignore this value
//  (or all call).
//
BOOL OALIoCtlHalInitRTC(
    UINT32 code, VOID *pInpBuffer, UINT32 inpSize, VOID *pOutBuffer, 
    UINT32 outSize, UINT32 *pOutSize
) {
    BOOL rc = FALSE;
    SYSTEMTIME *pTime = (SYSTEMTIME*)pInpBuffer;

    OALMSG(OAL_IOCTL&&OAL_FUNC, (L"+OALIoCtlHalInitRTC(...)\r\n"));

    if (pOutSize) {
        *pOutSize = 0;
    }

    // Validate inputs
    if (pInpBuffer == NULL || inpSize < sizeof(SYSTEMTIME)) {
        NKSetLastError(ERROR_INVALID_PARAMETER);
        OALMSG(OAL_ERROR, (
            L"ERROR: OALIoCtlHalInitRTC: Invalid parameter\r\n"
        ));
        goto cleanUp;
    }

    rc = OEMSetRealTime(pTime);
    
cleanUp:
    OALMSG(OAL_IOCTL&&OAL_FUNC, (L"-OALIoCtlHalInitRTC(rc = %d)\r\n", rc));
    return rc;
}

//------------------------------------------------------------------------------
//
//  Function:  OEMGetRealTime
//
//  This function is called by the kernel to retrieve the time from
//  the real-time clock.
//
BOOL OEMGetRealTime(SYSTEMTIME *pTime)
{
    BOOL rc = FALSE;
    ULARGE_INTEGER time;
    FILETIME fileTime;
    UINT64 ticks;
    BOOL enabled;
    INT32 tickCorrection;

    OALMSG(OAL_RTC&&OAL_FUNC, (L"+OEMGetRealTime\r\n"));

    if(!pTime) goto cleanUp;

    enabled = INTERRUPTS_ENABLE(FALSE);
    ticks = *g_pOALRTCTicks;
    tickCorrection = OALTimerCountsSinceSysTick();
    INTERRUPTS_ENABLE(enabled);

    // With variable tick timing, the RTC tick value might not be incremented
    // on a "normal" tick interval.  This means that the RTC may jump forward
    // in time depending on the tick variation.  To smooth this out, we need
    // to sample the timer to see how far we've made it past the last match
    // event, then convert that into milliseconds and adjust the derived value.
    //
    if (tickCorrection >= (INT32)g_oalTimer.countsPerMSec)
    {
        ticks += (tickCorrection / g_oalTimer.countsPerMSec);
    }

    // Convert time to appropriate format
    time.QuadPart = ticks * 10000;
    fileTime.dwLowDateTime = time.LowPart;
    fileTime.dwHighDateTime = time.HighPart;

    if((rc = NKFileTimeToSystemTime(&fileTime, pTime)))
    {
        OALMSG(OAL_RTC&&OAL_FUNC, (
            L"-OEMGetRealTime(rc = %d %d/%d/%d %d:%d:%d.%03d)\r\n", rc, 
            pTime->wYear, pTime->wMonth, pTime->wDay, pTime->wHour, pTime->wMinute,
            pTime->wSecond, pTime->wMilliseconds
        ));
    }


cleanUp:
    return rc;
}

//------------------------------------------------------------------------------
//
//  Function:  OEMSetRealTime
//
//  This function is called by the kernel to set the real-time clock.
//
BOOL OEMSetRealTime(SYSTEMTIME *pTime)
{
    BOOL rc = FALSE;
    ULARGE_INTEGER time;
    FILETIME fileTime;
    BOOL enabled;

    if(!pTime) goto cleanUp;

    OALMSG(OAL_RTC&&OAL_FUNC, (
        L"+OEMSetRealTime(%d/%d/%d %d:%d:%d.%03d)\r\n", 
        pTime->wYear, pTime->wMonth, pTime->wDay, pTime->wHour, pTime->wMinute,
        pTime->wSecond, pTime->wMilliseconds
    ));
    
    if((rc = NKSystemTimeToFileTime(pTime, &fileTime)))
    {
        // Convert time to miliseconds since Jan 1, 1601
        time.LowPart = fileTime.dwLowDateTime;
        time.HighPart = fileTime.dwHighDateTime;
        time.QuadPart /= 10000;

        enabled = INTERRUPTS_ENABLE(FALSE);
        *g_pOALRTCTicks = time.QuadPart;
        INTERRUPTS_ENABLE(enabled);
    }
    
cleanUp:
    OALMSG(OAL_RTC&&OAL_FUNC, (L"-OEMSetRealTime(rc = %d)\r\n", rc));
    return rc;
}

//------------------------------------------------------------------------------
//
//  Function:  OEMSetAlarmTime
//
//  This function is called by the kernel to set the real-time clock alarm.
//
BOOL OEMSetAlarmTime(SYSTEMTIME *pTime)
{
    BOOL rc = FALSE;
    ULARGE_INTEGER time;
    FILETIME fileTime;
    BOOL enabled;

    if(!pTime) goto cleanUp;

    OALMSG(OAL_RTC&&OAL_FUNC, (
        L"+OEMSetAlarmTime(%d/%d/%d %d:%d:%d.%03d)\r\n", 
        pTime->wYear, pTime->wMonth, pTime->wDay, pTime->wHour, pTime->wMinute,
        pTime->wSecond, pTime->wMilliseconds
    ));

    if((rc = NKSystemTimeToFileTime(pTime, &fileTime)))
    {
        // Convert time to miliseconds since Jan 1, 1601
        time.LowPart = fileTime.dwLowDateTime;
        time.HighPart = fileTime.dwHighDateTime;
        time.QuadPart /= 10000;

        enabled = INTERRUPTS_ENABLE(FALSE);
        g_oalRTCAlarm = time.QuadPart - *g_pOALRTCTicks;
        INTERRUPTS_ENABLE(enabled);
    }

cleanUp:
    OALMSG(OAL_RTC&&OAL_FUNC, (L"-OEMSetAlarmTime(rc = %d)\r\n", rc));
    return rc;
}

//------------------------------------------------------------------------------
