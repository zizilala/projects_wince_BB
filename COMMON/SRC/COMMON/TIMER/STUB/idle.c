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
//  File: idle.c
//
//  This file contains OEMIdle function stub.
//
#include <windows.h>
#include <nkintr.h>
#include <oal.h>

//------------------------------------------------------------------------------
//
//  Function:     OEMIdle
//
//  This function is called by the kernel when there are no threads ready to 
//  run. The CPU should be put into a reduced power mode if possible and halted. 
//  It is important to be able to resume execution quickly upon receiving an 
//  interrupt.
//
//  It is assumed that interrupts are off when OEMIdle is called. Interrrupts
//  are also turned off when OEMIdle returns.
//
VOID OEMIdle(DWORD idleParam)
{
}

//------------------------------------------------------------------------------
