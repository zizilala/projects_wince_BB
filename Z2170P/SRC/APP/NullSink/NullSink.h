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

#pragma once
#include <assert.h>
#include <dshow.h>
#include "Resources.h"

#define FILTERNAME L"NullSinkFilter"

// Media types
const AMOVIESETUP_MEDIATYPE sudPinTypes [] =
{
    {&MEDIATYPE_Video,&MEDIASUBTYPE_NULL},
    {&MEDIATYPE_VideoBuffered,&MEDIASUBTYPE_NULL}
};

// Pins
const AMOVIESETUP_PIN sudpPins[] =
{
    {
        L"Input",                   // Pin string name
        FALSE,                      // Is it rendered
        FALSE,                      // Is it an output
        FALSE,                      // Allowed none
        FALSE,                      // Likewise many
        &CLSID_NULL,                // Connects to filter
        NULL,                       // Connects to pin
        2,                          // Number of types
        sudPinTypes                 // Pin information
    }
};

// Filters
const AMOVIESETUP_FILTER sudNullSink =
{
    &CLSID_NullSink,        // Filter CLSID
    FILTERNAME,             // String name
    MERIT_DO_NOT_USE,       // Filter merit
    1,                      // Number of pins
    sudpPins                // Pin information
};

namespace MyNullSink {

class CNullInputPin;

// define the filter class
class CNullSink : public CBaseFilter
{
private:
    CCritSec       m_Lock;
    CNullInputPin *m_pInput;
public:
    DECLARE_IUNKNOWN
    // instantiation
    CNullSink(wchar_t * pName, IUnknown * pOuter, HRESULT * phr);
    ~CNullSink();

    static CUnknown* CreateInstance(IUnknown* pUnk, HRESULT*  pHr);
    CBasePin* GetPin(int Index);
    int GetPinCount();
    LPAMOVIESETUP_FILTER GetSetupData();

};


//
// -------------- CNullInputPin --------------
//

class CNullInputPin: public CBaseInputPin
{
public:
    CNullInputPin(
        wchar_t*     pClassName,
        CNullSink*   pFilter,
        CCritSec*    pLock,
        HRESULT*     pHr,
        wchar_t*     pPinName
        );

    ~CNullInputPin();

    HRESULT
    CheckMediaType(
        const CMediaType* pMediaType
        );

    HRESULT
    GetMediaType(
        int         Position,
        CMediaType* pMediaType
        );

    STDMETHODIMP
    Receive(
        IMediaSample* pSample
        );
};

}

