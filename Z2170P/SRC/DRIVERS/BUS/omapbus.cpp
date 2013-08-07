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
/* File:    OmapBus.cpp
 *
 * Purpose: Device Loader 
 *
 */
#include <windows.h>
#include <types.h>
#include "omapbus.h"
#include "pcibus.h"

#include <bsp.h>
#include <omap.h>
#include <oal_clock.h>

#ifdef DEBUG
#define DBG_ERROR      1
#define DBG_WARNING    2
#define DBG_FUNCTION   4
#define DBG_INIT       8
#define DBG_ENUM      16
#define DBG_PCMCIA     32
#define DBG_ACTIVE     64
#define DBG_TAPI       128
#define DBG_FSD        256
#define DBG_DYING      512

DBGPARAM dpCurSettings = {
    TEXT("BUSENUM"), {
    TEXT("Errors"),TEXT("Warnings"),TEXT("Functions"),TEXT("Initialization"),
    TEXT("Enumeration"),TEXT("PCMCIA Devices"),TEXT("Active Devices"),TEXT("TAPI stuff"),
    TEXT("File System Drvr"),TEXT("Dying Devs"),TEXT("Undefined"),TEXT("Undefined"),
    TEXT("Undefined"),TEXT("Undefined"),TEXT("Undefined"),TEXT("Undefined") },
    DBG_ERROR | DBG_WARNING
};
#undef ZONE_ERROR
#undef 1
#define ZONE_ERROR      DEBUGZONE(0)
#define 1       DEBUGZONE(3)
#define ZONE_ENUM       DEBUGZONE(4)
#endif

OmapBus::OmapBus(LPCTSTR lpActiveRegPath)
:   DefaultBusDriver(lpActiveRegPath)
,   m_DeviceKey(lpActiveRegPath)
,   m_lpBusName(NULL)
{
    RETAILMSG(1,(TEXT("OmapBus::OmapBus (ActivateRegPath=%s)\r\n"),lpActiveRegPath));

    // Get DEVLOAD_DEVKEY_VALNAME name
    m_lpActiveRegPath = NULL;
    HANDLE hThisDevice = GetDeviceHandleFromContext(lpActiveRegPath);
    if(hThisDevice != NULL) {
        DEVMGR_DEVICE_INFORMATION di;
        memset(&di, 0, sizeof(di));
        di.dwSize = sizeof(di);
        if(GetDeviceInformationByDeviceHandle(hThisDevice, &di)) {
            DWORD dwKeyLen = wcslen(di.szDeviceKey) + 1;
            m_lpActiveRegPath = new TCHAR [dwKeyLen];
            if ( m_lpActiveRegPath) {
                wcscpy(m_lpActiveRegPath, di.szDeviceKey);
            }
            LPCWSTR pszName = wcschr(di.szBusName, L'\\');
            if(pszName != NULL) {
                pszName++;
                if(*pszName != 0) {
                    dwKeyLen = wcslen(pszName) + 1;
                    m_lpBusName = new TCHAR[dwKeyLen];
                    if (m_lpBusName) {
                        wcscpy(m_lpBusName, pszName);
                    }
                }
            }
        }
    }
        
    m_lpStrInitParam = NULL;
    m_dwNumOfInitParam =0 ;
    m_dwBusNumber = 0;
    m_dwDeviceIndex = 0;

}
OmapBus::~OmapBus()
{
    if (m_lpStrInitParam )
        delete m_lpStrInitParam ;
    if ( m_lpBusName)
        delete  m_lpBusName;
    if (m_lpActiveRegPath) 
        delete m_lpActiveRegPath;
}
#define BUSNAMEUNKNOWN TEXT("UnknownBus")
BOOL OmapBus::Init()
{
    BOOL  bReturn = FALSE;
    // Initialize InitParam
    if ( m_lpActiveRegPath==NULL) {
        RETAILMSG(1,(TEXT("REGENUM!Init RegOpenKeyEx() return FALSE!!!\r\n")));
        return FALSE;
    }
    if (m_DeviceKey.IsKeyOpened() && m_lpStrInitParam == NULL ) {
        DWORD dwKeyLen=0;
        DWORD dwType;
        LONG status=::RegQueryValueEx(m_DeviceKey.GetHKey(),DEVLOAD_REPARMS_VALNAME,NULL, &dwType,NULL,&dwKeyLen);
        if ( (status == ERROR_SUCCESS || status == ERROR_MORE_DATA) 
                && dwType== REG_MULTI_SZ && dwKeyLen!=0 ) {
            dwKeyLen = (dwKeyLen + sizeof(TCHAR) -1 ) / sizeof(TCHAR); 
            m_lpStrInitParam = new TCHAR [ dwKeyLen ];
            if (m_lpStrInitParam) {
                if (m_DeviceKey.GetRegValue(DEVLOAD_REPARMS_VALNAME,(LPBYTE)m_lpStrInitParam,dwKeyLen * sizeof(TCHAR))) {
                    DWORD dwUnitCount= dwKeyLen;                        
                    for (LPTSTR lpCurPos = m_lpStrInitParam;
                            m_dwNumOfInitParam< MAX_INITPARAM && dwUnitCount!=0 && *lpCurPos!=0 ; 
                            m_dwNumOfInitParam++) {
                        m_lpInitParamArray[m_dwNumOfInitParam] = lpCurPos;
                        while (*lpCurPos!=0 && dwUnitCount!=0  ) {
                            lpCurPos++;
                            dwUnitCount --;
                        }
                        if (dwUnitCount) {
                            lpCurPos ++;
                            dwUnitCount --;
                        }
                        else 
                            break;
                    }
                            
                }
            }
        }
    }
    // Scan And Enum All the driver in registry.
    if ( GetDeviceHandle()!=NULL &&  m_DeviceKey.IsKeyOpened()) {
        DWORD dwKeyLen=0;
        if (!m_DeviceKey.GetRegValue(DEVLOAD_INTERFACETYPE_VALNAME,(PUCHAR) &m_dwBusType,sizeof(m_dwBusType))) {
            // No interface type defined
            m_dwBusType=InterfaceTypeUndefined;
        };
        if (!m_DeviceKey.GetRegValue(DEVLOAD_BUSNUMBER_VALNAME , (LPBYTE)&m_dwBusNumber,sizeof(m_dwBusNumber)))
            m_dwBusNumber=0;
        
        if ( AssignChildDriver() )
            return TRUE;
    };
    RETAILMSG(1,(TEXT("-BusENUM!Init eturn FALSE!!!\r\n")));
    return FALSE;
    
};
BOOL OmapBus::PostInit()
{
    BOOL bReturn = FALSE;
    if ( GetDeviceHandle()!=NULL &&  m_DeviceKey.IsKeyOpened()) {
        bReturn=ActivateAllChildDrivers();
    }
    return bReturn;
}

BOOL OmapBus::AssignChildDriver()
{
    DWORD NumSubKeys;
    DWORD MaxSubKeyLen;
    DWORD MaxClassLen;
    DWORD NumValues;
    DWORD MaxValueNameLen;
    DWORD MaxValueLen;
    // Get info on Template Key
    BOOL bSuccess = m_DeviceKey.RegQueryInfoKey(
                NULL,               // class name buffer (lpszClass)
                NULL,               // ptr to length of class name buffer (lpcchClass)
                NULL,               // reserved
                &NumSubKeys,        // ptr to number of sub-keys (lpcSubKeys)
                &MaxSubKeyLen,      // ptr to longest subkey name length (lpcchMaxSubKeyLen)
                &MaxClassLen,       // ptr to longest class string length (lpcchMaxClassLen)
                &NumValues,         // ptr to number of value entries (lpcValues)
                &MaxValueNameLen,  // ptr to longest value name length (lpcchMaxValueNameLen)
                &MaxValueLen,       // ptr to longest value data length (lpcbMaxValueData)
                NULL,               // ptr to security descriptor length
                NULL);              // ptr to last write time
                    
    if (!bSuccess) {
        RETAILMSG(1,
            (TEXT("PCMCIA::RegCopyKey RegQueryInfoKey returned fails.\r\n")));
        return FALSE;
    }
    // Recurse for each sub-key
    
    for (DWORD Key = 0; Key < NumSubKeys; Key++) {
        // Get TKey sub-key according to Key
        WCHAR ValName[PCI_MAX_REG_NAME];
        DWORD ValLen = sizeof(ValName) / sizeof(WCHAR);
        if (! m_DeviceKey.RegEnumKeyEx(  Key, ValName, &ValLen, NULL,  NULL,  NULL, NULL)){
            RETAILMSG(1,
                (TEXT("OmapBus::RegCopyKey RegEnumKeyEx(%d) returned Error\r\n"), ValName));
            break;
        }
        else {
            // Open sub-key under TKey
            CRegistryEdit TSubKey(m_DeviceKey.GetHKey(),ValName);
            if (!TSubKey.IsKeyOpened()) {
                RETAILMSG(1,
                    (TEXT("PCIBUS::RegCopyKey RegOpenKeyEx(%s) returned Error\r\n"), ValName));
                
                continue;
            }
            else { // Get Bus Info.
                DDKPCIINFO dpi;
                dpi.cbSize = sizeof (DDKPCIINFO);                
                dpi.dwDeviceNumber = (DWORD)-1;
                dpi.dwFunctionNumber = (DWORD)-1;
                if (TSubKey.GetPciInfo(&dpi)!=ERROR_SUCCESS || dpi.dwDeviceNumber == (DWORD)-1 || dpi.dwFunctionNumber == (DWORD)-1) {
                    dpi.dwDeviceNumber = m_dwDeviceIndex++;
                    dpi.dwFunctionNumber = 0;
                }
                DWORD dwBusNumber = (DWORD)-1;
                if ( !TSubKey.GetRegValue(PCIBUS_BUSNUMBER_VALNAME, (LPBYTE)&dwBusNumber, sizeof(dwBusNumber)) || dwBusNumber == (DWORD)-1) {
                    dwBusNumber = m_dwBusNumber;
                }
                // We Create Foler for this driver.
                TCHAR lpChildPath[DEVKEY_LEN];
                _tcsncpy(lpChildPath,m_lpActiveRegPath,DEVKEY_LEN-1);
                lpChildPath[DEVKEY_LEN-2] = 0;
                DWORD dwLen=_tcslen(lpChildPath);
                lpChildPath[dwLen]=_T('\\');
                dwLen++;
                lpChildPath[dwLen]=0;
                _tcsncat(lpChildPath,ValName,DEVKEY_LEN-1-dwLen);
                lpChildPath[DEVKEY_LEN-1]=0;

                DeviceFolder * nDevice =
                    new DeviceFolder (m_lpBusName!=NULL?m_lpBusName:BUSNAMEUNKNOWN,
                            lpChildPath,m_dwBusType,dwBusNumber,dpi.dwDeviceNumber,dpi.dwFunctionNumber,GetDeviceHandle());
                if (nDevice) {
                    InsertChild(nDevice);
                }
            }
            
        }
    }
    
    return TRUE;
}
DWORD OmapBus::GetBusNamePrefix(__out_ecount(dwSizeInUnit) LPTSTR lpReturnBusName,DWORD dwSizeInUnit)
{
    if (m_lpBusName && lpReturnBusName &&  dwSizeInUnit) {
        DWORD dwCopyUnit = min(_tcslen(m_lpBusName) +1 , dwSizeInUnit);
        _tcsncpy(lpReturnBusName,m_lpBusName,dwCopyUnit);
        lpReturnBusName[dwCopyUnit-1]=0;
        return dwCopyUnit;
    }
    else
        return DefaultBusDriver::GetBusNamePrefix(lpReturnBusName,dwSizeInUnit);
}

#define MAX_TEMP_BUFFER_SIZE 0x200
BOOL OmapBus::ActivateAllChildDrivers()
{
    TakeReadLock();
    DeviceFolder * pCurDevice = GetDeviceList ();
    while (pCurDevice) {
        RETAILMSG(1,(TEXT("ActivateChild: Template reg path is %s\r\n"),pCurDevice->GetRegPath()));
        // Create Initial Active Registry.
        for (DWORD dwIndex=0; dwIndex< m_dwNumOfInitParam; dwIndex++) {
            BYTE tempBuffer[ MAX_TEMP_BUFFER_SIZE] ;
            DWORD dwSize=MAX_TEMP_BUFFER_SIZE;
            DWORD dwType;
            if (m_DeviceKey.IsKeyOpened() && 
                    m_DeviceKey.RegQueryValueEx(m_lpInitParamArray[dwIndex],&dwType, tempBuffer,&dwSize) ) {
                REGINI Reg;
                Reg.lpszVal = m_lpInitParamArray[dwIndex];
                Reg.dwType = dwType;
                Reg.dwLen = dwSize;
                Reg.pData = tempBuffer;
                pCurDevice->AddInitReg(1, &Reg);
                RETAILMSG(1,(TEXT("ActivateChild add %s to %s ActivePath\r\n"),m_lpInitParamArray[dwIndex],pCurDevice->GetRegPath()));                
            }
        }
        pCurDevice = pCurDevice->GetNextDeviceFolder();
    }
    // Activate Device 
    DWORD dwCurOrder = 0;
    while (dwCurOrder != MAXDWORD) {
        RETAILMSG(1,(TEXT(" OmapBus::ActivateChild load device driver at order %d \r\n"),dwCurOrder));
        DWORD dwNextOrder = MAXDWORD;
        pCurDevice = GetDeviceList ();
        while (pCurDevice) {
            DWORD dwDeviceLoadOrder = pCurDevice->GetLoadOrder();
            if ( dwDeviceLoadOrder == dwCurOrder)
                pCurDevice->LoadDevice();
            else 
            if (dwDeviceLoadOrder> dwCurOrder  && dwDeviceLoadOrder < dwNextOrder)
                dwNextOrder = dwDeviceLoadOrder;
            pCurDevice = pCurDevice->GetNextDeviceFolder();
        }
        dwCurOrder = dwNextOrder;
    }
    LeaveReadLock();
    return TRUE;
}
BOOL OmapBus::Open(DWORD AccessCode, DWORD Share)
{
    return TRUE;
}

BOOL OmapBus::IOControl(DWORD dwCode, PBYTE pBufIn, DWORD dwLenIn, PBYTE pBufOut, DWORD dwLenOut, PDWORD pdwActualOut)
{
    BOOL rc = FALSE;
	DWORD deviceID = OMAP_DEVICE_NONE;

    switch (dwCode)
    {
    case IOCTL_BUS_REQUEST_CLOCK: 
		{
			if ((pBufIn == NULL) || (dwLenIn != sizeof(DWORD)))
			{
                SetLastError(ERROR_INVALID_PARAMETER);
                break;
            }


			deviceID = *pBufIn;
		
            rc = EnableDeviceClocks(deviceID, TRUE);

            break;
        }
    case IOCTL_BUS_RELEASE_CLOCK: 
		{
			if ((pBufIn == NULL) || (dwLenIn != sizeof(DWORD)))
			{
                SetLastError(ERROR_INVALID_PARAMETER);
                break;
            }

			deviceID = *pBufIn;

            rc = EnableDeviceClocks(deviceID, FALSE);

            break;
        }

    case IOCTL_BUS_SOURCE_CLOCKS:
        {
            if ((pBufIn == NULL) || (dwLenIn < sizeof(CE_BUS_DEVICE_SOURCE_CLOCKS)))
            {
                SetLastError(ERROR_INVALID_PARAMETER);
                break;
            }
            CE_BUS_DEVICE_SOURCE_CLOCKS* pClockInfo = (CE_BUS_DEVICE_SOURCE_CLOCKS*)pBufIn;
            rc = SelectDeviceSourceClocks(pClockInfo->devId,
                pClockInfo->count, pClockInfo->rgSourceClocks
                );
            break;
        }
    case IOCTL_BUS_DEVICESTATE_CALLBACKS:
        {
            if ((pBufIn == NULL) || (dwLenIn < sizeof(CE_BUS_DEVICESTATE_CALLBACKS)))

            {
                SetLastError(ERROR_INVALID_PARAMETER);
                break;
            }
/*
            pDeviceStateCallbacks = (CE_BUS_DEVICESTATE_CALLBACKS*)pInBuffer;
            rc = pBus->SetDevicePowerStateChangeCallbacks(
                pDeviceStateCallbacks->PreDeviceStateChange, 
                pDeviceStateCallbacks->PostDeviceStateChange
                );*/
            break;
        }
    default:
        rc = DefaultBusDriver::IOControl(dwCode, pBufIn, dwLenIn, pBufOut, dwLenOut, pdwActualOut);
        break;
    }
    return rc;
}

extern "C" BOOL WINAPI
DllEntry(HINSTANCE DllInstance, INT Reason, LPVOID Reserved)
{
    switch(Reason) {
    case DLL_PROCESS_ATTACH:
        DEBUGREGISTER(DllInstance);
        RETAILMSG(1,(TEXT("OmapBus.DLL DLL_PROCESS_ATTACH\r\n")));
        DisableThreadLibraryCalls((HMODULE) DllInstance);
        break;
    case DLL_PROCESS_DETACH:
        RETAILMSG(1,(TEXT("OmapBus.DLL DLL_PROCESS_DETACH\r\n")));
        break;
    };
    return TRUE;
}

// Init() - called by RegisterDevice when we get loaded
extern "C" DWORD
Init (DWORD dwInfo)
{
    DefaultBusDriver * pBusDriver = new OmapBus((LPCTSTR)dwInfo);
    if ( pBusDriver) {
        if ( pBusDriver->Init())
            return  (DWORD)pBusDriver;
        else 
            delete  pBusDriver;
    }
    return NULL;
}


extern "C" void
Deinit (DWORD dwData)
{
    DefaultBusDriver * pBusDriver = (DefaultBusDriver * ) dwData;
    if (pBusDriver )
        delete pBusDriver ;    
}
extern "C" BOOL 
PowerUp(DWORD dwData)
{
    RETAILMSG(1,(TEXT("OmapBus.DLL : +PowerUp dwData=%x \r\n"),dwData));
    DefaultBusDriver * pBusDriver = (DefaultBusDriver * )dwData;
    if (pBusDriver)
        pBusDriver->PowerUp();
    RETAILMSG(1,(TEXT("OmapBus.DLL : -PowerUp dwData=%x \r\n")));
    return TRUE;
}
extern "C" BOOL 
PowerDown(DWORD dwData)
{
    RETAILMSG(1,(TEXT("OmapBus.DLL : +PowerDown dwData=%x \r\n"),dwData));
    DefaultBusDriver * pBusDriver = (DefaultBusDriver * )dwData;
    if (pBusDriver)
        pBusDriver->PowerDown();
    RETAILMSG(1,(TEXT("OmapBus.DLL : -PowerDown dwData=%x \r\n")));
    return TRUE;
}

extern "C" HANDLE
Open(
        HANDLE  pHead,          // @parm Handle returned by COM_Init.
        DWORD   AccessCode,     // @parm access code.
        DWORD   ShareMode       // @parm share mode - Not used in this driver.
        )
{
    DefaultBusDriver * pBusDriver = (DefaultBusDriver * )pHead;
    if (pBusDriver && pBusDriver->Open(AccessCode,ShareMode))
        return (HANDLE)pBusDriver;
    return NULL;
}
extern "C" BOOL
Close(HANDLE pOpenHead)
{
    DefaultBusDriver * pBusDriver = (DefaultBusDriver * )pOpenHead;
    if (pBusDriver)
        return pBusDriver->Close();
    return FALSE;
}
extern "C" BOOL
IOControl(HANDLE pOpenHead,
              DWORD dwCode, PBYTE pBufIn,
              DWORD dwLenIn, PBYTE pBufOut, DWORD dwLenOut,
              PDWORD pdwActualOut)
{
    DefaultBusDriver * pBusDriver = (DefaultBusDriver * )pOpenHead;
    if (pBusDriver)
        return pBusDriver->IOControl(dwCode,pBufIn,dwLenIn,pBufOut,dwLenOut,pdwActualOut);
    return FALSE;
}

