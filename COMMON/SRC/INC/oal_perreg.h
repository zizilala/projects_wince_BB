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
//  Header: oal_perreg.h
//
//  This header define functions and structures used by optional persistent
//  registry implementation.
//
#ifndef __OAL_PERREG_H
#define __OAL_PERREG_H

#if __cplusplus
extern "C" {
#endif

//------------------------------------------------------------------------------

typedef struct {
    UINT32 base;
    UINT32 start;
    UINT32 size;
} OAL_PERREG_REGION;

//------------------------------------------------------------------------------

BOOL OALPerRegInit(BOOL clean, OAL_PERREG_REGION *pArea);
BOOL OALPerRegWrite(UINT32 flags, UINT8 *pData, UINT32 dataSize);
UINT32 OALPerRegRead(UINT32 flags, UINT8 *pData, UINT32 dataSize);

//------------------------------------------------------------------------------

#if __cplusplus
}
#endif

#endif
