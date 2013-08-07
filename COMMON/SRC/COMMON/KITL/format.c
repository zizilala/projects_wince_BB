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
//------------------------------------------------------------------------------
//
//  File:  format.c
//
#include <windows.h>
#include <oal.h>


//------------------------------------------------------------------------------
//
//  Function:  OALKitlIPtoString
//
LPWSTR OALKitlIPtoString(UINT32 ip4)
{
    static WCHAR szBuffer[16];

    OALLogPrintf(
        szBuffer, sizeof(szBuffer)/sizeof(WCHAR), L"%d.%d.%d.%d",
        (UINT8)ip4, (UINT8)(ip4 >> 8), (UINT8)(ip4 >> 16), (UINT8)(ip4 >> 24)
    );
    return szBuffer;
}


//------------------------------------------------------------------------------
//
//  Function:  OALKitlMACtoString
//
LPWSTR OALKitlMACtoString(UINT16 mac[])
{
    static WCHAR szBuffer[18];

    OALLogPrintf(
        szBuffer, sizeof(szBuffer)/sizeof(WCHAR), 
        L"%02x:%02x:%02x:%02x:%02x:%02x", mac[0]&0xFF, mac[0]>>8, mac[1]&0xFF,
        mac[1]>>8, mac[2]&0xFF, mac[2]>>8
    );
    return szBuffer;
}    


//------------------------------------------------------------------------------
//
//  Function: OALKitlStringToIP
//
UINT32 OALKitlStringToIP(LPCWSTR szIP)
{
    UINT32 ip = 0, count, part;
    LPCWSTR psz;
    
    // Replace the dots with NULL terminators
    psz = szIP;
    count = 0;
    part = 0;
    while (count < 4) {
        if (*psz == L'.' || *psz == L'\0') {
            ip |= part << (count << 3);
            part = 0;
            count++;
        } else if (*psz >= L'0' && *psz <= L'9') {
            part = part * 10 + (*psz - L'0');
            if (part > 255) {
                ip = 0;
                break;
            }
        } else {
            break;
        }
        if (*psz == L'\0') break;
        psz++;
    }
    return count >= 4 ? ip : 0;
} 


//------------------------------------------------------------------------------
//
//  Function:  OALKitlStringToMAC
//
BOOL OALKitlStringToMAC(LPCWSTR szMAC, UINT16 mac[3])
{
    INT32 i, j;
    LPCWSTR pos;
    WCHAR ch;
    UINT8 m[6];

    // Convert string to MAC address
    memset(m, 0, sizeof(m));
    i = j = 0;
    pos = szMAC;
    while (i < 6) {
        ch = *pos;
        if (ch == L'-' || ch == L':' || ch == L'.') {
            i++;
            j = 0;
        } else {
            if (j >= 2) {
                i++;
                j = 0;
            }
            if (ch >= L'0' && ch <= L'9') {
                m[i] = (m[i] << 4) + (ch - L'0');
            } else if (ch >= L'a' && ch <= L'f') {
                m[i] = (m[i] << 4) + (ch - L'a' + 10);
            } else if (ch >= 'A' && ch <= 'F') {
                m[i] = (m[i] << 4) + (ch - 'A' + 10);
            } else {
                break;
            }
            j++;
        }
        pos++;
    }   

    // Convert type
    mac[0] = (m[1] << 8)|m[0];
    mac[1] = (m[3] << 8)|m[2];
    mac[2] = (m[5] << 8)|m[4];

    return (*pos == L'\0');
}

//------------------------------------------------------------------------------
