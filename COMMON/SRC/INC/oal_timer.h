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
//  Header: oal_timer.h
//
//  This header define OAL timer module. Module code implements system ticks,
//  idle/power saving HAL functionality. It is also used by other OAL modules
//  for action or information related to time.
//
//  Interaction with kernel:
//      * Interrupt handler (SYSINTR_RESCHED)
//      * CurMSec
//      * OEMIdle
//      * OEMGetTickCount
//      * pQueryPerformanceFrequency
//      * pQueryPerformanceCounter
//      * pOEMUpdateRescheduleTime
//
//  Interaction with other OAL modules:
//      * OALTimerInit (system initialization)
//      * OALTimerRecharge (kernel profiling)
//      * OALTimerUpdate (kernel profiling)
//      * OALTimerCountsSinceSysTick (kernel profiling, IL timing)
//      * g_oalTimer (kernel profiling, IL timing)
//      * OALGetTickCount
//      * OALStall
//
//  Internal module functions:
//      * OALTimerRecharge
//      * OALCPUIdle
//
//  Internal module functions for system with count/compare counters:
//      * OALTimerInitCount
//      * OALTimerGetCount
//      * OALTimerGetCompare
//      * OALTimerSetCompare
//
//
#ifndef __OAL_TIMER_H
#define __OAL_TIMER_H

#if __cplusplus
extern "C" {
#endif

//------------------------------------------------------------------------------
//
//  Type:   OAL_TIMER_STATE
//
//  Timer module component control block - it unites all internal module
//  variables which can be used by other OAL modules.
//
//  The countsPerMSec specifies input clock frequency in kHz (or how many
//  input clock ticks fits to 1 ms time frame). The countsMargin defines
//  how many ticks before timer interrupt is safe to change timer value. It
//  is used in OEMIdle/pOEMUpdateRescheduleTime as parameter for functions
//  manipulating timer. The maxPeriodMSec specifies maximal period time
//  supported by hardware. Some modules (profiler, ILT) change this value
//  to reduce maximal time between system timer interrupt. The msecPerSysTick
//  and countsPerSysTick are base timer period values (countsPerSysTick is
//  always equal to msecPerSysTick * countsPerMSec). The actualMSecPerSysTick
//  and actualCountsPerSysTick are timer period values for actual tick.
//  Interrupt handler must use this actual values to update system counters
//  CurMSec and curCounts. At beginning of period handler must recharge/restore
//  actual values from base variables (msecPerSysTick/countsPerSysTick).
//
typedef struct {
    UINT32 countsPerMSec;                   // counts per 1 msec
    UINT32 countsMargin;                    // counts margin
    UINT32 maxPeriodMSec;                   // maximal timer period in MSec
    UINT32 msecPerSysTick;                  // msec per system tick
    UINT32 countsPerSysTick;                // counts per system tick
    UINT32 actualMSecPerSysTick;            // actual msec per system tick
    UINT32 actualCountsPerSysTick;          // actual counts per system tick
    volatile UINT64 curCounts;              // counts at last system tick
} OAL_TIMER_STATE, *POAL_TIMER_STATE;

//------------------------------------------------------------------------------
//
//  Extern:  g_oalTimer
//
//  Exports the OAL timer control block. This is global instance of internal
//  timer module variables.
//
extern OAL_TIMER_STATE g_oalTimer;

//------------------------------------------------------------------------------
//
//  Function:  OALTimerInit
//
//  This function initialize system timer. It is called usually from OEMInit.
//  The msecPerSysTick parameter defines system tick period. The
//  countsPerMSec parameter states timer input clock frequency (value is
//  frequency divided by 1000). Last margin parameter is used in timer
//  manipulation routines (OALTimerRecharge, OALTimerUpdate) to define safe
//  time range where timer can be modified without possible hazard.
//
//  For variable tick period first value should be set to maximal period
//  supported by hardware (the implementation should also check and fix
//  value is bigger than supported - this allows use -1 as parameter).
//
BOOL OALTimerInit(UINT32 msecPerSysTick, UINT32 countsPerMSec, UINT32 margin);

//------------------------------------------------------------------------------
//
//  Function:  OALTimerIntrHandler
//
//  This function implements timer interrupt handler. Depending on hardware
//  it is called from general interrupt handler (e.g. ARM) or it is used
//  as interrupt handler (e.g. MIPS). It returns SYSINTR value as expected 
//  from interrupt handler.
//
UINT32 OALTimerIntrHandler();

//------------------------------------------------------------------------------
//
//  Function:  OALTimerQueryPerformanceFrequency
//
//  This function is exported through pQueryPerformanceFrequency.
//
BOOL OALTimerQueryPerformanceFrequency(LARGE_INTEGER *pFrequency);

//------------------------------------------------------------------------------
//
//  Function:  OALTimerQueryPerformanceCounter
//
//  This function is exported through pQueryPerformanceCounter. It returns
//  high resolution counter value.
//
BOOL OALTimerQueryPerformanceCounter(LARGE_INTEGER *pCounter);

//------------------------------------------------------------------------------
//
//  Function:  OALTimerUpdateRescheduleTime
//
//  This function should be implemented if implementation supports variable
//  tick. It is exported through pOEMUpdateRescheduleTime.
//
VOID OALTimerUpdateRescheduleTime(DWORD time);

//------------------------------------------------------------------------------
//
//  Function:  OALTimerCountsSinceSysTick
//
//  This function returns time which expires from current system tick. 
//
INT32 OALTimerCountsSinceSysTick();

//------------------------------------------------------------------------------
//
//  Function:  OALTimerRecharge
//
//  This function is called usually form timer interrupt handler to recharge
//  timer for next system ticks. Depending on timer hardware implementation
//  it can be stubed (in cases when timer has auto-recharge mode).
//
VOID OALTimerRecharge(UINT32 period, UINT32 margin);

//------------------------------------------------------------------------------
//
//  Function:  OALTimerUpdate
//
//  This function is called to modify actual timer period. The new
//  system tick period is defined in first parameter. It returns time which
//  already expires from actual period in new system tick period quants. 
//  This value is usefull to avoid system tick drift and/or for idle time
//  counting. If actual time is to close to tick (near then second parameter)
//  tick time isn't changed. If new tick time is close to actual time it
//  should be shifted forward (by count given in second parameter).
//
//  This function must be implemented only when OAL OEMIdle, IL timing or
//  profilling implementation are used.
//
UINT32 OALTimerUpdate(UINT32 period, UINT32 margin);

//------------------------------------------------------------------------------
//
//  Function:   OALCPUIdle
//
//  This function is called to put CPU/SoC to idle state. The CPU/SoC should
//  exit idle state when interrupt occurs. It is called with disabled
//  interrupts. When it returns interrupt must be disabled also.
//
//  This function must be implemented only when OAL OEMIdle implementation is
//  used. The common library contains busy loop implementation which can be
//  used for development.
//  
VOID OALCPUIdle();

//------------------------------------------------------------------------------
//
//  Function:  OALGetTickCount
//
//  This function returns number of 1 ms ticks which elapsed since system boot
//  or reset (absolute value isn't important). The counter can overflow but
//  overflow period should not be shorter then approx 30 seconds. Function 
//  is used in  system boot so it must work before interrupt subsystem
//  is active.
//
UINT32 OALGetTickCount();

//------------------------------------------------------------------------------
//
//  Function:  OALStall
//
//  This function returns stalls CPU for time defined in parameters. The unit
//  is 1 microseconds. Function is used in  system boot so it must work before
//  interrupt subsystem is active.
//
VOID OALStall(UINT32 uSecs);

//------------------------------------------------------------------------------
//
//  Function:  OALTimerInitCount
//
//  Initialize system timer with given base period. Value margin specify number
//  of clocks used by count/compare counter to decide if recharge can be done
//  based on previous period or on current clock value.
//
VOID OALTimerInitCount(UINT32 period);

//------------------------------------------------------------------------------
//
//  Function:  OALTimerGetCount
//
//  This function returns actual timer value. This function is inted to be used
//  for OALGetTickCount/OALStall implementation in situation when global timer
//  variable isn't initialized. It is also used for OALTimerRecharge/
//  OALTimerCountsSinceSysTick/OALTimerReduceSysTick and OALTimerExtendSysTick
//  implementation for systems with count/compare timer. This function call
//  should also clear timer interrupt.
//
UINT32 OALTimerGetCount();

//------------------------------------------------------------------------------
//
//  Function:  OALTimerGetCompare
//
//  This function returns timer compare value. This function should be
//  implemented only for systems with count/compare timer type. It is used
//  for OALTimerRecharge/OALTimerCountsSinceSysTick/OALTimerReduceSysTick and
//  OALTimerExtendSysTick implementation for systems with count/compare timer.
//
UINT32 OALTimerGetCompare();

//------------------------------------------------------------------------------
//
//  Function:  OALTimerSetCompare
//
//  This function sets timer compare value. This function should be
//  implemented only for systems with count/compare timer type. It is used
//  for OALTimerRecharge/OALTimerCountsSinceSysTick/OALTimerReduceSysTick and
//  OALTimerExtendSysTick implementation for systems with count/compare timer.
//
VOID OALTimerSetCompare(UINT32 compare);

//------------------------------------------------------------------------------

#if __cplusplus
}
#endif

#endif
