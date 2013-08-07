// All rights reserved ADENEO EMBEDDED 2010
// Copyright (c) 2007, 2008 BSQUARE Corporation. All rights reserved.

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
//  File:  kitlreg.c
//

//-------------------------------------------------------------------------------

#include <bsp.h>
#include <devload.h>

//------------------------------------------------------------------------------
//  Local definition

#ifndef HKEY_LOCAL_MACHINE
#define HKEY_LOCAL_MACHINE          ((HKEY)(ULONG_PTR)0x80000002)
#endif

static BOOL
SetDeviceGroup(
    LPCWSTR szKeyPath,
    LPCWSTR groupName
    )
{
    LONG code;
    HKEY hKey;
    DWORD value;

    // Open/create key
    code = NKRegCreateKeyEx(
            HKEY_LOCAL_MACHINE, szKeyPath, 0, NULL, 0, 0, NULL, &hKey, &value
            );          

    if (code != ERROR_SUCCESS) goto cleanUp;

    // Set value
    code = NKRegSetValueEx(
        hKey, L"Group", 0, REG_SZ, (UCHAR*)groupName,  (wcslen(groupName) + 1) * sizeof(WCHAR)
        );

    // Close key
    NKRegCloseKey(hKey);

cleanUp:
    return (code == ERROR_SUCCESS);
}

//------------------------------------------------------------------------------
static BOOL
SetDeviceDriverFlags(
    LPCWSTR szKeyPath,
    DWORD flags
    )
{
    LONG code;
    HKEY hKey;
    DWORD value;

    // Open/create key
    code = NKRegCreateKeyEx(
            HKEY_LOCAL_MACHINE, szKeyPath, 0, NULL, 0, 0, NULL, &hKey, &value
            );          

    if (code != ERROR_SUCCESS) goto cleanUp;

    // Set value
    code = NKRegSetValueEx(
        hKey, L"Flags", 0, REG_DWORD, (UCHAR*)&flags, sizeof(DWORD)
        );

    // Close key
    NKRegCloseKey(hKey);

cleanUp:
    return (code == ERROR_SUCCESS);
}

//------------------------------------------------------------------------------

void OEMEthernetDriverEnable(BOOL bEnable)
{
    // Note that this requires a custom check in the ethernet
    // driver since it is loaded by NDIS and not devmgr.
    // We are using the same registry key implementation since 
    // it does not otherwise conflict
    if (bEnable == TRUE)
    {
        SetDeviceGroup(
            L"Comm\\SMSC9118", L"NDIS"
            );
    }
    else
    {
        SetDeviceGroup(
            L"Comm\\SMSC9118", L""
            );
    }
}

//------------------------------------------------------------------------------

void OEMUsbDriverEnable(BOOL bEnable)
{
    if (bEnable == TRUE)
    {
        SetDeviceDriverFlags(
            L"Drivers\\BuiltIn\\MUsbOtg", DEVFLAGS_NONE
            );
        SetDeviceDriverFlags(
            L"Drivers\\BuiltIn\\MUsbOtg\\Hcd", DEVFLAGS_NONE
            );
        SetDeviceDriverFlags(
            L"Drivers\\BuiltIn\\MUsbOtg\\UsbFn", DEVFLAGS_NONE
            );
    }
    else
    {
        SetDeviceDriverFlags(
            L"Drivers\\BuiltIn\\MUsbOtg", DEVFLAGS_NOLOAD
            );
        SetDeviceDriverFlags(
            L"Drivers\\BuiltIn\\MUsbOtg\\Hcd", DEVFLAGS_NOLOAD
            );
        SetDeviceDriverFlags(
            L"Drivers\\BuiltIn\\MUsbOtg\\UsbFn", DEVFLAGS_NOLOAD
            );
    }
}
