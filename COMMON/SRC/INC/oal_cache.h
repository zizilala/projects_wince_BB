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
//  Header:  oal_cache.h
//
//  This header file defines OAL cache module interface. The module implements
//  functions and varibles related to cache and TLB operations.
//
//  Export for kernel/public interface:
//      * OEMCacheRangeFlush
//      * OEMIoControl/IOCTL_HAL_GET_CACHE_INFO
//
//  Export for other OAL modules/protected interface:
//      * OALCacheGlobalsInit
//
//  Intenal variables/functions
//      * g_oalCacheInfo
//      * OALClearTLB/OALClearTLBEntry
//      * OALClearITLB/OALClearITLBEntry
//      * OALClearDTLB/OALClearDTLBEntry
//
#ifndef __OAL_CACHE_H
#define __OAL_CACHE_H

#ifndef __MIPS_ASSEMBLER

#if __cplusplus
extern "C" {
#endif

//------------------------------------------------------------------------------
//
//  Structure:  OAL_CACHE_INFO
//

typedef struct {

    UINT32 L1Flags;
    UINT32 L1ISetsPerWay;
    UINT32 L1INumWays;
    UINT32 L1ILineSize;
    UINT32 L1ISize;
    UINT32 L1DSetsPerWay;
    UINT32 L1DNumWays;
    UINT32 L1DLineSize;
    UINT32 L1DSize;

    UINT32 L2Flags;
    UINT32 L2ISetsPerWay;
    UINT32 L2INumWays;
    UINT32 L2ILineSize;
    UINT32 L2ISize;
    UINT32 L2DSetsPerWay;
    UINT32 L2DNumWays;
    UINT32 L2DLineSize;
    UINT32 L2DSize;
    
} OAL_CACHE_INFO;

//------------------------------------------------------------------------------
//
//  Global:  g_oalCacheInfo
//
extern OAL_CACHE_INFO g_oalCacheInfo;

//------------------------------------------------------------------------------
//
//  Function:  OALIoCtlHalGetCacheInfo
//
//  This function implements OEMIoControl/IOCTL_HAL_GET_CACHE_INFO. It fill
//  CacheInfo structure with information about CPU/SoC cache.
//
BOOL OALIoCtlHalGetCacheInfo(UINT32, VOID*, UINT32, VOID*, UINT32, UINT32*);

//------------------------------------------------------------------------------
//
//  Function:  OALCacheGlobalsInit
//
//  This function initializes globals variables which holds cache parameters.
//  It must be called before any other cache/TLB function.
//
VOID OALCacheGlobalsInit();

//------------------------------------------------------------------------------
//
//  Function:  OALFlushDCache/OALFlushDCacheLines
//
//  This functions flush data cache (= write back to main memory modified
//  cache entries for write back cache and invalidate cache location).
//
VOID OALFlushDCache();
VOID OALFlushDCacheLines(VOID* pAddress, UINT32 size);

//------------------------------------------------------------------------------
//
//  Function:  OALCleanDCache/OALCleanDCacheLines
//
//  This functions clean data cache (= write back to main memory modified
//  cache entries for write back cache). It makes data in cache and main memory
//  consistent.
//
VOID OALCleanDCache();
VOID OALCleanDCacheLines(VOID* pAddress, UINT32 size);

//------------------------------------------------------------------------------
//
//  Function:  OALFlushICache/OALFlushICacheLines
//
//  This functions flush instruction cache (= invalidate cache location). 
//
VOID OALFlushICache();
VOID OALFlushICacheLines(VOID* pAddress, UINT32 size);

//------------------------------------------------------------------------------
//
//  Function: OALClearTLB/OALClearTLBEntry
//
//  This functions clear full TLB or TLB entry. If system support separate
//  data and instruction TLBs both must be clear.
//
VOID OALClearTLB();
VOID OALClearTLBEntry(VOID*);

//------------------------------------------------------------------------------
//
//  Function: OALClearITLB/OALClearITLBEntry
//
//  This functions clear full instruction TLB or instruction TLB entry.
//
VOID OALClearITLB();
VOID OALClearITLBEntry(VOID*);

//------------------------------------------------------------------------------
//
//  Function: OALClearDTLB/OALClearDTLBEntry
//
//  This functions clear full data TLB or data TLB entry.
//
VOID OALClearDTLB();
VOID OALClearDTLBEntry(VOID*);

//------------------------------------------------------------------------------

#if __cplusplus
}
#endif

#else // __MIPS_ASSEMBLER

//------------------------------------------------------------------------------
//
//  Defines: L1xxx/L2xxx
//
//  Following defines are used in MIPS assember to access OAL_CACHE_INFO
//  structure members. It must be synchronized with OAL_CACHE_INFO definition.
//
#define L1Flags             0
#define L1ISetsPerWay       4
#define L1INumWays          8
#define L1ILineSize         12
#define L1ISize             16
#define L1DSetsPerWay       20
#define L1DNumWays          24
#define L1DLineSize         28
#define L1DSize             32

#define L2Flags             36
#define L2ISetsPerWay       40
#define L2INumWays          44
#define L2ILineSize         48
#define L2ISize             52
#define L2DSetsPerWay       56
#define L2DNumWays          60
#define L2DLineSize         64
#define L2DSize             68

//------------------------------------------------------------------------------

#endif // __MIPS_ASSEMBLER

#endif // __OAL_CACHE_H

