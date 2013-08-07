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
//  File:  wrappers.c
//
#include <windows.h>
#include <nkintr.h>
#include <oal.h>

//------------------------------------------------------------------------------
//
//  Global:  g_szBuffer
//
//  Global static variable used as buffer for debug output functions. As long
//  as we suppose that code is run in single thread enviroment no protection
//  is needed.
//
static WCHAR g_szBuffer[384];

//------------------------------------------------------------------------------
//
//  Function:  NKDbgPrintfW
//
//
VOID NKDbgPrintfW(LPCWSTR pszFormat, ...)  
{
    va_list pArgList;

    va_start(pArgList, pszFormat);
    NKvDbgPrintfW(pszFormat, pArgList);
    va_end(pArgList);
}

//------------------------------------------------------------------------------
//
//  Function:  NKvDbgPrintfW
//
//
VOID NKvDbgPrintfW(LPCWSTR pszFormat, va_list pArgList)
{
    // Format string in buffer - it safe to use global buffer
    // as long as boot loader is nonpreemptive....
    NKwvsprintfW(
        g_szBuffer, pszFormat, pArgList, sizeof(g_szBuffer)/sizeof(WCHAR)
    );
    OEMWriteDebugString(g_szBuffer);
}

//------------------------------------------------------------------------------

