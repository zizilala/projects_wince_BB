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

/*
    This file was obtained from http://www.codeproject.com/KB/windows/samplegrabberfilter-wm6.aspx.
    The site states it is licensed under "The Creative Commons Attribution-ShareAlike 2.5 License". 
    There were no licensing headers in the downloaded source.  You can view the license here:
    http://creativecommons.org/licenses/by-sa/2.5/
*/

#include "stdafx.h"
#include <windows.h>
#include <winbase.h>
#include <objbase.h>
#include <commctrl.h>
#include <initguid.h>
//#include "Resources.h"
#include "SampleGrabber.h"

//// {AD5DB5B4-D1AB-4f37-A60D-215154B4ECC1}
//DEFINE_GUID(CLSID_SampleGrabber, 
//						0xad5db5b4, 0xd1ab, 0x4f37, 0xa6, 0xd, 0x21, 0x51, 0x54, 0xb4, 0xec, 0xc1);
//DEFINE_GUID(IID_ISampleGrabber, 
//						0x4951bff, 0x696a, 0x4ade, 0x82, 0x8d, 0x42, 0xa5, 0xf1, 0xed, 0xb6, 0x31);

// Templates
CFactoryTemplate g_Templates[] = 
{
	{
		FILTERNAME, 
		&CLSID_SampleGrabber, 
		CSampleGrabber::CreateInstance, 
		NULL,
		&sudSampleGrabber 
	}
};

int g_cTemplates = sizeof(g_Templates) / sizeof(g_Templates[0]);

extern "C" BOOL WINAPI DllEntryPoint(HINSTANCE, ULONG, LPVOID);

STDAPI DllRegisterServer(void)
{
	HRESULT hr = AMovieDllRegisterServer2( TRUE );
	return hr;
}

STDAPI DllUnregisterServer()
{
	return AMovieDllRegisterServer2( FALSE );
}

BOOL APIENTRY DllMain( HANDLE hModule, 
                       DWORD  ul_reason_for_call, 
                       LPVOID lpReserved
					 )
{
	try {
		if (ul_reason_for_call==DLL_PROCESS_ATTACH)
			CoInitializeEx(NULL, COINIT_MULTITHREADED);
		if (ul_reason_for_call==DLL_PROCESS_DETACH)
			CoUninitialize();
		return DllEntryPoint((HINSTANCE)(hModule), ul_reason_for_call, lpReserved);
	}
	catch( ... ) {
		return FALSE; // exception catched
	}
}
