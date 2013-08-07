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
//  Header: oal_intr.h
//
//  This header define OAL interrupt module. Module code implements all
//  functionality related to interrupt management.
//
//  Depending on CPU/SoC model the module can be implemented with platform
//  callbacks. This model is usefull on hardware where external secondary
//  interrupt controller is used.
//
//  Export for kernel/public interface:
//      * Interrupt handlers/SYS_INTR_XXXX
//      * OEMInterruptEnable
//      * OEMInterruptDisable
//      * OEMInterruptDone
//      * OEMInterruptMask
//      * OEMIoControl/IOCTL_HAL_REQUEST_SYSINTR
//      * OEMIoControl/IOCTL_HAL_RELEASE_SYSINTR
//      * OEMIoControl/IOCTL_HAL_REQUEST_IRQ
//
//  Export for other OAL modules/protected interface:
//      * OALIntrInit (initialization)
//      * OALIntrStaticTranslate (initialization)
//      * OALIntrRequestSysIntr (KITL)
//      * Interrupt handler (timer)
//      * OAL_INTR_IRQ_MAXIMUM (power)
//      * OALPAtoVA (memory)
//
//  Internal module functions:
//      * OALIntrMapInit
//      * OALIntrReleaseSysIntr
//      * OALIntrTranslateSysIntr
//      * OALIntrTranslateIrq
//  
//  Platform callback module functions:
//      * BSPIntrInit
//      * BSPIntrRequestIrqs
//      * BSPIntrEnableIrq
//      * BSPIntrDisableIrq
//      * BSPIntrDoneIrq
//      * BSPActiveIrq
//
//
#ifndef __OAL_INTR_H
#define __OAL_INTR_H

#if __cplusplus
extern "C" {
#endif

//------------------------------------------------------------------------------
//
//  Define:  IRQ_MAXIMUM
//
//  This value define maximum number of IRQs. Even if there isn't any
//  limitation for this number in OAL library, Windows CE resource manager
//  support only 64 IRQs currently.
//
#define OAL_INTR_IRQ_MAXIMUM        64

//------------------------------------------------------------------------------
//
//  Define:  IRQ_UNDEFINED
//
//  Invalid IRQ number used to verify SYSINTR/IRQ translations
//
#define OAL_INTR_IRQ_UNDEFINED      (-1)

//------------------------------------------------------------------------------
//
//  Define:  OAL_INTR_STATIC/OAL_INTR_FORCE_STATIC
//
//  This constants are used to specify type of IRQ to SYSINTR mapping sets in
//  OALIntrRequestSysIntr function. One of these flags must be specified when
//  calling OALIoCtlHalRequestSysIntr() with the new calling convention (first
//  element in input buffer == -1).
//  Flag OAL_INTR_TRANSLATE allows to use obsolete IOCTL_HAL_TRANSLATE_IRQ semantic
//  (if static mapping exists return mapped SYSINTR otherwise create new mapping).
//
#define OAL_INTR_DYNAMIC            (1 << 0)
#define OAL_INTR_STATIC             (1 << 1)
#define OAL_INTR_FORCE_STATIC       (1 << 2)
#define OAL_INTR_TRANSLATE          (1 << 3)

//------------------------------------------------------------------------------
//
//  Function: OALIoCtlHalRequestSysIntr
//
//  This function is called form OEMIoControl for IOCTL_HAL_REQUEST_SYSINTR.
//
BOOL OALIoCtlHalRequestSysIntr(UINT32, VOID*, UINT32, VOID*, UINT32, UINT32*);

//------------------------------------------------------------------------------
//
//  Function: OALIoCtlHalReleaseSysIntr
//
//  This function is called form OEMIoControl for IOCTL_HAL_RELEASE_SYSINTR.
//
BOOL OALIoCtlHalReleaseSysIntr(UINT32, VOID*, UINT32, VOID*, UINT32, UINT32*);

//------------------------------------------------------------------------------
//
//  Function: OALIoCtlHalRequestIrq
//
//  This function is called form OEMIoControl for IOCTL_HAL_REQUEST_IRQ.
//
BOOL OALIoCtlHalRequestIrq(UINT32, VOID*, UINT32, VOID*, UINT32, UINT32*);

//------------------------------------------------------------------------------
//
//  Function:  OALIntrInit/BSPIntrInit
//
//  This function initialize interrupt hardware. It is usally called at system
//  initialization. If implementation uses platform callback it will call
//  BPSIntrInit.
//
BOOL OALIntrInit();
BOOL BSPIntrInit();

//------------------------------------------------------------------------------
//
//  Function:  OALIntrMapInit
//
//  This function must be called from OALIntrInit to initialize IRQ/SYSINTR
//  mapping structure.
//
VOID OALIntrMapInit();

//------------------------------------------------------------------------------
//
//  Function:  OALIntrRequestSysIntr
//
//  This function allocate new SYSINTR value for given IRQ list. Based
//  on flags parameter it also can set backward static mapping (IRQ -> SYSINTR)
//  see OAL_INTR_xxx above.
//
UINT32 OALIntrRequestSysIntr(UINT32 count, const UINT32 *pIrqs, UINT32 flags);

//------------------------------------------------------------------------------
//
//  Function:  OALIntrReleaseSysIntr
//
//  This function release SYSINTR. It also clears backward static mapping
//  (IRQ -> SYSINTR) if it exists.
//
BOOL OALIntrReleaseSysIntr(UINT32 sysIntr);

//------------------------------------------------------------------------------
//
//  Function:  OALIntrStaticTranslate
//
//  This function set static IRQ <-> SYSINTR mapping for given values. It is
//  typically used in system initialization for legacy drives.
//
VOID OALIntrStaticTranslate(UINT32 sysIntr, UINT32 irq);

//------------------------------------------------------------------------------
//
//  Function:  OALIntrTranslateSysIntr
//
//  This function return list of IRQs for give SYSINTR.
//
BOOL OALIntrTranslateSysIntr(
    UINT32 sysIntr, UINT32 *pCount, const UINT32 **ppIrqs
);

//------------------------------------------------------------------------------
//
//  Function:  OALIntrRequestIrq
//
//  This function return SYSINTR for given IRQ based on static mapping.
//
UINT32 OALIntrTranslateIrq(UINT32 irq);

//------------------------------------------------------------------------------
//
//  Function:  OALIntrRequestIrq/BSPIntrRequestIrq
//
//  This function return list of IRQs for device identified by DEVICE_LOCATION
//  parameter. If implementation uses platform callbacks it should call
//  BSPIntrRequestIrq  in case that it doesn't recognise device.
//  On input *pCount contains maximal number of IRQs allowed in list. On return
//  it returns number of IRQ in list.
//
BOOL OALIntrRequestIrqs(DEVICE_LOCATION *pDevLoc, UINT32 *pCount, UINT32 *pIrqs);
BOOL BSPIntrRequestIrqs(DEVICE_LOCATION *pDevLoc, UINT32 *pCount, UINT32 *pIrqs);

//------------------------------------------------------------------------------
//
//  Function:  OALIntrEnableIrqs/BSPIntrEnableIrqs
//
//  This function enable list of interrupts identified by IRQ. If
//  implementation uses platform callbacks OALIntrEnableIrqs must call
//  BSPIntrEnableIrq for IRQ before it enables this IRQ. The BSPIntrEnableIrq
//  can do board specific action and optionaly modify IRQ.
//
BOOL OALIntrEnableIrqs(UINT32 count, const UINT32 *pIrqs);
UINT32 BSPIntrEnableIrq(UINT32 irq);

//------------------------------------------------------------------------------
//
//  Function:  OALIntrDisableIrqs/BSPIntrDisableIrqs
//
//  This function disable list of interrupts identified by IRQ. If
//  implementation uses platform callbacks OALIntrDisableIrqs must call
//  BSPIntrDisableIrq for each IRQ in list before it disables this IRQ.
//  The BSPIntrEnableIrq can do board specific action and optionaly modify IRQ.
//
VOID OALIntrDisableIrqs(UINT32 count, const UINT32 *pIrqs);
UINT32 BSPIntrDisableIrq(UINT32 irq);

//------------------------------------------------------------------------------
//
//  Function:  OALIntrDoneIrqs/BSPIntrDoneIrqs
//
//  This function finish list of interrupts identified by IRQ. If
//  implementation uses platform callbacks OALIntrDoneIrqs must call
//  BSPIntrDoneIrq for each IRQ in list before it re-enable this IRQ.
//  The BSPIntrDoneIrq can do board specific action and optionaly modify IRQ.
//
VOID OALIntrDoneIrqs(UINT32 count, const UINT32 *pIrqs);
UINT32 BSPIntrDoneIrq(UINT32 irq);

//------------------------------------------------------------------------------
//
//  Function:  BSPIntrActiveIrq
//
//  This function is called from interrupt handler if implementation uses
//  platform callbacks. It allows translate IRQ for chaining controller
//  interrupt.
//
UINT32 BSPIntrActiveIrq(UINT32 irq);

//------------------------------------------------------------------------------

#if __cplusplus
}
#endif

#endif

