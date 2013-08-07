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
////////////////////////////////////////////////////////////////////////////////
//
//  CETK_FSTest TUX DLL
//
//  Module: globals.h
//          Declares all global variables and test function prototypes EXCEPT
//          when included by globals.cpp, in which case it DEFINES global
//          variables, including the function table.
//
//  Revision History:
//
////////////////////////////////////////////////////////////////////////////////

#ifndef __GLOBALS_H__
#define __GLOBALS_H__


#define MODE_NONE       0
#define MODE_READ       1
#define MODE_WRITE      2
#define MODE_WRITEREAD  3
#define MODE_VERIFY     4

#define FILE_SIZE_MIN                 1
#define FILE_SIZE_DEFAULT             8
#define FILE_SIZE_MAX               512

#define BLOCK_SIZE_MIN              128
#define BLOCK_SIZE_DEFAULT            0
#define BLOCK_SIZE_MAX      (256 * 1024)

#define NUM_TESTS_MIN                 1
#define NUM_TESTS_DEFAULT             1
#define NUM_TESTS_MAX              2500




////////////////////////////////////////////////////////////////////////////////
// Local macros

#ifdef __GLOBALS_CPP__
#define GLOBAL
#define INIT(x) = x
#else // __GLOBALS_CPP__
#define GLOBAL  extern
#define INIT(x)
#endif // __GLOBALS_CPP__

////////////////////////////////////////////////////////////////////////////////
// Global macros

#define countof(x)  (sizeof(x)/sizeof(*(x)))




////////////////////////////////////////////////////////////////////////////////
// Global function prototypes

void            Debug(LPCTSTR, ...);
SHELLPROCAPI    ShellProc(UINT, SPPARAM);

////////////////////////////////////////////////////////////////////////////////
// TUX Function table

#include "ft.h"

////////////////////////////////////////////////////////////////////////////////
// Globals

// Global CKato logging object. Set while processing SPM_LOAD_DLL message.
GLOBAL CKato            *g_pKato INIT(NULL);

// Global shell info structure. Set while processing SPM_SHELL_INFO message.
GLOBAL SPS_SHELL_INFO   *g_pShellInfo;







// Add more globals of your own here. There are two macros available for this:
//  GLOBAL  Precede each declaration/definition with this macro.
//  INIT    Use this macro to initialize globals, instead of typing "= ..."
//
// For example, to declare two DWORDs, one uninitialized and the other
// initialized to 0x80000000, you could enter the following code:
//
//  GLOBAL DWORD        g_dwUninit,
//                      g_dwInit INIT(0x80000000);
////////////////////////////////////////////////////////////////////////////////

// Commandline options
GLOBAL bool   g_bVerboseOutput        INIT(FALSE);
GLOBAL DWORD  g_dwTestFileSize        INIT((1024 * 1024) * FILE_SIZE_DEFAULT);
GLOBAL DWORD  g_dwBlockSize           INIT(BLOCK_SIZE_DEFAULT);
GLOBAL DWORD  g_dwNumTests            INIT(NUM_TESTS_DEFAULT);
GLOBAL DWORD  g_dwMode                INIT(MODE_NONE);
GLOBAL TCHAR *g_pFileName             INIT(NULL);
GLOBAL DWORD  g_dwNumErrors           INIT(0);
GLOBAL bool   g_bDoAllTests           INIT(FALSE);
GLOBAL bool   g_bNoCaching            INIT(FALSE);
GLOBAL bool   g_bWriteThrough         INIT(FALSE);
GLOBAL bool   g_bFlushBuffersOnWrite  INIT(FALSE);
GLOBAL bool   g_bFlushBuffersOnClose  INIT(FALSE);

#endif // __GLOBALS_H__
