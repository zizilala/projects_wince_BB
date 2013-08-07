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
#include <ddraw.h>
#include <initguid.h>
#include <objbase.h>
#include <commctrl.h>
#include <initguid.h>
#include "SampleGrabber.h"

#define APP_NAME L"Sample Grabber"

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

// Debug Zones.
#ifndef SHIP_BUILD
    #define DBGZONE_INFO    DEBUGZONE(0) /* verbose info msgs */
    #define DBGZONE_STATS   DEBUGZONE(1) /* periodically stats msgs */
    #define DBGZONE_WARNING DEBUGZONE(2)
    #define DBGZONE_ERROR   DEBUGZONE(3)
   
    DBGPARAM dpCurSettings ={
    TEXT("Sample Grabber Filter DLL"),
    {
        TEXT("Info"),
        TEXT("Stats"),
        TEXT("Warning"),
        TEXT("Error"),
        NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL
    },
    0x00000000E
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
    RETAILMSG(DBGZONE_INFO, (TEXT("%s: In DllMain - DllEntryPoint returned 0x%08X\r\n"), APP_NAME, hr));
    return hr;
}

/////////////////////// Instantiation //////////////////////////
CUnknown *WINAPI 
CSampleGrabber::CreateInstance(LPUNKNOWN punk, HRESULT *phr) {
    static HRESULT hr = S_OK;
    if (!phr)
        phr = &hr;
    CSampleGrabber *pNewObject = new CSampleGrabber(punk, phr, FALSE);
    if (pNewObject == NULL)
        *phr = E_OUTOFMEMORY;
    
    RETAILMSG(DBGZONE_INFO, (TEXT("%s: CSampleGrabber::CreateInstance obj=0x%08X hr=0x%08X \r\n"), 
        APP_NAME, pNewObject, *phr));
    return pNewObject;
}

CSampleGrabber::CSampleGrabber(IUnknown * pOuter, HRESULT * phr, BOOL ModifiesData) 
    : CTransInPlaceFilter( FILTERNAME, (IUnknown*) pOuter, CLSID_SampleGrabber, phr) 
{

    RETAILMSG(DBGZONE_INFO, (TEXT("%s: CSampleGrabber constructor \r\n"), APP_NAME));
    callback = NULL;
}

CSampleGrabber::~CSampleGrabber() 
{
    RETAILMSG(DBGZONE_INFO, (TEXT("%s: CSampleGrabber destructor \r\n"), APP_NAME));
    callback = NULL;
}

/////////////////////// IUnknown //////////////////////////
HRESULT
CSampleGrabber::NonDelegatingQueryInterface(const IID &riid, void **ppv) {
    if ( riid == IID_ISampleGrabber ) {
        return GetInterface(static_cast<ISampleGrabber*>(this), ppv);
    }
    else
        return CTransInPlaceFilter::NonDelegatingQueryInterface(riid, ppv);
}


/////////////////////// IUnknown methods //////////////////////////
LPAMOVIESETUP_FILTER
CSampleGrabber::GetSetupData()
{
    return (LPAMOVIESETUP_FILTER) &sudSampleGrabber;
}


/////////////////////// CTransInPlaceFilter methods //////////////////////////
CBasePin *
CSampleGrabber::GetPin(int n)
{
    HRESULT hr = S_OK;

    // Create an input pin if not already done

    if (m_pInput == NULL) {

        m_pInput = new CSampleGrabberInputPin( NAME("Sample Grabber input pin")
                                            , this        // Owner filter
                                            , &hr         // Result code
                                            , L"Input"    // Pin name
                                            );

        // Constructor for CTransInPlaceInputPin can't fail
        ASSERT(SUCCEEDED(hr));
    }

    // Create an output pin if not already done

    if (m_pInput!=NULL && m_pOutput == NULL) {

        m_pOutput = new CSampleGrabberOutputPin( NAME("Sample Grabber output pin")
                                              , this       // Owner filter
                                              , &hr        // Result code
                                              , L"Output"  // Pin name
                                              );

        // a failed return code should delete the object

        ASSERT(SUCCEEDED(hr));
        if (m_pOutput == NULL) {
            delete m_pInput;
            m_pInput = NULL;
        }
    }

    // Return the appropriate pin

    ASSERT (n>=0 && n<=1);
    if (n == 0) {
        return m_pInput;
    } else if (n==1) {
        return m_pOutput;
    } else {
        return NULL;
    }

} // GetPin



HRESULT 
CSampleGrabber::CheckInputType(const CMediaType *pmt)
{
    if (pmt->majortype == MEDIATYPE_Video) {
        return S_OK;
    }
    else {
        return E_FAIL;
    }
}

HRESULT 
CSampleGrabber::SetMediaType(PIN_DIRECTION direction, const CMediaType *pmt)
{
    HRESULT hr = S_OK;

    if (pmt->majortype != MEDIATYPE_Video ) {

        RETAILMSG(DBGZONE_ERROR, (TEXT("%s: SetMediaType failed \r\n"), APP_NAME));
        hr = E_FAIL;
    }

    return hr;
}

HRESULT 
CSampleGrabber::Transform(IMediaSample *pMediaSample)
{    

    if ( !pMediaSample )
        return E_FAIL;

    // invoke managed delegate
    if ( callback )
        callback(pMediaSample);

    return S_OK;
}

HRESULT 
CSampleGrabber::DecideBufferSize(IMemAllocator *pAlloc,ALLOCATOR_PROPERTIES *pProperties)
{

    RETAILMSG(DBGZONE_INFO, (TEXT("%s: DecideBufferSize \r\n"), APP_NAME));

    if (m_pInput->IsConnected() == FALSE) {
        RETAILMSG(DBGZONE_ERROR, (TEXT("%s: DecideBufferSize failed- input not connected\r\n"), APP_NAME));
        return E_UNEXPECTED;
    }

    ASSERT(pAlloc);
    ASSERT(pProperties);
    HRESULT hr = NOERROR;

    pProperties->cBuffers = 1;
    pProperties->cbBuffer = m_pInput->CurrentMediaType().GetSampleSize();
    ASSERT(pProperties->cbBuffer);

    // Ask the allocator to reserve us some sample memory, NOTE the function
    // can succeed (that is return NOERROR) but still not have allocated the
    // memory that we requested, so we must check we got whatever we wanted
    ALLOCATOR_PROPERTIES Actual;
    hr = pAlloc->SetProperties(pProperties,&Actual);
    if (FAILED(hr)) {
        RETAILMSG(DBGZONE_ERROR, (TEXT("%s: DecideBufferSize failed- SetProperties failed\r\n"), APP_NAME));
        return hr;
    }

    ASSERT( Actual.cBuffers >= 1 );

    if (pProperties->cBuffers > Actual.cBuffers ||
        pProperties->cbBuffer > Actual.cbBuffer) {
        RETAILMSG(DBGZONE_ERROR, (TEXT("%s: DecideBufferSize failed- SetProperties failed\r\n"), APP_NAME));
        return E_FAIL;
    }
    RETAILMSG(DBGZONE_INFO, (TEXT("%s: DecideBufferSize successful\r\n"), APP_NAME));
    return NOERROR;
}

HRESULT 
CSampleGrabber::GetMediaType(int iPosition, CMediaType *pMediaType)
{
    
    RETAILMSG(DBGZONE_INFO, (TEXT("%s: GetMediaType \n"), APP_NAME));
    // Is the input pin connected
    if (m_pInput->IsConnected() == FALSE) {
        RETAILMSG(DBGZONE_ERROR, (TEXT("%s: GetMediaType- ERROR - input pin not connected\r\n"), APP_NAME));
        return E_UNEXPECTED;
    }

    // This should never happen
    if (iPosition < 0) {
        RETAILMSG(DBGZONE_ERROR, (TEXT("%s: GetMediaType- ERROR - iPosition < 0\r\n"), APP_NAME));
        return E_INVALIDARG;
    }

    // Do we have more items to offer
    if (iPosition > 0) {
        RETAILMSG(DBGZONE_ERROR, (TEXT("%s: GetMediaType- ERROR - iPosition > 0\r\n"), APP_NAME));
        return VFW_S_NO_MORE_ITEMS;
    }

    *pMediaType = m_pInput->CurrentMediaType();
    
    RETAILMSG(DBGZONE_INFO, (TEXT("%s: GetMediaType successful\r\n"), APP_NAME));
    return NOERROR;
}

/////////////////////// ISampleGrabber //////////////////////////
STDMETHODIMP
CSampleGrabber::RegisterCallback( MANAGEDCALLBACKPROC mdelegate ) 
{
    // Set pointer to managed delegate
    callback = mdelegate;
    return S_OK;
}




////////////////////// CSampleGrabberInputPin class ////////////////////

CSampleGrabberInputPin::CSampleGrabberInputPin(
    TCHAR               *pObjectName,
    CSampleGrabber      *pFilter,
    HRESULT             *phr,
    LPCWSTR              pName):
    CTransInPlaceInputPin(pObjectName,
                        pFilter,
                        phr,
                        pName),
    m_pGrabberFilter(pFilter)
  
{


}

STDMETHODIMP 
CSampleGrabberInputPin::EnumMediaTypes(
               IEnumMediaTypes **ppEnum)
{
    // Can only pass through if connected
    if(!m_pGrabberFilter->m_pOutput->IsConnected())
        return CBasePin::EnumMediaTypes(ppEnum);

    return CTransInPlaceInputPin::EnumMediaTypes(ppEnum);

}


HRESULT 
CSampleGrabberInputPin::GetMediaType(int iPosition, CMediaType *pMediaType)
{

    RETAILMSG(DBGZONE_INFO, (TEXT("CSampleGrabberInputPin::GetMediaType for position %d\r\n"), iPosition));
    CheckPointer(pMediaType,E_POINTER);

    FreeMediaType(*pMediaType);
    pMediaType->InitMediaType();
    VIDEOINFOHEADER *p_vih = NULL;


    if (iPosition < 0 ) { 
        RETAILMSG(DBGZONE_ERROR, 
            (TEXT("CSampleGrabberInputPin::GetMediaType : ERROR - Invalid media type position %d! \r\n"), 
            iPosition));
        return E_INVALIDARG;
    }

    p_vih = (VIDEOINFOHEADER*) pMediaType->AllocFormatBuffer(sizeof(VIDEOINFOHEADER));
    p_vih->bmiHeader.biBitCount       = 16;
    p_vih->bmiHeader.biSize           = sizeof(BITMAPINFOHEADER);
    p_vih->bmiHeader.biWidth          = 0;
    p_vih->bmiHeader.biHeight         = 0;
    p_vih->bmiHeader.biPlanes         = 1;
    p_vih->bmiHeader.biClrImportant   = 0;
    p_vih->bmiHeader.biClrUsed        = 0;
    p_vih->bmiHeader.biXPelsPerMeter  = 0;
    p_vih->bmiHeader.biYPelsPerMeter  = 0;
    p_vih->dwBitRate                  = 0;
    p_vih->dwBitErrorRate             = 0;
    p_vih->AvgTimePerFrame            = 0;

    switch (iPosition) {

        case 0:
            p_vih->bmiHeader.biCompression    = MEDIASUBTYPE_UYVY.Data1;
            break;

        case 1:
            p_vih->bmiHeader.biCompression    = MEDIASUBTYPE_YUY2.Data1;
            break;

        default: // should never get here
            FreeMediaType(*pMediaType);
            return VFW_S_NO_MORE_ITEMS;
    }

    SetRectEmpty(&(p_vih->rcSource)); // we want the whole image area rendered.
    SetRectEmpty(&(p_vih->rcTarget)); // no particular destination rectangle

    p_vih->bmiHeader.biSizeImage  = GetBitmapSize(&p_vih->bmiHeader);

    pMediaType->SetType(&MEDIATYPE_Video);

    pMediaType->SetFormatType(&FORMAT_VideoInfo);

    pMediaType->SetTemporalCompression(FALSE);

    // Work out the GUID for the subtype from the header info.
    const GUID subTypeGUID = GetBitmapSubtype(&p_vih->bmiHeader);
    pMediaType->SetSubtype(&subTypeGUID);

    // Indicate that the encoded output will be varying in size
    pMediaType->SetSampleSize(0);
    pMediaType->SetVariableSize();

    return NOERROR;
} 


////////////////////// CSampleGrabberOutputPin class ////////////////////

CSampleGrabberOutputPin::CSampleGrabberOutputPin(
    TCHAR               *pObjectName,
    CSampleGrabber      *pFilter,
    HRESULT             *phr,
    LPCWSTR              pName):
    CTransInPlaceOutputPin(pObjectName,
                        pFilter,
                        phr,
                        pName),
    m_pGrabberFilter(pFilter)
  
{


}

STDMETHODIMP 
CSampleGrabberOutputPin::EnumMediaTypes(
               IEnumMediaTypes **ppEnum)
{
    // Can only pass through if connected
    if(!m_pGrabberFilter->m_pInput->IsConnected())
        return CBasePin::EnumMediaTypes(ppEnum);

    return CTransInPlaceOutputPin::EnumMediaTypes(ppEnum);

}


HRESULT 
CSampleGrabberOutputPin::GetMediaType(int iPosition, CMediaType *pMediaType)
{

    RETAILMSG(DBGZONE_INFO, (TEXT("CSampleGrabberOutputPin::GetMediaType for position %d\r\n"), iPosition));
    CheckPointer(pMediaType,E_POINTER);

    FreeMediaType(*pMediaType);
    pMediaType->InitMediaType();
    VIDEOINFOHEADER *p_vih = NULL;


    if (iPosition < 0 ) { 
        RETAILMSG(DBGZONE_ERROR, 
            (TEXT("CSampleGrabberOutputPin::GetMediaType : ERROR - Invalid media type position %d! \r\n"), 
            iPosition));
        return E_INVALIDARG;
    }

    p_vih = (VIDEOINFOHEADER*) pMediaType->AllocFormatBuffer(sizeof(VIDEOINFOHEADER));
    p_vih->bmiHeader.biBitCount       = 16;
    p_vih->bmiHeader.biSize           = sizeof(BITMAPINFOHEADER);
    p_vih->bmiHeader.biWidth          = 0;
    p_vih->bmiHeader.biHeight         = 0;
    p_vih->bmiHeader.biPlanes         = 1;
    p_vih->bmiHeader.biClrImportant   = 0;
    p_vih->bmiHeader.biClrUsed        = 0;
    p_vih->bmiHeader.biXPelsPerMeter  = 0;
    p_vih->bmiHeader.biYPelsPerMeter  = 0;
    p_vih->dwBitRate                  = 0;
    p_vih->dwBitErrorRate             = 0;
    p_vih->AvgTimePerFrame            = 0;

    switch (iPosition) {

        case 0:
            p_vih->bmiHeader.biCompression    = MEDIASUBTYPE_UYVY.Data1;
            break;

        case 1:
            p_vih->bmiHeader.biCompression    = MEDIASUBTYPE_YUY2.Data1;
            break;

        default: // should never get here
            FreeMediaType(*pMediaType);
            return VFW_S_NO_MORE_ITEMS;
    }

    SetRectEmpty(&(p_vih->rcSource)); // we want the whole image area rendered.
    SetRectEmpty(&(p_vih->rcTarget)); // no particular destination rectangle

    p_vih->bmiHeader.biSizeImage  = GetBitmapSize(&p_vih->bmiHeader);

    pMediaType->SetType(&MEDIATYPE_Video);

    pMediaType->SetFormatType(&FORMAT_VideoInfo);

    pMediaType->SetTemporalCompression(FALSE);

    // Work out the GUID for the subtype from the header info.
    const GUID subTypeGUID = GetBitmapSubtype(&p_vih->bmiHeader);
    pMediaType->SetSubtype(&subTypeGUID);

    // Indicate that the encoded output will be varying in size
    pMediaType->SetSampleSize(0);
    pMediaType->SetVariableSize();

    return NOERROR;
} 


