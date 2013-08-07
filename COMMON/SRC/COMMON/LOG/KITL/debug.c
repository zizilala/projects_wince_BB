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
#include <kitlprot.h>
#include <oal.h>

#define DEFAULT_SETTINGS    (ZONE_WARNING|ZONE_INIT|ZONE_ERROR) //|ZONE_FRAMEDUMP|ZONE_RECV|ZONE_SEND|ZONE_RETRANSMIT|ZONE_CMD|ZONE_KITL_OAL|ZONE_KITL_ETHER)

DBGPARAM dpCurSettings = {
    TEXT("KITL"), {
    TEXT("Warning"),    TEXT("Init"),       TEXT("Frame Dump"),     TEXT("Timer"),
    TEXT("Send"),       TEXT("Receive"),    TEXT("Retransmit"),     TEXT("Command"),
    TEXT("Interrupt"),  TEXT("Adapter"),    TEXT("LED"),            TEXT("DHCP"),
    TEXT("OAL"),        TEXT("Ethernet"),   TEXT("Unused"),         TEXT("Error"), },
    DEFAULT_SETTINGS,
};

