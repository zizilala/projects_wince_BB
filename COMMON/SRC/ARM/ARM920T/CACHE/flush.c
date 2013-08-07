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
//  File:  flush.c
//
//  This file implements OEMCacheRangeFlush function suitable for ARM CPU
//  with separate data and instruction caches.
//
#include <windows.h>
#include <ceddk.h>
#include <oal_log.h>
#include <oal_cache.h>

//------------------------------------------------------------------------------
//
//  Function:  OEMCacheRangeFlush
//
//  Implements entry point for all cache and TLB flush operations. It takes
//  following parameters:
//    
//      pAddress - starting VA on which the cache operation is to be performed
//      length - length of flush
//      flags  - specifies the cache operation to be performed:
//      
//          CACHE_SYNC_WRITEBACK: write back DATA cache
//          CACHE_SYNC_DISCARD: write back and discard DATA cache
//          CACHE_SYNC_INSTRUCTIONS: discard I-Cache
//          CACHE_SYNC_FLUSH_I_TLB: flush instruction TLB
//          CACHE_SYNC_FLUSH_D_TLB: flush data TLB
//          CACHE_SYNC_FLUSH_TLB: flush both I/D TLB
//          CACHE_SYNC_L2_WRITEBACK: write back L2 cache
//          CACHE_SYNC_L2_DISCARD: write back and discard L2 cache
//          CACHE_SYNC_ALL: perform all the above operations.
//            
//  If both pAddress and length are 0, the entire cache (or TLB, as directed by
//  flags) will be flushed. Only the kernel can set the TLB flush flags when
//  it calls this routine, and when a TLB flush is performed with 
//  length == PAGE_SIZE, pAddress is guaranteed to be on a page boundary.
//                

void OEMCacheRangeFlush(VOID *pAddress, DWORD length, DWORD flags)
{
    OALMSG(OAL_CACHE&&OAL_VERBOSE, (
        L"+OEMCacheRangeFlush(0x%08x, %d, 0x%08x)\r\n", pAddress, length, flags
    ));

    if ((flags & CACHE_SYNC_DISCARD) != 0) {
        // Write back and invalidate the selected portions of the data cache
        if (length == 0) {
            if (pAddress == NULL) OALFlushDCache();
        } else {
            // Normalize address to cache line alignment
            UINT32 mask = g_oalCacheInfo.L1DLineSize - 1;
            UINT32 address = (UINT32)pAddress & ~mask;
            // Adjust size to reflect cache line alignment
            length += (UINT32)pAddress - address;
            // If range is bigger than cache size flush all
            if (length >= g_oalCacheInfo.L1DSize) {
                OALFlushDCache();
            } else {                
                // Flush all the indicated cache entries
                OALFlushDCacheLines((VOID*)address, length);
            }                
        }
    } else if ((flags & CACHE_SYNC_WRITEBACK) != 0) {
        // Write back the selected portion of the data cache
        if (length == 0) {
            if (pAddress == NULL) OALCleanDCache();
        } else {
            // Normalize address to cache line alignment
            UINT32 mask = g_oalCacheInfo.L1DLineSize - 1;
            UINT32 address = (UINT32)pAddress & ~mask;
            // Adjust size to reflect cache line alignment
            length += (UINT32)pAddress - address;
            // If range is bigger than cache size clean all
            if (length >= g_oalCacheInfo.L1DSize) {
                OALCleanDCache();
            } else {                
                // Flush all the indicated cache entries
                OALCleanDCacheLines((VOID*)address, length);
            }                
        }
    }
    
    if ((flags & CACHE_SYNC_INSTRUCTIONS) != 0) {
        if (length == 0) {
            if (pAddress == NULL) OALFlushICache();
        } else {
            // Normalize address to cache line alignment
            UINT32 mask = g_oalCacheInfo.L1ILineSize - 1;
            UINT32 address = (UINT32)pAddress & ~mask;
            length += (UINT32)pAddress - address;
            if (length >= g_oalCacheInfo.L1ISize) {
                OALFlushICache();
            } else {        
                OALFlushICacheLines((VOID*)address, length);
            }                
        }
    }

    if ((flags & CACHE_SYNC_FLUSH_I_TLB) != 0) {
        if (length == PAGE_SIZE) {
            // flush one TLB entry
            OALClearITLBEntry(pAddress);
        } else {
            // flush the whole TLB
            OALClearITLB();
        }
    }

    if ((flags & CACHE_SYNC_FLUSH_D_TLB) != 0) {
        // check first for TLB updates forced by paging
        if (length == PAGE_SIZE) {
            // flush one TLB entry
            OALClearDTLBEntry(pAddress);
        } else {
            // flush the whole TLB
            OALClearDTLB();
        }
    }

    OALMSG(OAL_CACHE&&OAL_VERBOSE, (L"-OEMCacheRangeFlush\r\n"));
}

//------------------------------------------------------------------------------

