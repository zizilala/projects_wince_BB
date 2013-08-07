// All rights reserved ADENEO EMBEDDED 2010
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
// Portions Copyright (c) Texas Instruments.  All rights reserved.
//
//------------------------------------------------------------------------------
//
//  intialization routine.
//

#include <windows.h>
#include <wavedev.h>
#include <mmreg.h>
#include <pm.h>
#include <mmddk.h>
#include <ceddk.h>
#include <ceddkex.h>
#include <wavext.h>

#include "debug.h"
#include "wavemain.h"
#include "audioctrl.h"
#include "audiolin.h"
#include "mixermgr.h"
#include "audiomgr.h"
#include "istrmmgr.h"
#include "ostrmmgr.h"
#include "omap35xx_hwbridge.h"

//------------------------------------------------------------------------------

struct AudioDriverInitInfo_t
{
    DWORD                   StreamAttenMax;
    DWORD                   DeviceAttenMax;
    DWORD                   SecondaryAttenMax;
    DWORD                   dwAudioProfile;
    DWORD                   dwNumOfPlayChannels;
    DWORD                   requestedPlayChannels[MAX_HW_CODEC_CHANNELS];
    DWORD                   dwNumOfRecChannels;
    DWORD                   requestedRecChannels[MAX_HW_CODEC_CHANNELS];
    PortConfigInfo_t       *pPlayPortConfigInfo;
    PortConfigInfo_t       *pRecPortConfigInfo;
    WCHAR                   szDMTPortDriver[16];
    WCHAR                   szSerialPortDriver[16];
    WCHAR                   szHwCodecAdapterPath[MAX_PATH];
    HwCodecConfigInfo_t    *pHwCodecConfigInfo;
    DWORD                   dwHwCodecInMainMicDigitalGain;
    DWORD                   dwHwCodecInSubMicDigitalGain;
    DWORD                   dwHwCodecInHeadsetMicDigitalGain;
    DWORD                   dwHwCodecInMainMicAnalogGain;
    DWORD                   dwHwCodecInSubMicAnalogGain;
    DWORD                   dwHwCodecInHeadsetMicAnalogGain;
    DWORD                   dwHwCodecOutStereoSpeakerDigitalGain;
    DWORD                   dwHwCodecOutStereoHeadsetDigitalGain;
    DWORD                   dwHwCodecOutHeadsetMicDigitalGain;
    DWORD                   dwHwCodecOutStereoSpeakerAnalogGain;
    DWORD                   dwHwCodecOutStereoHeadsetAnalogGain;
    DWORD                   dwHwCodecOutHeadsetMicAnalogGain;
    DWORD                   dwHwCodecInHeadsetAuxDigitalGain;
    DWORD                   dwHwCodecInHeadsetAuxAnalogGain;
    DWORD                   dwAudioRoute;
};

static WCHAR const *_pSerialPortDefault = _T("ICX9:");

static const
DEVICE_REGISTRY_PARAM s_deviceRegParams[] = {
    {
        L"StreamAttenMax", PARAM_DWORD, FALSE,
        offset(AudioDriverInitInfo_t, StreamAttenMax),
        fieldsize(AudioDriverInitInfo_t, StreamAttenMax),
        (VOID*)STREAM_ATTEN_MAX
    }, {
        L"DeviceAttenMax", PARAM_DWORD, FALSE,
        offset(AudioDriverInitInfo_t, DeviceAttenMax),
        fieldsize(AudioDriverInitInfo_t, DeviceAttenMax),
        (VOID*)DEVICE_ATTEN_MAX
    }, {
        L"SecondaryAttenMax", PARAM_DWORD, FALSE,
        offset(AudioDriverInitInfo_t, SecondaryAttenMax),
        fieldsize(AudioDriverInitInfo_t, SecondaryAttenMax),
        (VOID*)SECOND_ATTEN_MAX
    }, {
        L"ExternalPortDriver", PARAM_STRING, TRUE,
        offset(AudioDriverInitInfo_t, szDMTPortDriver),
        fieldsize(AudioDriverInitInfo_t, szDMTPortDriver),
        NULL
    }, {
        L"SerialPortDriver", PARAM_STRING, FALSE,
        offset(AudioDriverInitInfo_t, szSerialPortDriver),
        fieldsize(AudioDriverInitInfo_t, szSerialPortDriver),
        (VOID*)_pSerialPortDefault
    }, {
        L"AudioProfile", PARAM_DWORD, FALSE,
        offset(AudioDriverInitInfo_t, dwAudioProfile),
        fieldsize(AudioDriverInitInfo_t, dwAudioProfile),
        NULL
    }, {
        L"NumOfPlayChannels", PARAM_DWORD, FALSE,
        offset(AudioDriverInitInfo_t, dwNumOfPlayChannels),
        fieldsize(AudioDriverInitInfo_t, dwNumOfPlayChannels),
        NULL
    }, {
        L"EnableAudioPlayChannels", PARAM_MULTIDWORD, FALSE,
        offset(AudioDriverInitInfo_t, requestedPlayChannels),
        fieldsize(AudioDriverInitInfo_t, requestedPlayChannels),
        NULL
    }, {
        L"NumOfRecChannels", PARAM_DWORD, FALSE,
        offset(AudioDriverInitInfo_t, dwNumOfRecChannels),
        fieldsize(AudioDriverInitInfo_t, dwNumOfRecChannels),
        NULL
    }, {
        L"EnableAudioRecChannels", PARAM_MULTIDWORD, FALSE,
        offset(AudioDriverInitInfo_t, requestedRecChannels),
        fieldsize(AudioDriverInitInfo_t, requestedRecChannels),
        NULL
    }, {
        L"DASFHwCodecAdpaterPath", PARAM_STRING, FALSE,
        offset(AudioDriverInitInfo_t, szHwCodecAdapterPath),
        fieldsize(AudioDriverInitInfo_t, szHwCodecAdapterPath),
        NULL
    }, {
        L"HwCodecInMainMicDigitalGain", PARAM_DWORD, FALSE,
        offset(AudioDriverInitInfo_t, dwHwCodecInMainMicDigitalGain),
        fieldsize(AudioDriverInitInfo_t, dwHwCodecInMainMicDigitalGain),
        NULL
    }, {
        L"HwCodecInSubMicDigitalGain", PARAM_DWORD, FALSE,
        offset(AudioDriverInitInfo_t, dwHwCodecInSubMicDigitalGain),
        fieldsize(AudioDriverInitInfo_t, dwHwCodecInSubMicDigitalGain),
        NULL
    }, {
        L"HwCodecInHeadsetMicDigitalGain", PARAM_DWORD, FALSE,
        offset(AudioDriverInitInfo_t, dwHwCodecInHeadsetMicDigitalGain),
        fieldsize(AudioDriverInitInfo_t, dwHwCodecInHeadsetMicDigitalGain),
        NULL
    }, {
        L"HwCodecInMainMicAnalogGain", PARAM_DWORD, FALSE,
        offset(AudioDriverInitInfo_t, dwHwCodecInMainMicAnalogGain),
        fieldsize(AudioDriverInitInfo_t, dwHwCodecInMainMicAnalogGain),
        NULL
    }, {
        L"HwCodecInSubMicAnalogGain", PARAM_DWORD, FALSE,
        offset(AudioDriverInitInfo_t, dwHwCodecInSubMicAnalogGain),
        fieldsize(AudioDriverInitInfo_t, dwHwCodecInSubMicAnalogGain),
        NULL
    }, {
        L"HwCodecInHeadsetMicAnalogGain", PARAM_DWORD, FALSE,
        offset(AudioDriverInitInfo_t, dwHwCodecInHeadsetMicAnalogGain),
        fieldsize(AudioDriverInitInfo_t, dwHwCodecInHeadsetMicAnalogGain),
        NULL
    }, {
        L"HwCodecOutStereoSpeakerDigitalGain", PARAM_DWORD, FALSE,
        offset(AudioDriverInitInfo_t, dwHwCodecOutStereoSpeakerDigitalGain),
        fieldsize(AudioDriverInitInfo_t, dwHwCodecOutStereoSpeakerDigitalGain),
        NULL
    }, {
        L"HwCodecOutStereoHeadsetDigitalGain", PARAM_DWORD, FALSE,
        offset(AudioDriverInitInfo_t, dwHwCodecOutStereoHeadsetDigitalGain),
        fieldsize(AudioDriverInitInfo_t, dwHwCodecOutStereoHeadsetDigitalGain),
        NULL
    }, {
        L"HwCodecOutHeadsetMicDigitalGain", PARAM_DWORD, FALSE,
        offset(AudioDriverInitInfo_t, dwHwCodecOutHeadsetMicDigitalGain),
        fieldsize(AudioDriverInitInfo_t, dwHwCodecOutHeadsetMicDigitalGain),
        NULL
    }, {
        L"HwCodecOutStereoSpeakerAnalogGain", PARAM_DWORD, FALSE,
        offset(AudioDriverInitInfo_t, dwHwCodecOutStereoSpeakerAnalogGain),
        fieldsize(AudioDriverInitInfo_t, dwHwCodecOutStereoSpeakerAnalogGain),
        NULL
    }, {
        L"HwCodecOutStereoHeadsetAnalogGain", PARAM_DWORD, FALSE,
        offset(AudioDriverInitInfo_t, dwHwCodecOutStereoHeadsetAnalogGain),
        fieldsize(AudioDriverInitInfo_t, dwHwCodecOutStereoHeadsetAnalogGain),
        NULL
    }, {
        L"HwCodecOutHeadsetMicAnalogGain", PARAM_DWORD, FALSE,
        offset(AudioDriverInitInfo_t, dwHwCodecOutHeadsetMicAnalogGain),
        fieldsize(AudioDriverInitInfo_t, dwHwCodecOutHeadsetMicAnalogGain),
        NULL
    }, {
        L"HwCodecInHeadsetAuxDigitalGain", PARAM_DWORD, FALSE,
        offset(AudioDriverInitInfo_t, dwHwCodecInHeadsetAuxDigitalGain),
        fieldsize(AudioDriverInitInfo_t, dwHwCodecInHeadsetAuxDigitalGain),
        NULL
    }, {
        L"HwCodecInHeadsetAuxAnalogGain", PARAM_DWORD, FALSE,
        offset(AudioDriverInitInfo_t, dwHwCodecInHeadsetAuxAnalogGain),
        fieldsize(AudioDriverInitInfo_t, dwHwCodecInHeadsetAuxAnalogGain),
        NULL
    }, {
        L"AudioRoute", PARAM_DWORD, FALSE,
        offset(AudioDriverInitInfo_t, dwAudioRoute),
        fieldsize(AudioDriverInitInfo_t, dwAudioRoute),
        (VOID*)OMAP35XX_HwAudioBridge::kAudioRoute_Handset
    }
};

//------------------------------------------------------------------------------

static AudioDriverInitInfo_t    s_AudioDriverInitInfo;
static OMAP35XX_HwAudioBridge  s_HardwareBridge;
static COutputStreamManager     s_OutputStreamManager;
static CInputStreamManager      s_InputStreamManager;


//------------------------------------------------------------------------------
// InitializeMixers
//
EXTERN_C CAudioManager*
CreateAudioManager(
    )
{
    DEBUGMSG(ZONE_FUNCTION, (L"WAV:+CreateAudioManager()"));

    // register all destination audio lines
    //
    CAudioManager* pAudioManager = new CAudioManager();

    DEBUGMSG(ZONE_FUNCTION,
        (L"WAV:-CreateAudioManager(CAudioManager*=0x%08X), pAudioManager")
        );

    return pAudioManager;
}


//------------------------------------------------------------------------------
// InitializeMixers
//
EXTERN_C void
DeleteAudioManager(
    CAudioManager* pAudioManager
    )
{
    DEBUGMSG(ZONE_FUNCTION,
        (L"WAV:+DeleteAudioManager(CAudioManager*=0x%08X), pAudioManager")
        );

    if (pAudioManager)
        {
    delete pAudioManager;
        pAudioManager = NULL;
        }

    DEBUGMSG(ZONE_FUNCTION, (L"WAV:-DeleteAudioManager()"));
}


//------------------------------------------------------------------------------
// InitializeHardware
//
EXTERN_C BOOL
InitializeHardware(
    LPCWSTR szContext,
    LPCVOID pBusContext
    )
{
    UNREFERENCED_PARAMETER(pBusContext);
        
    DEBUGMSG(ZONE_FUNCTION,
        (L"WAV:+InitializeHardware(szContext*=0x%08X)", szContext)
        );

    BOOL bResult = FALSE;
    UINT nCount = 0;

    // Read device parameters
    if (GetDeviceRegistryParams(
            szContext, &s_AudioDriverInitInfo, dimof(s_deviceRegParams),
            s_deviceRegParams) != ERROR_SUCCESS)
        {
        DEBUGMSG(ZONE_ERROR, (L"WAV: !ERROR "
            L"Failed read WAV driver registry parameters\r\n"
            ));
        goto cleanUp;
        }

    s_AudioDriverInitInfo.pHwCodecConfigInfo =
        (HwCodecConfigInfo_t *)LocalAlloc(LPTR, sizeof(HwCodecConfigInfo_t));
    if (s_AudioDriverInitInfo.pHwCodecConfigInfo == NULL)
        {
        DEBUGMSG(ZONE_ERROR, (L"WAV: !ERROR "
            L"Failed to allocate gain settings structure\r\n"
            ));
        goto cleanUp;
        }

    // MemSet playback port config structure with 0
    //
    memset(s_AudioDriverInitInfo.pHwCodecConfigInfo , 0,
        sizeof(HwCodecConfigInfo_t)
        );

    // Populate Hw Codec config info from registry
    //

    // Input config
    //
    s_AudioDriverInitInfo.pHwCodecConfigInfo->dwHwCodecInMainMicDigitalGain =
        s_AudioDriverInitInfo.dwHwCodecInMainMicDigitalGain;

    s_AudioDriverInitInfo.pHwCodecConfigInfo->dwHwCodecInSubMicDigitalGain =
        s_AudioDriverInitInfo.dwHwCodecInSubMicDigitalGain;

    s_AudioDriverInitInfo.pHwCodecConfigInfo->dwHwCodecInHeadsetMicDigitalGain =
        s_AudioDriverInitInfo.dwHwCodecInHeadsetMicDigitalGain;

    s_AudioDriverInitInfo.pHwCodecConfigInfo->dwHwCodecInMainMicAnalogGain =
        s_AudioDriverInitInfo.dwHwCodecInMainMicAnalogGain;

    s_AudioDriverInitInfo.pHwCodecConfigInfo->dwHwCodecInSubMicAnalogGain =
        s_AudioDriverInitInfo.dwHwCodecInSubMicAnalogGain;

    s_AudioDriverInitInfo.pHwCodecConfigInfo->dwHwCodecInHeadsetMicAnalogGain =
        s_AudioDriverInitInfo.dwHwCodecInHeadsetMicAnalogGain;

    s_AudioDriverInitInfo.pHwCodecConfigInfo->dwHwCodecInHeadsetAuxDigitalGain =
        s_AudioDriverInitInfo.dwHwCodecInHeadsetAuxDigitalGain;

    s_AudioDriverInitInfo.pHwCodecConfigInfo->dwHwCodecInHeadsetAuxAnalogGain =
        s_AudioDriverInitInfo.dwHwCodecInHeadsetAuxAnalogGain;

    // Output config
    //
    s_AudioDriverInitInfo.pHwCodecConfigInfo->
        dwHwCodecOutStereoSpeakerDigitalGain =
        s_AudioDriverInitInfo.dwHwCodecOutStereoSpeakerDigitalGain;

    s_AudioDriverInitInfo.pHwCodecConfigInfo->
        dwHwCodecOutStereoHeadsetDigitalGain =
        s_AudioDriverInitInfo.dwHwCodecOutStereoHeadsetDigitalGain;

    s_AudioDriverInitInfo.pHwCodecConfigInfo->
        dwHwCodecOutHeadsetMicDigitalGain =
        s_AudioDriverInitInfo.dwHwCodecOutHeadsetMicDigitalGain;

    s_AudioDriverInitInfo.pHwCodecConfigInfo->
        dwHwCodecOutStereoSpeakerAnalogGain =
        s_AudioDriverInitInfo.dwHwCodecOutStereoSpeakerAnalogGain;

    s_AudioDriverInitInfo.pHwCodecConfigInfo->
        dwHwCodecOutStereoHeadsetAnalogGain =
        s_AudioDriverInitInfo.dwHwCodecOutStereoHeadsetAnalogGain;

    s_AudioDriverInitInfo.pHwCodecConfigInfo->
        dwHwCodecOutHeadsetMicAnalogGain =
        s_AudioDriverInitInfo.dwHwCodecOutHeadsetMicAnalogGain;

    // Playback port config information
    //
    s_AudioDriverInitInfo.pPlayPortConfigInfo = 
        (PortConfigInfo_t *)LocalAlloc(LPTR, sizeof(PortConfigInfo_t)
        );
    if (s_AudioDriverInitInfo.pPlayPortConfigInfo == NULL)
        {
        DEBUGMSG(ZONE_ERROR, (L"WAV: !ERROR "
            L"Failed to allocate gain settings structure\r\n"
            ));
        goto cleanUp;
        }

    // MemSet playback port config structure with 0
    //
    memset(s_AudioDriverInitInfo.pPlayPortConfigInfo, 0,
        sizeof(PortConfigInfo_t)
        );

    s_AudioDriverInitInfo.pPlayPortConfigInfo->numOfChannels = s_AudioDriverInitInfo.dwNumOfPlayChannels;
    s_AudioDriverInitInfo.pPlayPortConfigInfo->portProfile = s_AudioDriverInitInfo.dwAudioProfile;
    
    for ( nCount= 0; nCount < MAX_HW_CODEC_CHANNELS; nCount++)
        {
        s_AudioDriverInitInfo.pPlayPortConfigInfo->requestedChannels[nCount] = 
            s_AudioDriverInitInfo.requestedPlayChannels[nCount];
        }

    // Record port config information
    //
    s_AudioDriverInitInfo.pRecPortConfigInfo = 
	    (PortConfigInfo_t *)LocalAlloc(LPTR, sizeof(PortConfigInfo_t));

    if (s_AudioDriverInitInfo.pRecPortConfigInfo == NULL)
        {
        DEBUGMSG(ZONE_ERROR, (L"WAV: !ERROR "
            L"Failed to allocate gain settings structure\r\n"
            ));
        goto cleanUp;
        }

    // MemSet record port config structure with 0
    //
    memset(s_AudioDriverInitInfo.pRecPortConfigInfo, 0,
        sizeof(PortConfigInfo_t)
        );

    s_AudioDriverInitInfo.pRecPortConfigInfo->numOfChannels = 
        s_AudioDriverInitInfo.dwNumOfRecChannels;

    s_AudioDriverInitInfo.pRecPortConfigInfo->portProfile = 
	    s_AudioDriverInitInfo.dwAudioProfile;

    for ( nCount= 0; nCount < MAX_HW_CODEC_CHANNELS; nCount++)
        {
        s_AudioDriverInitInfo.pRecPortConfigInfo->requestedChannels[nCount] = 
            s_AudioDriverInitInfo.requestedRecChannels[nCount];
        }

    bResult = TRUE;

    s_HardwareBridge.initialize(s_AudioDriverInitInfo.szDMTPortDriver,
        s_AudioDriverInitInfo.szHwCodecAdapterPath,
        s_AudioDriverInitInfo.pHwCodecConfigInfo,
        (HANDLE)s_AudioDriverInitInfo.pPlayPortConfigInfo, 
        (HANDLE)s_AudioDriverInitInfo.pRecPortConfigInfo,
		(OMAP35XX_HwAudioBridge::AudioRoute_e)s_AudioDriverInitInfo.dwAudioRoute
        );

cleanUp:

    DEBUGMSG(ZONE_FUNCTION, (L"WAV:-InitializeHardware()"));
    return bResult;
}

//------------------------------------------------------------------------------
// UninitializeHardware
//
EXTERN_C void
UninitializeHardware(
    DWORD dwData
    )
{
    DEBUGMSG(ZONE_FUNCTION,
        (L"WAV:+UninitializeHardware(dwData=0x%08X)", dwData)
        );

    UNREFERENCED_PARAMETER(dwData);

    // Free the allocated resources of T2 Hw codec configuration structure
    //
    if (s_AudioDriverInitInfo.pHwCodecConfigInfo)
        {
        LocalFree(s_AudioDriverInitInfo.pHwCodecConfigInfo);
        s_AudioDriverInitInfo.pHwCodecConfigInfo = NULL;
        }

    // Free play config info structure
    //
    if (s_AudioDriverInitInfo.pPlayPortConfigInfo != NULL)
        {
        LocalFree(s_AudioDriverInitInfo.pPlayPortConfigInfo);
        s_AudioDriverInitInfo.pPlayPortConfigInfo = NULL;
        }

    // Free record config info structure
    //
    if (s_AudioDriverInitInfo.pRecPortConfigInfo != NULL)
        {
        LocalFree(s_AudioDriverInitInfo.pRecPortConfigInfo);
        s_AudioDriverInitInfo.pRecPortConfigInfo = NULL;
        }

    DEBUGMSG(ZONE_FUNCTION, (L"WAV:-UninitializeHardware()"));
}


//------------------------------------------------------------------------------
// InitializeMixers
//
EXTERN_C void
InitializeMixers(CAudioManager* pManager)
{
    DEBUGMSG(ZONE_FUNCTION, (L"WAV:+InitializeMixers()"));

    // set sample rates
    //
    s_OutputStreamManager.put_AudioSampleRate(CStreamCallback::k44khz);
    s_InputStreamManager.put_AudioSampleRate(CStreamCallback::k44khz);

    // Setup the hardware bridge
    //
    pManager->put_HardwareAudioBridge(&s_HardwareBridge);
    s_OutputStreamManager.put_AudioManager(pManager);
    s_InputStreamManager.put_AudioManager(pManager);

    s_HardwareBridge.put_StreamCallback(CHardwareAudioBridge::kInput,
        &s_InputStreamManager
        );

    s_HardwareBridge.put_StreamCallback(CHardwareAudioBridge::kOutput,
        &s_OutputStreamManager
        );


    // Set audio gain maximums
    //
    CStreamManager::SetAttenMax(
        s_AudioDriverInitInfo.StreamAttenMax,
        s_AudioDriverInitInfo.DeviceAttenMax,
        s_AudioDriverInitInfo.SecondaryAttenMax
        );

    // input
    //
    pManager->put_InputStreamManager(&s_InputStreamManager);
    pManager->get_AudioMixerManager()->register_DestinationAudioLine(
        &s_InputStreamManager
        );

    // output
    //
    pManager->put_OutputStreamManager(&s_OutputStreamManager);
    pManager->get_AudioMixerManager()->register_DestinationAudioLine(
        &s_OutputStreamManager
        );


    DEBUGMSG(ZONE_FUNCTION, (L"WAV:-InitializeMixers()"));
}
