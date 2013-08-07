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

#ifndef __OMAP35XX_HWBRIDGE_H__
#define __OMAP35XX_HWBRIDGE_H__

#include "hwaudiobrdg.h"
#include "mcbsp.h"
#include "memtxapi.h"
#include "dmtaudioport.h"
#include "dasfaudioport.h"

//------------------------------------------------------------------------------
//
//  Serves as a base class for all hw audio bridge
//  - defines a common interface so all audio components can communicate with it
//
class OMAP35XX_HwAudioBridge : public CHardwareAudioBridge, 
                                public AudioStreamPort_Client
{
public:
    // Note: The values of this enum are used in the registry to select default audio route
    typedef enum 
        {
        kAudioRoute_Unknown = 0,
        kAudioRoute_Handset,
        kAudioRoute_Headset,
        kAudioRoute_Carkit,
        kAudioRoute_Speaker,
        kAudioRoute_BTHeadset,
        kAudioRoute_AuxHeadset,
        kAudioRoute_HdmiAudioRequested,
        kAudioRoute_HdmiAudio,
        kAudioRoute_HdmiAudioDetached,
        kAudioRoute_Count
        }
    AudioRoute_e;

    typedef enum
        {
        kAudioRender_Idle,
        kAudioRender_Starting,
        kAudioRender_Active,
        kAudioRender_Stopping
        }
    AudioRenderState_e;

    typedef enum
        {
        kAudioDMTPort,
        kAudioDASFPort
        }
    AudioActiveRenderPort_e;

    //--------------------------------------------------------------------------
    // member variables
    //
protected:

    HANDLE                                  m_hICX;
    HANDLE                                  m_hHwCodec;

    CEDEVICE_POWER_STATE                    m_ReqestedPowerState;
    BOOL                                    m_bRxAbortRequested;
    BOOL                                    m_bTxAbortRequested;

    AudioStreamPort                        *m_pActivePort;
    DMTAudioStreamPort                      m_DMTPort;
    DASFAudioStreamPort                     m_DASFPort;

    AudioRoute_e                            m_RequestAudioRoute;
    AudioRoute_e                            m_CurrentAudioRoute;
    DWORD                                   m_fRequestAudioRouteDirty;
    AudioRenderState_e                      m_ReceiverState;
    AudioRenderState_e                      m_TransmitterState;
    DWORD                                    m_dwAudioProfile;
    BOOL                                    m_bPortSwitch;
    AudioActiveRenderPort_e                 m_CurrActivePort;
    BOOL                                    m_bPreviousPortIsDASF;
    
    //--------------------------------------------------------------------------
    // constructor/destructor
    //
public:

    OMAP35XX_HwAudioBridge() :
        CHardwareAudioBridge(),
        m_DMTPort(),
        m_DASFPort(),
        m_pActivePort(NULL),
        m_hICX(NULL)
    {
        m_ReceiverState = kAudioRender_Idle;
        m_TransmitterState = kAudioRender_Idle;
        m_fRequestAudioRouteDirty = TRUE;
        m_CurrentAudioRoute = kAudioRoute_Unknown;
        m_RequestAudioRoute = kAudioRoute_Handset;
        m_dwAudioProfile = 0;
        m_bPortSwitch = FALSE;
        m_CurrActivePort = kAudioDMTPort;
        m_bPreviousPortIsDASF = FALSE;
    }

    //--------------------------------------------------------------------------
    // AudioStreamPort_Client methods
    //
public:    

    virtual BOOL OnAudioStreamMessage(AudioStreamPort *pPort,
                                      DWORD msg,
                                      void *pvData);
    
    void initialize(WCHAR const *szDMTDriver,
                    WCHAR const *szSerialDriver,
                    HwCodecConfigInfo_t *pHwCodecConfigInfo,
                    HANDLE hPlayPortConfigInfo,
                    HANDLE hRecPortConfigInfo,
					AudioRoute_e eAudioRoute);

    //--------------------------------------------------------------------------
    
protected:
    BOOL update_AudioRouting();
    BOOL query_AudioRouteRequestPending() const 
    { 
        return m_fRequestAudioRouteDirty;
    }

    //--------------------------------------------------------------------------
    // public methods
public:

    void SetAudioPath(AudioRoute_e audioroute,
                      DWORD dwAudioProfile);

    // calling this routine does not necessarily immediately change the
    // power state; ie when audio is currently being rendered
    //
    virtual void request_PowerState(CEDEVICE_POWER_STATE powerState) 
    {
        DEBUGMSG(1, (L"WAV: request_PowerState - %x\r\n", powerState));

        if (powerState >= D3)
            {
            m_ReqestedPowerState = powerState;

            // if the RX is running, then abort it.
            if (m_ReceiverState == kAudioRender_Starting ||
                m_ReceiverState == kAudioRender_Active)
                {
                m_bRxAbortRequested = TRUE;
                abort_Stream(kInput, NULL);
                }

            // if the TX is running, then abort it.
            if (m_TransmitterState == kAudioRender_Starting ||
                m_TransmitterState == kAudioRender_Active)
                {
                m_bTxAbortRequested = TRUE;
                m_TransmitterState=kAudioRender_Stopping;
                abort_Stream(kOutput, NULL);
                }
            }
		else
            {
            CEDEVICE_POWER_STATE prevReqestedPowerState = m_ReqestedPowerState;
            m_ReqestedPowerState = powerState;

            // Check if previously D3/D4 was requested!
            if (prevReqestedPowerState >= D3)
                {
                // If RX was aborted earlier, update the flag & re-start RX.
                if (m_bRxAbortRequested)
                    {
                    m_bRxAbortRequested = FALSE;
                    start_Stream(kInput);
                    }

                // If TX was aborted earlier, just update the flag, No need
                // re-start it.
                if (m_bTxAbortRequested)
                    {
                    m_bTxAbortRequested = FALSE;
                    start_Stream(kOutput);
                    }
                }
            }

        if (powerState == D0 || powerState == D4)
            {
            m_PowerState = powerState;
            }
    }
    
    virtual BOOL start_AudioPort(StreamType type);
    virtual BOOL stop_AudioPort(StreamType type);
    virtual BOOL start_Stream(StreamType type,
                              HANDLE hStreamContext);
    virtual BOOL start_Stream(StreamType type);
    virtual BOOL stop_Stream(StreamType type,
                             HANDLE hStreamContext);
    virtual BOOL abort_Stream(StreamType type,
                              HANDLE hStreamContext);
    virtual BOOL set_StreamGain(StreamType type,
                                HANDLE hStreamContext,
                                DWORD dwContextData);
    virtual BOOL switch_AudioStreamPort(BOOL bPortRequest);
    virtual BOOL query_AudioStreamPort();
    virtual BOOL enable_I2SClocks(BOOL bClkEnable);

    // we're always in wideband mode and speaker mode is not supported
    //
    virtual BOOL query_WideBand() const
    {
        return TRUE;
    }

    virtual BOOL enable_Headset(BOOL bEnable)
    {
        CHardwareAudioBridge::enable_Headset(bEnable);
        if (bEnable)
            {
            m_RequestAudioRoute = kAudioRoute_Headset;
            }
        else
            {
            m_RequestAudioRoute = kAudioRoute_Handset;
            }
        update_AudioRouting();
        return TRUE;
    }

    virtual BOOL enable_AuxHeadset(BOOL bEnable)
    {
        CHardwareAudioBridge::enable_AuxHeadset(bEnable);
        if (bEnable)
            {
            m_RequestAudioRoute = kAudioRoute_AuxHeadset;
            }
        else
            {
            m_RequestAudioRoute = kAudioRoute_Handset;
            }
        update_AudioRouting();
        return TRUE;
    }

    virtual BOOL enable_Speaker(BOOL bEnable)
    {
        CHardwareAudioBridge::enable_Speaker(bEnable);
        update_AudioRouting();
        return TRUE;
    }
    
    virtual void notify_BTHeadsetAttached(BOOL bAttached)
    {
        CHardwareAudioBridge::notify_BTHeadsetAttached(bAttached);
        update_AudioRouting();
        return;
    }

    virtual void notify_HdmiAudioAttached(BOOL bAttached)
    {
        CHardwareAudioBridge::notify_HdmiAudioAttached(bAttached);
        update_AudioRouting();
    }

    virtual BOOL query_SpeakerEnable() const
    {
        return m_CurrentAudioRoute == kAudioRoute_Speaker;
    }

    virtual BOOL query_CarkitEnable() const   
    { 
        return m_CurrentAudioRoute == kAudioRoute_Carkit; 
    }

    
    virtual BOOL query_HeadsetEnable() const
    {
        return m_CurrentAudioRoute == kAudioRoute_Headset ||
               m_CurrentAudioRoute == kAudioRoute_AuxHeadset ||
               m_CurrentAudioRoute == kAudioRoute_BTHeadset;
    }   

};


#endif //__OMAP35XX_HWBRIDGE_H__
