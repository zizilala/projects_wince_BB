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
//------------------------------------------------------------------------------
//
//  File:  SecureKeys.c
//
//  This file implements the IOCTL_HAL_GETREGSECUREKEYS handler.
//
#include <windows.h>
#include <oal.h>

//------------------------------------------------------------------------------
//
//  Function:  OALIoCtlHalGetRegSecureKeys
//
//  Implements a common IOCTL_HAL_GETREGSECUREKEYS handler.  
//
BOOL OALIoCtlHalGetRegSecureKeys( 
    UINT32 dwIoControlCode, VOID *lpInBuf, UINT32 nInBufSize, VOID *lpOutBuf, 
    UINT32 nOutBufSize, UINT32* lpBytesReturned)
{

    BOOL rc;

#if defined ( project_smartfon ) || defined ( project_wpc )  // Smartphone or PocketPC build
    // List of Secure registry keys
    RegSecureKey OEMSecNames[] = {
        { REGSEC_HKLM | REGSEC_HKCU, 8,  L"security" },
        { REGSEC_HKLM, 6, L"loader" }
    };

    RegSecureKeyList OEMSecList = {
        sizeof(OEMSecNames) / sizeof(RegSecureKey),
        OEMSecNames,
    };

    DWORD dwName;
    DWORD dwBufSize;

    // size of structs without names
    dwBufSize = sizeof(OEMSecList) + sizeof(OEMSecNames);

    for (dwName = 0; dwName < OEMSecList.dwNumKeys; dwName++) 
    {
        dwBufSize += OEMSecNames[dwName].wLen * sizeof(WCHAR); // no nulls
    }

    // First call: return the required buffer size
    if (!lpInBuf && !nInBufSize && lpOutBuf && (nOutBufSize == sizeof(DWORD))) 
    {
        *((DWORD*)lpOutBuf) = dwBufSize;
        if (lpBytesReturned)
        {
            *lpBytesReturned = sizeof(dwBufSize);
        }
        
        rc = TRUE;
    }
    // Second call: fill the provided buffer
    // nOutBufSize should be the same as returned on first call
    else if (!lpInBuf && !nInBufSize && lpOutBuf && (nOutBufSize >= dwBufSize)) 
    {                
        RegSecureKeyList *pKeys = (RegSecureKeyList*)lpOutBuf;

        // pStr moves through the buffer as strings are written
        LPWSTR pStr = (LPWSTR)((LPBYTE)lpOutBuf + sizeof(OEMSecList) + sizeof(OEMSecNames));

        pKeys->dwNumKeys = OEMSecList.dwNumKeys;
        pKeys->pList = (RegSecureKey*) ((LPBYTE)lpOutBuf + sizeof(OEMSecList));

        for (dwName = 0; dwName < OEMSecList.dwNumKeys; dwName++) 
        {
            pKeys->pList[dwName].wRoots = OEMSecNames[dwName].wRoots;
            pKeys->pList[dwName].wLen = OEMSecNames[dwName].wLen;
            pKeys->pList[dwName].pName = pStr;
            memcpy(pStr, OEMSecNames[dwName].pName, OEMSecNames[dwName].wLen * sizeof(WCHAR));

            pStr += OEMSecNames[dwName].wLen;
        }

        if (lpBytesReturned)
        {
            *lpBytesReturned = dwBufSize;
        }

        rc = TRUE;
    } 
#else  // Not Smartphone or PocketPC build; we have no registry keys to secure
    // First call: return buffer size of 0.  We should not be called again.
    if (!lpInBuf && !nInBufSize && lpOutBuf && (nOutBufSize == sizeof(DWORD))) 
    {
        *((DWORD*)lpOutBuf) = (DWORD)0;
        if (lpBytesReturned)
        {
            *lpBytesReturned = sizeof(DWORD);
        }

        rc = TRUE;
    }
#endif
    else 
    {
        // Invalid args
        NKSetLastError(ERROR_INVALID_PARAMETER);
        rc = FALSE;
    }

    OALMSG(OAL_IOCTL&&OAL_FUNC, (L"-OALIoCtlHalGetRegSecureKeys(rc = %d)\r\n", rc));
    return (rc);
}

