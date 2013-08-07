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
#include <oal_log.h>
#include <oal_memory.h>

extern OAL_ADDRESS_TABLE g_oalAddressTable[];
 

//------------------------------------------------------------------------------
//
//  Function:  OALPAtoVA
//
//  Converts a physical address (PA) to a virtual address (VA).
//
VOID* OALPAtoVA(UINT32 pa, BOOL cached)
{
    OAL_ADDRESS_TABLE *pTable = g_oalAddressTable;
    VOID *va = NULL;

    OALMSG(OAL_MEMORY&&OAL_FUNC, (L"+OALPAtoVA(0x%x, %d)\r\n", pa, cached));

    // Search the table for address range
    while (pTable->size != 0) {
        if (
            pa >= pTable->PA && 
            pa <= (pTable->PA + (pTable->size << 20) - 1)
        ) break;                      // match found 
        pTable++;
    }

    // If address table entry is valid compute the VA
    if (pTable->size != 0) {
        va = (VOID *)(pTable->CA + (pa - pTable->PA));
        // If VA is uncached, set the uncached bit
        if (!cached) (UINT32)va |= OAL_MEMORY_CACHE_BIT;
    }

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
    OAL_ADDRESS_TABLE *pTable = g_oalAddressTable;
    UINT32 va = (UINT32)pVA;
    UINT32 pa = 0;

    OALMSG(OAL_MEMORY&&OAL_FUNC, (L"+OALVAtoPA(0x%08x)\r\n", pVA));

    // Virtual address must be in CACHED or UNCACHED regions.
    if (va < 0x80000000 || va >= 0xC0000000) {
        OALMSG(OAL_ERROR, (
            L"ERROR:OALVAtoPA: invalid virtual address 0x%08x\r\n", pVA
        ));
        goto cleanUp;
    }

    // Address must be cached, as entries in OEMAddressTable are cached address.
    va = va&~OAL_MEMORY_CACHE_BIT;

    // Search the table for address range
    while (pTable->size != 0) {
        if (va >= pTable->CA && va <= pTable->CA + (pTable->size << 20) - 1) {
            break;
        }
        pTable++;
    }

    // If address table entry is valid compute the PA
    if (pTable->size != 0) pa = pTable->PA + va - pTable->CA;

cleanUp:
    // Indicate physical address 
    OALMSG(OAL_MEMORY&&OAL_FUNC, (L"-OALVAtoPA(pa = 0x%x)\r\n", pa));
    return pa;
}

//------------------------------------------------------------------------------
