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
// THE SAMPLE SOURCE CODE IS PROVIDED "AS IS", WITH NO WARRANTIES OR INDEMNITIES.
//
/*
================================================================================
*             Texas Instruments OMAP(TM) Platform Software
* (c) Copyright Texas Instruments, Incorporated. All Rights Reserved.
*
* Use of this software is controlled by the terms and conditions found
* in the license agreement under which this software has been supplied.
*
================================================================================
*/
//
//  File: rtc.cpp
//
#pragma warning(push)
#pragma warning(disable : 4201)
#include <windows.h>
#include <ceddk.h>
#include <ceddkex.h>
#include <oal.h>
#include <oalex.h>
#include <initguid.h>

#include "bsp.h"
#include "oal_padcfg.h"
#include "bsp_padcfg.h"
#include "twl.h"
#include "triton.h"
#include "tps659xx_internals.h"
#pragma warning(pop)

// disable PREFAST warning for use of EXCEPTION_EXECUTE_HANDLER
#pragma warning (disable: 6320)

//------------------------------------------------------------------------------
//
//  Global:  dpCurSettings
//
//  Set debug zones names and initial setting for driver
//
#ifndef SHIP_BUILD

#undef ZONE_ERROR
#undef ZONE_WARN
#undef ZONE_FUNCTION
#undef ZONE_INIT
#undef ZONE_INFO

#define ZONE_ERROR          DEBUGZONE(0)
#define ZONE_WARN           DEBUGZONE(1)
#define ZONE_FUNCTION       DEBUGZONE(2)
#define ZONE_INIT           DEBUGZONE(3)
#define ZONE_INFO           DEBUGZONE(4)
#define ZONE_INT_THREAD     DEBUGZONE(5)
#define ZONE_SYNC_THREAD    DEBUGZONE(6)
#define ZONE_ALARM          DEBUGZONE(7)

//------------------------------------------------------------------------------
//
//  Global:  dpCurSettings
//
DBGPARAM dpCurSettings = {
    L"RTC", {
        L"Errors",      L"Warnings",    L"Function",    L"Init",
        L"Info",        L"Intr Thread", L"Sync Thread", L"Alarm",
        L"Undefined",   L"Undefined",   L"Undefined",   L"Undefined",
        L"Undefined",   L"Undefined",   L"Undefined",   L"Undefined"
    },
    0x00F3
};

#endif


//------------------------------------------------------------------------------
//  Local Definitions

#define RTC_DEVICE_COOKIE       'rtcD'


//------------------------------------------------------------------------------
//  Local Structures

typedef struct {
    DWORD cookie;
    DWORD priority256;
    DWORD syncPeriod;
    HANDLE hTWL;
    HANDLE hIntrEvent;
    HANDLE hIntrThread;
    BOOL threadsExit;
    DWORD enableWake;
    CEDEVICE_POWER_STATE powerState;
} Device_t;

//------------------------------------------------------------------------------
//  Local Functions

BOOL
RTC_Deinit(
    DWORD context
    );

static 
DWORD 
RTC_IntrThread(
    VOID *pContext
    );

//------------------------------------------------------------------------------
//  Device registry parameters

// UNDONE:
//  Eventually we want the timer interrupt to be triggered 1 every day
//
static const DEVICE_REGISTRY_PARAM s_deviceRegParams[] = {
    {
        L"Priority256", PARAM_DWORD, FALSE, offset(Device_t, priority256),
        fieldsize(Device_t, priority256), (VOID*)102
    }, {
        L"SyncPeriod", PARAM_DWORD, FALSE, offset(Device_t, syncPeriod),
        fieldsize(Device_t, syncPeriod), (VOID*)TWL_RTC_INTERRUPTS_EVERY_HOUR
    }, {
        L"EnableWake", PARAM_DWORD, FALSE, offset(Device_t, enableWake),
        fieldsize(Device_t, enableWake), (VOID*)1
    }     
};

//------------------------------------------------------------------------------
//
//  Function:  SetPowerState
//
//  process the power state change request.
//
BOOL
SetPowerState(
    Device_t *pDevice,
    CEDEVICE_POWER_STATE dx)
{
    DEBUGMSG(ZONE_FUNCTION, (L"+RTC:SetPowerState(0x%08x, %d)\r\n", pDevice, dx));
    
    // only process request is the power states are different
    UINT8  intr = 0;
    if (pDevice->powerState != dx)
        {

        // always enable rtc timer interrupt
       intr = TWL_RTC_INTERRUPTS_IT_ALARM;
        
        switch (dx)
            {
            case D0:
            case D2:
                // enable timer if we're not in suspend
                intr |= TWL_RTC_INTERRUPTS_IT_TIMER; 
                intr |= (UINT8)pDevice->syncPeriod;
                break;
            }
        pDevice->powerState = dx;
        TWLWriteRegs(pDevice->hTWL, TWL_RTC_INTERRUPTS_REG, &intr, sizeof(intr));
        }

    DEBUGMSG(ZONE_FUNCTION, (L"-RTC:SetPowerState(0x%08x, %d)\r\n", pDevice, dx));
    return TRUE;
}

//------------------------------------------------------------------------------
//
//  Function:  RTC_Init
//
//  Called by device manager to initialize device.
//
DWORD
RTC_Init(
    LPCWSTR szContext, 
    LPCVOID pBusContext
    )
{
    DWORD rc = (DWORD)NULL;
    Device_t *pDevice = NULL;

	UNREFERENCED_PARAMETER(pBusContext);

    DEBUGMSG(ZONE_FUNCTION, (
        L"+RTC_Init(%s, 0x%08x)\r\n", szContext, pBusContext
        ));

    // Create device structure
    pDevice = (Device_t *)LocalAlloc(LPTR, sizeof(Device_t));
    if (pDevice == NULL)
        {
        DEBUGMSG(ZONE_ERROR, (L"ERROR: RTC_Init: "
            L"Failed allocate TWL controller structure\r\n"
            ));
        goto cleanUp;
        }

    // Set cookie
    pDevice->cookie = RTC_DEVICE_COOKIE;
    pDevice->powerState = D4;

    // Read device parameters
    if (GetDeviceRegistryParams(
            szContext, pDevice, dimof(s_deviceRegParams), s_deviceRegParams
            ) != ERROR_SUCCESS)
        {
        DEBUGMSG(ZONE_ERROR, (L"ERROR: RTC_Init: "
            L"Failed read TWL driver registry parameters\r\n"
            ));
        goto cleanUp;
        }

    // Open parent bus
    pDevice->hTWL = TWLOpen();
    if (pDevice->hTWL == NULL)
        {
        DEBUGMSG(ZONE_ERROR, (L"ERROR: RTC_Init: "
            L"Failed open TWL bus driver\r\n"
            ));
        goto cleanUp;
        }


    // Create interrupt event
    pDevice->hIntrEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
    if (pDevice->hIntrEvent == NULL)
        {
        DEBUGMSG(ZONE_ERROR, (L"ERROR: RTC_Init: "
            L"Failed create interrupt event\r\n"
            ));
        goto cleanUp;
        }

    // Associate event with TWL RTC interrupt
    if (!TWLInterruptInitialize(pDevice->hTWL, TWL_INTR_RTC_IT, pDevice->hIntrEvent))
        {
        DEBUGMSG(ZONE_ERROR, (L"ERROR: RTC_Init: "
            L"Failed associate event with TWL RTC interrupt\r\n"
            ));
        goto cleanUp;
        }
  
    // Enable RTC event
    if (!TWLInterruptMask(pDevice->hTWL, TWL_INTR_RTC_IT, FALSE))
        {
        DEBUGMSG(ZONE_ERROR, (L"ERROR: RTC_Init: "
            L"Failed associate event with TWL fake interrupt\r\n"
            ));
        }

    // Enable alarm wakeup
    if (pDevice->enableWake)
        {
        TWLWakeEnable(pDevice->hTWL, TWL_INTR_RTC_IT, TRUE);
        }

    // Start interrupt service thread
    pDevice->threadsExit = FALSE;
    pDevice->hIntrThread = CreateThread(
        NULL, 0, RTC_IntrThread, pDevice, 0,NULL
        );
    if (!pDevice->hIntrThread)
        {
        DEBUGMSG(ZONE_ERROR, (L"ERROR: RTC_Init: "
            L"Failed create interrupt thread\r\n"
            ));
        goto cleanUp;
        }

    // Set thread priority
    CeSetThreadPriority(pDevice->hIntrThread, pDevice->priority256);

    // update power state
    SetPowerState(pDevice, D0);

	{
		HANDLE hGPIO = GPIOOpen();
		GPIOSetBit(hGPIO,TPS659XX_MSECURE_GPIO);
		GPIOClose(hGPIO);
	}

    // Return non-null value
    rc = (DWORD)pDevice;
    
cleanUp:
    if (rc == 0) RTC_Deinit((DWORD)pDevice);
    DEBUGMSG(ZONE_FUNCTION, (L"-RTC_Init(rc = %d\r\n", rc));
    return rc;
}

//------------------------------------------------------------------------------
//
//  Function:  RTC_Deinit
//
//  Called by device manager to uninitialize device.
//
BOOL
RTC_Deinit(
    DWORD context
    )
{
    BOOL rc = FALSE;
    Device_t *pDevice = (Device_t*)context;


    DEBUGMSG(ZONE_FUNCTION, (L"+RTC_Deinit(0x%08x)\r\n", context));

    // Check if we get correct context
    if ((pDevice == NULL) || (pDevice->cookie != RTC_DEVICE_COOKIE))
        {
        DEBUGMSG (ZONE_ERROR, (L"ERROR: RTC_Deinit: "
            L"Incorrect context paramer\r\n"
            ));
        goto cleanUp;
        }

    // Signal stop to threads
    pDevice->threadsExit = TRUE;

    // Close interrupt thread
    if (pDevice->hIntrThread != NULL)
        {
        // Set event to wake it
        SetEvent(pDevice->hIntrEvent);
        // Wait until thread exits
        WaitForSingleObject(pDevice->hIntrThread, INFINITE);
        // Close handle
        CloseHandle(pDevice->hIntrThread);
        }

    // Close TWL driver
    if (pDevice->hTWL != NULL)
        {
        TWLInterruptMask(pDevice->hTWL, TWL_INTR_RTC_IT, TRUE);
        if (pDevice->hIntrEvent != NULL)
            {
            TWLInterruptDisable(pDevice->hTWL, TWL_INTR_RTC_IT);
            }
        TWLClose(pDevice->hTWL);
        }

    if (pDevice->hIntrEvent != NULL)
        {
        CloseHandle(pDevice->hIntrEvent);
        }

    // Free device structure
    LocalFree(pDevice);

    // Done
    rc = TRUE;

cleanUp:
    DEBUGMSG(ZONE_FUNCTION, (L"-TWL_Deinit(rc = %d)\r\n", rc));
    return rc;
}

//------------------------------------------------------------------------------
//
//  Function:  TWL_Open
//
//  Called by device manager to open a device for reading and/or writing.
//
DWORD
RTC_Open(
    DWORD context, 
    DWORD accessCode, 
    DWORD shareMode
    )
{
	UNREFERENCED_PARAMETER(context);
	UNREFERENCED_PARAMETER(accessCode);
	UNREFERENCED_PARAMETER(shareMode);

    return context;
}

//------------------------------------------------------------------------------
//
//  Function:  TWL_Close
//
//  This function closes the device context.
//
BOOL
RTC_Close(
    DWORD context
    )
{
	UNREFERENCED_PARAMETER(context);

    return TRUE;
}

//------------------------------------------------------------------------------
//
//  Function:  RTC_IOControl
//
//  This function sends a command to a device.
//
BOOL
RTC_IOControl(
    DWORD context, 
    DWORD code, 
    UCHAR *pInBuffer, 
    DWORD inSize, 
    UCHAR *pOutBuffer,
    DWORD outSize, 
    DWORD *pOutSize
    )
{
    BOOL rc = FALSE;
    Device_t *pDevice = (Device_t*)context;

	UNREFERENCED_PARAMETER(inSize);
	UNREFERENCED_PARAMETER(pInBuffer);

    DEBUGMSG(ZONE_FUNCTION, (
        L"+RTC_IOControl(0x%08x, 0x%08x, 0x%08x, %d, 0x%08x, %d, 0x%08x)\r\n",
        context, code, pInBuffer, inSize, pOutBuffer, outSize, pOutSize
        ));
        
    // Check if we get correct context
    if ((pDevice == NULL) || (pDevice->cookie != RTC_DEVICE_COOKIE))
        {
        RETAILMSG(ZONE_ERROR, (L"ERROR: RTC_IOControl: "
            L"Incorrect context parameter\r\n"
            ));
        goto cleanUp;
        }
    
    switch (code)
        {
        case IOCTL_POWER_CAPABILITIES: 
            DEBUGMSG(ZONE_INFO, (L"RTC: Received IOCTL_POWER_CAPABILITIES\r\n"));
            if (pOutBuffer && outSize >= sizeof (POWER_CAPABILITIES) && 
                pOutSize) 
                {
                    __try 
                        {
                        PPOWER_CAPABILITIES PowerCaps;
                        PowerCaps = (PPOWER_CAPABILITIES)pOutBuffer;
         
                        // Only supports D0 (permanently on) and D4(off).         
                        memset(PowerCaps, 0, sizeof(*PowerCaps));
                        PowerCaps->DeviceDx = DX_MASK(D0) | DX_MASK(D4);
                        *pOutSize = sizeof(*PowerCaps);
                        
                        rc = TRUE;
                        }
                    __except(EXCEPTION_EXECUTE_HANDLER) 
                        {
                        RETAILMSG(ZONE_ERROR, (L"exception in ioctl\r\n"));
                        }
                }
            break;


        // requests a change from one device power state to another
        case IOCTL_POWER_SET: 
            DEBUGMSG(ZONE_INFO,(L"RTC: Received IOCTL_POWER_SET\r\n"));
            if (pOutBuffer && outSize >= sizeof(CEDEVICE_POWER_STATE)) 
                {
                __try 
                    {
                    CEDEVICE_POWER_STATE ReqDx = *(PCEDEVICE_POWER_STATE)pOutBuffer;
                    //handle only supported states
                    if((ReqDx == D0) || (ReqDx == D4))
                    {
                    if (SetPowerState(pDevice, ReqDx))
                        {   
                        *(PCEDEVICE_POWER_STATE)pOutBuffer = pDevice->powerState;
                        *pOutSize = sizeof(CEDEVICE_POWER_STATE);
 
                        rc = TRUE;
                        DEBUGMSG(ZONE_INFO, (L"RTC: "
                            L"IOCTL_POWER_SET to D%u \r\n",
                            pDevice->powerState
                            ));
                        }
                    else 
                        {
                        RETAILMSG(ZONE_ERROR, (L"RTC: "
                            L"Invalid state request D%u \r\n", ReqDx
                            ));
                        }
                    }
               }
                __except(EXCEPTION_EXECUTE_HANDLER) 
               {
                    RETAILMSG(ZONE_ERROR, (L"Exception in ioctl\r\n"));
               }
            }
            break;

        // gets the current device power state
        case IOCTL_POWER_GET: 
            RETAILMSG(ZONE_INFO, (L"RTC: Received IOCTL_POWER_GET\r\n"));
            if (pOutBuffer != NULL && outSize >= sizeof(CEDEVICE_POWER_STATE)) 
                {
                __try 
                    {
                    *(PCEDEVICE_POWER_STATE)pOutBuffer = pDevice->powerState;
 
                    rc = TRUE;

                    DEBUGMSG(ZONE_INFO, (L"RTC: "
                            L"IOCTL_POWER_GET to D%u \r\n",
                            pDevice->powerState
                            ));
                    }
                __except(EXCEPTION_EXECUTE_HANDLER) 
                    {
                    RETAILMSG(ZONE_ERROR, (L"Exception in ioctl\r\n"));
                    }
                }     
            break;
        }

cleanUp:
    DEBUGMSG(ZONE_FUNCTION, (L"-RTC_IOControl(rc = %d)\r\n", rc));
    return rc;
}

//------------------------------------------------------------------------------
//
//  Function:  RTC_IntrThread
//
//  This function acts as the IST for the RTC.
//
DWORD
RTC_IntrThread(
    VOID *pContext
    )
{
    Device_t *pDevice = (Device_t*)pContext;
    DWORD timeout = INFINITE;
    UINT8 status;
    UINT8 clrStatus = 0;

    // Loop until we are not stopped...
    while (!pDevice->threadsExit)
        {
        // Wait for event
        WaitForSingleObject(pDevice->hIntrEvent, timeout);
        if (pDevice->threadsExit) break;

        // Get status
        status = 0;
        TWLReadRegs(pDevice->hTWL, TWL_RTC_STATUS_REG, &status, sizeof(status));

        DEBUGMSG(ZONE_INT_THREAD, (L"RTC: interrupt: status=%02X\r\n", status));


        // Disable the RTC interrupts (this also clears the periodic timer interrupt status)
        clrStatus = 0;
        TWLWriteRegs(pDevice->hTWL, TWL_RTC_INTERRUPTS_REG, &clrStatus, sizeof(clrStatus));


        // Clear alarm status (regardless if set or not)
        clrStatus = TWL_RTC_STATUS_ALARM;
        TWLWriteRegs(pDevice->hTWL, TWL_RTC_STATUS_REG, &clrStatus, sizeof(clrStatus));

        
        // Is interrupt caused by alarm?
        if ((status & TWL_RTC_STATUS_ALARM) != 0)
            {
            DEBUGMSG(ZONE_ALARM, (L"RTC: alarm interrupt\r\n"));

            // Simply tell kernel, that ALARM event interrupt occured
            KernelIoControl(IOCTL_HAL_RTC_ALARM, NULL, 0, NULL, 0, NULL);
            }

        
        //  Is interrupt caused by time update?
        if ((status & TWM_RTC_STATUS_TIME_EVENT) != 0)
            {
            DEBUGMSG(ZONE_ALARM, (L"RTC: time event interrupt\r\n"));

            // Simply tell kernel, that TIME event interrupt occured
            KernelIoControl(IOCTL_HAL_RTC_TIME, NULL, 0, NULL, 0, NULL);
            }


        // Re-enable the RTC interrupts
        status = TWL_RTC_INTERRUPTS_IT_ALARM | TWL_RTC_INTERRUPTS_IT_TIMER | (UINT8)pDevice->syncPeriod;
        TWLWriteRegs(pDevice->hTWL, TWL_RTC_INTERRUPTS_REG, &status, sizeof(status));
        }

    return ERROR_SUCCESS;
}

//------------------------------------------------------------------------------
//
//  Function:  DllMain
//
//  Standard Windows DLL entry point.
//
BOOL
__stdcall
DllMain(
    HANDLE hDLL,
    DWORD reason,
    VOID *pReserved
    )
{
	UNREFERENCED_PARAMETER(pReserved);

    switch (reason)
        {
        case DLL_PROCESS_ATTACH:
            RETAILREGISTERZONES((HMODULE)hDLL);
            DisableThreadLibraryCalls((HMODULE)hDLL);
            break;
        }
    return TRUE;
}

//------------------------------------------------------------------------------

