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
//  File:  log.c
//
//  This file implements log functions.
//
#include <windows.h>
#include <nkintr.h>
#include <oal.h>

//------------------------------------------------------------------------------
//
//  Function:  OALLogSerial
//
//  This function format string and write it to serial debug output.
//
VOID OALLogSerial(LPCWSTR format, ...)
{
    va_list pArgList;
    WCHAR buffer[128];

    va_start(pArgList, format);
    NKwvsprintfW(buffer, format, pArgList, 128);
    OEMWriteDebugString(buffer);
}

//------------------------------------------------------------------------------
//
//  Function:  OALLogPrintf
//
//  It is trivial wrapper for NKwvsprintfW function...
//
VOID OALLogPrintf(LPWSTR buffer, UINT32 maxChars, LPCWSTR format, ...)
{
    va_list pArgList;
    
    va_start(pArgList, format);
    NKwvsprintfW(buffer, format, pArgList, maxChars);
}

//------------------------------------------------------------------------------


