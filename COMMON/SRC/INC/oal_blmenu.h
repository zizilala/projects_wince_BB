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
#ifndef __OAL_BLMENU_H
#define __OAL_BLMENU_H

#if __cplusplus
extern "C" {
#endif

//------------------------------------------------------------------------------

typedef struct OAL_MENU_ITEM {
    WCHAR key;
    LPCWSTR text;
    VOID (*pfnAction)(struct OAL_MENU_ITEM *);
    VOID *pParam1;
    VOID *pParam2;
    VOID *pParam3;
} OAL_BLMENU_ITEM;

//------------------------------------------------------------------------------

VOID OALBLMenuActivate(UINT32 delay, OAL_BLMENU_ITEM *pMenu);
VOID OALBLMenuHeader(LPCWSTR format, ...);

VOID OALBLMenuShow(OAL_BLMENU_ITEM *pMenu);
VOID OALBLMenuEnable(OAL_BLMENU_ITEM *pMenu);
VOID OALBLMenuSetDeviceId(OAL_BLMENU_ITEM *pMenu);
VOID OALBLMenuSetMacAddress(OAL_BLMENU_ITEM *pMenu);
VOID OALBLMenuSetIpAddress(OAL_BLMENU_ITEM *pMenu);
VOID OALBLMenuSetIpMask(OAL_BLMENU_ITEM *pMenu);
VOID OALBLMenuSelectDevice(OAL_BLMENU_ITEM *pMenu);

UINT32 OALBLMenuReadLine(LPWSTR buffer, UINT32 charCount);
WCHAR  OALBLMenuReadKey(BOOL wait);

//------------------------------------------------------------------------------

#if __cplusplus
}
#endif

#endif
