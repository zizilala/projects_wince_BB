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
// Copyright (c) Microsoft Corporation.  All rights reserved.
//
//
// Use of this source code is subject to the terms of the Microsoft shared
// source or premium shared source license agreement under which you licensed
// this source code. If you did not accept the terms of the license agreement,
// you are not authorized to use this source code. For the terms of the license,
// please see the license agreement between you and Microsoft or, if applicable,
// see the SOURCE.RTF on your install media or the root of your tools installation.
// THE SOURCE CODE IS PROVIDED "AS IS", WITH NO WARRANTIES.
//

#include <windows.h>
#include <windowsx.h>
#include <shellapi.h>

// Debug Zones.
#ifndef SHIP_BUILD
    #define DBGZONE_INFO	 DEBUGZONE(0) /* verbose info msgs */
    #define DBGZONE_STATS	 DEBUGZONE(1) /* periodically print cpu load */
    #define DBGZONE_PER	     DEBUGZONE(2) /* verbose performance-related msgs */
    #define DBGZONE_WARNING  DEBUGZONE(3)
    #define DBGZONE_ERROR    DEBUGZONE(4)

    extern DBGPARAM dpCurSettings;    
#else
    #define DBGZONE_INFO			0
    #define DBGZONE_STATS		    0
    #define DBGZONE_PER		    	0
    #define DBGZONE_WARNING 	    0
    #define DBGZONE_ERROR   		0
#endif


class AutoCoInit_t
{
    unsigned long m_Count;
public:
    AutoCoInit_t() :
        m_Count(0)
    {
    }

    ~AutoCoInit_t()
    {
        while (m_Count-- > 0)
            {
            CoUninitialize();
            }
    }

    HRESULT Init(DWORD dwCoInit)
    {
        HRESULT hr = CoInitializeEx(NULL, dwCoInit);
        if (SUCCEEDED(hr))
            {
            m_Count++;
            }

        return hr;
    }
};

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPTSTR, int);
LRESULT CALLBACK MainWindowProc(HWND, UINT, WPARAM, LPARAM);
bool BuildGraph();
bool CreateMainWindow(LPCTSTR);
bool OnKeyDown(HWND, WPARAM);

