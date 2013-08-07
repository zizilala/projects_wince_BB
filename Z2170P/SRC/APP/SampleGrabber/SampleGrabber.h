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

#pragma once
#include <assert.h>
#include <dshow.h>
#include "Resources.h"

#define FILTERNAME L"SampleGrabber"

// Media types
const AMOVIESETUP_MEDIATYPE sudPinTypes [] =
{
    {&MEDIATYPE_Video,&MEDIASUBTYPE_UYVY},
    {&MEDIATYPE_Video,&MEDIASUBTYPE_YUY2},
};

// Pins
const AMOVIESETUP_PIN sudpPins[] =
{
    {
        L"VideoInput",              // Pin string name
        FALSE,                      // Is it rendered
        FALSE,                      // Is it an output
        FALSE,                      // Allowed none
        FALSE,                      // Likewise many
        &CLSID_NULL,                // Connects to filter
        NULL,                       // Connects to pin
        2,                          // Number of types
        sudPinTypes                 // Pin information
    },
    {
        L"VideoOutput",             // Pin string name
        FALSE,                      // Is it rendered
        TRUE,                       // Is it an output
        FALSE,                      // Allowed none
        FALSE,                      // Likewise many
        &CLSID_NULL,                // Connects to filter
        NULL,                       // Connects to pin
        2,                          // Number of types
        sudPinTypes                 // Pin information
    }
};

// Forward declaration
class CSampleGrabberInputPin;

// Filters
const AMOVIESETUP_FILTER sudSampleGrabber =
{
    &CLSID_SampleGrabber,   // Filter CLSID
    FILTERNAME,             // String name
    MERIT_DO_NOT_USE,        // Filter merit
    2,                      // Number of pins
    sudpPins                // Pin information
};


 
class CSampleGrabber : 
public CTransInPlaceFilter, public ISampleGrabber
{
private:
    MANAGEDCALLBACKPROC callback;
    long m_Width;
    long m_Height;
    long m_SampleSize;
    long m_Stride;

protected:
    friend class CSampleGrabberInputPin;
    friend class CSampleGrabberOutputPin;

public:
    // instantiation
    CSampleGrabber( IUnknown * pOuter, HRESULT * phr, BOOL ModifiesData );
    ~CSampleGrabber();
    static CUnknown *WINAPI CreateInstance(LPUNKNOWN punk, HRESULT *phr);

    // IUnknown
    DECLARE_IUNKNOWN;
    STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void ** ppv);
    LPAMOVIESETUP_FILTER GetSetupData();

    // CTransInPlaceFilter
    CBasePin *GetPin(int n);
    HRESULT CheckInputType(const CMediaType *pmt);
    HRESULT SetMediaType(PIN_DIRECTION direction, const CMediaType *pmt);
    HRESULT Transform(IMediaSample *pMediaSample);
    HRESULT DecideBufferSize(IMemAllocator *pAlloc, ALLOCATOR_PROPERTIES *pProperties);
    HRESULT GetMediaType(int iPosition, CMediaType *pMediaType);
    HRESULT CheckTransform(const CMediaType *mtIn, const CMediaType *mtOut) {
            return NOERROR; }
    // HRESULT CompleteConnect(PIN_DIRECTION dir,IPin *pReceivePin);

    // ISampleGrabber
    STDMETHODIMP RegisterCallback(MANAGEDCALLBACKPROC mdelegate);
};


class CSampleGrabberInputPin : 
public CTransInPlaceInputPin
{
private:
    CSampleGrabber *m_pGrabberFilter;

public:
    CSampleGrabberInputPin(
        TCHAR               *pObjectName,
        CSampleGrabber      *pFilter,
        HRESULT             *phr,
        LPCWSTR              pName
    );
    
    STDMETHODIMP 
    EnumMediaTypes(
                   IEnumMediaTypes **ppEnum);
    HRESULT 
    GetMediaType(int iPosition, CMediaType *pMediaType);

};

class CSampleGrabberOutputPin : 
public CTransInPlaceOutputPin
{
private:
    CSampleGrabber *m_pGrabberFilter;

public:
    CSampleGrabberOutputPin(
        TCHAR               *pObjectName,
        CSampleGrabber      *pFilter,
        HRESULT             *phr,
        LPCWSTR              pName
    );
    
    STDMETHODIMP 
    EnumMediaTypes(
                   IEnumMediaTypes **ppEnum);
    HRESULT 
    GetMediaType(int iPosition, CMediaType *pMediaType);

};

