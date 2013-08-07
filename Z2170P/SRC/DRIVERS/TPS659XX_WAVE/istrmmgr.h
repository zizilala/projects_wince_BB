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

#ifndef __ISTRMMGR_H__
#define __ISTRMMGR_H__


//------------------------------------------------------------------------------
//
//  Audio mixer control, expose microphone volume control
//
class CAudioControl_RecordVolume :
                       public CAudioControlType<MIXERCONTROL_CONTROLTYPE_VOLUME>
{
public:
    virtual WCHAR const* get_Name() const { return L"Record Volume"; }
    virtual WCHAR const* get_ShortName() const { return L"Record Volume"; }
    virtual DWORD get_StatusFlag() const
    {
        return MIXERCONTROL_CONTROLF_DISABLED;
    }

public:
    virtual void copy_ControlInfo(MIXERCONTROL *pControlInfo)
    {
        CAudioControlType<MIXERCONTROL_CONTROLTYPE_VOLUME>::
            copy_ControlInfo(pControlInfo);

        pControlInfo->Bounds.dwMinimum = LOGICAL_VOLUME_MIN;
        pControlInfo->Bounds.dwMaximum = LOGICAL_VOLUME_MAX;
        pControlInfo->Metrics.cSteps   = LOGICAL_VOLUME_STEPS;
    }
};


//------------------------------------------------------------------------------
//
//  Audio mixer control, expose microphone mute control
//
class CAudioControl_MicMute :
                      public CAudioControlType<MIXERCONTROL_CONTROLTYPE_BOOLEAN>
{
public:
    virtual WCHAR const* get_Name() const { return L"Mic Mute"; }
    virtual WCHAR const* get_ShortName() const { return L"Mute"; }

public:
    virtual void copy_ControlInfo(MIXERCONTROL *pControlInfo)
    {
        CAudioControlType<MIXERCONTROL_CONTROLTYPE_BOOLEAN>::
            copy_ControlInfo(pControlInfo);

        pControlInfo->Bounds.dwMinimum = 0;
        pControlInfo->Bounds.dwMaximum = 1;
        pControlInfo->Metrics.cSteps =  0;
    }
};


//------------------------------------------------------------------------------
//
//  Manages all input streams.  Represents the WAV-IN logical audio mixer line
//
class CInputStreamManager : public CStreamManager,
                            public CAudioLine_WavInVolume
{
    //--------------------------------------------------------------------------
    // member variables
    //
public:

    CAudioControl_RecordVolume  m_RecordVolumeControl;


    //--------------------------------------------------------------------------
    // Constructor
    //
public:
    CInputStreamManager() : CStreamManager(),
                            CAudioLine_WavInVolume() {}


    //--------------------------------------------------------------------------
    // CStream specific methods
    //
public:
    StreamContext *create_Stream(LPWAVEOPENDESC lpWOD);
    virtual DWORD get_ExtDevCaps(PVOID pCaps, DWORD dwSize);
    virtual DWORD get_DevCaps(PVOID pCaps, DWORD dwSize);
    void StreamReadyToRender(StreamContext *pStreamContext);

    virtual DWORD GetProperty(PWAVEPROPINFO pPropInfo);
    virtual DWORD SetProperty(PWAVEPROPINFO pPropInfo);

    DWORD GetInputProperty(PWAVEPROPINFO pPropInfo)
    {
        return CommonGetProperty(pPropInfo, TRUE);
    }

    DWORD SetInputProperty(PWAVEPROPINFO pPropInfo)
    {
        return CommonSetProperty(pPropInfo, TRUE);
    }


    //--------------------------------------------------------------------------
    // CStreamCallback specific methods
    //
public:

    virtual DWORD copy_AudioData(void* pStart, DWORD nSize);


    //--------------------------------------------------------------------------
    // CAudioLine specific methods
    //
public:
    virtual WCHAR const* get_Name() const        { return L"Recorder Volume"; }
    virtual WCHAR const* get_ShortName() const   { return L"Recorder Volume"; }

    virtual BOOL initialize_AudioLine(CAudioMixerManager *pAudioMixerManager);

    virtual DWORD get_AudioValue(
        CAudioControlBase *pControl,
        PMIXERCONTROLDETAILS pDetail,
        DWORD dwFlag) const;

    virtual DWORD put_AudioValue(
        CAudioControlBase *pControl,
        PMIXERCONTROLDETAILS pDetail,
        DWORD dwFlag);

};


//------------------------------------------------------------------------------
//
//  Represents the microphone logical input mixer line
//
class CMicrophoneInputSource : public CAudioLineBase
{
    //--------------------------------------------------------------------------
    // member variables
    //
public:

    CInputStreamManager        *m_pInputStreamManager;
    CAudioControl_MicMute       m_MicMuteControl;


    //--------------------------------------------------------------------------
    // Constructor
    //
public:
    CMicrophoneInputSource() : CAudioLineBase()
    {
        m_ComponentType = MIXERLINE_COMPONENTTYPE_SRC_MICROPHONE;
        m_countChannels = 1;
        m_TargetType = MIXERLINE_TARGETTYPE_WAVEIN;
        m_ffLineStatus = MIXERLINE_LINEF_ACTIVE | MIXERLINE_LINEF_SOURCE;
    }

    //--------------------------------------------------------------------------
    // CAudioLine specific methods
    //
public:
    virtual WCHAR const* get_Name() const        { return L"Microphone"; }
    virtual WCHAR const* get_ShortName() const   { return L"Microphone"; }

    virtual BOOL initialize_AudioLine(CAudioMixerManager *pAudioMixerManager);

    virtual DWORD get_AudioValue(
        CAudioControlBase *pControl,
        PMIXERCONTROLDETAILS pDetail,
        DWORD dwFlag) const;

    virtual DWORD put_AudioValue(
        CAudioControlBase *pControl,
        PMIXERCONTROLDETAILS pDetail,
        DWORD dwFlag);

};


#endif // __ISTRMMGR_H__

