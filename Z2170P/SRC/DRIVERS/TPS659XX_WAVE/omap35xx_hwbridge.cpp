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

#include "bsp.h"
#include "ceddkex.h"
#include "wavext.h"
#include <memtxapi.h>
#include "mcbsp.h"
#include <mmddk.h>


#include "wavemain.h"
#include "debug.h"
#include "strmctxt.h"
#include "tps659xx_wave.h"
#include "omap35xx_hwbridge.h"


//------------------------------------------------------------------------------
static
void
DisableAudio(
    HANDLE m_hHwCodec
    )
{
    SetHwCodecMode_Disabled(m_hHwCodec);
}

//------------------------------------------------------------------------------
static
void
EnableT2AudioClk(
    HANDLE m_hHwCodec
    )
{
    SetHwCodecMode_EnableT2AudioClkOnly(m_hHwCodec);
}


//------------------------------------------------------------------------------
static
void
RouteAudioToHandset(
    HANDLE m_hHwCodec,
    DWORD dwAudioProfile,
    BOOL bEnableMic
    )
{
    if (bEnableMic)
        {
        SetHwCodecMode_Microphone(m_hHwCodec, (AudioProfile_e)dwAudioProfile);
        }
    else
        {
        SetHwCodecMode_Speaker(m_hHwCodec, (AudioProfile_e)dwAudioProfile);
        }
}


//------------------------------------------------------------------------------
static
void
RouteAudioToHeadset(
    HANDLE m_hHwCodec,
    DWORD dwAudioProfile,
    BOOL bEnableMic
    )
{
    if (bEnableMic)
        {
        SetHwCodecMode_MicHeadset(m_hHwCodec, (AudioProfile_e)dwAudioProfile);
        }
    else
        {
        SetHwCodecMode_Headset(m_hHwCodec, (AudioProfile_e)dwAudioProfile);
        }
}

//------------------------------------------------------------------------------
static
void
RouteAudioToAuxHeadset(
    HANDLE m_hHwCodec,
    DWORD dwAudioProfile,
    BOOL bEnableMic
    )
{
    if (bEnableMic)
        {
        SetHwCodecMode_AuxHeadset(m_hHwCodec, (AudioProfile_e)dwAudioProfile);
        }
    else
        {
        SetHwCodecMode_StereoHeadset(m_hHwCodec, (AudioProfile_e)dwAudioProfile);
        }
}


//------------------------------------------------------------------------------
// configures triton to route audio appropriately
//
void
OMAP35XX_HwAudioBridge::SetAudioPath(
    AudioRoute_e audioroute,
    DWORD dwAudioProfile
    )
{
    DEBUGMSG(ZONE_FUNCTION,
        (L"WAV:+OMAP35XX_HwAudioBridge:SetAudioPath(%d)\r\n",
        audioroute)
        );

    BOOL bReceiverIdle = m_ReceiverState == kAudioRender_Idle;

    if (audioroute == OMAP35XX_HwAudioBridge::kAudioRoute_HdmiAudio)
        {
        EnableT2AudioClk(m_hHwCodec);
        }
    else
        {
        if (m_TransmitterState == kAudioRender_Idle && bReceiverIdle)
            {
            DisableAudio(m_hHwCodec);
            }
        else
            {
            switch (audioroute)
                {
                case OMAP35XX_HwAudioBridge::kAudioRoute_AuxHeadset:
                    RouteAudioToAuxHeadset(m_hHwCodec, dwAudioProfile, 
                        bReceiverIdle == FALSE);
                    break;
                    
                case OMAP35XX_HwAudioBridge::kAudioRoute_BTHeadset:
                case OMAP35XX_HwAudioBridge::kAudioRoute_Headset:
                    RouteAudioToHeadset(m_hHwCodec, dwAudioProfile,
                        bReceiverIdle == FALSE);
                    break;

                case OMAP35XX_HwAudioBridge::kAudioRoute_Carkit:
                case OMAP35XX_HwAudioBridge::kAudioRoute_Speaker:
                case OMAP35XX_HwAudioBridge::kAudioRoute_Handset:
                    RouteAudioToHandset(m_hHwCodec, dwAudioProfile,
                        bReceiverIdle == FALSE);
                    break;

                default:
                    break;
                }
            }
        }

    DEBUGMSG(ZONE_FUNCTION,
        (L"WAV:-OMAP35XX_HwAudioBridge:SetAudioPath()\r\n")
        );
}

//------------------------------------------------------------------------------
//  initializes the hardware bridge
//
void
OMAP35XX_HwAudioBridge::initialize(
    WCHAR const *szDMTDriver,
    WCHAR const *szDASFDriver,
    HwCodecConfigInfo_t *pHwCodecConfigInfo,
    HANDLE hPlayPortConfigInfo,
    HANDLE hRecPortConfigInfo,
	AudioRoute_e eAudioRoute
    )
{
    PortConfigInfo_t *pPortConfigInfo = (PortConfigInfo_t*)hPlayPortConfigInfo;

    DEBUGMSG(ZONE_FUNCTION,
        (L"WAV:+OMAP35XX_HwAudioBridge:initialize()\r\n")
        );

    DEBUGMSG(ZONE_HWBRIDGE, (L"WAV:initializing hardware\r\n"));

    m_RequestAudioRoute = eAudioRoute;

    m_dwAudioProfile = pPortConfigInfo->portProfile;

    m_hHwCodec = OpenHwCodecHandle(pHwCodecConfigInfo);

    if (m_hHwCodec == NULL)
        {
        RETAILMSG(ZONE_ERROR,
            (L"WAV:!ERROR Can't load Triton Driver err=0x%08X\r\n",
            GetLastError()));
        goto code_exit;
        }

    DisableAudio(m_hHwCodec);


    // open and register direct memory transfer port
    //
    m_DMTPort.register_PORTHost(this);
    m_DMTPort.open_Port(szDMTDriver, hPlayPortConfigInfo, hRecPortConfigInfo);

    // open and register DASF transfer port
    //
    m_DASFPort.register_PORTHost(this);
    m_DASFPort.open_Port(szDASFDriver, hPlayPortConfigInfo, hRecPortConfigInfo);

    // set default port to the DMT port
    //
    m_pActivePort = &m_DMTPort;
    m_DMTPort.set_DMTProfile(DMTAudioStreamPort::DMTProfile_I2SSlave);

code_exit:
    DEBUGMSG(ZONE_FUNCTION,
        (L"WAV:-OMAP35XX_HwAudioBridge:initialize()\r\n")
        );
}


//------------------------------------------------------------------------------
//  updates the audio routing
//
BOOL
OMAP35XX_HwAudioBridge::update_AudioRouting()
{
    DEBUGMSG(ZONE_FUNCTION,
        (L"WAV:+OMAP35XX_HwAudioBridge::update_AudioRouting\r\n")
        );

    AudioStreamPort *pActivePort = NULL;
    BOOL bPortUpdated = FALSE;

    // UNDONE:
    // In certain elaborate use case scenario's we can get out of sync
    // with smartphone's audio profile because we override certain
    // profiles.  It could be the smartphone shell sheilds us from
    // these elaborate use case scenarios by preventing the user to
    // change audio profile when certain audio profiles are forced
    //

    // clear the audio request change dirty bit
    //
    m_fRequestAudioRouteDirty = FALSE;

    // Bluetooth headsets are a special case so check for this first
    //
    if (query_BTHeadsetAttached() == TRUE)
        {
        // for bluetooth we do the following
        //  1) set recording/rendering rate to 8khz
        //  2) switch to ICX based data port
        //  3) update audio routing to go through bluetooth
        //
        if (m_CurrentAudioRoute != kAudioRoute_BTHeadset)
            {
            // when receiver is enabled we need to set the sample
            // rate to 8 khz for both input and output
            //
            m_prgStreams[kOutput]->put_AudioSampleRate(
                    CStreamCallback::k8khz
                    );

            m_prgStreams[kInput]->put_AudioSampleRate(
                    CStreamCallback::k8khz
                    );

            m_CurrentAudioRoute = kAudioRoute_BTHeadset;
            pActivePort = &m_DASFPort;
            bPortUpdated = TRUE;
            }
        }
    // HDMI is attached!
    //
    else if (query_HdmiAudioAttached() == TRUE)
        {
        // for hdmi audio, we do the following
        //  1) set recording/rendering rate to 44.1khz
        //  2) switch to data port (McBSP I2S Master)
        //  3) Update the audio routing to go through hdmi controller
        //
        if (m_CurrentAudioRoute != kAudioRoute_HdmiAudioRequested)
            {
            // set transceiver port to MCBSP port
            //
            pActivePort = &m_DMTPort;
            m_DMTPort.set_DMTProfile(DMTAudioStreamPort::DMTProfile_I2SMaster);
            bPortUpdated = TRUE;

            m_CurrentAudioRoute = kAudioRoute_HdmiAudioRequested;
            }
        else if (m_CurrentAudioRoute == kAudioRoute_HdmiAudioRequested)
            {

            if (m_ReceiverState != kAudioRender_Idle)
                {
                // when receiver is enabled we need to set the sample
                // rate to 44.1 khz for both input and output
                //
                m_prgStreams[kOutput]->put_AudioSampleRate(
                        CStreamCallback::k44khz
                        );

                m_prgStreams[kInput]->put_AudioSampleRate(
                        CStreamCallback::k44khz
                        );
                }
            else
                {
                // set default output rate of 44.1 khz
                //
                m_prgStreams[kOutput]->put_AudioSampleRate(
                        CStreamCallback::k44khz
                        );
                }

            m_CurrentAudioRoute = kAudioRoute_HdmiAudio;
            }
        }
    else
        {
        // for non-bluetooth case do the following
        //  1) set rcording/rendering rate
        //  2) switch to using MCBSP based data port
        //  3) update audio routing
        //
        if (m_ReceiverState != kAudioRender_Idle)
            {
            // when receiver is enabled we need to set the sample
            // rate to 44.1 khz for both input and output
            //
            m_prgStreams[kOutput]->put_AudioSampleRate(
                    CStreamCallback::k44khz
                    );

            m_prgStreams[kInput]->put_AudioSampleRate(
                    CStreamCallback::k44khz
                    );
            }
        else
            {
            // set default output rate of 44.1 khz
            //
            m_prgStreams[kOutput]->put_AudioSampleRate(
                    CStreamCallback::k44khz
                    );
            }

        // set transceiver port to MCBSP port
        //
        if (m_bPortSwitch == TRUE)
            {
            DEBUGMSG(ZONE_HWBRIDGE, (L"WAV:DASF port Rendering\r\n"));
            // set DASF as active port
            //
            pActivePort = &m_DASFPort;
            bPortUpdated = TRUE;

            // update the active port state
            //
            m_CurrActivePort = kAudioDASFPort;
            }
        else
            {
            DEBUGMSG(ZONE_HWBRIDGE, (L"WAV:McBSP port Rendering\r\n"));
            // set transceiver port to MCBSP port
            //
            if ((m_pActivePort != &m_DMTPort) ||
                (m_DMTPort.get_DMTProfile() != DMTAudioStreamPort::DMTProfile_I2SSlave))
                {
                pActivePort = &m_DMTPort;
                m_DMTPort.set_DMTProfile(DMTAudioStreamPort::DMTProfile_I2SSlave);
                bPortUpdated = TRUE;
                }

            // update the active port state
            //
            m_CurrActivePort = kAudioDMTPort;
            }

        // NOTE: This is a special state only to handle HDMI detach, since we are
        // required to stop the T2 only after the McBSP has stopped completely.
        if ((m_CurrentAudioRoute == kAudioRoute_HdmiAudio) &&
            (m_TransmitterState == kAudioRender_Active))
            {
            m_CurrentAudioRoute = kAudioRoute_HdmiAudioDetached;
            }
            else
            {
            // check if speaker mode should be forced
            //
            if (m_dwSpeakerCount > 0)
                {
                m_CurrentAudioRoute = kAudioRoute_Speaker;
                }
            else
                {
                m_CurrentAudioRoute = m_RequestAudioRoute;
                }
            }
        }
    SetAudioPath(m_CurrentAudioRoute, m_dwAudioProfile);

    // if no activity then update power state to D4
    //
    if ((m_ReceiverState == kAudioRender_Idle) &&
        (m_TransmitterState == kAudioRender_Idle))
        {
        m_PowerState = D4;

        // Check if render port has changed if so update the same.
        //
        if (bPortUpdated == TRUE)
            {
            m_pActivePort = pActivePort;
            }
        }
    else
        {
        if (bPortUpdated == TRUE)
            {
            // if a port is busy, stop previous port and activate new port.
            //

            if (m_ReceiverState != kAudioRender_Idle)
                {
                // need to check if there's any data left to render in port
                // if not then don't bother activating port
                //
                m_pActivePort->signal_Port(ASPS_STOP_RX, NULL, 0);
                }

            if (m_TransmitterState != kAudioRender_Idle)
                {
                // need to check if there's any data left to render in port
                // if not then don't bother activating port
                //
                m_pActivePort->signal_Port(ASPS_STOP_TX, NULL, 0);
                }
            m_pActivePort = pActivePort;
            }
        m_PowerState = D0;
        }

    DEBUGMSG(ZONE_FUNCTION,
        (L"WAV:-OMAP35XX_HwAudioBridge::update_AudioRouting\r\n")
        );

    return TRUE;
}


//------------------------------------------------------------------------------
//  starts audio port
//
BOOL
OMAP35XX_HwAudioBridge::start_AudioPort(
    StreamType type
    )
{
    DEBUGMSG(ZONE_FUNCTION,
        (L"WAV:+OMAP35XX_HwAudioBridge::start_AudioPort\r\n")
        );

    // check if audio stream is already turned-on.  If
    // so then don't turn-on again.  There is a edge case
    // where we will try to turn-on audio while
    // the last bit of data was pushed-out.  In this case
    // we still send the on message to the hardware
    //
    int nRet = FALSE;
    switch (type)
        {
        case kInput:
            DEBUGMSG(ZONE_HWBRIDGE, (L"WAV:HWBridge - Starting input stream\r\n"));
            if ((m_ReceiverState == kAudioRender_Idle) ||
                (m_ReceiverState == kAudioRender_Stopping))
                {
                switch (m_ReceiverState)
                    {
                    case kAudioRender_Idle:
                        DEBUGMSG(ZONE_HWBRIDGE,
                            (L"WAV:HWBridge - rx idle --> starting")
                            );
                        m_ReceiverState = kAudioRender_Starting;
                        update_AudioRouting();
                        break;

                    case kAudioRender_Stopping:
                        DEBUGMSG(ZONE_HWBRIDGE,
                            (L"WAV:HWBridge - rx stopping --> active")
                            );
                        m_ReceiverState = kAudioRender_Active;
                        break;
                    }
                nRet = m_pActivePort->signal_Port(ASPS_START_RX, NULL, 0);
                }
            break;

        case kOutput:
            DEBUGMSG(ZONE_HWBRIDGE, (L"WAV:HWBridge - Starting output stream\r\n"));
            if ((m_TransmitterState == kAudioRender_Idle) ||
                (m_TransmitterState == kAudioRender_Stopping))
                {
                switch (m_TransmitterState)
                    {
                    case kAudioRender_Idle:
                        DEBUGMSG(ZONE_HWBRIDGE,
                            (L"WAV:HWBridge - tx idle --> starting")
                            );
                        m_TransmitterState = kAudioRender_Starting;
                        update_AudioRouting();
                        break;

                    case kAudioRender_Stopping:
                        DEBUGMSG(ZONE_HWBRIDGE,
                            (L"WAV:HWBridge - tx stopping --> active")
                            );
                        m_TransmitterState = kAudioRender_Active;
                        break;
                    }
                nRet = m_pActivePort->signal_Port(ASPS_START_TX, NULL, 0);
                }
            break;
        }

    DEBUGMSG(ZONE_FUNCTION,
        (L"WAV:-OMAP35XX_HwAudioBridge::start_AudioPort\r\n")
        );

    return !!nRet;
}


//------------------------------------------------------------------------------
// stop audio port
//
BOOL
OMAP35XX_HwAudioBridge::stop_AudioPort(
    StreamType type
    )
{
    int nRet = FALSE;

    DEBUGMSG(ZONE_FUNCTION,
        (L"WAV:+OMAP35XX_HwAudioBridge::stop_AudioPort\r\n")
        );

    switch (type)
        {
        case kInput:
            DEBUGMSG(ZONE_HWBRIDGE,
                (L"WAV:HWBridge - Stopping input stream\r\n")
                );
            if ((m_ReceiverState == kAudioRender_Active) ||
                (m_ReceiverState == kAudioRender_Starting))
                {
                m_ReceiverState = kAudioRender_Stopping;
                nRet = m_pActivePort->signal_Port(ASPS_STOP_RX, NULL, 0);
                }
            break;

        case kOutput:
            DEBUGMSG(ZONE_HWBRIDGE,
                (L"WAV:HWBridge - Stopping output stream\r\n")
                );
            if ((m_TransmitterState == kAudioRender_Active) ||
                (m_TransmitterState == kAudioRender_Starting))
                {
                m_TransmitterState = kAudioRender_Stopping;
                nRet = m_pActivePort->signal_Port(ASPS_STOP_TX, NULL, 0);
                }
            break;
        }

    DEBUGMSG(ZONE_FUNCTION,
        (L"WAV:-OMAP35XX_HwAudioBridge::stop_AudioPort\r\n")
        );
    return !!nRet;
}

//------------------------------------------------------------------------------
//  starts audio streaming
//
BOOL
OMAP35XX_HwAudioBridge::start_Stream(
    StreamType type,
    HANDLE hStreamContext
    )
{
    int nRet = FALSE;

    DEBUGMSG(ZONE_FUNCTION,
        (L"WAV:+OMAP35XX_HwAudioBridge::start_Stream\r\n")
        );

    // check if audio stream is already turned-on.  If
    // so then don't turn-on again.  There is a edge case
    // where we will try to turn-on audio while
    // the last bit of data was pushed-out.  In this case
    // we still send the on message to the hardware
    //

    switch (type)
        {
        case kInput:
            DEBUGMSG(ZONE_HWBRIDGE,
                (L"WAV:HWBridge - Starting input stream\r\n")
                );
            RETAILMSG(m_ReceiverState != kAudioRender_Active && ZONE_ERROR,
                (L"WAV:HWBridge - Can't start stream when port is inactive\r\n")
                );
            nRet = m_pActivePort->signal_Port(ASPS_START_STREAM_RX,
                hStreamContext, 0
                );
            break;

        case kOutput:
            DEBUGMSG(ZONE_HWBRIDGE,
                (L"WAV:HWBridge - Starting output stream\r\n")
                );
            RETAILMSG(m_TransmitterState != kAudioRender_Active && ZONE_ERROR,
                (L"WAV:HWBridge - Can't start stream when port is inactive\r\n")
                );
            nRet = m_pActivePort->signal_Port(ASPS_START_STREAM_TX,
                hStreamContext, 0
                );
            break;
        }

    DEBUGMSG(ZONE_FUNCTION,
        (L"WAV:-OMAP35XX_HwAudioBridge::start_Stream\r\n")
        );
    return !!nRet;
}

//------------------------------------------------------------------------------
//  starts audio streaming
//
BOOL
OMAP35XX_HwAudioBridge::start_Stream(
    StreamType type
    )
{
    DEBUGMSG(ZONE_FUNCTION,
        (L"WAV:+OMAP35XX_HwAudioBridge::start_Stream\r\n")
        );

    // if PowerManagement(PM) has requested for D4 state (Due to Suspend mode),
    // audio driver must not start the stream till the PM has again requested
    // for D0 State
    //
    if (m_ReqestedPowerState == D4)
        {
        DEBUGMSG(1, (L"WAV: startstream not started due to PM's D4 state\r\n"));
        goto Exit;
        }

    // check if audio stream is already turned-on.  If
    // so then don't turn-on again.  There is a edge case
    // where we will try to turn-on audio while
    // the last bit of data was pushed-out.  In this case
    // we still send the on message to the hardware
    //
    switch (type)
        {
        case kInput:
            DEBUGMSG(ZONE_HWBRIDGE,
                (L"WAV:HWBridge - Starting input stream\r\n")
                );

            if ((m_ReceiverState == kAudioRender_Idle) ||
                (m_ReceiverState == kAudioRender_Stopping))
                {
                switch (m_ReceiverState)
                    {
                    case kAudioRender_Idle:
                        DEBUGMSG(ZONE_HWBRIDGE,
                            (L"WAV:HWBridge - rx idle --> starting\r\n")
                            );
                        m_ReceiverState = kAudioRender_Starting;
                        update_AudioRouting();
                        break;

                    case kAudioRender_Stopping:
                        DEBUGMSG(ZONE_HWBRIDGE,
                            (L"WAV:HWBridge - rx stopping --> active\r\n")
                            );
                        m_ReceiverState = kAudioRender_Active;
                        break;
                    }

                m_pActivePort->signal_Port(ASPS_START_RX, NULL, 0);
                }
            break;

        case kOutput:
            DEBUGMSG(ZONE_HWBRIDGE,
                (L"WAV:HWBridge - Starting output stream\r\n")
                );

            if ((m_TransmitterState == kAudioRender_Idle) ||
                (m_TransmitterState == kAudioRender_Stopping))
                {
                switch (m_TransmitterState)
                    {
                    case kAudioRender_Idle:
                        DEBUGMSG(ZONE_HWBRIDGE,
                            (L"WAV:HWBridge - tx idle --> starting\r\n")
                            );
                        m_TransmitterState = kAudioRender_Starting;
                        update_AudioRouting();
                        break;

                    case kAudioRender_Stopping:
                        DEBUGMSG(ZONE_HWBRIDGE,
                            (L"WAV:HWBridge - tx stopping --> active\r\n")
                            );
                        m_TransmitterState = kAudioRender_Active;
                        break;
                    }

                m_pActivePort->signal_Port(ASPS_START_TX, NULL, 0);
                }
            break;
        }
Exit:
    DEBUGMSG(ZONE_FUNCTION,
        (L"WAV:-OMAP35XX_HwAudioBridge::start_Stream\r\n")
        );

    return TRUE;
}

//------------------------------------------------------------------------------
// end audio streaming
//
BOOL
OMAP35XX_HwAudioBridge::stop_Stream(
    StreamType type,
    HANDLE hStreamContext
    )
{
    int nRet = FALSE;

    DEBUGMSG(ZONE_FUNCTION,
        (L"WAV:+OMAP35XX_HwAudioBridge::stop_Stream\r\n")
        );

    switch (type)
        {
        case kInput:
            DEBUGMSG(ZONE_HWBRIDGE,
                (L"WAV:HWBridge - Stopping input stream\r\n")
                );
            nRet = m_pActivePort->signal_Port(ASPS_STOP_STREAM_RX,
                hStreamContext, 0
                );
            break;

        case kOutput:
            DEBUGMSG(ZONE_HWBRIDGE,
                (L"WAV:HWBridge - Stopping output stream\r\n")
                );
            nRet = m_pActivePort->signal_Port(ASPS_STOP_STREAM_TX,
                hStreamContext, 0
                );
            break;
        }

    DEBUGMSG(ZONE_FUNCTION,
        (L"WAV:-OMAP35XX_HwAudioBridge::stop_Stream\r\n")
        );

    return !!nRet;
}


//------------------------------------------------------------------------------
// abort audio streaming
//
BOOL
OMAP35XX_HwAudioBridge::abort_Stream(
    StreamType type,
    HANDLE hStreamContext
    )
{
    int nRet = FALSE;

    DEBUGMSG(ZONE_FUNCTION,
        (L"WAV:+OMAP35XX_HwAudioBridge::abort_Stream\r\n")
        );

    switch (type)
        {
        case kInput:
            DEBUGMSG(ZONE_HWBRIDGE,
                (L"WAV:HWBridge - Aborting input stream\r\n")
                );
                nRet = m_pActivePort->signal_Port(ASPS_ABORT_RX,
                hStreamContext, 0
                );
            break;

        case kOutput:
            DEBUGMSG(ZONE_HWBRIDGE,
                (L"WAV:HWBridge - Aborting output stream\r\n")
                );
                nRet = m_pActivePort->signal_Port(ASPS_ABORT_TX,
                hStreamContext, 0
                );
            break;
        }

    DEBUGMSG(ZONE_FUNCTION,
        (L"WAV:-OMAP35XX_HwAudioBridge::abort_Stream\r\n")
        );
    return !!nRet;
}

//------------------------------------------------------------------------------
// set audio stream gain
//
BOOL
OMAP35XX_HwAudioBridge::set_StreamGain(
    StreamType type,
    HANDLE hStreamContext,
    DWORD dwContextData
    )
{
    int nRet = FALSE;

    DEBUGMSG(ZONE_FUNCTION,
        (L"WAV:+OMAP35XX_HwAudioBridge::set_StreamGain\r\n")
        );

    switch (type)
        {
        case kInput:
            DEBUGMSG(ZONE_HWBRIDGE,
                (L"WAV:HWBridge - set_StreamGain input stream\r\n")
                );
            nRet = m_pActivePort->signal_Port(ASPS_GAIN_STREAM_RX,
                hStreamContext, dwContextData
                );
            break;

        case kOutput:
            DEBUGMSG(ZONE_HWBRIDGE,
                (L"WAV:HWBridge - set_StreamGain output stream\r\n")
                );
            nRet = m_pActivePort->signal_Port(ASPS_GAIN_STREAM_TX,
                hStreamContext, dwContextData
                );
            break;
        }

    DEBUGMSG(ZONE_FUNCTION,
        (L"WAV:-OMAP35XX_HwAudioBridge::set_StreamGain\r\n")
        );

    return !!nRet;
}

//------------------------------------------------------------------------------
// swicthes audio port
//
BOOL
OMAP35XX_HwAudioBridge::switch_AudioStreamPort(
    BOOL bPortRequest
    )
{
    int nRet = FALSE;

    DEBUGMSG(ZONE_FUNCTION,
        (L"WAV:+OMAP35XX_HwAudioBridge::switch_AudioStreamPort\r\n")
        );

    // Check if audio port is idle and check the current active port and switch
    // the port accordingly.
    //
    if ((m_ReceiverState == kAudioRender_Idle) &&
        (m_TransmitterState == kAudioRender_Idle))
        {
         if (m_CurrActivePort == kAudioDASFPort)
            {
            m_bPortSwitch = bPortRequest;
            update_AudioRouting();
            if (m_bPreviousPortIsDASF == TRUE)
                {
                nRet = m_pActivePort->signal_Port(ASPS_PORT_RECONFIG, NULL, 0);
                }
            m_bPreviousPortIsDASF = FALSE;
            }
         else if (m_CurrActivePort == kAudioDMTPort)
            {
            m_bPortSwitch = bPortRequest;
            update_AudioRouting();

            m_bPreviousPortIsDASF = TRUE;
            }
         nRet = TRUE;
        }

    DEBUGMSG(ZONE_FUNCTION,
        (L"WAV:-OMAP35XX_HwAudioBridge::switch_AudioStreamPort\r\n")
        );
    return nRet;

}

//------------------------------------------------------------------------------
// Queries current active port
//
BOOL
OMAP35XX_HwAudioBridge:: query_AudioStreamPort()
{
    DEBUGMSG(ZONE_FUNCTION,
        (L"WAV:+/- OMAP35XX_HwAudioBridge::switch_AudioStreamPort\r\n")
        );

    return (BOOL)m_CurrActivePort;
}

//------------------------------------------------------------------------------
// Enables/Disables T2 Audio clocks
//
BOOL
OMAP35XX_HwAudioBridge:: enable_I2SClocks(BOOL bClkEnable)
{

    DEBUGMSG(ZONE_FUNCTION,
        (L"WAV:+OMAP35XX_HwAudioBridge::enable_I2SClocks\r\n")
        );

    if (bClkEnable == TRUE)
        {
        SetHwCodecMode_Speaker(m_hHwCodec, (AudioProfile_e)m_dwAudioProfile);
        }
    else
        {
        SetHwCodecMode_Disabled(m_hHwCodec);
        }

    DEBUGMSG(ZONE_FUNCTION,
        (L"WAV:-OMAP35XX_HwAudioBridge::enable_I2SClocks\r\n")
        );

    return TRUE;
}

//------------------------------------------------------------------------------
//  Called by AudioStream port
//
BOOL
OMAP35XX_HwAudioBridge::OnAudioStreamMessage(
    AudioStreamPort *pPort,
    DWORD msg,
    void *pvData
    )
{
    DEBUGMSG(ZONE_FUNCTION,
        (L"WAV::+OMAP35XX_HwAudioBridge::OnAudioStreamMessage(msg=%d)\r\n",
        msg));

    BOOL bRet = TRUE;
    ASPM_STREAM_DATA* pStreamData;

    ASSERT(m_pActivePort == pPort);
    if (m_pActivePort == pPort)
        {
        EnterMutex();
        switch (msg)
            {
            case ASPM_START_TX:
                DEBUGMSG(ZONE_HWBRIDGE, (L"WAV:ASPM_START_TX\r\n"));
                m_TransmitterState = kAudioRender_Active;
                break;

            case ASPM_PROCESSDATA_TX:
                DEBUGMSG(ZONE_HWBRIDGE, (L"WAV:ASPM_PROCESSDATA_TX\r\n"));
                pStreamData = (ASPM_STREAM_DATA*)pvData;
                if (pStreamData->hStreamContext == NULL)
                    {
                    bRet = m_prgStreams[kOutput]->copy_AudioData(
                        pStreamData->pBuffer, pStreamData->dwBufferSize
                        );
                    }
                else
                    {
                    // get a specific streams data
                    bRet = m_prgStreams[kOutput]->copy_StreamData(
                        pStreamData->hStreamContext, pStreamData->pBuffer,
                        pStreamData->dwBufferSize
                        );
                    }
                break;

            case ASPM_START_RX:
                DEBUGMSG(ZONE_HWBRIDGE, (L"WAV:ASPM_START_RX\r\n"));
                m_ReceiverState = kAudioRender_Active;
                break;

            case ASPM_PROCESSDATA_RX:
                DEBUGMSG(ZONE_HWBRIDGE, (L"WAV:ASPM_PROCESSDATA_RX\r\n"));
                pStreamData = (ASPM_STREAM_DATA*)pvData;
                if (pStreamData->hStreamContext == NULL)
                    {
                    bRet = m_prgStreams[kInput]->copy_AudioData(
                        pStreamData->pBuffer, pStreamData->dwBufferSize
                        );
                    }
                else
                    {
                    // get a specific streams data
                    bRet = m_prgStreams[kInput]->copy_StreamData(
                        pStreamData->hStreamContext, pStreamData->pBuffer,
                        pStreamData->dwBufferSize
                        );
                    }
                break;

            case ASPM_STOP_TX:
                DEBUGMSG(ZONE_HWBRIDGE, (L"WAV:ASPM_STOP_TX\r\n"));
                if (m_TransmitterState == kAudioRender_Stopping)
                    {
                    m_TransmitterState = kAudioRender_Idle;
                    }

                // a stop was forced to change frequency or audio routing.
                // Cause audio rendering to start again.
                //
                update_AudioRouting();

                if (m_TransmitterState != kAudioRender_Idle)
                    {
                    m_pActivePort->signal_Port(ASPS_START_TX, NULL, 0);
                    }

                if (m_ReceiverState == kAudioRender_Starting)
                    {
                    m_pActivePort->signal_Port(ASPS_START_RX, NULL, 0);
                    }
                break;

            case ASPM_STOP_RX:
                DEBUGMSG(ZONE_HWBRIDGE, (L"WAV:ASPM_STOP_RX\r\n"));
                if (m_ReceiverState != kAudioRender_Idle )
                    {
                    m_ReceiverState = kAudioRender_Idle;

                    }
                update_AudioRouting();
                break;
            }

        ExitMutex();
        }

    DEBUGMSG(ZONE_FUNCTION,
        (L"WAV::-OMAP35XX_HwAudioBridge::OnAudioStreamMessage(msg=%d)\r\n",
        msg));

    return bRet;
}



