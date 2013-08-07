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


#include "stdafx.h"
#include <ddraw.h>
#include <initguid.h>
#include <objbase.h>
#include <commctrl.h>
#include <initguid.h>
#include "NullSink.h"

#define APP_NAME L"Null Sink"

using namespace MyNullSink;


// Templates
CFactoryTemplate g_Templates[] = 
{
    {
        FILTERNAME, 
        &CLSID_NullSink, 
        CNullSink::CreateInstance, 
        NULL,
        &sudNullSink 
    }
};

int g_cTemplates = sizeof(g_Templates) / sizeof(g_Templates[0]);

// Debug Zones.
#ifndef SHIP_BUILD
    #define DBGZONE_INFO    DEBUGZONE(0) /* verbose info msgs */
    #define DBGZONE_STATS   DEBUGZONE(1) /* periodically stats msgs */
    #define DBGZONE_WARNING DEBUGZONE(2)
    #define DBGZONE_ERROR   DEBUGZONE(3)
   
    DBGPARAM dpCurSettings ={
    TEXT("Null Sink Filter DLL"),
    {
        TEXT("Info"),
        TEXT("Stats"),
        TEXT("Warning"),
        TEXT("Error"),
        NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL
    },
    0x00000000F
};
#else
    #define DBGZONE_INFO    0
    #define DBGZONE_STATS   0
    #define DBGZONE_WARNING 0
    #define DBGZONE_ERROR   0
#endif



extern "C" BOOL WINAPI DllEntryPoint(HINSTANCE, ULONG, LPVOID);

STDAPI DllRegisterServer(void)
{
	HRESULT hr = AMovieDllRegisterServer2( TRUE );
    RETAILMSG(DBGZONE_INFO, (TEXT("In DllRegisterServer  hr = 0x%08X\r\n"), hr));
	return hr;
}

STDAPI DllUnregisterServer()
{
    RETAILMSG(DBGZONE_INFO, (TEXT("In DllUnregisterServer\r\n")));
	return AMovieDllRegisterServer2( FALSE );
}

BOOL APIENTRY DllMain( HANDLE hModule, 
                       DWORD  dwReason, 
                       LPVOID lpReserved) {

    BOOL hr = TRUE;
    
#ifndef SHIP_BUILD
    if (dwReason == DLL_PROCESS_ATTACH) {
        RegisterDbgZones((HMODULE)hModule, &dpCurSettings);
    }
#endif
    hr = DllEntryPoint((HINSTANCE)(hModule), dwReason, lpReserved);
    return hr;
}

/////////////////////// instantiation //////////////////////////
CUnknown *WINAPI 
CNullSink::CreateInstance(LPUNKNOWN punk, HRESULT *phr) {
    static HRESULT hr = S_OK;
    if (!phr)
        phr = &hr;
    CNullSink *pNewObject = new CNullSink(L"NullSink", punk, phr);
    if (pNewObject == NULL) {
        RETAILMSG(DBGZONE_INFO, (TEXT("%s: Cannot create CNullSink object, hr=0x%08X \r\n"), APP_NAME, hr));
        *phr = E_OUTOFMEMORY;
    }
    
    RETAILMSG(DBGZONE_INFO, (TEXT("%s: CNullSink::CreateInstance obj=0x%08X hr=0x%08X \r\n"), 
        APP_NAME, pNewObject, *phr));
    return pNewObject;
}

CNullSink::CNullSink(wchar_t* pName, IUnknown * pUnk, HRESULT * phr) 
: CBaseFilter( pName, pUnk, &m_Lock, CLSID_NullSink) 
{

    RETAILMSG(DBGZONE_INFO, (TEXT("%s: CNullSink constructor \r\n"), APP_NAME));

    m_pInput = new CNullInputPin(
        L"CNullInputPin",
        this,
        &m_Lock,
        phr,
        L"Input"
        );

    if (m_pInput && SUCCEEDED(*phr)){
        return;
    }

    if (m_pInput){
        delete m_pInput;
        m_pInput = NULL;
    }

    if (SUCCEEDED(*phr)){
        *phr = E_OUTOFMEMORY;
    }
}

CNullSink::~CNullSink() 
{
    RETAILMSG(DBGZONE_INFO, (TEXT("%s: CNullSink destructor \r\n"), APP_NAME));
    
    if (m_pInput){
        delete m_pInput;
        m_pInput = NULL;
    }
}

CBasePin * CNullSink::GetPin(int Index)
{
    switch (Index)
    {
    case 0:
        return m_pInput;
        break;
    };

    return NULL;
}

int CNullSink::GetPinCount()
{
    return NUMELMS(sudpPins);
}

LPAMOVIESETUP_FILTER
CNullSink::GetSetupData()
{
    return (LPAMOVIESETUP_FILTER) &sudNullSink;
}

//
// -------------- CNullInputPin --------------
//

CNullInputPin::CNullInputPin(
    wchar_t*     pClassName,
    CNullSink*   pFilter,
    CCritSec*    pLock,
    HRESULT*     pHr,
    wchar_t*     pPinName
    ) :CBaseInputPin(pClassName, pFilter, pLock, pHr, pPinName)
{
}

CNullInputPin::
~CNullInputPin()
{
}

HRESULT
CNullInputPin::CheckMediaType( const CMediaType* pMediaType ) {
    if (!pMediaType)
        {
        ASSERT(false);
        return E_POINTER;
        }

    // accept any input media type
    return S_OK;
}

HRESULT
CNullInputPin::GetMediaType(
    int         pos,
    CMediaType* pMediaType
    ) {
    if (!pMediaType){
        ASSERT(false);
        return E_POINTER;
    }

    if (pos < 0 || pos >= NUMELMS(sudPinTypes)) {
        return VFW_S_NO_MORE_ITEMS;
    }
    
    pMediaType->InitMediaType();
    pMediaType->SetType(sudPinTypes[pos].clsMajorType);
    pMediaType->SetSubtype(sudPinTypes[pos].clsMinorType);

    return S_OK;
}

HRESULT
CNullInputPin::Receive(IMediaSample* pSample) {

    // discard the sample
    return S_OK;
}



