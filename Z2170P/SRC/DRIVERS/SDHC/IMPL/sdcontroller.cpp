// All rights reserved ADENEO EMBEDDED 2010
// Copyright (c) 2007, 2008 BSQUARE Corporation. All rights reserved.

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
//  File: sdcontroller.cpp
//
//

#include <windows.h>
#include <initguid.h>
#include "SDController.h"
#include <nkintr.h>
#include <twl.h>
// needed for CPU revision constants
#include <oalex.h>

#define DEBOUNCE_TIME           100     // 100ms debounce timeout

//------------------------------------------------------------------------------
//  Global variables

CSDIOController::CSDIOController()
    : CSDIOControllerBase()
{
}

CSDIOController::~CSDIOController()
{
}

BOOL CSDIOController::InitializeWPDetect(void)
{
    if (m_dwCardWPGPIO != (DWORD) MMC_NO_GPIO_CARD_WP)
    {
        GPIOSetMode( m_hGPIO, m_dwCardWPGPIO, GPIO_DIR_INPUT );
        IsWriteProtected();
    }
    return TRUE;
}

BOOL CSDIOController::DeInitializeWPDetect(void)
{
    return TRUE;
}

// Is this card write protected?
BOOL CSDIOController::IsWriteProtected()
{
    BOOL fWriteProtected;

    if (m_dwCardWPGPIO == MMC_NO_GPIO_CARD_WP)
	    return FALSE;

    if ( GPIOGetBit( m_hGPIO, m_dwCardWPGPIO ) == m_dwCardWriteProtectedState )
    {
        DEBUGMSG(SDCARD_ZONE_INIT, (TEXT("IsWriteProtected: YES.\r\n")));
        #ifdef ENABLE_RETAIL_OUTPUT
            RETAILMSG(1, (TEXT("IsWriteProtected(%d): YES.\r\n"), m_dwCardWPGPIO));
        #endif
        fWriteProtected = TRUE;
    }
    else
    {
        DEBUGMSG(SDCARD_ZONE_INIT, (TEXT("IsWriteProtected: NO.\r\n")));
        #ifdef ENABLE_RETAIL_OUTPUT
            RETAILMSG(1, (TEXT("IsWriteProtected(%d): NO.\r\n"), m_dwCardWPGPIO));
        #endif
        fWriteProtected = FALSE;
    }

    return fWriteProtected;
}

// Is the card present?
BOOL CSDIOController::SDCardDetect()
{
    return GPIOGetBit(m_hGPIO, m_dwCardDetectGPIO) == m_dwCardInsertedGPIOState;
}

//-----------------------------------------------------------------------------
BOOL CSDIOController::InitializeCardDetect()
{
    GPIOSetMode(m_hGPIO, m_dwCardDetectGPIO, GPIO_DIR_INPUT | GPIO_INT_LOW_HIGH | GPIO_INT_HIGH_LOW | GPIO_DEBOUNCE_ENABLE);
    GPIOPullup(m_hGPIO, m_dwCardDetectGPIO, TRUE);
    return TRUE;
}

//-----------------------------------------------------------------------------
BOOL CSDIOController::DeInitializeCardDetect()
{
    if ( m_hGPIO )
    {
        // disable wakeup on card detect interrupt
        if ( m_dwWakeupSources & WAKEUP_CARD_INSERT_REMOVAL )
        {
            GPIOInterruptWakeUp(m_hGPIO, m_dwCardDetectGPIO, m_dwCDSysintr, FALSE);
        }

        GPIOInterruptMask(m_hGPIO, m_dwCardDetectGPIO, m_dwCDSysintr, TRUE);
        if (m_hCardDetectEvent != NULL)
        {
            GPIOInterruptDisable(m_hGPIO, m_dwCardDetectGPIO, m_dwCDSysintr);
        }
    }
    return TRUE;
}

//-----------------------------------------------------------------------------
BOOL CSDIOController::InitializeHardware()
{
    DWORD               dwCountStart;

    if (INVALID_HANDLE_VALUE == (m_hGPIO = GPIOOpen()))
	{
        RETAILMSG(1, (L"Can't open GPIO driver\r\n"));
		goto cleanUp;
	}

    // check for required configuration values
    if (m_dwCardDetectGPIO == MMC_NO_GPIO_CARD_DETECT || m_dwCardInsertedGPIOState == MMC_NO_GPIO_CARD_DETECT_STATE)
	{
        RETAILMSG(1, (L"No card detect GPIO pin and state selected for slot %d (check registry)\r\n", m_dwSlot));
	    return FALSE;
	}
	
    if (m_dwCardWPGPIO != MMC_NO_GPIO_CARD_WP && m_dwCardWriteProtectedState == MMC_NO_GPIO_CARD_WP_STATE)
	{
        RETAILMSG(1, (L"No write protect GPIO state selected for slot %d (check registry)\r\n", m_dwSlot));
	    return FALSE;
	}

    // Reset the controller
    OUTREG32(&m_pbRegisters->MMCHS_SYSCONFIG, MMCHS_SYSCONFIG_SOFTRESET);

    // calculate timeout conditions
    dwCountStart = GetTickCount();

    // Verify that reset has completed.
    while (!(INREG32(&m_pbRegisters->MMCHS_SYSSTATUS) & MMCHS_SYSSTATUS_RESETDONE))
    {
        Sleep(0);

        // check for timeout
        if ((GetTickCount() - dwCountStart) > m_dwMaxTimeout)
        {
            DEBUGMSG(ZONE_ENABLE_ERROR, (TEXT("InitializeModule() - exit: TIMEOUT.\r\n")));
            goto cleanUp;
        }
    }

    InitializeCardDetect();
    InitializeWPDetect();

    return TRUE;
cleanUp:
    return FALSE;
}

void CSDIOController::DeinitializeHardware()
{
    DeInitializeCardDetect();
    DeInitializeWPDetect();
}

//-----------------------------------------------------------------------------
//  SDHCCardDetectIstThreadImpl - card detect IST thread for driver
//  Input:  lpParameter - pController - controller instance
//  Output:
//  Return: Thread exit status
DWORD CSDIOController::SDHCCardDetectIstThreadImpl()
{
    BOOL    bDebounce       = TRUE;

    if (!CeSetThreadPriority(GetCurrentThread(), m_dwCDPriority ))
    {
        DEBUGMSG(SDCARD_ZONE_WARN, (TEXT("SDHCCardDetectIstThread: warning, failed to set CEThreadPriority\r\n")));
    }

    // Associate event with GPIO interrupt
    if (!GPIOInterruptInitialize(m_hGPIO, m_dwCardDetectGPIO, &m_dwCDSysintr, m_hCardDetectEvent)) 
    {
        RETAILMSG (SDCARD_ZONE_ERROR, (L"ERROR: CSDIOController::SDHCCardDetectIstThreadImpl: Failed associate event with CD GPIO interrupt\r\n"));
        goto cleanUp;
    }
    
    // enable wakeup on card detect interrupt
    if ( m_dwWakeupSources & WAKEUP_CARD_INSERT_REMOVAL )
    {
        GPIOInterruptWakeUp(m_hGPIO, m_dwCardDetectGPIO, m_dwCDSysintr, TRUE);
    }

    // check for the card already inserted when the driver is loaded
    SetEvent( m_hCardDetectEvent );

    // Loop until we are not stopped...
    for(;;)
    {
        DWORD dwWaitStatus = 0;

        // Wait for event
        dwWaitStatus = WaitForSingleObject(m_hCardDetectEvent, ((bDebounce==TRUE) ? DEBOUNCE_TIME : INFINITE));

        if (m_fDriverShutdown)
            break;

        SDHCDAcquireHCLock(m_pHCContext);

        if (m_fCardPresent == TRUE)
        {
            // Notify card de-assertion
            #ifdef ENABLE_RETAIL_OUTPUT
                RETAILMSG(1, (L"SDHC slot %d card removal\r\n", m_dwSlot));
            #endif
            CardInterrupt(FALSE);
            bDebounce = TRUE;
        }
        else
        {
            if ((SDCardDetect() == TRUE) && (dwWaitStatus == WAIT_TIMEOUT))
            {
                #ifdef ENABLE_RETAIL_OUTPUT
                RETAILMSG(1, (L"SDHC slot %d card insertion\r\n", m_dwSlot));
                #endif
                CardInterrupt(TRUE);
                bDebounce = FALSE;
            }
            else
            {
                bDebounce = (dwWaitStatus == WAIT_TIMEOUT) ? FALSE : TRUE;
            }
        }

        SDHCDReleaseHCLock(m_pHCContext);               
		GPIOInterruptDone(m_hGPIO,m_dwCardDetectGPIO,m_dwCDSysintr);
    }

cleanUp:
    return ERROR_SUCCESS;
}

//-----------------------------------------------------------------------------
VOID CSDIOController::PreparePowerChange(CEDEVICE_POWER_STATE curPowerState, BOOL bInPowerHandler)
{
    UNREFERENCED_PARAMETER(bInPowerHandler);
	UNREFERENCED_PARAMETER(curPowerState);
}

VOID CSDIOController::PostPowerChange(CEDEVICE_POWER_STATE curPowerState, BOOL bInPowerHandler)
{
    UNREFERENCED_PARAMETER(bInPowerHandler);
	UNREFERENCED_PARAMETER(curPowerState);
}

VOID CSDIOController::TurnCardPowerOn()
{
    SetSlotPowerState( D0 );
}

VOID CSDIOController::TurnCardPowerOff()
{
    SetSlotPowerState( D4 );
}

CSDIOControllerBase *CreateSDIOController()
{
    return new CSDIOController();
}

