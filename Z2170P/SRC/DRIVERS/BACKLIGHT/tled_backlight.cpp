// All rights reserved ADENEO EMBEDDED 2010
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
//  File: tled_backlight.cpp
//
#include "bsp.h"
#include "ceddkex.h"
#include "bsp_backlight_class.h"
#include <initguid.h>
#include "tps659xx_tled.h"

//------------------------------------------------------------------------------
//  Device registry parameters

static const DEVICE_REGISTRY_PARAM s_deviceRegParams[] = {
    {
    L"TLedChannel", PARAM_DWORD, TRUE, offset(TLED_Backlight, m_channel),
        fieldsize(TLED_Backlight, m_channel), NULL
    }, {
    L"DutyCycles", PARAM_MULTIDWORD, TRUE, offset(TLED_Backlight, m_dutyCycles),
        fieldsize(TLED_Backlight, m_dutyCycles), NULL
    }
};

//------------------------------------------------------------------------------
//
//  Function:  Initialize
//
//  class initializer
//
BOOL
TLED_Backlight::Initialize(LPCTSTR szContext)
{
    RETAILMSG(1, (L"+TLED_Backlight::Initialize(%s)\r\n", szContext));

    BOOL rc = FALSE;
    if (ParentClass::Initialize(szContext) == FALSE)
	{
        DEBUGMSG(ZONE_ERROR, (L"ERROR: TLED_Backlight::Initialize: "
            L"Parent class failed to initialize\r\n"));
        goto cleanUp;
	}

    // Read device parameters
    if (GetDeviceRegistryParams(
            szContext, this, dimof(s_deviceRegParams), s_deviceRegParams
            ) != ERROR_SUCCESS)
	{
        DEBUGMSG(ZONE_ERROR, (L"ERROR: TLED_Backlight::Initialize: "
            L"Failed read gpio driver registry parameters\r\n"));
        goto cleanUp;
	}

    m_hTled = TLEDOpen();
    if (m_hTled == NULL)
	{
        DEBUGMSG(ZONE_ERROR, (L"ERROR: TLED_Backlight::Initialize: "
            L"Failed allocate TLED handle\r\n"));
        goto cleanUp;
	}

    //brian TLEDSetChannel(m_hTled, m_channel);

    rc = TRUE;

cleanUp:
    RETAILMSG(1, (L"-TLED_Backlight::Initialize\r\n"));

    return rc;
}


//------------------------------------------------------------------------------
//
//  Function:  Uninitialize
//
//  class uninitializer
//
BOOL
TLED_Backlight::Uninitialize()
{
    DEBUGMSG(ZONE_FUNCTION, (L"+TLED_Backlight::Uninitialize()\r\n"));

    ParentClass::Uninitialize();

    // release gpio handle
    if (m_hTled != NULL)
	{
        TLEDClose(m_hTled);
        m_hTled = NULL;
	}

    DEBUGMSG(ZONE_FUNCTION, (L"-TLED_Backlight::Uninitialize()\r\n"));

    return TRUE;
}


//------------------------------------------------------------------------------
//
//  Function:  SetPowerState
//
//  Sets the device power state
//
BOOL
TLED_Backlight::SetPowerState(CEDEVICE_POWER_STATE power)
{
    RETAILMSG(1, (L"+TLED_Backlight::SetPowerState(0x%08X)\r\n", power));

    ParentClass::SetPowerState(power);

	RETAILMSG(1, (L" TLED_Backlight::SetPowerState(0x%08X)\r\n", m_dutyCycles[power]));

    if (D0 <= power && power <= D4)
	{
        TLEDSetDutyCycle(m_hTled, m_dutyCycles[power]);
	}
	
    RETAILMSG(1, (L"-TLED_Backlight::SetPowerState(0x%08X)\r\n", power));

    return TRUE;
}



