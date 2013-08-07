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

#include <ddraw.h>
#include <dshow.h>
#include <strmif.h>
#pragma warning(push)
#pragma warning(disable : 4127)
#include <atlcomcli.h>
#pragma warning(pop)
#include <initguid.h>
#include <streams.h>
#include "bsp_camera_prop.h"
#include "propertybag.h"
#include "main.h"

#undef ZONE_ERROR
#undef ZONE_WARNING

#define BITRATE(DX,DY,DBITCOUNT,FRAMERATE)    (DX * abs(DY) * DBITCOUNT * FRAMERATE)
#define SAMPLESIZE(DX,DY,DBITCOUNT)           (DX * abs(DY) * DBITCOUNT / 8)

#define FRAME_STATS_INTERVAL 120 // Stats are printed after every 120 frames

#define APP_NAME                    _T("CaptureTest")

#define WM_GRAPHNOTIFY              WM_APP+1


typedef enum {
    VIDENC_NONE = 0, 
    VIDENC_GRABBER, // Use sample grabber
    VIDENC_H264,
    VIDENC_MPEG4
} videncType;

// Video input names
// enum input mode
const TCHAR* videoInputName[] = 
{
    _T("YPbPr"),
    _T("Composite"),
    _T("S-Video")
};


typedef enum {
    CAPTURE_STOPPED = 0,
    CAPTURE_STARTED=1,
} captureState;

typedef enum {
    PREVIEW_STOPPED = 0,
    PREVIEW_STARTED=1,
} previewState;

typedef enum {
    GRAPH_STOPPED = 0,
    GRAPH_RUNNING = 1,
} graphState;


DBGPARAM dpCurSettings =
{
    TEXT("CapTest"),
    {
        TEXT("Info"),           // 0
        TEXT("Stats"),          // 1
        TEXT("Perf"),           // 2
        TEXT("Warning"),        // 3
        TEXT("Error"),          // 4
        NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL
    },
    0x0000001E
};

static GUID MEDIASUBTYPE_H264 = { MAKEFOURCC('h','2','6','4'), 
                                 0x0000, 0x0010, 0x80, 0x00, 0x00, 
                                 0xaa, 0x00, 0x38, 0x9b, 0x71};
static GUID MEDIASUBTYPE_H264U= { MAKEFOURCC('H','2','6','4'), 
                                 0x0000, 0x0010, 0x80, 0x00, 0x00, 
                                 0xaa, 0x00, 0x38, 0x9b, 0x71};
static GUID MEDIASUBTYPE_VSSH= { MAKEFOURCC('V','S','S','H'), 
                                 0x0000, 0x0010, 0x80, 0x00, 0x00, 
                                 0xaa, 0x00, 0x38, 0x9b, 0x71};
static GUID MEDIASUBTYPE_MP4V= { MAKEFOURCC('M','P','4','V'), 
                                 0x0000, 0x0010, 0x80, 0x00, 0x00, 
                                 0xaa, 0x00, 0x38, 0x9b, 0x71};



// callback definition
typedef void (CALLBACK *MANAGEDCALLBACKPROC)(IMediaSample* pdata);

// {AD5DB5B4-D1AB-4f37-A60D-215154B4ECC1}
//DEFINE_GUID(CLSID_SampleGrabber, 
//0xad5db5b4, 0xd1ab, 0x4f37, 0xa6, 0xd, 0x21, 0x51, 0x54, 0xb4, 0xec, 0xc1);


// ISampleGrabber interface definition
#ifdef __cplusplus
extern "C" {
#endif 
    // {04951BFF-696A-4ade-828D-42A5F1EDB631}
    DEFINE_GUID(IID_ISampleGrabber, 
    0x4951bff, 0x696a, 0x4ade, 0x82, 0x8d, 0x42, 0xa5, 0xf1, 0xed, 0xb6, 0x31);

    DECLARE_INTERFACE_(ISampleGrabber, IUnknown) {
        STDMETHOD(RegisterCallback)(MANAGEDCALLBACKPROC callback) PURE;
    };
#ifdef __cplusplus
}
#endif 


#define DEFAULT_OUTFILE _T("NUL")

// App parameters
TCHAR *f_appName        = APP_NAME;// app name
DWORD  f_bitRate          = 2000000;      // default 2.0 Mbps
int  f_frameRate        = 30;           // default: 30fps
int  f_captureWidth     = 720;          // default: 720
int  f_captureHeight    = 480;          // default: 480
BOOL f_enablePreview    = FALSE;        // disable preview by default
BOOL f_enableCapture    = TRUE;         // disable preview by default
videncType f_videncType = VIDENC_NONE;  // Use sample grabber by default
BOOL f_autoStart        = FALSE;        // Start capturing without user input
UINT f_graphRuntime     = 0;            // graph timer period
CameraInputPort_t f_videoInput = CameraInput_AV; // input video port

TCHAR f_outputFile[MAX_PATH];           // output ASF filename
TCHAR f_extOutputFile[MAX_PATH];         // full path of output file on external device
BOOL f_outputFileOnSDCard = FALSE;      // if TRUE, output file will be created on SD card
BOOL f_outputFileOnNAND = FALSE;      // if TRUE, output file will be created on NAND flash
BOOL f_outputFileOnUSB = FALSE;      // if TRUE, output file will be created on Usb storage
BOOL f_useNullSink      = FALSE;         // TRUE: use NULL sink; FALSE: use ASF writer
int f_numSamples        = 0;            // number of samples processed
captureState  f_captureState = CAPTURE_STOPPED; // capture state
previewState  f_previewState = PREVIEW_STOPPED; // preview state
graphState  f_graphState   = GRAPH_STOPPED; // graph state

LONG f_renderWidth = 720;
LONG f_renderHeight = 480;

BOOL	bStatExitThread = FALSE;
HANDLE	hStatThread = NULL;

// Windows handles
HINSTANCE                       f_AppInstance;
HWND                            f_MainWinHandle;
UINT_PTR                        f_graphTimerId = 123;


// Media Interfaces
CComPtr<IMediaControl>          pMediaControl = NULL;
CComPtr<IMediaEventEx>          pMediaEvent = NULL;
CComPtr<IMediaSeeking>          pMediaSeeking  = NULL;

// Video Window interface
CComPtr<IVideoWindow>           pVideoWindow = NULL;
CComPtr<IBasicVideo>            pBasicVideo = NULL;

// Graph Builder
CComPtr<IGraphBuilder>          pFilterGraph = NULL;
CComPtr<ICaptureGraphBuilder2>  pCaptureGraphBuilder = NULL;

// File Source Filter
CComPtr<IBaseFilter>            pFileSource = NULL;

// File source interface
CComPtr<IFileSourceFilter>      pFileInterface = NULL;

// Video Capture Filter
CComPtr<IBaseFilter>            pVideoCapture = NULL;

// Color converter
CComPtr<IBaseFilter>            pColorConverter = NULL;

// Renderer
CComPtr<IBaseFilter>            pVideoRenderer = NULL;

// Capture interfaces
CComPtr<IAMVideoControl>        pVideoControl = NULL;
CComPtr<IAMCameraControl>       pCameraControl = NULL;
CComPtr<IAMDroppedFrames>       pDroppedFrames = NULL;
CComPtr<IAMVideoProcAmp>        pVideoProcAmp = NULL;
CComPtr<IKsPropertySet>         pKsPropertySet = NULL;
CComPtr<IPersistPropertyBag>    pVideoPropertyBag = NULL;
CComPtr<IAMStreamConfig>        pCaptureStreamConfig = NULL;



// Encoder Filters
CComPtr<IBaseFilter>            pVideoEncoder = NULL;
CComPtr<IAMStreamConfig>        pEncoderStreamConfig = NULL;


// MUX Filter
CComPtr<IBaseFilter>            pAsfWriter = NULL;
CComPtr<IFileSinkFilter>        pFileSink = NULL;

// Null Renderer
// CComPtr<IBaseFilter>            pNullRenderer = NULL;

// Sample Grabber filter
CComPtr<IBaseFilter>            pGrabberFilter = NULL;
CComPtr<ISampleGrabber>         pSampleGrabber = NULL;


// Null Sink filter
CComPtr<IBaseFilter>            pNullSink = NULL;


// Forward declarations
HRESULT GetFirstCameraDriver( WCHAR *pwzName );
HRESULT SelectVideoCaptureDevice();
HRESULT SelectVideoCaptureInput();

void    DumpMediaType(AM_MEDIA_TYPE* pFormat);

HRESULT GetVideoCaptureInterfaces();
void    ReleaseVideoCaptureInterfaces();

HRESULT FindInterfaces();
void    ReleaseInterfaces();


HRESULT CreateVideoSource();
void    ReleaseVideoSource();

HRESULT CreateVideoEncoder();
void    ReleaseVideoEncoder();

HRESULT CreateVideoRenderer();
void    ReleaseVideoRenderer();

HRESULT CreateASFWriter(LPCOLESTR pszFileName);
void    ReleaseASFWriter();

HRESULT CreateNullRenderer();
void    ReleaseNullRenderer();

HRESULT CreateGrabber();
void    ReleaseGrabber();


HRESULT CreateNullSink();
void    ReleaseNullSink();

HRESULT CreateFilters(LPCOLESTR pszOutputFileName);
HRESULT AddFiltersToGraph();
HRESULT RemoveFiltersFromGraph();

void    ReleaseFilters();

HRESULT CreateGraph();
void    ReleaseGraph();
HRESULT RenderCaptureGraph();
HRESULT RenderPreviewGraph();
HRESULT RunGraph();
HRESULT StopGraph();

HRESULT StartCapture();
HRESULT StopCapture();
HRESULT StartPreview();
HRESULT StopPreview();


#define STATS_INTERVAL 120 // dump stats every 100 frames

void 
PrintCpuUsage()
{
    static DWORD dwStartTick = 0;
    static DWORD dwStopTick = 0;
    static DWORD dwIdleStart = 0;
    static DWORD dwIdleEnd = 0;
    DWORD dwPercentIdle = 0;

    dwStartTick = dwStopTick;
    dwIdleStart = dwIdleEnd;
    dwStopTick = GetTickCount();
    dwIdleEnd = GetIdleTime();
    dwPercentIdle = ((100*(dwIdleEnd - dwIdleStart)) / (dwStopTick - dwStartTick));

    RETAILMSG(DBGZONE_STATS, \
        (TEXT("%s: CPU Utilization = %d\r\n"),
        APP_NAME,
        (100 - dwPercentIdle)));

}

void
ComputeFrameStats()
{
    static LONGLONG numFrames = 0;
    static LONGLONG startTime = 0;
    static LONGLONG totalTime = 0;
    static LONGLONG minTime   = MAXLONGLONG;
    static LONGLONG maxTime   = 0;
    static LONGLONG interval  = FRAME_STATS_INTERVAL;
    LONGLONG timeDelta = 0;

    // Update stats
    if (numFrames == 0) {
        startTime = GetTickCount();
        totalTime = 0;
    }
    else {
        timeDelta = GetTickCount() - startTime;
        startTime += timeDelta;
        totalTime += timeDelta;

        // Update max time between received frames 
        maxTime = ((maxTime < timeDelta) ? timeDelta : maxTime);
        minTime = ((minTime > timeDelta) ? timeDelta : minTime);
    }

    // Increment frame counter
    numFrames++;

    // Print out stats after 'NUMFRAMESPERSTATS' frames 
    if (numFrames % interval == 0) {
        LONGLONG fps = 0;

        if (totalTime == 0) {
            RETAILMSG(DBGZONE_INFO, \
                (TEXT("%s: Num Frames = %d, totalTime = 0\r\n"),
                APP_NAME,
                numFrames));
            fps = 0;
        }
        else {
            fps = (1000LL * (LONGLONG)interval)/totalTime;
        }

        RETAILMSG(DBGZONE_STATS, \
            (TEXT("%s: Stats for the last %d frames------->\r\n"),
            APP_NAME,
            interval));

        RETAILMSG(DBGZONE_STATS, \
            (TEXT("%s: avg fps = %I64d, time bet frames (ms)-")
             TEXT(" avg = %I64d, max = %I64d, min = %I64d\r\n"), 
            APP_NAME,
            fps,
            totalTime/(LONGLONG)interval,
            maxTime,
            minTime));

        // Reset stats counters
        totalTime = 0;
        maxTime = 0;
        minTime = 0;

        PrintCpuUsage();
    }

};


void 
DumpStreamStats()
{
    HRESULT hr = S_OK;
    long avgFrameSize = 0;
    long numDropped = 0;
    long numNotDropped = 0;

    RETAILMSG(DBGZONE_STATS, 
            (TEXT("%s: Stream Stats - Number of captured samples = %ld\r\n"), 
            APP_NAME, f_numSamples));

    if (!pDroppedFrames) {
        RETAILMSG(DBGZONE_ERROR, 
            (TEXT("%s: ERROR in DumpFrameStats- Dropped frames interface is NULL\r\n"), 
            APP_NAME));
        return;
    }

    pDroppedFrames->GetAverageFrameSize(&avgFrameSize);
    if (FAILED(hr)) {
        RETAILMSG(DBGZONE_ERROR, 
            (TEXT("%s: ERROR in DumpFrameStats- GetAverageFrameSize failed, hr = 0x%08X\r\n"), 
            APP_NAME, hr));
        return;
    }
    RETAILMSG(DBGZONE_STATS, 
        (TEXT("%s: Stream Stats- Average Frame Size = %ld\r\n"), 
        APP_NAME, avgFrameSize));

    pDroppedFrames->GetNumDropped(&numDropped);
    if (FAILED(hr)) {
        RETAILMSG(DBGZONE_ERROR, 
            (TEXT("%s: ERROR in DumpFrameStats- GetNumDropped failed, hr = 0x%08X\r\n"), 
            APP_NAME, hr));
        return;
    }
    RETAILMSG(DBGZONE_STATS, 
        (TEXT("%s: Stream Stats- Frames Dropped = %ld\r\n"), 
        APP_NAME, numDropped));

    pDroppedFrames->GetNumNotDropped(&numNotDropped);
    if (FAILED(hr)) {
        RETAILMSG(DBGZONE_ERROR, 
            (TEXT("%s: ERROR in DumpFrameStats- GetNumNotDropped failed, hr = 0x%08X\r\n"), 
            APP_NAME, hr));
        return;
    }
    RETAILMSG(DBGZONE_STATS, 
        (TEXT("%s: Stream Stats- Frames Not Dropped = %ld\r\n"), 
        APP_NAME, numNotDropped));

    return;

}


void SampleCB(IMediaSample *pSample) {

    ComputeFrameStats();

}

static DWORD StatisticsOutputFunc(LPVOID param)
{
	BOOL *pbExit = (BOOL*)param;

	for(;;)
	{
		if (*pbExit)
		{
			break;
		}

		DumpStreamStats();

		Sleep(2000);
	}

	return 0;
}

HRESULT 
GetFirstCameraDriver( WCHAR *pwzName ) {
    HRESULT hr = S_OK;
    HANDLE handle = NULL;
    DEVMGR_DEVICE_INFORMATION di;
    GUID guidCamera = { 0xCB998A05, 0x122C, 0x4166, 0x84, 0x6A,
                      0x93, 0x3E, 0x4D, 0x7E, 0x3C, 0x86 };
//    GUID guidCamera = DEVCLASS_CAMERA_GUID;
    if( pwzName == NULL ) {
        return E_POINTER;
    }

    di.dwSize = sizeof(di);

    handle = FindFirstDevice( DeviceSearchByGuid, &guidCamera, &di );
    if(( handle == NULL ) || ( di.hDevice == NULL )) {
        hr = HRESULT_FROM_WIN32( GetLastError() );
        RETAILMSG(DBGZONE_ERROR, (TEXT("%s: ERROR - Find first camera device failed, hr=0x%08X\r\n"), APP_NAME, hr));
        goto cleanup;
    }

    StringCchCopy( pwzName, MAX_PATH, di.szLegacyName );

cleanup:
    
    FindClose( handle );
    return hr;
}


void
DumpMediaType(AM_MEDIA_TYPE* pFormat, TCHAR *formatString, size_t len) {

    TCHAR majortype[128], subtype[128];
    RECT *src, *tgt;
    DWORD *bitrate;
    REFERENCE_TIME *avgtimeperframe;
    BITMAPINFOHEADER *pBIH;

    
    if(IsEqualGUID(pFormat->majortype, MEDIATYPE_Video)) {
        _tcscpy(majortype, TEXT("Video"));
    }
    else if(IsEqualGUID(pFormat->majortype, MEDIATYPE_VideoBuffered)) {
        _tcscpy(majortype, TEXT("VideoBuffered"));
    }
    else {
        _tcscpy(majortype, TEXT("Unknown"));
    }



    if(IsEqualGUID(pFormat->subtype, MEDIASUBTYPE_UYVY)) {
        _tcscpy(subtype, TEXT("UYVY"));
    }
    else if(IsEqualGUID(pFormat->subtype, MEDIASUBTYPE_YV12)) {
        _tcscpy(subtype, TEXT("YV12"));
    }
    else if(IsEqualGUID(pFormat->subtype, MEDIASUBTYPE_YUY2)) {
        _tcscpy(subtype, TEXT("YUY2"));
    }
    else if(IsEqualGUID(pFormat->subtype, MEDIASUBTYPE_IJPG)) {
        _tcscpy(subtype, TEXT("IJPG"));
    }
    else if(IsEqualGUID(pFormat->subtype, MEDIASUBTYPE_RGB565)) {
        _tcscpy(subtype, TEXT("RGB565"));
    }
    else if(IsEqualGUID(pFormat->subtype, MEDIASUBTYPE_RGB555)) {
        _tcscpy(subtype, TEXT("RGB555"));
    }
    else if(IsEqualGUID(pFormat->subtype, MEDIASUBTYPE_RGB24)) {
        _tcscpy(subtype, TEXT("RGB24"));
    }
    else if(IsEqualGUID(pFormat->subtype, MEDIASUBTYPE_H264)) {
        _tcscpy(subtype, TEXT("H264"));
    }
    else if(IsEqualGUID(pFormat->subtype, MEDIASUBTYPE_H264U)) {
        _tcscpy(subtype, TEXT("H264U"));
    }
    else if(IsEqualGUID(pFormat->subtype, MEDIASUBTYPE_VSSH)) {
        _tcscpy(subtype, TEXT("VSSH"));
    }
    else if(IsEqualGUID(pFormat->subtype, MEDIASUBTYPE_MP4V)) {
        _tcscpy(subtype, TEXT("MP4V"));
    }
    else {
        _tcscpy(subtype, TEXT("Unknown"));
    }

    if (pFormat->formattype == FORMAT_VideoInfo) {
        VIDEOINFOHEADER *pVIH = (VIDEOINFOHEADER *) pFormat->pbFormat;
        src = &pVIH->rcSource;
        tgt = &pVIH->rcTarget;
        bitrate = &pVIH->dwBitRate;
        avgtimeperframe = &pVIH->AvgTimePerFrame;
        pBIH = &pVIH->bmiHeader;
        // DumpVideoInfoHeader((VIDEOINFOHEADER *) pFormat->pbFormat);
    } 
    else if (pFormat->formattype == FORMAT_VideoInfo2) {
        VIDEOINFOHEADER2 *pVIH = (VIDEOINFOHEADER2 *) pFormat->pbFormat;
        src = &pVIH->rcSource;
        tgt = &pVIH->rcTarget;
        bitrate = &pVIH->dwBitRate;
        avgtimeperframe = &pVIH->AvgTimePerFrame;
        pBIH = &pVIH->bmiHeader;
        // DumpVideoInfoHeader2((VIDEOINFOHEADER2 *) pFormat->pbFormat);
    } 
    else {
        _sntprintf(formatString, len-1, (TEXT("Unknown format structure in media type\r\n")));
        return;
    } 

    LONG mbps = *bitrate/1000000L;
    LONG kbps = (*bitrate%1000000L)/1000L;
    ASSERT(*avgtimeperframe != 0);
    LONG fps = 10000000L/((LONG) *avgtimeperframe);


    _sntprintf(formatString, len-1, (TEXT("%s, %s, %ldx%ld, %d bpp, %ld fps, %ld.%ld mbps")), 
        majortype,
        subtype,
        abs(pBIH->biWidth),
        abs(pBIH->biHeight),
        pBIH->biBitCount,
        fps,
        mbps,
        kbps);

}


void
DumpStreamCaps(IAMStreamConfig *pStreamConfig) {

    HRESULT hr = S_OK;
    int count=0, size=0;
    AM_MEDIA_TYPE *pMediaType = NULL;
    BYTE *pSCC = NULL;
    

    hr = pStreamConfig->GetNumberOfCapabilities(&count, &size);
    if (FAILED(hr) || (count == 0) || (size != sizeof(VIDEO_STREAM_CONFIG_CAPS))) {
        RETAILMSG(DBGZONE_ERROR, 
            (TEXT("%s: ERROR - DumpStreamCaps: GetNumberOfCapabilities  failed, hr=0x%08X\r\n"), 
            APP_NAME, hr));
        return;
    }

    
    pSCC = new BYTE[size];
    if (pSCC == NULL)
    {
        RETAILMSG(DBGZONE_ERROR, 
            (TEXT("%s: ERROR - DumpStreamCaps: Memory alloc failed\r\n"), 
            APP_NAME));
        return;
    }

    int i = 0;

    while(i < count) {
        static TCHAR formatString[256];
    
        hr = pStreamConfig->GetStreamCaps(
                                i,
                                &pMediaType,
                                pSCC);
        if (FAILED(hr) || (pMediaType == NULL)) {
            RETAILMSG(DBGZONE_ERROR, 
                (TEXT("%s: ERROR - DumpStreamCaps: GetStreamCaps for index %d failed, hr=0x%08X\r\n"), 
                APP_NAME, i, hr));
            goto cleanup;
        }


        DumpMediaType(pMediaType, formatString, 256);
        RETAILMSG(DBGZONE_STATS, 
            (TEXT("%s:%d - (%s)\r\n"), 
            APP_NAME, i, formatString));
        DeleteMediaType(pMediaType);
        i++;

    }

cleanup:
    delete [] pSCC;
}

HRESULT
SetCaptureParameters()
{

    HRESULT hr = NOERROR;
    AM_MEDIA_TYPE* pMediaType = NULL;

    if (!pCaptureStreamConfig) {
        return E_FAIL;
    }

    hr = pCaptureStreamConfig->GetFormat(&pMediaType);

    if (FAILED(hr)) {
        RETAILMSG(DBGZONE_ERROR, 
            (TEXT("%s: GetFormat on video capture stream failed, hr=0x%08X\r\n"),
            APP_NAME, 
            hr));
        return hr;
    }

    VIDEOINFOHEADER * pVIH = NULL;
    pVIH = reinterpret_cast<VIDEOINFOHEADER*>(pMediaType->pbFormat);

    if (f_frameRate == 0) {
        f_frameRate = 30;
    }
    
    pVIH->bmiHeader.biWidth = f_captureWidth; // new with
    pVIH->bmiHeader.biHeight = -f_captureHeight; // new height
    pVIH->bmiHeader.biSizeImage = SAMPLESIZE(f_captureWidth, f_captureHeight, 16);
    pVIH->AvgTimePerFrame = 10000000L/f_frameRate;
    pVIH->dwBitRate = BITRATE(f_captureWidth, f_captureHeight, 16, f_frameRate);
    

    hr = pCaptureStreamConfig->SetFormat(pMediaType);

    // Rease media type resource
    DeleteMediaType(pMediaType);

    if (FAILED(hr)) {
        RETAILMSG(DBGZONE_ERROR, 
            (TEXT("%s: SetFormat on video capture stream failed, hr=0x%08X\r\n"),
            APP_NAME, 
            hr));
        return hr;
    }

    return (hr);

}


HRESULT
SetEncoderParameters()
{

    HRESULT hr = NOERROR;
    AM_MEDIA_TYPE* pMediaType = NULL;

    if (!pEncoderStreamConfig) {
        return E_FAIL;
    }

    hr = pEncoderStreamConfig->GetFormat(&pMediaType);

    if (FAILED(hr)) {
        RETAILMSG(DBGZONE_ERROR, 
            (TEXT("%s: GetFormat on video encoder failed, hr=0x%08X\r\n"),
            APP_NAME, 
            hr));
        return hr;
    }

    VIDEOINFOHEADER * pVIH = NULL;
    pVIH = reinterpret_cast<VIDEOINFOHEADER*>(pMediaType->pbFormat);

    DWORD currentBitrate = pVIH->dwBitRate;
    
    if (f_bitRate != currentBitrate) {
        pVIH->dwBitRate = f_bitRate;
        RETAILMSG(DBGZONE_INFO, 
            (TEXT("%s: bitrate: current=%d, new=%d\r\n"), APP_NAME, currentBitrate, f_bitRate));
    }

    pVIH->dwBitRate = f_bitRate;


    hr = pEncoderStreamConfig->SetFormat(pMediaType);

    // Rease media type resource
    DeleteMediaType(pMediaType);

    if (FAILED(hr)) {
        RETAILMSG(DBGZONE_ERROR, 
            (TEXT("%s: SetFormat on video encoder failed, hr=0x%08X\r\n"),
            APP_NAME, 
            hr));
        return hr;
    }

    return (hr);



}


HRESULT
SelectVideoCaptureDevice()
{
    HRESULT hr = S_OK;
    WCHAR   wzCameraDeviceName[MAX_PATH+1];

    // Get first camera device name
    hr = GetFirstCameraDriver(wzCameraDeviceName);

    if(FAILED(hr))
    {
        RETAILMSG(DBGZONE_ERROR, (TEXT("%s: ERROR - Could not obtain camera driver name, hr=0x%08X\r\n"), APP_NAME, hr));
        return (hr);
    }

    RETAILMSG(DBGZONE_INFO, (TEXT("%s: First Camera Device Name = %s\r\n"), APP_NAME, wzCameraDeviceName));

    CPropertyBag  propBag;
    CComVariant   varCameraName;

    // Initialize the video capture filter
    VariantInit(&varCameraName);
    VariantClear(&varCameraName);

    varCameraName = wzCameraDeviceName;

    if( varCameraName.vt != VT_BSTR ){
        hr = E_OUTOFMEMORY;
        RETAILMSG(DBGZONE_INFO, (TEXT("%s: ERROR - setting varCameraName failed, hr=0x%08X\r\n"), APP_NAME, hr));
        goto failed;
    }

    hr = propBag.Write(TEXT("VCapName"), &varCameraName ); 
    if(FAILED(hr)){
        RETAILMSG(DBGZONE_INFO, (TEXT("%s: ERROR - Writing camera name to property bag failed, hr=0x%08X\r\n"), APP_NAME, hr));
        goto failed;
    }


    hr = pVideoPropertyBag->Load(&propBag, NULL);
    if(FAILED(hr)){
        RETAILMSG(DBGZONE_INFO, (TEXT("%s: ERROR - Loading the video property bag failed, hr=0x%08X\r\n"), APP_NAME, hr));
        goto failed;
    }

    return S_OK;

failed:
    return (hr);

}


HRESULT
SelectVideoCaptureInput()
{

    HRESULT hr = S_OK;

    // Set the capture input port
    hr = pCameraControl->Set((long) CameraControl_InputPort, (long) f_videoInput, 1L);

    return (hr);
}

HRESULT
GetVideoCaptureInterfaces()
{
    HRESULT hr = S_OK;
    
    // Obtain video capture interfaces
    hr = pVideoCapture.QueryInterface( &pVideoControl );
    if(FAILED(hr) || pVideoControl == NULL) {
        RETAILMSG(DBGZONE_ERROR, (TEXT("%s: ERROR - Retrieving the video control interface failed, hr=0x%08X\r\n"), APP_NAME, hr));
        goto failed;
    }

    hr = pVideoCapture.QueryInterface( &pCameraControl );
    if(FAILED(hr) || pCameraControl == NULL) {
        RETAILMSG(DBGZONE_ERROR, (TEXT("%s: ERROR - Retrieving the camera control interface failed, hr=0x%08X\r\n"), APP_NAME, hr));
        goto failed;
    }

    hr = pVideoCapture.QueryInterface( &pDroppedFrames );
    if(FAILED(hr) || pDroppedFrames == NULL) {
        RETAILMSG(DBGZONE_ERROR, (TEXT("%s: ERROR - Retrieving the dropped frames interface failed, hr=0x%08X\r\n"), APP_NAME, hr));
        goto failed;
    }

    hr = pVideoCapture.QueryInterface( &pVideoProcAmp );
    if(FAILED(hr) || pVideoProcAmp == NULL) {
        RETAILMSG(DBGZONE_ERROR, (TEXT("%s: ERROR - Retrieving the video proc amp interface failed, hr=0x%08X\r\n"), APP_NAME, hr));
        goto failed;
    }

    hr = pVideoCapture.QueryInterface( &pKsPropertySet );
    if(FAILED(hr) || pKsPropertySet == NULL) {
        RETAILMSG(DBGZONE_ERROR, (TEXT("%s: ERROR - Retrieving the property set interface failed, hr=0x%08X\r\n"), APP_NAME, hr));
        goto failed;
    }

    hr = pVideoCapture.QueryInterface( &pVideoPropertyBag );
    if(FAILED(hr) || pVideoPropertyBag == NULL) {
        RETAILMSG(DBGZONE_ERROR, (TEXT("%s: ERROR - Retrieving the video capture property bag interface failed, hr=0x%08X\r\n"), APP_NAME, hr));
        goto failed;
    }

    return S_OK;

failed:
    ReleaseVideoCaptureInterfaces();
    return (hr);
}



void
ReleaseVideoCaptureInterfaces()
{
    if (pVideoControl) {
        pVideoControl.Release();
    }

    if (pCameraControl) {
        pCameraControl.Release();
    }

    if (pCaptureStreamConfig) {
        pCaptureStreamConfig.Release();
    }

    if (pDroppedFrames) {
        pDroppedFrames.Release();
    }

    if (pVideoProcAmp) {
        pVideoProcAmp.Release();
    }

    if (pKsPropertySet) {
        pKsPropertySet.Release();
    }

    if (pVideoPropertyBag) {
        pVideoPropertyBag.Release();
    }



}


HRESULT
CreateVideoSource()
{

    HRESULT hr = S_OK;


    hr = pVideoCapture.CoCreateInstance( CLSID_VideoCapture, NULL, CLSCTX_INPROC);
    if (FAILED(hr) || (pVideoCapture == NULL)){
        RETAILMSG(DBGZONE_ERROR, (TEXT("%s: ERROR - Could not create video capture filter instance, hr=0x%08X\r\n"), APP_NAME, hr));
        return (hr);
    }

    hr = GetVideoCaptureInterfaces();
    if (FAILED(hr)) {
        RETAILMSG(DBGZONE_ERROR, (TEXT("%s: ERROR - Could not get one or more  video capture interfaces, hr=0x%08X\r\n"), APP_NAME, hr));
        return (hr);
    }

    hr = SelectVideoCaptureDevice();
    if (FAILED(hr)) {
        RETAILMSG(DBGZONE_ERROR, (TEXT("%s: ERROR - Could not set video capture device, hr=0x%08X\r\n"), APP_NAME, hr));
        ReleaseVideoCaptureInterfaces();
        return (hr);
    }
    RETAILMSG(DBGZONE_INFO, (TEXT("%s: Video capture filter instance created\r\n"), APP_NAME));

    // Select composite video input
    hr = SelectVideoCaptureInput();
    if (FAILED(hr)) {
        RETAILMSG(DBGZONE_ERROR, 
            (TEXT("%s: ERROR - Could not select %s input for video capture, hr=0x%08X\r\n"), 
            APP_NAME, videoInputName[f_videoInput], hr));
        ReleaseVideoCaptureInterfaces();
        return (hr);
    }

    RETAILMSG(DBGZONE_INFO, (TEXT("%s: %s input selected for video capture\r\n"), 
        APP_NAME, videoInputName[f_videoInput]));


    return (hr);
}

void
ReleaseVideoSource()
{
    if (pVideoCapture) {
        pVideoCapture.Release();
        RETAILMSG(DBGZONE_INFO, (TEXT("%s: Video capture device released\r\n"), APP_NAME));
    }

    ReleaseVideoCaptureInterfaces();

}


HRESULT
CreateVideoEncoder()
{
    HRESULT hr = S_OK;
    GUID guidH264 = { 0xb2e82896, 0x5bd7, 0x4299, 0xb6, 0x5d, 0x8f, 0x95, 0xa0, 0x11, 0xa2, 0xe7};
    GUID guidMPEG4 = { 0x92B89CFA, 0x52DA, 0x11DF, 0xBE, 0x33, 0x76, 0x89, 0xDF, 0xD7, 0x20, 0x85};

    switch(f_videncType) {
        case VIDENC_NONE:
        case VIDENC_GRABBER:
            return E_FAIL;

        case VIDENC_H264:
            hr = pVideoEncoder.CoCreateInstance( guidH264, NULL, CLSCTX_INPROC);
            break;

        case VIDENC_MPEG4:
            hr = pVideoEncoder.CoCreateInstance( guidMPEG4, NULL, CLSCTX_INPROC);
            break;

        default:
            RETAILMSG(DBGZONE_ERROR, 
                (TEXT("%s: ERROR - Invalid video encoder type %d\r\n"), 
                f_appName, 
                f_videncType));
            return E_FAIL;
    }

    if (FAILED(hr) || (pVideoEncoder == NULL)){
        RETAILMSG(DBGZONE_ERROR, (TEXT("%s: ERROR - Could not create Video Encoder Filter instance, hr=0x%08X\r\n"), APP_NAME, hr));
        return hr;
    }
    else {
        RETAILMSG(DBGZONE_INFO, (TEXT("%s: Video Encoder instance created\r\n"), APP_NAME));
    }

    return hr;

}

void 
ReleaseVideoEncoder()
{
    if (pEncoderStreamConfig) {
        pEncoderStreamConfig.Release();
        RETAILMSG(DBGZONE_INFO, (TEXT("%s: Video encoder stream config interface released\r\n"), APP_NAME));
    }
    

    if (pVideoEncoder) {
        pVideoEncoder.Release();
        RETAILMSG(DBGZONE_INFO, (TEXT("%s: Video encoder released\r\n"), APP_NAME));
    }

}


HRESULT 
CreateVideoRenderer()
{

    HRESULT hr = pVideoRenderer.CoCreateInstance(CLSID_VideoRenderer, NULL, CLSCTX_INPROC);
    if (FAILED(hr) || (pVideoRenderer == NULL)){
        RETAILMSG(DBGZONE_ERROR, 
            (TEXT("%s: ERROR - Could not create Video Renderer instance, hr=0x%08X\r\n"), 
            APP_NAME, hr));
    }
    else {
        RETAILMSG(DBGZONE_INFO, (TEXT("%s: Video Renderer instance created\r\n"), APP_NAME));
    }

    return hr;

}

void 
ReleaseVideoRenderer()
{
    if (pVideoRenderer) {
        pVideoRenderer.Release();
        RETAILMSG(DBGZONE_INFO, (TEXT("%s: Video Renderer released\r\n"), APP_NAME));
    }
}

HRESULT
CreateGrabber()
{
    HRESULT hr = S_OK;

    GUID guidGrabber = { 0xad5db5b4, 0xd1ab, 0x4f37, 0xa6, 0xd, 0x21, 0x51, 0x54, 0xb4, 0xec, 0xc1};

    // Create the Sample Grabber filter.
    hr = pGrabberFilter.CoCreateInstance(guidGrabber, NULL, CLSCTX_INPROC);
    if (FAILED(hr) || (pGrabberFilter == NULL)) {
        RETAILMSG(DBGZONE_ERROR, (TEXT("%s: ERROR - Could not create Sample Grabber Filter instance, hr=0x%08X\r\n"), APP_NAME, hr));
        return (hr);
    }
    else {
        RETAILMSG(DBGZONE_INFO, (TEXT("%s: Sample Grabber Filter instance created\r\n"), APP_NAME));
    }
    

    hr = pGrabberFilter->QueryInterface(IID_ISampleGrabber, (void **) &pSampleGrabber);
    if (FAILED(hr) || pSampleGrabber == NULL) {
        RETAILMSG(DBGZONE_ERROR, (TEXT("%s: ERROR - Could not get Sample Grabber interface, hr=0x%08X\r\n"), APP_NAME, hr));
    }
    else {
        RETAILMSG(DBGZONE_INFO, (TEXT("%s: Retrieved Sample Grabber interface\r\n"), APP_NAME));
    }

    hr = pSampleGrabber->RegisterCallback(&SampleCB);
    if(FAILED(hr)){
        RETAILMSG(DBGZONE_ERROR, 
            (TEXT("%s: ERROR - Setting the sample grabber callback failed, hr = 0x%08X\r\n"), 
            APP_NAME, hr));
    }
    return hr;
}


void 
ReleaseGrabber()
{

    if (pSampleGrabber) {
        pSampleGrabber.Release();
    }

    if (pGrabberFilter) {
        pGrabberFilter.Release();
        RETAILMSG(DBGZONE_INFO, (TEXT("%s: Sample grabber released\r\n"), APP_NAME));
    }


}


HRESULT
CreateNullSink()
{

    HRESULT hr = S_OK;

    GUID guidGrabber = { 0x1322bae0,0x35ff,0x11df,0x98,0x79,0x08,0x00,0x20,0x0c,0x9a,0x66};

    // Create the Sample Grabber filter.
    hr = pNullSink.CoCreateInstance(guidGrabber, NULL, CLSCTX_INPROC);
    if (FAILED(hr) || (pNullSink == NULL)) {
        RETAILMSG(DBGZONE_ERROR, (TEXT("%s: ERROR - Could not create Null Sink Filter instance, hr=0x%08X\r\n"), APP_NAME, hr));
        return (hr);
    }
    else {
        RETAILMSG(DBGZONE_INFO, (TEXT("%s: Null Sink Filter instance created\r\n"), APP_NAME));
    }

    return (hr);

}


void
ReleaseNullSink()
{
    if (pNullSink) {
        pNullSink.Release();
        RETAILMSG(DBGZONE_INFO, (TEXT("%s: Null sink released\r\n"), APP_NAME));
    }

}




HRESULT
CreateASFWriter(LPCOLESTR pszFileName)
{

    HRESULT hr = S_OK;


    hr = pCaptureGraphBuilder->SetOutputFileName(
                            &MEDIASUBTYPE_Asf, 
                            pszFileName, 
                            &pAsfWriter, 
                            &pFileSink);
    if (FAILED(hr)){
        RETAILMSG(DBGZONE_ERROR, 
            (TEXT("%s: ERROR - Could not setoutput filename, hr=0x%08X\r\n"), 
            APP_NAME, 
            hr));
        goto failed;
    }


    return S_OK;

failed:
    ReleaseASFWriter();
    return (hr);

}

void 
ReleaseASFWriter()
{

    if (pAsfWriter) {
        pAsfWriter.Release();
        RETAILMSG(DBGZONE_INFO, (TEXT("%s: ASF writer released\r\n"), APP_NAME));
    }

    if (pFileSink) {
        pFileSink.Release();
    }

}




HRESULT
CreateFilters(LPCOLESTR pszOutputFileName)
{
    HRESULT hr = S_OK;

    // video source
    hr = CreateVideoSource();

    if (FAILED(hr)){
        goto failed;
    }

    if (f_videncType == VIDENC_GRABBER) {
        // video grabber
        hr = CreateGrabber();
    }
    else if (f_videncType != VIDENC_NONE) {
        // video encoder
        hr = CreateVideoEncoder();
    
    }

    if (FAILED(hr)){
        goto failed;
    }

    hr = CreateVideoRenderer();
    if (FAILED(hr)){
        goto failed;
    }

    if (f_useNullSink) {
        // null sink
        hr = CreateNullSink();
    }
    else { 
        // asf writer
        hr = CreateASFWriter(pszOutputFileName);
    }

    if (FAILED(hr)){
        goto failed;
    }

    return S_OK;

failed:
    ReleaseFilters();
    return (hr);
}


HRESULT
AddFiltersToGraph()
{
    HRESULT hr = S_OK;

    // Video capture filter
    hr = pFilterGraph->AddFilter(pVideoCapture, TEXT("Video Capture Filter"));
    if (FAILED(hr)){
        RETAILMSG(DBGZONE_ERROR, (TEXT("%s: ERROR - Loading the video capture filter failed, hr=0x%08X\r\n"), APP_NAME, hr));
        return hr;
    }
    RETAILMSG(DBGZONE_INFO, (TEXT("%s: Video Capture Filter added to Filter Graph\r\n"), APP_NAME));

    // Get Stream config interface
    hr = pCaptureGraphBuilder->FindInterface(&PIN_CATEGORY_CAPTURE,
                                              &MEDIATYPE_Video, 
                                              pVideoCapture,
                                              IID_IAMStreamConfig, 
                                              (void **)&pCaptureStreamConfig);

    if (FAILED(hr) || (pCaptureStreamConfig == NULL)){
        RETAILMSG(DBGZONE_ERROR, (TEXT("%s: ERROR - Failed to find stream config interface, hr=0x%08X\r\n"), APP_NAME, hr));
    }
    else {
        
        RETAILMSG(DBGZONE_STATS, 
            (TEXT("%s: Capabilities of capture driver --->\r\n"), 
            APP_NAME));
        DumpStreamCaps(pCaptureStreamConfig);
    }


    if (f_videncType == VIDENC_GRABBER) {
        // Sample video grabber
        hr = pFilterGraph->AddFilter(pGrabberFilter, TEXT("Sample Grabber"));
        if (FAILED(hr)){
            RETAILMSG(DBGZONE_ERROR, (TEXT("%s: ERROR - Loading the sample grabber filter failed, hr=0x%08X\r\n"), APP_NAME, hr));
            RemoveFiltersFromGraph();
            return hr;
        }

        RETAILMSG(DBGZONE_INFO, (TEXT("%s: Sample Grabber Filter added to Filter Graph\r\n"), APP_NAME));
    }
    else if (f_videncType != VIDENC_NONE) {
        hr = pFilterGraph->AddFilter(pVideoEncoder, TEXT("Video Encoder Filter"));
        if (FAILED(hr)){
            RETAILMSG(DBGZONE_ERROR, (TEXT("%s: ERROR - Loading the video encoder filter failed, hr=0x%08X\r\n"), APP_NAME, hr));
            RemoveFiltersFromGraph();
            return hr;
        }
        RETAILMSG(DBGZONE_INFO, (TEXT("%s: Video Encoder Filter added to Filter Graph\r\n"), APP_NAME));
    }

    // Add renderer
    hr = pFilterGraph->AddFilter(pVideoRenderer, TEXT("Video Renderer"));
    if (FAILED(hr)){
        RETAILMSG(DBGZONE_ERROR, 
            (TEXT("%s: ERROR - Loading the video renderer failed, hr=0x%08X\r\n"), APP_NAME, hr));
        RemoveFiltersFromGraph();
        return hr;
    }

    if (f_useNullSink) {
        // null sink
        hr = pFilterGraph->AddFilter(pNullSink, TEXT("Null Sink"));
        if (FAILED(hr)){
            RETAILMSG(DBGZONE_ERROR, (TEXT("%s: ERROR - Loading the null sink filter failed, hr=0x%08X\r\n"), APP_NAME, hr));
            RemoveFiltersFromGraph();
            return hr;
        }
        RETAILMSG(DBGZONE_INFO, (TEXT("%s: Null sink filter added to Filter Graph\r\n"), APP_NAME));
    }
    else {
        // File sink
        hr = pFilterGraph->AddFilter(pAsfWriter, TEXT("ASF Writer Filter"));
        if (FAILED(hr)){
            RETAILMSG(DBGZONE_ERROR, (TEXT("%s: ERROR - Loading the ASF file writer filter failed, hr=0x%08X\r\n"), APP_NAME, hr));
            RemoveFiltersFromGraph();
            return hr;
        }
    }

    return S_OK;
}

HRESULT
RemoveFiltersFromGraph()
{

    HRESULT hr = S_OK;

    // Remove Video capture filter
    hr = pFilterGraph->RemoveFilter(pVideoCapture);
    if (FAILED(hr)){
        RETAILMSG(DBGZONE_ERROR, 
            (TEXT("%s: ERROR - Removing the video capture filter failed, hr=0x%08X\r\n"), 
            APP_NAME, hr));
    }
    else {
        RETAILMSG(DBGZONE_INFO, 
            (TEXT("%s: Video Capture Filter removed from Filter Graph\r\n"), 
            APP_NAME));
    }

    if (f_videncType == VIDENC_GRABBER) {
        // Remove Sample video grabber
        hr = pFilterGraph->RemoveFilter(pGrabberFilter);
        if (FAILED(hr)){
            RETAILMSG(DBGZONE_ERROR, 
                (TEXT("%s: ERROR - Removing the sample grabber filter failed, hr=0x%08X\r\n"), 
                APP_NAME, hr));
        }
        else {
            RETAILMSG(DBGZONE_INFO, 
                (TEXT("%s: Sample Grabber Filter removed from Filter Graph\r\n"), 
                APP_NAME));
        }
    }
    else if (f_videncType != VIDENC_NONE) {
        hr = pFilterGraph->RemoveFilter(pVideoEncoder);
        if (FAILED(hr)){
            RETAILMSG(DBGZONE_ERROR, 
                (TEXT("%s: ERROR - Removing the video encoder filter failed, hr=0x%08X\r\n"), 
                APP_NAME, hr));
        }
        else {
            RETAILMSG(DBGZONE_INFO, 
                (TEXT("%s: Video Encoder Filter removed from Filter Graph\r\n"), 
                APP_NAME));
        }
    }

    if (f_useNullSink) {
        // null sink
        hr = pFilterGraph->RemoveFilter(pNullSink);
        if (FAILED(hr)){
            RETAILMSG(DBGZONE_ERROR, 
                (TEXT("%s: ERROR - Removing the null sink filter failed, hr=0x%08X\r\n"), 
                APP_NAME, hr));
        }
        else {
            RETAILMSG(DBGZONE_INFO, 
                (TEXT("%s: Null sink filter removed from Filter Graph\r\n"), 
                APP_NAME));
        }
    }
    else {
        // File sink
        hr = pFilterGraph->RemoveFilter(pAsfWriter);
        if (FAILED(hr)){
            RETAILMSG(DBGZONE_ERROR, 
                (TEXT("%s: ERROR - Removing the ASF file writer filter failed, hr=0x%08X\r\n"), 
                APP_NAME, hr));
        }
        else {
            RETAILMSG(DBGZONE_INFO, 
                (TEXT("%s: ASF file writer filter removed from Filter Graph\r\n"), 
                APP_NAME));
        }
    }

    hr = pFilterGraph->RemoveFilter(pVideoRenderer);
    if (FAILED(hr)){
        RETAILMSG(DBGZONE_ERROR, 
            (TEXT("%s: ERROR - Removing the video renderer failed, hr=0x%08X\r\n"), 
            APP_NAME, hr));
    }
    else {
        RETAILMSG(DBGZONE_INFO, 
            (TEXT("%s: Video renderer removed from Filter Graph\r\n"), 
            APP_NAME));
    }

    return hr;

}

void
ReleaseFilters()
{
    ReleaseVideoSource();
    ReleaseVideoEncoder();
    ReleaseVideoRenderer();
    ReleaseASFWriter();
    ReleaseGrabber();
    ReleaseNullSink();

}

HRESULT
FindInterfaces()
{

    HRESULT hr = S_OK;
    
    // Get Capture graph builder's media seeking interface
    hr = pFilterGraph->QueryInterface(&pMediaSeeking );
    if (FAILED(hr) || pMediaSeeking == NULL){
        RETAILMSG(DBGZONE_ERROR, (TEXT("%s: ERROR - Failed to retrieve media seeking interface, hr=0x%08X\r\n"), APP_NAME, hr));
        goto failed;
     }

    // Get the Media Control Interface
    hr = pFilterGraph->QueryInterface(&pMediaControl);
    if (FAILED(hr)){
        RETAILMSG(DBGZONE_ERROR, (TEXT("%s: ERROR - Failed to retrieve Media Control interface, hr=0x%08X\r\n"), APP_NAME, hr));
        goto failed;
    }
    RETAILMSG(DBGZONE_INFO, (TEXT("%s: Retrieved Media Control interface\r\n"), APP_NAME));
    ASSERT(pMediaControl.p);

    // Get the IVideoWindow interface
    if (f_enablePreview) {
        hr = pVideoRenderer->QueryInterface(&pVideoWindow);
        if (FAILED(hr)){
            RETAILMSG(DBGZONE_ERROR, (TEXT("%s: ERROR - Failed to retrieve IVideoWindow interface, hr=0x%08X\r\n"), APP_NAME, hr));
            goto failed;
        }
        RETAILMSG(DBGZONE_INFO, (TEXT("%s: Retrieved Video Window interface\r\n"), APP_NAME));
    }


    // Get the IBasicVideo interface
    hr = pVideoRenderer->QueryInterface(&pBasicVideo);
    if (FAILED(hr)){
        RETAILMSG(DBGZONE_ERROR, (TEXT("%s: ERROR - Failed to retrieve IBasicVideo interface, hr=0x%08X\r\n"), APP_NAME, hr));
        goto failed;
    }
    RETAILMSG(DBGZONE_INFO, (TEXT("%s: Retrieved IBasicVideointerface\r\n"), APP_NAME));


    // Get the Media Event interface
    hr = pFilterGraph->QueryInterface( IID_IMediaEventEx, (void**)&pMediaEvent );
    
    if (FAILED(hr)){
        RETAILMSG(DBGZONE_ERROR, (TEXT("%s: ERROR - Failed to retrieve Media Event interface, hr=0x%08X\r\n"), APP_NAME, hr));
        goto failed;
    }

    RETAILMSG(DBGZONE_INFO, (TEXT("%s: Retrieved Media Event interface\r\n"), APP_NAME));


    // Register main window to receive media events
    hr = pMediaEvent->SetNotifyWindow((OAHWND)f_MainWinHandle, WM_GRAPHNOTIFY,0);
    if (FAILED(hr)){
        RETAILMSG(DBGZONE_ERROR, (TEXT("%s: ERROR - SetNotifyEvent failed, hr=0x%08X\r\n"), APP_NAME, hr));
        goto failed;
    }
    RETAILMSG(DBGZONE_INFO, (TEXT("%s: Window registered to receive Graph Notification events\r\n"), APP_NAME));


    return S_OK;

failed:
    ReleaseInterfaces();
    return hr;

}


void
ReleaseInterfaces()
{

    if (pMediaSeeking) {
        pMediaSeeking.Release();
        RETAILMSG(DBGZONE_INFO, (TEXT("%s: IMediaSeeking released\r\n"), APP_NAME));
    }
    
    if (pMediaEvent) {
        pMediaEvent.Release();
        RETAILMSG(DBGZONE_INFO, (TEXT("%s: IMediaEvent released\r\n"), APP_NAME));
    }

    if (pMediaControl) {
        pMediaControl.Release();
        RETAILMSG(DBGZONE_INFO, (TEXT("%s: IMediaControl released\r\n"), APP_NAME));
    }

    if (pVideoWindow) {
        pVideoWindow.Release();
        RETAILMSG(DBGZONE_INFO, (TEXT("%s: IVideoWindow released\r\n"), APP_NAME));
    }

    if (pBasicVideo) {
        pBasicVideo.Release();
        RETAILMSG(DBGZONE_INFO, (TEXT("%s: IBasicVideo released\r\n"), APP_NAME));
    }


}


HRESULT
CreateGraph()
{
    HRESULT hr = pFilterGraph.CoCreateInstance(CLSID_FilterGraph, NULL, CLSCTX_INPROC);
    if (FAILED(hr)){
        RETAILMSG(DBGZONE_ERROR, (TEXT("%s: ERROR - Could not create Filter Graph instance, hr=0x%08X\r\n"), APP_NAME, hr));
        goto failed;
    }
    RETAILMSG(DBGZONE_INFO, (TEXT("%s: Filter Graph instance created\r\n"), APP_NAME));


    
    //
    // Capture Graph Builder
    //
    hr = pCaptureGraphBuilder.CoCreateInstance(CLSID_CaptureGraphBuilder, NULL, CLSCTX_INPROC);
    if (FAILED(hr)){
        RETAILMSG(DBGZONE_ERROR, 
            (TEXT("%s: ERROR - Could not create Capture Filter Graph Builder instance, hr=0x%08X\r\n"), 
            APP_NAME, hr));
        goto failed;
    }
    RETAILMSG(DBGZONE_INFO, (TEXT("%s: Capture Filter Graph Builder instance created\r\n"), APP_NAME));


    // Set Capture Builder's filter graph to pFilterGraph
    hr = pCaptureGraphBuilder->SetFiltergraph( pFilterGraph );
    if (FAILED(hr)){
        RETAILMSG(DBGZONE_ERROR, (TEXT("%s: ERROR - SetFiltergaph failed, hr=0x%08X\r\n"), APP_NAME, hr));
        goto failed;
    }

    // Get Capture graph builder's media seeking interface
    hr = pFilterGraph->QueryInterface(&pMediaSeeking );
    if (FAILED(hr) || pMediaSeeking == NULL){
        RETAILMSG(DBGZONE_ERROR, (TEXT("%s: ERROR - Failed to retrieve media seeking interface, hr=0x%08X\r\n"), APP_NAME, hr));
        goto failed;
     }

    // Get the Media Control Interface
    hr = pFilterGraph->QueryInterface(&pMediaControl);
    if (FAILED(hr)){
        RETAILMSG(DBGZONE_ERROR, (TEXT("%s: ERROR - Failed to retrieve Media Control interface, hr=0x%08X\r\n"), APP_NAME, hr));
        goto failed;
    }
    RETAILMSG(DBGZONE_INFO, (TEXT("%s: Retrieved Media Control interface\r\n"), APP_NAME));
    ASSERT(pMediaControl.p);

    // Get the IVideoWindow interface
    if (f_enablePreview) {
        hr = pFilterGraph->QueryInterface(&pVideoWindow);
        if (FAILED(hr)){
            RETAILMSG(DBGZONE_ERROR, (TEXT("%s: ERROR - Failed to retrieve IVideoWindow interface, hr=0x%08X\r\n"), APP_NAME, hr));
            goto failed;
        }
        RETAILMSG(DBGZONE_INFO, (TEXT("%s: Retrieved Video Window interface\r\n"), APP_NAME));
    }


    // Get the IBasicVideo interface
    hr = pFilterGraph->QueryInterface(&pBasicVideo);
    if (FAILED(hr)){
        RETAILMSG(DBGZONE_ERROR, (TEXT("%s: ERROR - Failed to retrieve IBasicVideo interface, hr=0x%08X\r\n"), APP_NAME, hr));
        goto failed;
    }
    RETAILMSG(DBGZONE_INFO, (TEXT("%s: Retrieved IBasicVideointerface\r\n"), APP_NAME));


    // Get the Media Event interface
    hr = pFilterGraph->QueryInterface( IID_IMediaEventEx, (void**)&pMediaEvent );
    
    if (FAILED(hr)){
        RETAILMSG(DBGZONE_ERROR, (TEXT("%s: ERROR - Failed to retrieve Media Event interface, hr=0x%08X\r\n"), APP_NAME, hr));
        goto failed;
    }

    RETAILMSG(DBGZONE_INFO, (TEXT("%s: Retrieved Media Event interface\r\n"), APP_NAME));


    // Register main window to receive media events
    hr = pMediaEvent->SetNotifyWindow((OAHWND)f_MainWinHandle, WM_GRAPHNOTIFY,0);
    if (FAILED(hr)){
        RETAILMSG(DBGZONE_ERROR, (TEXT("%s: ERROR - SetNotifyEvent failed, hr=0x%08X\r\n"), APP_NAME, hr));
        goto failed;
    }
    RETAILMSG(DBGZONE_INFO, (TEXT("%s: Window registered to receive Graph Notification events\r\n"), APP_NAME));


    return S_OK;

failed:
    ReleaseGraph();
    return (hr);
}


void
ReleaseGraph()
{


    if (pCaptureGraphBuilder) {
        pCaptureGraphBuilder.Release();
        RETAILMSG(DBGZONE_INFO, (TEXT("%s: Capture Graph Builder released\r\n"), APP_NAME));
    }
    
    if (pFilterGraph) {
        pFilterGraph.Release();
        RETAILMSG(DBGZONE_INFO, (TEXT("%s: IFilterGraph released\r\n"), APP_NAME));
    }

}




HRESULT
RenderPreviewGraph()
{

    HRESULT hr = S_OK;
    IBaseFilter* pIntermediate = NULL;

    if (f_videncType == VIDENC_GRABBER) {
        pIntermediate = pGrabberFilter;
    }

    hr = pCaptureGraphBuilder->RenderStream( 
            &PIN_CATEGORY_PREVIEW,
            &MEDIATYPE_Video,
            pVideoCapture,
            pIntermediate,
            pVideoRenderer);

    if (FAILED(hr) && hr != VFW_S_NOPREVIEWPIN){
        RETAILMSG(DBGZONE_ERROR, (TEXT("%s: ERROR - Failed to render preview graph, hr=0x%08X\r\n"), APP_NAME, hr));
        return (hr);
    }

    RETAILMSG(DBGZONE_ERROR, (TEXT("%s: Preview active\r\n"), APP_NAME));

    return (hr);
}


HRESULT
RenderCaptureGraph()
{
    HRESULT hr = S_OK;
    IBaseFilter* pIntermediate = NULL;
    IBaseFilter* pSink = NULL;


    switch (f_videncType) {
        case VIDENC_GRABBER:
            pIntermediate = pGrabberFilter;
            pSink = pNullSink;
            break;

        
        case VIDENC_NONE:
            return E_UNEXPECTED;

        default:
            pIntermediate = pVideoEncoder;
            if (f_useNullSink) {
                pSink = pNullSink;
            }
            else {
                pSink = pAsfWriter;
            }
            break;
    }


    hr = pCaptureGraphBuilder->RenderStream( 
            &PIN_CATEGORY_CAPTURE,
            &MEDIATYPE_Video,
            pVideoCapture,
            pIntermediate,
            pSink
            );

    if (FAILED(hr)){
        RETAILMSG(DBGZONE_ERROR, (TEXT("%s: ERROR - Failed to render capture graph, hr=0x%08X\r\n"), APP_NAME, hr));
        return hr;
    }

    RETAILMSG(DBGZONE_INFO, (TEXT("%s: Capture graph rendered OK\r\n"), APP_NAME));

    
    // Block the capture till we run the graph
    hr = pCaptureGraphBuilder->ControlStream( 
                            &PIN_CATEGORY_CAPTURE,
                            &MEDIATYPE_Video, 
                            pVideoCapture,
                            0, 0, 0, 0);
    if (FAILED(hr)) {
        RETAILMSG(DBGZONE_ERROR, 
            (TEXT("%s: ERROR - RenderCaptureGraph: ControlStream to block video capture failed, hr=0x%08X\r\n"), 
            APP_NAME, hr));
        return (hr);
    }

    if (pVideoEncoder) {
        hr = pVideoEncoder.QueryInterface( &pEncoderStreamConfig);
        if(FAILED(hr) || pEncoderStreamConfig == NULL) {
            RETAILMSG(DBGZONE_ERROR, 
                (TEXT("%s: ERROR - Retrieving the encoder's stream config interface failed, hr=0x%08X\r\n"), 
                APP_NAME, 
                hr));
        }
    }

    return S_OK;
}


HRESULT RunGraph()
{

    HRESULT hr = S_OK;

    if(!pMediaControl) {
        return E_FAIL;
    }

    if (f_graphState == GRAPH_RUNNING) {
        // Filter graph already running
        return S_OK;
    }

    hr = pMediaControl->Run();
    if (FAILED(hr)) {
        RETAILMSG(DBGZONE_ERROR, 
            (TEXT("%s: pMediaControl->Run() Failed, hr=0x%08X\r\n"), 
            APP_NAME, hr));
        return (hr);
    }

    f_graphState = GRAPH_RUNNING;

    // Sleep for 1 second to give filter graph some time
    // to make sure that all of its buffers are allocated 
    // and all processes are synchronized.
    Sleep(1000);

    return S_OK;

}

HRESULT StopGraph()
{
    HRESULT hr = S_OK;

    if(!pMediaControl) {
        return E_FAIL;
    }

    if (f_graphState == GRAPH_STOPPED) {
        // Filter graph already stopped
        return S_OK;
    }

    hr = pMediaControl->Stop();

    if (FAILED(hr)) {
        RETAILMSG(DBGZONE_ERROR, 
            (TEXT("%s: pMediaControl->Stop() Failed, hr=0x%08X\r\n"), 
            APP_NAME, hr));
        return (hr);

    }

    f_graphState = GRAPH_STOPPED;


    return S_OK;
}

HRESULT
StartCapture()
{
    HRESULT hr = S_OK;

    if (f_captureState == CAPTURE_STARTED) {
        // Capture already started
        return S_OK;
    }


    // Start the capture
    REFERENCE_TIME dwStart = 0;
    REFERENCE_TIME dwEnd=MAXLONGLONG;

    hr = pCaptureGraphBuilder->ControlStream( &PIN_CATEGORY_CAPTURE,
                                         &MEDIATYPE_Video, 
                                         pVideoCapture,
                                         &dwStart, 
                                         &dwEnd, 
                                         0, 0 );
    if (FAILED(hr)) {
        RETAILMSG(DBGZONE_ERROR, 
            (TEXT("%s: ERROR - StartCapture: ControlStream to start video capture failed, hr=0x%08X\r\n"), 
            APP_NAME, hr));
        return (hr);
    }

    f_captureState = CAPTURE_STARTED;
    RETAILMSG(DBGZONE_WARNING, (TEXT("%s: CAPTURE STARTED!\r\n"), APP_NAME));
    return S_OK;

}


HRESULT
StopCapture()
{
    HRESULT hr = S_OK;

    if (f_captureState == CAPTURE_STOPPED) {
        // Capture already stopped
        return S_OK;
    }

    if(!pMediaControl || !pMediaSeeking) {
        return E_FAIL;
    }

    REFERENCE_TIME dwStart = 0;
    REFERENCE_TIME dwEnd = 0;

    pMediaSeeking->GetCurrentPosition( &dwEnd );
    hr = pCaptureGraphBuilder->ControlStream( &PIN_CATEGORY_CAPTURE,
                                         &MEDIATYPE_Video, pVideoCapture,
                                         &dwStart, 
                                         &dwEnd, 
                                         1, 2 );
    if (FAILED(hr)) {
        RETAILMSG(DBGZONE_ERROR, 
            (TEXT("%s: ERROR - StopCapture: ControlStream to stop video capture failed, hr=0x%08X\r\n"), 
            APP_NAME, hr));

        return (hr);
    }

    // Wait for Stream control stopped event
    long leventCode, param1, param2;
#pragma warning(push)
#pragma warning(disable : 4127)
    do
    {
        pMediaEvent->GetEvent( &leventCode, &param1, &param2, INFINITE );
        pMediaEvent->FreeEventParams( leventCode, param1, param2 );

        if( leventCode == EC_STREAM_CONTROL_STOPPED ) {
            RETAILMSG(DBGZONE_ERROR, 
                (TEXT("%s: EC_STREAM_CONTROL_STOPPED event received!\r\n"),
                APP_NAME));
            break;
        }
    } while(TRUE);
#pragma warning(pop)
    f_captureState = CAPTURE_STOPPED;

    RETAILMSG(DBGZONE_WARNING, (TEXT("%s: CAPTURE STOPPED!\r\n"), APP_NAME));

    return (hr);

}




HRESULT StartPreview()
{
    HRESULT hr = S_OK;
    
    if (!f_enablePreview)
        return S_OK;
        
    if (f_previewState == PREVIEW_STARTED) {
        // Preview already started
        return S_OK;
    }
        
    hr = pCaptureGraphBuilder->ControlStream(
                        &PIN_CATEGORY_PREVIEW,
                        &MEDIATYPE_Video, 
                        pVideoCapture,
                        NULL,    // Start now.
                        NULL,       // Don't care
                        0, 0); 
    if (FAILED(hr)) {
        RETAILMSG(DBGZONE_ERROR, 
            (TEXT("%s: ERROR: ControlStream to start video preview failed, hr=0x%08X\r\n"), 
            APP_NAME, hr));
    }

    f_previewState = PREVIEW_STARTED;

    RETAILMSG(DBGZONE_WARNING, (TEXT("%s: PREVIEW STARTED!\r\n"), APP_NAME));
    return (hr);

}

HRESULT StopPreview()
{

    HRESULT hr = S_OK;
    
    if (!f_enablePreview)
        return S_OK;
        
    if (f_previewState == PREVIEW_STOPPED) {
        // Preview already started
        return S_OK;
    }
    REFERENCE_TIME dwEnd=MAXLONGLONG;
 
    hr = pCaptureGraphBuilder->ControlStream(
                        &PIN_CATEGORY_PREVIEW,
                        &MEDIATYPE_Video, 
                        pVideoCapture,
                        0,       // Don't care
                        &dwEnd,  // stop now
                        0, 0); 
    if (FAILED(hr)) {
        RETAILMSG(DBGZONE_ERROR, 
            (TEXT("%s: ERROR: ControlStream to stop video preview failed, hr=0x%08X\r\n"), 
            APP_NAME, hr));
    }

    f_previewState = PREVIEW_STOPPED;

    RETAILMSG(DBGZONE_WARNING, (TEXT("%s: PREVIEW STOPPED!\r\n"), APP_NAME));
    return (hr);

}



void PrintUsage()
{

    RETAILMSG(DBGZONE_ERROR, 
        (TEXT("Usage: %s [options]\r\n"), f_appName));
    RETAILMSG(DBGZONE_ERROR, 
        (TEXT("Options:\r\n")));
    RETAILMSG(DBGZONE_ERROR, 
        (TEXT("\t/auto                 - auto-run graph without user input\r\n")));
    RETAILMSG(DBGZONE_ERROR, 
        (TEXT("\t/br {bitrate}         - Specify encode bitrate in bps\r\n")));
    RETAILMSG(DBGZONE_ERROR, 
        (TEXT("\t                        default: %ld bps\r\n"), f_bitRate));
    RETAILMSG(DBGZONE_ERROR, 
        (TEXT("\t/cap {wxh@fps}        - Specify capture params in the specified format e.g. 640x480@30,")
         TEXT("default: %ldx%ld@%ld\r\n"), f_captureWidth, f_captureHeight, f_frameRate));
    RETAILMSG(DBGZONE_ERROR, 
        (TEXT("\t/file {filename}      - Specify output ASF file\r\n")));
    RETAILMSG(DBGZONE_ERROR, 
        (TEXT("\t                        If /file option is not specified, or /venc is none, NULL sink is used\r\n")));
    RETAILMSG(DBGZONE_ERROR, 
        (TEXT("\t/sd                   - output file will be created on SD card\r\n")));
    RETAILMSG(DBGZONE_ERROR, 
        (TEXT("\t/nand                 - output file will be created on NAND flash\r\n")));
    RETAILMSG(DBGZONE_ERROR, 
        (TEXT("\t/usb                  - output file will be created on USB storage\r\n")));
    RETAILMSG(DBGZONE_ERROR, 
        (TEXT("\t/pv                   - Enable preview (disabled by default)\r\n")));
    RETAILMSG(DBGZONE_ERROR, 
        (TEXT("\t/time                 - time in ms to run graph and exit app\r\n")));
    RETAILMSG(DBGZONE_ERROR, 
        (TEXT("\t                       (ignored if /auto is not specified)\r\n")));
    RETAILMSG(DBGZONE_ERROR, 
        (TEXT("\t/venc {encoder}       - h264, mpeg4, raw or none\r\n")));

    TCHAR *venc;
    if (f_videncType == VIDENC_H264) {
        venc = _T("H.264");
    }
    else if (f_videncType == VIDENC_MPEG4) {
        venc = _T("MPEG4");
    }
    else {
        venc = _T("None");
    }
    RETAILMSG(DBGZONE_ERROR, 
        (TEXT("\t                        default: %s\r\n"), venc));
    RETAILMSG(DBGZONE_ERROR, 
        (TEXT("\t/vin {video input}    - svideo, ypbpr or av\r\n")));

    TCHAR *input;
    if (f_videoInput == CameraInput_SVideo) {
        input = _T("S-Video");
    }
    else if (f_videoInput == CameraInput_YPbPr) {
        input = _T("Component(YPbPr)");
    }
    else {
        input = _T("Composite(AV)");
    }

    RETAILMSG(DBGZONE_ERROR, 
        (TEXT("\t                        default: %s\r\n"), input));

}

BOOL ParseCaptureParams(TCHAR *params)
{
    TCHAR* token;

    // strip leading whitespace
    token = _tcstok(params, TEXT(" xX"));
    if (!token)
        return FALSE;
    f_captureWidth = _ttoi(token);
    if (!f_captureWidth || (f_captureWidth < 0)) {
        return FALSE;
    }

    token = _tcstok(NULL, TEXT("@"));
    if (!token)
        return FALSE;
    f_captureHeight = _ttoi(token);
    if (!f_captureHeight || (f_captureHeight < 0)) {
        return FALSE;
    }

    token = _tcstok(NULL, TEXT("\0"));
    if (!token)
        return FALSE;
    f_frameRate = _ttoi(token);
    if (!f_frameRate || (f_frameRate < 0)) {
        return FALSE;
    }

    return TRUE;

}

#define MAX_ARGS 256
#define MAX_CMDLINE_LEN 4096
void ParseCommandLine()
{

    // Parse argc and argv from command line string
    //  TODO: fix code to avoid possible buffer overruns
    static TCHAR cmdline[MAX_CMDLINE_LEN] ;
    static TCHAR* _argv[MAX_ARGS] ;
    static TCHAR** argv = &_argv[0];

    int argc = 0 ;
    _tcsncpy( cmdline, GetCommandLine(), MAX_CMDLINE_LEN-1 );
	cmdline[MAX_CMDLINE_LEN-1]='\0';
    RETAILMSG(DBGZONE_ERROR, (TEXT("cmdline=%s\r\n"), cmdline));
    argv[argc] = _tcstok( cmdline, TEXT(" \t") ) ;
    while( _argv[argc] != 0 && argc < MAX_ARGS)
    {
        argc++ ;
        _argv[argc] = _tcstok( 0, TEXT(" \t") ) ;
    }
    RETAILMSG(DBGZONE_INFO, (TEXT("argc=%d\r\n"),argc));


    enum {
        ARG_NONE,
        ARG_BITRATE,
        ARG_CAP_PARAMS,
        ARG_FILE,
        ARG_VIDENC,
        ARG_VIN,
        ARG_TIME,
        ARG_SD,
        ARG_NAND,
        ARG_USB
    };

    int  nextArg = ARG_NONE;

    while (argc > 0) {
        if (nextArg != ARG_NONE && !argc) {
            PrintUsage();
            exit(1);
        }
        
        if (nextArg == ARG_BITRATE) {
            f_bitRate = _ttoi(argv[0]);
            RETAILMSG(DBGZONE_ERROR, (TEXT("bitrate=%ld\r\n"),f_bitRate));
            nextArg = ARG_NONE;
            argc--;
            argv++;
        }
        else if (nextArg == ARG_TIME) {
            f_graphRuntime = _ttoi(argv[0]);
            RETAILMSG(DBGZONE_ERROR, (TEXT("graph runtime=%ld ms\r\n"),f_graphRuntime));
            nextArg = ARG_NONE;
            argc--;
            argv++;
        }
        else if (nextArg == ARG_FILE) {
            _tcsncpy(f_outputFile, argv[0], MAX_PATH);
            f_useNullSink = FALSE;
            RETAILMSG(DBGZONE_ERROR, (TEXT("output file = %s\r\n"),f_outputFile));
            nextArg = ARG_NONE;
            argc--;
            argv++;
        }
        else if (nextArg == ARG_VIN) {
            if (_tcscmp(argv[0], _T("av")) == 0) {
                f_videoInput = CameraInput_AV;
            }
            else if (_tcscmp(argv[0], _T("svideo")) == 0) {
                f_videoInput = CameraInput_SVideo;
            }
            else if (_tcscmp(argv[0], _T("ypbpr")) == 0) {
                f_videoInput = CameraInput_YPbPr;
            }
            else {
                PrintUsage();
                exit(1);
            }
            nextArg=ARG_NONE;
            argc--;
            argv++;
        }
        else if (nextArg == ARG_VIDENC) {
            if (_tcscmp(argv[0], _T("h264")) == 0) {
                f_videncType = VIDENC_H264;
            }
            else if (_tcscmp(argv[0], _T("mpeg4")) == 0) {
                f_videncType = VIDENC_MPEG4;
            }
            else if (_tcscmp(argv[0], _T("raw")) == 0) {
                f_videncType = VIDENC_GRABBER;
                f_useNullSink = TRUE;
            }
            else if (_tcscmp(argv[0], _T("none")) == 0) {
                f_videncType = VIDENC_NONE;
                f_useNullSink = FALSE;
            }
            else {
                PrintUsage();
                exit(1);
            }
            nextArg=ARG_NONE;
            argc--;
            argv++;
        }
        else if (nextArg == ARG_CAP_PARAMS) {
            if (ParseCaptureParams(argv[0]) == FALSE) {
                RETAILMSG(DBGZONE_ERROR, (TEXT("Invalid capture parameters specified - %s\r\n"),argv[0]));
                exit(1);
            }
            else {
                RETAILMSG(DBGZONE_INFO, 
                    (TEXT("capture params - width=%ld, height=%ld, fps=%ld\r\n"),
                    f_captureWidth,
                    f_captureHeight,
                    f_frameRate));
            }
            nextArg = ARG_NONE;
            argc--;
            argv++;
        }
        else if (_tcscmp(argv[0], _T("/br")) == 0) {// Next arg is bitrate
            nextArg = ARG_BITRATE;
            argc--;
            argv++;
        }
        else if (_tcscmp(argv[0], _T("/cap")) == 0) {// Next arg is capture params
            nextArg = ARG_CAP_PARAMS;
            argc--;
            argv++;
        }
        else if (_tcscmp(argv[0], _T("/file")) == 0) {// Next arg is output file name
            nextArg = ARG_FILE;
            argc--;
            argv++;
        }
        else if (_tcscmp(argv[0], _T("/sd")) == 0) {// Next arg is output file name
            f_outputFileOnSDCard = TRUE;
            argc--;
            argv++;
        }
        else if (_tcscmp(argv[0], _T("/nand")) == 0) {// Next arg is output file name
            f_outputFileOnNAND = TRUE;
            argc--;
            argv++;
        }
        else if (_tcscmp(argv[0], _T("/usb")) == 0) {// Next arg is output file name
            f_outputFileOnUSB = TRUE;
            argc--;
            argv++;
        }
        else if (_tcscmp(argv[0], _T("/vin")) == 0) {// Next arg is video codec
            nextArg = ARG_VIN;
            argc--;
            argv++;
        }
        else if (_tcscmp(argv[0], _T("/venc")) == 0) {// Next arg is video codec
            nextArg = ARG_VIDENC;
            argc--;
            argv++;
        }
        else if (_tcscmp(argv[0], _T("/pv")) == 0) {
            f_enablePreview = TRUE;
            nextArg = ARG_NONE;
            argc--;
            argv++;
        }
        else if (_tcscmp(argv[0], _T("/auto")) == 0) {
            f_autoStart = TRUE;
            nextArg = ARG_NONE;
            argc--;
            argv++;
        }
        else if (_tcscmp(argv[0], _T("/time")) == 0) {
            nextArg = ARG_TIME;
            argc--;
            argv++;
        }
        else if (_tcscmp(argv[0], _T("/?")) == 0) {
            PrintUsage();
            exit(0);
        }
        else {
            // print usage string
            PrintUsage();
            exit(1);
        }
    }

}


int WINAPI
WinMain(
    HINSTANCE hInstance,
    HINSTANCE hPrevInstance,
    LPTSTR lpCmdLine,
    int nShowCmd
    )
{
    RETAILREGISTERZONES(NULL);

    ParseCommandLine();

    f_enableCapture = TRUE;
    if ((f_videncType == VIDENC_NONE) ||
         (f_enablePreview && f_videncType == VIDENC_GRABBER))  { 
        // We will not render capture graph if user did not select an encoder
        // or if user selected grabber as 'encoder' and also enabled preview.
        // In the latter case, we will use Sample Grabber as intermediate filter 
        // in the preview graph so we can dump stats on preview frame rate etc.
        f_enableCapture = FALSE;
    }

    if (!f_enableCapture && !f_enablePreview) {
        RETAILMSG(DBGZONE_ERROR, (TEXT("%s: ERROR: Neither capture nor preview is enabled\r\n"), APP_NAME));
        exit(1);
    }

    AutoCoInit_t CoInit;
    HRESULT hr = CoInit.Init(COINIT_MULTITHREADED);
    if (FAILED(hr)){
        RETAILMSG(DBGZONE_ERROR, (TEXT("%s: ERROR initializing COM interface, hr=0x%08X\r\n"), APP_NAME, hr));
        exit(1);
    }

    hr = CreateGraph();
    if (FAILED(hr)){
        RETAILMSG(DBGZONE_ERROR, (TEXT("%s: ERROR: Failed to create filter graph\r\n"), APP_NAME));
        exit(1);
    }

    if (f_outputFileOnSDCard || f_outputFileOnNAND || f_outputFileOnUSB) {
        if (f_outputFileOnSDCard) {
            _tcscpy(f_extOutputFile, _T("\\Storage Card\\"));
        }
        else if (f_outputFileOnNAND) {
            _tcscpy(f_extOutputFile, _T("\\Mounted Volume\\"));
        }
        else {// f_outputFileOnUSB
            _tcscpy(f_extOutputFile, _T("\\Hard Disk\\"));
        }
        _tcscat(f_extOutputFile, f_outputFile);
        hr = CreateFilters((LPCOLESTR) f_extOutputFile);
        RETAILMSG(DBGZONE_INFO, (TEXT("%s:file name is %s\r\n"), APP_NAME, f_extOutputFile));
    }
    else {
        hr = CreateFilters((LPCOLESTR) f_outputFile);
    }
    if (FAILED(hr)){
        RETAILMSG(DBGZONE_ERROR, (TEXT("%s: ERROR: Failed to create filters\r\n"), APP_NAME));
        ReleaseGraph();
        exit(1);
    }

    hr = AddFiltersToGraph();
    if (FAILED(hr)){
        RETAILMSG(DBGZONE_ERROR, (TEXT("%s: ERROR: Failed to add filters to graph\r\n"), APP_NAME));
        ReleaseFilters();
        ReleaseGraph();
        exit(1);
    }

    hr = FindInterfaces();
    if (FAILED(hr)){
        RETAILMSG(DBGZONE_ERROR, (TEXT("%s: ERROR: Failed to get filter graph interfaces\r\n"), APP_NAME));
        RemoveFiltersFromGraph();
        ReleaseFilters();
        ReleaseGraph();
        exit(1);
    }

    // Set capture params before rendering graphs
    RETAILMSG(DBGZONE_ERROR, 
        (TEXT("%s: Setting capture parameters to %ldx%ld@%ldfps\r\n"), 
        APP_NAME, f_captureWidth, f_captureHeight, f_frameRate));
    hr = SetCaptureParameters();
    if (FAILED(hr)) {
        RETAILMSG(DBGZONE_ERROR, 
            (TEXT("%s: ERROR - Failed to set capture parameters, hr=0x%08X\r\n"), 
            APP_NAME, hr));
    }

	// Create a thread to output statistics
	hStatThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)StatisticsOutputFunc, (LPVOID)&bStatExitThread, 0, 0);
	if (hStatThread == NULL)
	{
		RETAILMSG(DBGZONE_ERROR, ( TEXT("CameraDShowApp: Failed to create statistics thread\r\n")));
	}

    if (f_enablePreview) {
        RenderPreviewGraph();
    }


    if (f_enableCapture) {
        hr = RenderCaptureGraph();
        if (FAILED(hr)){
            RemoveFiltersFromGraph();
            ReleaseInterfaces();
            ReleaseFilters();
            ReleaseGraph();
            exit(1);
        }

        // Set encoder params
        if (pVideoEncoder) {
            LONG mbps = f_bitRate/1000000L;
            LONG kbps = (f_bitRate%1000000L)/1000L;
            RETAILMSG(DBGZONE_ERROR, 
                (TEXT("%s: Setting encoder bitrate to %ld.%ld mbps\r\n"), 
                APP_NAME, mbps, kbps));
            hr = SetEncoderParameters();
            if (FAILED(hr)) {
                RETAILMSG(DBGZONE_ERROR, 
                    (TEXT("%s: ERROR - Failed to set encode parameters, hr=0x%08X\r\n"), 
                    APP_NAME, hr));
            }
        }
    }

    // Create main window
    if ( !CreateMainWindow( TEXT("Capture Test") ) ){
        RemoveFiltersFromGraph();
        ReleaseInterfaces();
        ReleaseFilters();
        ReleaseGraph();
        exit(1);
    }

    f_AppInstance = hInstance;
    ShowWindow(f_MainWinHandle, nShowCmd);

    UpdateWindow(f_MainWinHandle);

    MSG msg;

    while ( GetMessage(&msg, NULL, 0, 0) ) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

	if(hStatThread != NULL)
	{
		bStatExitThread = TRUE;
		WaitForSingleObject(hStatThread, 5000);
		CloseHandle(hStatThread);
	}

    return 0;

}




void
ProcessGraphMessage()
{
    HRESULT hr=S_OK;
    long leventCode, param1, param2;

    while((hr = pMediaEvent->GetEvent(&leventCode, &param1, &param2, 0)), SUCCEEDED(hr)) {

        hr = pMediaEvent->FreeEventParams(leventCode, param1, param2);

        if(EC_CAP_SAMPLE_PROCESSED == leventCode) {
            f_numSamples++;
        }
        else if (EC_STREAM_CONTROL_STARTED == leventCode) {
            RETAILMSG(DBGZONE_ERROR, (TEXT("%s: Graph Event Notification - EC_STREAM_CONTROL_STARTED\r\n"), APP_NAME));
            break;
        }
        if (EC_STREAM_CONTROL_STOPPED == leventCode) {
            RETAILMSG(DBGZONE_ERROR, (TEXT("%s: Graph Event Notification - EC_STREAM_CONTROL_STOPPED\r\n"), APP_NAME));
            break;
        }
        else if(EC_CAP_FILE_COMPLETED == leventCode) {
            RETAILMSG(DBGZONE_ERROR, (TEXT("%s: Graph Event Notification - EC_CAP_FILE_COMPLETED\r\n"), APP_NAME));
            //Handle the file capture completed event
        }
        else if(EC_CAP_FILE_WRITE_ERROR == leventCode) {
            RETAILMSG(DBGZONE_ERROR, (TEXT("%s: Graph Event Notification - EC_CAP_FILE_WRITE_ERROR\r\n"), APP_NAME));            
            StopCapture();
            break;
        }
        else if(EC_CLOCK_CHANGED == leventCode) {
            RETAILMSG(DBGZONE_INFO, (TEXT("%s: Graph Event Notification - EC_CLOCK_CHANGED\r\n"), APP_NAME));            
        }
        else if(EC_BUFFER_FULL == leventCode) {
            RETAILMSG(DBGZONE_INFO, (TEXT("%s: Graph Event Notification - EC_BUFFER_FULL\r\n"), APP_NAME));            
        }
        else if(EC_BUFFER_DOWNSTREAM_ERROR == leventCode) {
            RETAILMSG(DBGZONE_INFO, (TEXT("%s: Graph Event Notification - EC_BUFFER_DOWNSTREAM_ERROR\r\n"), APP_NAME));            
        }
        else {
            RETAILMSG(DBGZONE_ERROR, (TEXT("%s: Graph Event Notification - event code=0x%08X\r\n"), APP_NAME, leventCode));
        }
    }
}


bool
CreateMainWindow(
    LPCTSTR pWindowTitle
    )
{
    WNDCLASS Class;
    Class.style         = 0;
    Class.lpfnWndProc   = MainWindowProc;
    Class.cbClsExtra    = 0;
    Class.cbWndExtra    = 0;
    Class.hInstance     = f_AppInstance;
    Class.hIcon         = NULL;
    Class.hCursor       = NULL;
    Class.hbrBackground = CreateSolidBrush( RGB(0xC0, 0xC0, 0xC0) ); 
    Class.lpszMenuName  = NULL;
    Class.lpszClassName = TEXT("CaptureTest Class");

    ATOM ClassId = RegisterClass(&Class);
    if (!ClassId){
        return false;
    }

    RECT DesktopRect = {0};
    BOOL Succeeded = SystemParametersInfo(
        SPI_GETWORKAREA,
        NULL,
        &DesktopRect,
        NULL);
    if (!Succeeded)
        {
        return false;
        }

    LONG desktopWidth = DesktopRect.right - DesktopRect.left;
    LONG desktopHeight = DesktopRect.bottom - DesktopRect.top;

    if (desktopWidth > desktopHeight) {
        // Landscape mode
        if (((desktopWidth * f_captureHeight)/desktopHeight) > f_captureWidth) {
            // Desktop aspect ratio is greater than capture aspect ratio
            f_renderHeight = desktopHeight;
            f_renderWidth = (f_renderHeight * f_captureWidth)/f_captureHeight;
        }
        else {
            // Desktop aspect ratio is less than capture aspect ratio
            f_renderWidth = desktopWidth;
            f_renderHeight = (f_renderWidth * f_captureHeight)/f_captureWidth;
        }
    }
    else {
        // Portrait mode
        f_renderWidth = desktopWidth;
        f_renderHeight = (f_renderWidth * f_captureHeight)/f_captureWidth;
    }

    f_MainWinHandle = CreateWindow(
        Class.lpszClassName,
        pWindowTitle,
        WS_VISIBLE | WS_OVERLAPPEDWINDOW,
        DesktopRect.left,
        DesktopRect.top,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        NULL,
        NULL,
        f_AppInstance,
        NULL
        );

    return (f_MainWinHandle != NULL);
}


LRESULT CALLBACK
MainWindowProc(
    HWND hwnd,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam
    )
{
    switch (uMsg){
        case WM_CREATE:
            if (f_enablePreview) {
                // Set the main window to be the owner of the video window
                pVideoWindow->put_Owner((OAHWND)hwnd);
                pVideoWindow->put_WindowStyle(WS_CHILD | WS_CLIPSIBLINGS);
                pVideoWindow->put_MessageDrain((OAHWND) hwnd);

                RECT grc;
                GetClientRect(hwnd, &grc);

                pVideoWindow->SetWindowPosition(0, 0, f_renderWidth, f_renderHeight);
                pVideoWindow->put_WindowState(SW_SHOWNORMAL);
                pVideoWindow->put_Visible(OATRUE);
            }

            // Run immediately if autoStart is set
            if (f_autoStart) {
                HRESULT hr = S_OK;
                

                if (f_enableCapture) {
                    hr = StartCapture();
                    if (FAILED(hr)) {
                        PostMessage(hwnd, WM_CLOSE, 0, 0);
                        return (0);
                    }
                }
                hr = RunGraph();
                if (FAILED(hr)) {
                    PostMessage(hwnd, WM_CLOSE, 0, 0);
                    return (0);
                }
                
                if (f_graphRuntime) {
                    UINT_PTR result = SetTimer(hwnd, 
                                           f_graphTimerId, 
                                           f_graphRuntime, 
                                           NULL);
                    if (!result) {
                        RETAILMSG(DBGZONE_ERROR, (TEXT("%s: ERROR creating timer\r\n"), APP_NAME));
                        PostMessage(hwnd, WM_CLOSE, 0, 0);
                        return (0);
                    }

                    RETAILMSG(DBGZONE_ERROR, (TEXT("%s: Timer created, timeout=%ld ms\r\n"), APP_NAME, f_graphRuntime));
                }
            }
            break;

        case WM_DESTROY:
            PostQuitMessage(0);
            break;

        case WM_SIZE:
            if (f_enablePreview) {
                switch (wParam) {
                    case SIZE_RESTORED:
                    case SIZE_MAXIMIZED:
                    case SIZE_MAXSHOW:
                        pVideoWindow->put_WindowState(SW_SHOWNORMAL);
                        pVideoWindow->put_Visible(OATRUE);
                        break;

                    case SIZE_MAXHIDE:
                    case SIZE_MINIMIZED:
                    default:
                        pVideoWindow->put_Visible(OAFALSE);
                        pVideoWindow->put_WindowState(SW_HIDE);
                        break;
                }
            }
            break;


        case WM_CLOSE:
            RETAILMSG(DBGZONE_INFO, (TEXT("%s: WM_CLOSE received\r\n"), APP_NAME));

            if (f_enablePreview) {
                // Hide video window; otherwise it will stay on screen forever
                pVideoWindow->put_Visible(OAFALSE);
                pVideoWindow->put_MessageDrain((OAHWND) NULL);
                pVideoWindow->put_Owner((OAHWND) NULL);
            }

            if (f_enableCapture) {
                StopCapture();
            }
            StopGraph();
            if (f_graphRuntime) {
                RETAILMSG(DBGZONE_INFO, (TEXT("%s: Killing timer...\r\n"), APP_NAME));
                KillTimer(hwnd, f_graphTimerId);
            }
            RETAILMSG(DBGZONE_ERROR, (TEXT("%s: Releasing resources...\r\n"), APP_NAME));
            RemoveFiltersFromGraph();
            ReleaseInterfaces();
            ReleaseFilters();
            ReleaseGraph();
            DestroyWindow(hwnd);
            break;

        case WM_QUIT:
            RETAILMSG(DBGZONE_INFO, (TEXT("%s: WM_QUIT received\r\n"), APP_NAME));
            break;

        case WM_TIMER:
            RETAILMSG(DBGZONE_ERROR, (TEXT("%s: WM_TIMER received\r\n"), APP_NAME));
            if (wParam == f_graphTimerId) {
                PostMessage(hwnd, WM_CLOSE, 0, 0);
            }
            break;

        case WM_SETFOCUS:
        case WM_KILLFOCUS:
            break;

        case WM_SYSKEYDOWN:
        case WM_KEYDOWN:
            if ( VK_F4 == wParam && GetKeyState(VK_MENU) ){
                PostMessage(hwnd, WM_CLOSE, 0, 0);
                break;
            }

            if (OnKeyDown(hwnd, wParam)){
                break;
            }

            break;

        case WM_GRAPHNOTIFY:
            ProcessGraphMessage();
            break;

        default:
            return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }

    return 0;

}


bool
OnKeyDown(HWND hwnd, WPARAM wParam)
{
    if (!pMediaControl){
        return false;
    }
   

    switch (wParam)
        {
        case 'Q': 
            PostMessage(hwnd, WM_CLOSE, 0, 0);
            break;

        case 'R':
            if (f_enableCapture) {
                StartCapture();
            }
            RunGraph();
            break;

        case 'S':
            if (f_enableCapture) {
                StopCapture();
            }
            StopGraph();
            break;

        default:
            return false;
        }

    return true;
}



