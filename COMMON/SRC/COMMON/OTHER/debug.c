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
#include <windows.h>

//------------------------------------------------------------------------------

VOID OEMWriteDebugByte(UINT8 ch);

//------------------------------------------------------------------------------
//
//  Function:  OEMWriteDebugString
//
//  Output unicode string to debug serial port
//
VOID OEMWriteDebugString(UINT16 *string)
{
    while (*string != L'\0') OEMWriteDebugByte((UINT8)*string++);
}

//------------------------------------------------------------------------------
