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
//  File:  name.c
//
#include <windows.h>
#include <oal.h>

//------------------------------------------------------------------------------
//
//  Function:  OALKitlCreateName
//
//  This function create device name from prefix and mac address (usually last
//  two bytes of MAC address used for download).
//
VOID OALKitlCreateName(CHAR *pPrefix, UINT16 mac[3], CHAR *pBuffer)
{
    UINT32 count;
    UINT16 base, code;

    KITL_RETAILMSG(ZONE_KITL_OAL, (
        "+OALKitlCreateName('%hs', 0x%04x, 0x%08x)\r\n", pPrefix, mac, pBuffer
    ));
    
    count = 0;
    code = (mac[2] >> 8) | ((mac[2] & 0x00ff) << 8);
    while (count < OAL_KITL_ID_SIZE - 6 && pPrefix[count] != '\0') {
        pBuffer[count] = pPrefix[count];
        count++;
    }        
    base = 10000;
    while (base > code && base != 0) base /= 10;
    if (base == 0) { 
        pBuffer[count++] = '0';
    } else {
        while (base > 0) {
            pBuffer[count++] = code/base + '0';
            code %= base;
            base /= 10;
        }
    }        
    pBuffer[count] = '\0';

    KITL_RETAILMSG(ZONE_KITL_OAL, (
        "-OALKitlCreateName(pBuffer = '%hs')\r\n", pBuffer
    ));
}

//------------------------------------------------------------------------------
