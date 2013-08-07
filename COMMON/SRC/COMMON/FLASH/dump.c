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
//  File:  dump.c
//
//  This file implements CFI flash memory info dump.
//
#include <windows.h>
#include "oal_log.h"
#include "oal_memory.h"
#include "oal_io.h"
#include "oal_flash.h"

//------------------------------------------------------------------------------

VOID OALFlashDump(VOID *pBase)
{
    OAL_FLASH_INFO info;
    UINT32 base, i;

    // Erase must work from uncached memory
    base = (UINT32)OALCAtoUA(pBase);

    OALLog(L"---\r\n");
    
    while (TRUE) {

        // Ther read first chip info
        if (!OALFlashInfo((VOID*)base, &info)) {
            OALLog(
                L"Flash memory not detected at 0x%08x\r\n", 
                OALVAtoPA((VOID*)base)
            );
            break;
        }

        OALLog(
            L"Flash memory at 0x%08x width %d bytes %d parallel chip(s)\r\n",
            OALVAtoPA((VOID*)base), info.width, info.parallel
        );

        OALLog(
            L"Size %d bytes/chip, bank %d/0x%08x bytes\r\n",
            info.size, info.size * info.parallel, info.size * info.parallel
        );

        OALLog(
            L"Set %d, burst size %d, %d regions\r\n",
            info.set, info.burst, info.regions
        );

        for (i = 0; i < info.regions; i++) {
            OALLog(
                L"Region %d: %4d blocks with size %4d bytes\r\n",
                i, info.aBlocks[i], info.aBlockSize[i]
            );
        }

        base += info.size * info.parallel;
        
    }

    OALLog(L"---\r\n");

}    

//------------------------------------------------------------------------------

