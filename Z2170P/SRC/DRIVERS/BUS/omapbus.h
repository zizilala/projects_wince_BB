// All rights reserved ADENEO EMBEDDED 2010
//
// Copyright (c) Microsoft Corporation.  All rights reserved.
//
//
// Use of this sample source code is subject to the terms of the Microsoft
// license agreement under which you licensed this sample source code. If
// you did not accept the terms of the license agreement, you are not
// authorized to use this sample source code. For the terms of the license,
// please see the license agreement between you and Microsoft or, if applicable,
// see the LICENSE.RTF on your install media or the root of your tools installation.
// THE SAMPLE SOURCE CODE IS PROVIDED "AS IS", WITH NO WARRANTIES.
//
/* File:    OmapBus.h
 *
 * Purpose: Device Loader 
 *
 */
#ifndef __BUSENUM_H_
#define __BUSENUM_H_

#pragma warning (push)
#pragma warning (disable:6287)
#include <DefBus.h>
#pragma warning (pop)

#include "omap_bus.h"


#define MAX_LOAD_ORDER 255
#define MAX_INITPARAM 0x20

class OmapBus : public  DefaultBusDriver {
public:
    OmapBus(LPCTSTR lpActiveRegPath);
    virtual ~OmapBus();
    virtual BOOL Init(); 
    virtual BOOL PostInit() ;
    virtual BOOL AssignChildDriver();
    virtual BOOL ActivateAllChildDrivers();
    virtual DWORD GetBusNamePrefix(LPTSTR lpReturnBusName,DWORD dwSizeInUnit);
    virtual BOOL IOControl(DWORD dwCode, PBYTE pBufIn, DWORD dwLenIn, PBYTE pBufOut, DWORD dwLenOut, PDWORD pdwActualOut);
    virtual BOOL  Open( DWORD   AccessCode,   DWORD   Share) ;
private:
    LPTSTR  m_lpActiveRegPath;
    CRegistryEdit m_DeviceKey;
    DWORD   m_dwBusType;
    LPTSTR  m_lpBusName;
    DWORD   m_dwBusNumber;
    DWORD   m_dwDeviceIndex;
    
    LPTSTR  m_lpStrInitParam;
    DWORD   m_dwNumOfInitParam;
    LPTSTR  m_lpInitParamArray[MAX_INITPARAM];
    
};
#endif
