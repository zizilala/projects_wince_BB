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
#ifndef __OAL_LOG_H
#define __OAL_LOG_H

#if __cplusplus
extern "C" {
#endif

//------------------------------------------------------------------------------
//
//  Macro:  OALMSG/OALMSGS
//
//  This macro should be used for logging in OAL. It supports log zones. All
//  tracing messages based on this macro is removed when SHIP_BUILD is
//  defined. OALMSGS uses always serial output.
//

#ifdef SHIP_BUILD
#define OALMSG(cond, exp)   ((void)FALSE)
#define OALMSGS(cond, exp)  ((void)FALSE)
#define OALLED(data, mask)  ((void)FALSE)
#else
#define OALMSG(cond, exp)   ((void)((cond)?(OALLog exp), TRUE : FALSE))
#define OALMSGS(cond, exp)  ((void)((cond)?(OALLogSerial exp), TRUE : FALSE))
#define OALLED(index, data) (OEMWriteDebugLED(index, data))
#endif

//------------------------------------------------------------------------------
//
//  Define:  OAL_LOG_XXX
//
//  OAL trace zone bit numbers
//

#define OAL_LOG_ERROR       (0)
#define OAL_LOG_WARN        (1)
#define OAL_LOG_FUNC        (2)
#define OAL_LOG_INFO        (3)
#define OAL_LOG_STUB        (4)
#define OAL_LOG_KEYVAL      (4)
#define OAL_LOG_ARGS        (4)
#define OAL_LOG_CACHE       (5)
#define OAL_LOG_RTC         (6)
#define OAL_LOG_POWER       (7)
#define OAL_LOG_PCI         (8)
#define OAL_LOG_MEMORY      (9)
#define OAL_LOG_IO          (10)
#define OAL_LOG_TIMER       (11)
#define OAL_LOG_IOCTL       (12)
#define OAL_LOG_FLASH       (13)
#define OAL_LOG_INTR        (14)
#define OAL_LOG_VERBOSE     (15)

//------------------------------------------------------------------------------
//
//  Define:  OALZONE
//
//  Tests if a zone is being traced. Used in conditional in OALMSG macro.
//
extern DBGPARAM dpCurSettings;
#define OALZONE(n)          (dpCurSettings.ulZoneMask&(1<<n))

//------------------------------------------------------------------------------
//
//  Define:  OAL_XXX
//
//  OAL trace zones for OALMSG function. The following trace zones may
//  be combined, via boolean operators, to support OAL debug tracing.
//
#define OAL_ERROR           OALZONE(OAL_LOG_ERROR)
#define OAL_WARN            OALZONE(OAL_LOG_WARN)
#define OAL_FUNC            OALZONE(OAL_LOG_FUNC)
#define OAL_INFO            OALZONE(OAL_LOG_INFO)

#define OAL_VERBOSE         OALZONE(OAL_LOG_VERBOSE)
#define OAL_STUB            OALZONE(OAL_LOG_STUB)
#define OAL_KEYVAL          OALZONE(OAL_LOG_KEYVAL)

#define OAL_IO              OALZONE(OAL_LOG_IO)
#define OAL_CACHE           OALZONE(OAL_LOG_CACHE)
#define OAL_RTC             OALZONE(OAL_LOG_RTC)
#define OAL_POWER           OALZONE(OAL_LOG_POWER)
#define OAL_PCI             OALZONE(OAL_LOG_PCI)
#define OAL_ARGS            OALZONE(OAL_LOG_ARGS)
#define OAL_MEMORY          OALZONE(OAL_LOG_MEMORY)
#define OAL_IOCTL           OALZONE(OAL_LOG_IOCTL)
#define OAL_TIMER           OALZONE(OAL_LOG_TIMER)
#define OAL_FLASH           OALZONE(OAL_LOG_FLASH)
#define OAL_INTR            OALZONE(OAL_LOG_INTR)

//
// KITL is in different DLL from OAL. (ZONE_KITL_OAL defined in KitlProt.h)
//
#define OAL_KITL            (dpCurSettings.ulZoneMask&ZONE_KITL_OAL)
#define OAL_ETHER           (dpCurSettings.ulZoneMask&ZONE_KITL_ETHER)

//------------------------------------------------------------------------------
//
//  Define: OALMASK
//
//  Utility macro used to setup OAL trace zones.
//
#define OALMASK(n)          (1 << n)

//------------------------------------------------------------------------------
//
//  Function:  OALLog
//
//  This function formats string and write it to debug output. We are using
//  kernel implementation. For boot loader this function is implemented in
//  support library.
//
#define OALLog          NKDbgPrintfW
#define OALLogV         NKvDbgPrintfW

//------------------------------------------------------------------------------
//
//  Extern:  initialOALLogZones
//
//  Used by OEMInit to initialize the log zone mask.  This is a fix-up
//  variable that you can optionally override in config.bib
//
extern DWORD initialOALLogZones;

//------------------------------------------------------------------------------
//
//  Function:  OALLogSetZones
//
//  Updates the current log zone mask to the input value.
//
void OALLogSetZones(UINT32 newMask);

//------------------------------------------------------------------------------
//
//  Function:  OALLogSerial
//
//  This function format string and write it to serial debug output. Function
//  implementation uses OALLogPrintf function to format string in buffer on
//  stack. Buffer size is limited and under some circumstances it can cause
//  stack overflow.
//
VOID OALLogSerial(LPCWSTR format, ...);

//------------------------------------------------------------------------------
//
//  Function:  OALLogPrintf
//
//  This function formats string to buffer. It uses standard format string
//  same as wsprintf function (which is identical to printf format without
//  float point support). We are using kernel implementation. For boot loader
//  this function is implemented in support library.
//
VOID OALLogPrintf(
    LPWSTR szBuffer, UINT32 maxChars, LPCWSTR szFormat, ...
);

#if __cplusplus
}
#endif

#endif
