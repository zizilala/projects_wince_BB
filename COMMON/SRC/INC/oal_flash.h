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
//  Header:  oal_flash.h
//
#ifndef __OAL_FLASH_H
#define __OAL_FLASH_H

#if __cplusplus
extern "C" {
#endif

//------------------------------------------------------------------------------

typedef struct {
    UINT32 width;
    UINT32 parallel;
    UINT32 set;
    UINT32 size;
    UINT32 burst;
    UINT32 regions;
    UINT32 aBlockSize[8];
    UINT32 aBlocks[8];
} OAL_FLASH_INFO;

typedef struct {
    UINT32 writeDelay;
    UINT32 writeTimeout;
    UINT32 bufferDelay;
    UINT32 bufferTimeout;
    UINT32 eraseDelay;
    UINT32 eraseTimeout;
    UINT32 chipDelay;
    UINT32 chipTimeout;
} OAL_FLASH_TIMING;

//------------------------------------------------------------------------------
//
//  Enum:  OAL_FLASH_ERASE
//
typedef enum {
    OAL_FLASH_ERASE_PENDING = 0,
    OAL_FLASH_ERASE_DONE = 1,
    OAL_FLASH_ERASE_FAILED = -1
} OAL_FLASH_ERASE;

//------------------------------------------------------------------------------

BOOL OALFlashInfo(VOID *pBase, OAL_FLASH_INFO *pInfo);
BOOL OALFlashTiming(VOID *pBase, OAL_FLASH_TIMING *pInfo);
BOOL OALFlashLock(VOID *pBase, VOID *pStart, DWORD size, BOOL lock);
BOOL OALFlashLockDown(VOID *pBase, VOID *pStart, DWORD size);
BOOL OALFlashWrite(VOID *pBase, VOID *pStart, UINT32 size, VOID *pBuffer);
BOOL OALFlashErase(VOID *pBase, VOID *pStart, UINT32 size);
BOOL OALFlashEraseStart(VOID *pBase, VOID *pStart, UINT32 size);
UINT32 OALFlashEraseContinue();

VOID OALFlashDump(VOID *pBase);

//------------------------------------------------------------------------------

#if __cplusplus
}
#endif

#endif
