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
//  File: gpio_backlight.cpp
//

#include "bsp.h"
#include "ceddkex.h"
#include "bsp_backlight_class.h"
#include "sdk_gpio.h"

#define GPIO_NOT_USED   0xFFFFFFFF
//------------------------------------------------------------------------------
//  Device registry parameters

static const DEVICE_REGISTRY_PARAM s_deviceRegParams[] = {
    {
    L"Gpio", PARAM_DWORD, FALSE, offset(GPIO_Backlight, m_gpioId),
    fieldsize(GPIO_Backlight, m_gpioId), (PVOID)GPIO_NOT_USED
    },{
    L"PowerMask", PARAM_DWORD, TRUE, offset(GPIO_Backlight, m_powerMask),
    fieldsize(GPIO_Backlight, m_powerMask), NULL
    }
};


//------------------------------------------------------------------------------
//
//  Function:  Initialize
//
//  class initializer
//
BOOL
GPIO_Backlight::Initialize(LPCTSTR szContext)
{
    RETAILMSG(1, (L"+GPIO_Backlight::Initialize(%s)\r\n", szContext));

    BOOL rc = FALSE;

    if (ParentClass::Initialize(szContext) == FALSE)
	{
        DEBUGMSG(ZONE_ERROR, (L"ERROR: GPIO_Backlight::Initialize: "
            L"Parent class failed to initialize\r\n"));
        goto cleanUp;
	}

    // Read device parameters
    if (GetDeviceRegistryParams(
            szContext, this, dimof(s_deviceRegParams), s_deviceRegParams) != ERROR_SUCCESS)
	{
        DEBUGMSG(ZONE_ERROR, (L"ERROR: GPIO_Backlight::Initialize: "
            L"Failed read gpio driver registry parameters\r\n"));
        goto cleanUp;
	}
	RETAILMSG(1, (L" GPIO_Backlight::Initialize - gpio = %d\r\n", m_gpioId));
    // GPIO pin not configured, tled only
    if (m_gpioId == GPIO_NOT_USED)
    {
        DEBUGMSG(ZONE_FUNCTION, (
            L"GPIO_Backlight::Initialize: No GPIO pin allocated, continuing...\r\n"));
        rc = TRUE;
        goto cleanUp;
    }

    m_hGpio = GPIOOpen();
    if (m_hGpio == NULL)
	{
        DEBUGMSG(ZONE_ERROR, (L"ERROR: GPIO_Backlight::Initialize: "
            L"Failed allocate GPIO handle\r\n"));
        goto cleanUp;
	}

    // setup for output mode
    GPIOSetMode(m_hGpio, m_gpioId, GPIO_DIR_OUTPUT);

    rc = TRUE;
cleanUp:
    RETAILMSG(1, (L"-GPIO_Backlight::Initialize == %d\r\n", rc));

    return rc;
}


//------------------------------------------------------------------------------
//
//  Function:  Uninitialize
//
//  class uninitializer
//
BOOL
GPIO_Backlight::Uninitialize()
{
    DEBUGMSG(ZONE_FUNCTION, (L"+GPIO_Backlight::Uninitialize()\r\n"));

    ParentClass::Uninitialize();

    // release gpio handle
    if (m_hGpio != NULL)
	{
        GPIOClose(m_hGpio);
        m_hGpio = NULL;
	}

    DEBUGMSG(ZONE_FUNCTION, (L"-GPIO_Backlight::Uninitialize()\r\n"));

    return TRUE;
}

//------------------------------------------------------------------------------
//
//  Function:  SetPowerState
//
//  Sets the device power state
//
BOOL
GPIO_Backlight::SetPowerState(CEDEVICE_POWER_STATE power)
{
    RETAILMSG(1, (L"+GPIO_Backlight::SetPowerState(0x%08X)\r\n", power));

    ParentClass::SetPowerState(power);

    if (m_gpioId == GPIO_NOT_USED)
        goto cleanUp;
    
    switch (power)
	{
        case D0: // 0
        case D1: // 1
            GPIOSetBit(m_hGpio, m_gpioId);
            break;
            
        case D2: // 2
        	break; // brian test
        	
        case D3:
        case D4: // 4
            GPIOClrBit(m_hGpio, m_gpioId);
            // brian test
            GPIOSetBit(m_hGpio, 37);
            GPIOClrBit(m_hGpio, 38);
            GPIOClrBit(m_hGpio, 15);
            GPIOClrBit(m_hGpio, 16);
            GPIOClrBit(m_hGpio, 34);
            
            break;
	}    

cleanUp:
    RETAILMSG(1, (L"-GPIO_Backlight::SetPowerState(0x%08X)\r\n", power));
    
    return TRUE;
}

//------------------------------------------------------------------------------
//
//  Function:  BacklightGetSupportedStates
//
UCHAR 
GPIO_Backlight::BacklightGetSupportedStates( )
{
    DEBUGMSG( ZONE_INFO, (L"Backlight supported states bitmap: 0x%x\r\n",m_powerMask));

    return (UCHAR) m_powerMask;
}


CBacklightRoot* GetBacklightObject()
{
    return new GPIO_Backlight();
}