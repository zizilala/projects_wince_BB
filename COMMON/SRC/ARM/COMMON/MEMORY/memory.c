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
//  File:  memory.c
//
//  Memory interface routines.
//
#include <windows.h>
#include <nkexport.h>
#include <ceddk.h>
#include <oal_log.h>
#include <oal_memory.h>
 

//------------------------------------------------------------------------------
//
//  Function:  OALPAtoVA
//
//  Converts a physical address (PA) to a virtual address (VA).
//
VOID* OALPAtoVA(UINT32 pa, BOOL cached)
{
    UINT32 offset;
    UINT8 *va = NULL;

    OALMSG(OAL_MEMORY&&OAL_FUNC, (L"+OALPAtoVA(0x%x, %d)\r\n", pa, cached));

    offset = pa & (PAGE_SIZE - 1);
    pa &= ~(PAGE_SIZE - 1);
    va = NKPhysToVirt( pa >> 8, cached );
    va += offset;

    // Indicate the virtual address
    OALMSG(OAL_MEMORY&&OAL_FUNC, (L"-OALPAtoVA(va = 0x%08x)\r\n", va));
    return va;
}


//------------------------------------------------------------------------------
//
//  Function:  OALVAtoPA
//
//  Converts a virtual address (VA) to a physical address (PA).
//
UINT32 OALVAtoPA(VOID *pVA)
{
    UINT32 va = (UINT32)pVA;
    UINT32 offset;
    UINT32 pa;

    OALMSG(OAL_MEMORY&&OAL_FUNC, (L"+OALVAtoPA(0x%08x)\r\n", pVA));

    offset = va & (PAGE_SIZE - 1);
    va &= ~(PAGE_SIZE - 1);
    
    pa = (UINT32)NKVirtToPhys( (LPCVOID)va );

    if (INVALID_PHYSICAL_ADDRESS == pa) {
        pa = 0;     // BUGBUG: 0 can be a valid physical address...,
                    // BUGBUG: We should remove this line and change all calls to OALVAtoPA
                    // BUGBUG: to check INVALID_PHYSICAL_ADDRESS instead of 0.
    } else {
        pa = (pa << 8) + offset;
    }

    // Indicate physical address 
    OALMSG(OAL_MEMORY&&OAL_FUNC, (L"-OALVAtoPA(pa = 0x%x)\r\n", pa));
    return pa;
}

//------------------------------------------------------------------------------
