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
//  File:  debug.c
//
//  This file declares dpCurSettings for debug build. Need to put this in separate file 
//  because the log library is used in both KITL and OAL. While KITL's dpCurSettings is
//  already defined in kitl.lib
//
#include <windows.h>
#include <oal.h>

#define DEFAULT_ZONE        ((1<<OAL_LOG_ERROR)|(1<<OAL_LOG_WARN)|(1<<OAL_LOG_INFO))

// This is a fix-up variable that you can optionally override in config.bib
DWORD initialOALLogZones = DEFAULT_ZONE;

DBGPARAM dpCurSettings = {
    TEXT("OAL"), {
    TEXT("Error"),      TEXT("Warning"),    TEXT("Function"),   TEXT("Info"),
    TEXT("Stub/Keyv/Args"), TEXT("Cache"),  TEXT("RTC"),        TEXT("Power"),
    TEXT("PCI"),        TEXT("Memory"),     TEXT("IO"),         TEXT("Timer"),
    TEXT("IoCtl"),      TEXT("Flash"),      TEXT("Interrupts"), TEXT("Verbose") },
    DEFAULT_ZONE
};

//------------------------------------------------------------------------------
//
//  Function:  OALLogSetZones
//
//  Updates the current zone mask.
//
VOID OALLogSetZones(UINT32 newMask)
{
    // Update dpCurSettings mask which actually controls the zones
    dpCurSettings.ulZoneMask = newMask;

    OALMSG(OAL_INFO, (
        L"INFO:OALLogSetZones: dpCurSettings.ulZoneMask: 0x%x\r\n",
        dpCurSettings.ulZoneMask
    ));
}

