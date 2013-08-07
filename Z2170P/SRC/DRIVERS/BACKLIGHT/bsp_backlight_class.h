// All rights reserved ADENEO EMBEDDED 2010
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
//  File:  bsp_backlight_class.h
//
#ifndef __BSP_BACKLIGHT_CLASS_H
#define __BSP_BACKLIGHT_CLASS_H

#include "backlight_class.h"

//-----------------------------------------------------------------------------
//  backlight object which drives the triton LED
//
class TLED_Backlight : public CBacklightRoot
{
    typedef CBacklightRoot              ParentClass;

// member variables
//---------------
//
public:

    HANDLE          m_hTled;
    DWORD           m_channel;
    DWORD           m_dutyCycles[5];


// constructor
//------------
//
public:

    TLED_Backlight() : m_hTled(NULL), m_channel((DWORD) -1)     {};


// public virtual methods
//-----------------------
//
public:

    virtual BOOL Initialize(LPCTSTR szContext);
    virtual BOOL Uninitialize();
    virtual BOOL SetPowerState(CEDEVICE_POWER_STATE power);
};


//-----------------------------------------------------------------------------
//  backlight object which drives a GPIO line
//
class GPIO_Backlight : public TLED_Backlight
{
    typedef TLED_Backlight              ParentClass;

// member variables
//---------------
//
public:

    HANDLE          m_hGpio;
    DWORD           m_gpioId;
    DWORD           m_powerMask;


// constructor
//------------
//
public:

    GPIO_Backlight() : m_hGpio(NULL), m_gpioId((DWORD) -1)      {};


// public virtual methods
//-----------------------
//
public:

    virtual BOOL Initialize(LPCTSTR szContext);
    virtual BOOL Uninitialize();
    virtual BOOL SetPowerState(CEDEVICE_POWER_STATE power);
    virtual UCHAR BacklightGetSupportedStates( );

};

#endif