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
//  File:  oal_ioctl.h
//
//  This header file defines IO Control OAL module. This module implements
//  OEMIoControl function which is used to call kernel functions from user
//  space.
//
#ifndef __OAL_IOCTL_H
#define __OAL_IOCTL_H

#if __cplusplus
extern "C" {
#endif

//------------------------------------------------------------------------------
//
//  Definition: OAL_IOCTL_FLAG_xxx
//
//  This definition specifies flag codes for IOCTL table. When NOCS flag is
//  set handler function will be called in deserialized mode (so no critical
//  section will be taken/release before/after handler is called).
//
#define OAL_IOCTL_FLAG_NOCS     (1 << 0)

//------------------------------------------------------------------------------
//
//  Type: IOCTL_HANDLER    
//
//  This type defines the procedure to be called for an IOCTL code. The
//  global g_oalIoctlTable is an array of these types.
//
typedef struct {
    UINT32  code;
    UINT32  flags;
    BOOL    (*pfnHandler)(UINT32, VOID*, UINT32, VOID*, UINT32, UINT32*);
} OAL_IOCTL_HANDLER, *POAL_IOCTL_HANDLER;

//------------------------------------------------------------------------------
//
//  Extern: g_oalIoCtlPlatformType/OEM
//
//  Platform Type/OEM
//
extern LPCWSTR g_oalIoCtlPlatformType;
extern LPCWSTR g_oalIoCtlPlatformOEM;

//------------------------------------------------------------------------------
//
//  Global: g_oalIoCtlProcessorVendor/Name/Core
//
//  Processor information
//
extern LPCWSTR g_oalIoCtlProcessorVendor;
extern LPCWSTR g_oalIoCtlProcessorName;
extern LPCWSTR g_oalIoCtlProcessorCore;

//------------------------------------------------------------------------------
//
//  Global:  g_oalIoCtlInstructionSet/g_oalIoCtlClockSpeed
//
//  Processor instruction set identifier and clock speed
//

extern UINT32 g_oalIoCtlInstructionSet;
extern UINT32 g_oalIoCtlClockSpeed;

//------------------------------------------------------------------------------
//
//  Globaal:  g_oalIoctlTable
//
//  This extern references the global IOCTL table that is defined in
//  the platform code.
//
extern const OAL_IOCTL_HANDLER g_oalIoCtlTable[];

//------------------------------------------------------------------------------
//
//  Function: OALIoCtlXxx
//
//  This functions implement basic IOCTL code handlers.
//
BOOL OALIoCtlHalGetDeviceId(UINT32, VOID*, UINT32, VOID*, UINT32, UINT32*);
BOOL OALIoCtlHalGetHWEntropy(UINT32, VOID*, UINT32, VOID*, UINT32, UINT32*);
BOOL OALIoCtlHalGetRandomSeed(UINT32, VOID *, UINT32, VOID *, UINT32, UINT32 *);
BOOL OALIoCtlHalGetRegSecureKeys(UINT32, VOID *, UINT32, VOID *, UINT32, UINT32 *);
BOOL OALIoCtlHalGetDeviceInfo(UINT32, VOID*, UINT32, VOID*, UINT32, UINT32*);
BOOL OALIoCtlProcessorInfo(UINT32, VOID*, UINT32, VOID*, UINT32, UINT32*);
BOOL OALIoCtlHalInitRegistry(UINT32, VOID*, UINT32, VOID*, UINT32, UINT32*);
BOOL OALIoCtlHalReboot(UINT32, VOID*, UINT32, VOID*, UINT32, UINT32*);
BOOL OALIoCtlHalGetUUID (UINT32, VOID *, UINT32, VOID *, UINT32, UINT32 *);
BOOL OALIoCtlHalUpdateMode(UINT32, VOID *, UINT32, VOID *, UINT32, UINT32 *);
//------------------------------------------------------------------------------
//
//  Function: OALIoCtlHalDDIXxx
//
//  This functions implement IOCTL code handler used by HAL flat display
//  driver.
//
BOOL OALIoCtlHalDDI(UINT32, VOID*, UINT32, VOID*, UINT32, UINT32*);

//------------------------------------------------------------------------------


#if __cplusplus
}
#endif

#endif // __OAL_IOCTL_H
