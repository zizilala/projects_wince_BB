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
//  File:  ilt.c
//
#include <windows.h>
#include <nkintr.h>
#include <iltiming.h>
#include <oal.h>

//------------------------------------------------------------------------------
//
// Global:  g_oalILT
//
OAL_ILT_STATE g_oalILT;

//------------------------------------------------------------------------------
//
//  Global:  g_ilt
//
//  Save place for OAL_TIMER_STATE original values.
//
static struct {
    UINT32 msecPerSysTick;
    UINT32 countsPerSysTick;
    UINT32 maxPeriodMSec;
} g_ilt;    


//------------------------------------------------------------------------------
//
//  Function:  OALIoCtlHalILTiming
//
//  This function is called to start/stop IL timing. It is called from
//  OEMIoControl for IOCTL_HAL_ILTIMING code.
//
BOOL OALIoCtlHalILTiming( 
    UINT32 code, VOID *pInpBuffer, UINT32 inpSize, VOID *pOutBuffer, 
    UINT32 outSize, UINT32 *pOutSize ) 
{
    BOOL rc = FALSE;
    BOOL enabled;
    ILTIMING_MESSAGE *pITM = (ILTIMING_MESSAGE*)pInpBuffer;

    if (pOutSize) {
        *pOutSize = 0;
    }

    // Check message size
    if (inpSize < sizeof(ILTIMING_MESSAGE) || pInpBuffer == NULL) {
        NKSetLastError(ERROR_INVALID_PARAMETER);
        OALMSG(OAL_ERROR, (
            L"ERROR: OALIoCtlHalILTiming: Invalid parameter\r\n"
        ));
        rc = FALSE;
        goto cleanUp;
    }
 
    switch (pITM->wMsg) {

    case ILTIMING_MSG_ENABLE:
        g_oalILT.counter = g_oalILT.counterSet = pITM->dwFrequency;
        g_oalILT.interrupts = 0;
        g_oalILT.isrTime1 = 0xFFFFFFFF;
        // We don't want interrupt just now
        enabled = INTERRUPTS_ENABLE(FALSE);
        // Save original timer values
        g_ilt.msecPerSysTick = g_oalTimer.msecPerSysTick;
        g_ilt.countsPerSysTick = g_oalTimer.countsPerSysTick;
        //variable tick works off of maxPeriodMSec. The next line essentially changes variable tick timer into 1 ms fix tick timer.
        g_ilt.maxPeriodMSec = g_oalTimer.maxPeriodMSec;         
        // For timing we need period 1 ms
        g_oalTimer.msecPerSysTick = 1;
        g_oalTimer.countsPerSysTick = g_oalTimer.countsPerMSec;
        g_oalTimer.maxPeriodMSec = 1;
        // ILT is active now        
        g_oalILT.active = TRUE;
        // We are done with critical part
        INTERRUPTS_ENABLE(enabled);
        rc = TRUE;
        break;

    case ILTIMING_MSG_DISABLE:
        // We don't want interrupt just now
        enabled = INTERRUPTS_ENABLE(FALSE);
        // Restore original timer values
        g_oalTimer.msecPerSysTick = g_ilt.msecPerSysTick;
        g_oalTimer.countsPerSysTick = g_ilt.countsPerSysTick;
        g_oalTimer.maxPeriodMSec = g_ilt.maxPeriodMSec;
        // ITL is inactive
        g_oalILT.active = FALSE;
        // We are done with critical part
        INTERRUPTS_ENABLE(enabled);
        rc = TRUE;
        break;

    case ILTIMING_MSG_GET_TIMES:
        pITM->dwIsrTime1 = g_oalILT.isrTime1;
        pITM->dwIsrTime2 = g_oalILT.isrTime2;
        pITM->wNumInterrupts = g_oalILT.interrupts;
        pITM->dwSPC = g_oalILT.savedPC;
        pITM->dwFrequency = g_oalILT.counterSet;
        g_oalILT.interrupts = 0;
        rc = TRUE;
        break;

    case ILTIMING_MSG_GET_PFN:
        pITM->pfnPerfCountSinceTick = OALTimerCountsSinceSysTick;
        rc = TRUE;
        break;

    default:
        // Error, didn't match any known parameters
        NKSetLastError(ERROR_INVALID_PARAMETER);
        OALMSG(OAL_ERROR, (
            L"ERROR: OALIoCtlHalILTiming: Invalid parameter\r\n"
        ));
        rc = FALSE;
        break;
    }

cleanUp:
    return rc;
}

//------------------------------------------------------------------------------

