// Copyright (c) 2007, 2008 BSQUARE Corporation. All rights reserved.

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
// Portions Copyright (c) Texas Instruments.  All rights reserved.
//
//------------------------------------------------------------------------------
//


#include <windows.h>
#include <omap.h>
#include <ceddk.h>
#include <ceddkex.h>
#include <initguid.h>
#include <sdk_gpio.h>
#include <twl.h>
#include <wavext.h>

//
// Headset hardware presence from snapi.h in OSSVCS
//

////////////////////////////////////////////////////////////////////////////////
// HeadsetPresent
// Gets a value indicating whether a headset is present
#define SN_HEADSETPRESENT_ROOT HKEY_LOCAL_MACHINE
#define SN_HEADSETPRESENT_PATH TEXT("System\\State\\Hardware")
#define SN_HEADSETPRESENT_VALUE TEXT("Headset")

//------------------------------------------------------------------------------
//
//  Global:  dpCurSettings
//
//  Set debug zones names and initial setting for driver
//
#ifndef SHIP_BUILD
#undef ZONE_ERROR
#define ZONE_ERROR          DEBUGZONE(0)
#define ZONE_WARN           DEBUGZONE(1)
#define ZONE_FUNCTION       DEBUGZONE(2)
#define ZONE_INFO           DEBUGZONE(4)
#define ZONE_IST            DEBUGZONE(5)
#define ZONE_HDS            DEBUGZONE(15)

//------------------------------------------------------------------------------
//
//  Global:  dpCurSettings
//
DBGPARAM dpCurSettings = {
    L"HDS", {
        L"Errors",      L"Warnings",    L"Function",    L"Info",
        L"IST",         L"Undefined",   L"Undefined",   L"Undefined",
        L"Undefined",   L"Undefined",   L"Undefined",   L"Undefined",
        L"Undefined",   L"Undefined",   L"Undefined",   L"Undefined"
    },
    0x0013
};

#endif

//------------------------------------------------------------------------------
//  Local Definitions

#define HDS_DEVICE_COOKIE           'hdsD'
#define HDS_GPIO_HSDET_PU_PD_MASK   0xCF    // disable PU / PD

//------------------------------------------------------------------------------
//  Local Structures

typedef struct {
    DWORD cookie;
    DWORD priority256;
    HANDLE hTWL;
    HANDLE hGPIO;
    HANDLE hWAV;
    BOOL bPluggedIn;
    CRITICAL_SECTION cs;
    HANDLE hIntrEvent;
    HANDLE hIntrThread;
    BOOL intrThreadExit;
    DWORD hdstDetGpio;
    DWORD hdstMuteGpio;
	DWORD dwSysIntr;
} HeadsetDevice_t;

//------------------------------------------------------------------------------
//  Local Functions

BOOL
HDS_Deinit(
    DWORD context
    );

DWORD HDS_IntrThread(
    VOID *pContext
    );


//------------------------------------------------------------------------------
//  Device registry parameters

static const DEVICE_REGISTRY_PARAM s_deviceRegParams[] = {
    {
        L"Priority256", PARAM_DWORD, FALSE,
        offset(HeadsetDevice_t, priority256),
        fieldsize(HeadsetDevice_t, priority256), (VOID*)200
    }, {
        L"HdstDetGpio", PARAM_DWORD, TRUE,
        offset(HeadsetDevice_t, hdstDetGpio),
        fieldsize(HeadsetDevice_t, hdstDetGpio), NULL
    }, {
        L"HdstMuteGpio", PARAM_DWORD, TRUE,
        offset(HeadsetDevice_t, hdstMuteGpio),
        fieldsize(HeadsetDevice_t, hdstMuteGpio), NULL
    }
};


//------------------------------------------------------------------------------
//
//  Function:  HDS_Init
//
//  Called by device manager to initialize device.
//
DWORD
HDS_Init(
    LPCTSTR szContext,
    LPCVOID pBusContext
    )
{
    DWORD rc = (DWORD)NULL;
    HeadsetDevice_t *pDevice = NULL;

	UNREFERENCED_PARAMETER(pBusContext);

    DEBUGMSG(ZONE_FUNCTION, (
        L"+HDS_Init(%s, 0x%08x)\r\n", szContext, pBusContext
        ));

    // Create device structure
    pDevice = (HeadsetDevice_t *)LocalAlloc(LPTR, sizeof(HeadsetDevice_t));
    if (pDevice == NULL)
        {
        DEBUGMSG(ZONE_ERROR, (L"ERROR: HDS_Init: "
            L"Failed allocate HDS driver structure\r\n"
            ));
        goto cleanUp;
        }

    // initialize memory
    //
    memset(pDevice, 0, sizeof(HeadsetDevice_t));

    // Set cookie & initialize critical section
    pDevice->cookie = HDS_DEVICE_COOKIE;

    // Initialize crit section
    InitializeCriticalSection(&pDevice->cs);

    // Read device parameters
    if (GetDeviceRegistryParams(
            szContext, pDevice, dimof(s_deviceRegParams), s_deviceRegParams)
            != ERROR_SUCCESS)
        {
        DEBUGMSG(ZONE_ERROR, (L"ERROR: HDS_Init: "
            L"Failed read HDS driver registry parameters\r\n"
            ));
        goto cleanUp;
        }

    // Open WAV device
    pDevice->hWAV = CreateFile(L"WAV1:", GENERIC_READ | GENERIC_WRITE, 0,
                        NULL, OPEN_EXISTING, 0, NULL);

    if (pDevice->hWAV == INVALID_HANDLE_VALUE)
        {
        pDevice->hWAV = NULL;
        DEBUGMSG(ZONE_WARN,
            (L"WARN: HDS_Init: Failed open WAV1: device driver\r\n"));
        }

    // Open GPIO bus
    pDevice->hGPIO = GPIOOpen();
    if (pDevice->hGPIO == NULL)
        {
        DEBUGMSG(ZONE_ERROR, (L"ERROR: HDS_Init: "
            L"Failed open GPIO bus driver\r\n"
            ));
        goto cleanUp;
        }

    // Configure GPIO mute as output high
    GPIOSetMode(pDevice->hGPIO, pDevice->hdstMuteGpio, GPIO_DIR_OUTPUT);
    GPIOSetBit(pDevice->hGPIO, pDevice->hdstMuteGpio);

    // Note that the headset detect GPIO pullup and pulldown configuration
    // is not handled in the \src\boot\twl4030\bsp_twl4020.c file

    // Configure GPIO headset as input with both edge interrupts
    GPIOSetMode(pDevice->hGPIO, pDevice->hdstDetGpio,
        GPIO_DIR_INPUT | GPIO_INT_LOW_HIGH | GPIO_INT_HIGH_LOW);

    // update plugged-in field
    pDevice->bPluggedIn = (BOOL)GPIOGetBit(pDevice->hGPIO,
        pDevice->hdstDetGpio);

    // Create interrupt event
    pDevice->hIntrEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
    if (pDevice->hIntrEvent == NULL)
        {
        DEBUGMSG(ZONE_ERROR, (L"ERROR: HDS_Init: "
            L"Failed create interrupt event\r\n"
            ));
        goto cleanUp;
        }

    // Associate event with TWL headset interrupt
    if (!GPIOInterruptInitialize(pDevice->hGPIO, pDevice->hdstDetGpio,
		&pDevice->dwSysIntr, pDevice->hIntrEvent))
        {
        DEBUGMSG (ZONE_ERROR, (L"ERROR: HDS_Init: "
            L"Failed associate event with TWL headset interrupt\r\n"
            ));
        goto cleanUp;
        }

    // Start interrupt service thread
    pDevice->intrThreadExit = FALSE;
    pDevice->hIntrThread = CreateThread(
        NULL, 0, HDS_IntrThread, pDevice, 0,NULL
        );
    if (!pDevice->hIntrThread)
        {
        DEBUGMSG (ZONE_ERROR, (L"ERROR: HDS_Init: "
            L"Failed create interrupt thread\r\n"
            ));
        goto cleanUp;
        }

    // Set thread priority
    CeSetThreadPriority(pDevice->hIntrThread, pDevice->priority256);

    // Return non-null value
    rc = (DWORD)pDevice;

cleanUp:
    if (rc == 0)
        {
        HDS_Deinit((DWORD)pDevice);
        }
    DEBUGMSG(ZONE_FUNCTION, (L"-HDS_Init(rc = %d\r\n", rc));
    return rc;
}

//------------------------------------------------------------------------------
//
//  Function:  HDS_Deinit
//
//  Called by device manager to uninitialize device.
//
BOOL
HDS_Deinit(
    DWORD context
    )
{
    BOOL rc = FALSE;
    HeadsetDevice_t *pDevice = (HeadsetDevice_t*)context;

    DEBUGMSG(ZONE_FUNCTION, (L"+HDS_Deinit(0x%08x)\r\n", context));

    // Check if we get correct context
    if ((pDevice == NULL) || (pDevice->cookie != HDS_DEVICE_COOKIE))
        {
        DEBUGMSG (ZONE_ERROR, (L"ERROR: HDS_Deinit: "
            L"Incorrect context parameter\r\n"
            ));
        goto cleanUp;
        }

    // Signal stop to threads
    pDevice->intrThreadExit = TRUE;

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

    // Disable GPIO interrupt
    if (pDevice->hGPIO != NULL)
        {
        GPIOInterruptMask(pDevice->hGPIO, pDevice->hdstDetGpio, pDevice->dwSysIntr, TRUE);
        if (pDevice->hIntrEvent != NULL)
            {
            GPIOInterruptDisable(pDevice->hGPIO, pDevice->hdstDetGpio, pDevice->dwSysIntr);
            }
        GPIOClose(pDevice->hGPIO);
        }

    // Close GPIO driver
    if (pDevice->hGPIO != NULL)
        {
        GPIOClose(pDevice->hGPIO);
        }

    // Close interrupt handler
    if (pDevice->hIntrEvent != NULL) CloseHandle(pDevice->hIntrEvent);

    // Delete critical section
    DeleteCriticalSection(&pDevice->cs);

    // Free device structure
    LocalFree(pDevice);

    // Done
    rc = TRUE;

cleanUp:
    DEBUGMSG(ZONE_FUNCTION, (L"-HDS_Deinit(rc = %d)\r\n", rc));
    return rc;
}

//------------------------------------------------------------------------------
//
//  Function:  HDS_Open
//
//  Called by device manager to open a device for reading and/or writing.
//
DWORD
HDS_Open(
    DWORD context,
    DWORD accessCode,
    DWORD shareMode
    )
{
	UNREFERENCED_PARAMETER(accessCode);
	UNREFERENCED_PARAMETER(shareMode);

    return context;
}

//------------------------------------------------------------------------------
//
//  Function:  HDS_Close
//
//  This function closes the device context.
//
BOOL
HDS_Close(
    DWORD context
    )
{
	UNREFERENCED_PARAMETER(context);

    return TRUE;
}

//------------------------------------------------------------------------------
//
//  Function:  HDS_IOControl
//
//  This function sends a command to a device.
//
BOOL
HDS_IOControl(
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
    HeadsetDevice_t *pDevice = (HeadsetDevice_t*)context;

	UNREFERENCED_PARAMETER(pOutSize);
	UNREFERENCED_PARAMETER(outSize);
	UNREFERENCED_PARAMETER(pOutBuffer);
	UNREFERENCED_PARAMETER(inSize);
	UNREFERENCED_PARAMETER(pInBuffer);
	UNREFERENCED_PARAMETER(code);

    DEBUGMSG(ZONE_FUNCTION, (
        L"+HDS_IOControl(0x%08x, 0x%08x, 0x%08x, %d, 0x%08x, %d, 0x%08x)\r\n",
        context, code, pInBuffer, inSize, pOutBuffer, outSize, pOutSize
    ));

    // Check if we get correct context
    if (pDevice == NULL || pDevice->cookie != HDS_DEVICE_COOKIE)
        {
        DEBUGMSG(ZONE_ERROR, (L"ERROR: HDS_IOControl: "
            L"Incorrect context paramer\r\n"
        ));
        goto cleanUp;
        }

    // Nothing to do

cleanUp:
    DEBUGMSG(ZONE_FUNCTION, (L"-HDS_IOControl(rc = %d)\r\n", rc));
    return rc;
}

//------------------------------------------------------------------------------
//
//  Function: SetHeadsetHardwareState
//
//  Sets the headset hardware state in the registry.
//
VOID
SetHeadsetHardwareState(
    BOOL fHeadsetIn
    )
{
    HKEY hKey;
    DWORD dwDisposition;
    if (RegCreateKeyEx(
            SN_HEADSETPRESENT_ROOT,
            SN_HEADSETPRESENT_PATH,
            0, NULL,
            REG_OPTION_VOLATILE,
            0, NULL,
            &hKey,
            &dwDisposition) == ERROR_SUCCESS)
    {
        DWORD dwData = fHeadsetIn ? 1 : 0;
        RegSetValueEx(
            hKey,
            SN_HEADSETPRESENT_VALUE,
            0,
            REG_DWORD,
            (const PBYTE) &dwData,
            sizeof(dwData));

        RegCloseKey(hKey);
    }
}

//------------------------------------------------------------------------------
//
//  Function:  HDS_InterruptThread
//
//  This function acts as the IST for the headset.
//
DWORD HDS_IntrThread(VOID *pContext)
{
    HeadsetDevice_t *pDevice = (HeadsetDevice_t*)pContext;
    BOOL fInitialDetection = TRUE;

    // Set the interrupt event to trigger initial state setting.
    SetEvent(pDevice->hIntrEvent);

    // Loop until we are not stopped...
    while (!pDevice->intrThreadExit)
        {

        // Wait for event
        WaitForSingleObject(pDevice->hIntrEvent, INFINITE);
        if (pDevice->intrThreadExit) goto cleanup;

        // Need to debounce?

        // get current status of headset (plugged or unplugged)
        BOOL bPluggedIn = (BOOL)GPIOGetBit(pDevice->hGPIO,
            pDevice->hdstDetGpio);

        DEBUGMSG(ZONE_IST,
            (TEXT("HDS_IntrThread: Event occurred (status = %x)\r\n"),
            bPluggedIn
            ));

        if ((bPluggedIn != pDevice->bPluggedIn) || fInitialDetection)
        {
            fInitialDetection = FALSE;

            // Save new state.
            pDevice->bPluggedIn = bPluggedIn;

            // Send message to WAV driver when opened...
            //
            if (pDevice->hWAV != NULL)
                {
                DeviceIoControl(
                    pDevice->hWAV, IOCTL_NOTIFY_HEADSET, &pDevice->bPluggedIn,
                    sizeof(BOOL), 0, 0, NULL, NULL);
                }
            }

            // Set headset hardware state in registry.
            SetHeadsetHardwareState(bPluggedIn);
        }

cleanup:
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
