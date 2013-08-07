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
//  File:  oal_io.h
//
//  This header file defines OAL input-output module.
//
//  Depending on platform hardware I/O operations can be implemented as macros
//  or as functions. By default code uses functions. If platform doesn't allow
//  direct inline I/O operation the OAL_DDK_NOMACRO enviroment variable must
//  be set before OAL libraries are compiled.
//
//  Export for kernel/public interface:
//      * OEMIoControl/IOCTL_HAL_DDK_CALL
//
//  Export for other OAL modules/protected interface:
//      * INPORTx/OUTPORTx/SETPORTx/CLRPORTx
//      * INREGx/OUTREGx/SETREGx/CLREGx
//      * OALIoReadBusData
//      * OALIoWriteBusData
//      * OALIoTransBusAddress
//      * OALIoTransSystemAddress
//      * OALIoBusPowerOff
//      * OALIoBusPowerOn
//
#ifndef __OAL_IO_H
#define __OAL_IO_H

//------------------------------------------------------------------------------

#include <ceddk.h>
#include <pkfuncs.h>

//------------------------------------------------------------------------------

#if __cplusplus
extern "C" {
#endif

//------------------------------------------------------------------------------
//
//  Macros:  INPORTx/OUTPORTx/SETPORTx/CLRPORTx
//
//  This macros encapsulates basic I/O operations. Depending on OAL_DDK_NOMACRO
//  definition they will expand to direct memory operation or function call.
//  On x86 platform IO address space operation is generated. On other platforms
//  operation is identical with INREGx/OUTREGx/SETREGx/CLRREGx.
//
#define INPORT8(x)          READ_PORT_UCHAR(x)
#define OUTPORT8(x, y)      WRITE_PORT_UCHAR(x, (UCHAR)(y))
#define SETPORT8(x, y)      OUTPORT8(x, INPORT8(x)|(y))
#define CLRPORT8(x, y)      OUTPORT8(x, INPORT8(x)&~(y))
#define MASKPORT8(x, y, z)  OUTPORT8(x, (INPORT8(x)&~(y))|(z))

#define INPORT16(x)         READ_PORT_USHORT(x)
#define OUTPORT16(x, y)     WRITE_PORT_USHORT(x,(USHORT)(y))
#define SETPORT16(x, y)     OUTPORT16(x, INPORT16(x)|(y))
#define CLRPORT16(x, y)     OUTPORT16(x, INPORT16(x)&~(y))
#define MASKPORT16(x, y, z) OUTPORT16(x, (INPORT16(x)&~(y))|(z))

#define INPORT32(x)         READ_PORT_ULONG(x)
#define OUTPORT32(x, y)     WRITE_PORT_ULONG(x, (ULONG)(y))
#define SETPORT32(x, y)     OUTPORT32(x, INPORT32(x)|(y))
#define CLRPORT32(x, y)     OUTPORT32(x, INPORT32(x)&~(y))
#define MASKPORT32(x, y, z) OUTPORT32(x, (INPORT32(x)&~(y))|(z))

//------------------------------------------------------------------------------
//
//  Macros:  INREGx/OUTREGx/SETREGx/CLRREGx
//
//  This macros encapsulates basic I/O operations. Depending on OAL_DDK_NOMACRO
//  definition they will expand to direct memory operation or function call.
//  Memory address space operation is used on all platforms.
//
#define INREG8(x)           READ_REGISTER_UCHAR(x)
#define OUTREG8(x, y)       WRITE_REGISTER_UCHAR(x, (UCHAR)(y))
#define SETREG8(x, y)       OUTREG8(x, INREG8(x)|(y))
#define CLRREG8(x, y)       OUTREG8(x, INREG8(x)&~(y))
#define MASKREG8(x, y, z)   OUTREG8(x, (INREG8(x)&~(y))|(z))

#define INREG16(x)          READ_REGISTER_USHORT(x)
#define OUTREG16(x, y)      WRITE_REGISTER_USHORT(x,(USHORT)(y))
#define SETREG16(x, y)      OUTREG16(x, INREG16(x)|(y))
#define CLRREG16(x, y)      OUTREG16(x, INREG16(x)&~(y))
#define MASKREG16(x, y, z)  OUTREG16(x, (INREG16(x)&~(y))|(z))

#define INREG32(x)          READ_REGISTER_ULONG(x)
#define OUTREG32(x, y)      WRITE_REGISTER_ULONG(x, (ULONG)(y))
#define SETREG32(x, y)      OUTREG32(x, INREG32(x)|(y))
#define CLRREG32(x, y)      OUTREG32(x, INREG32(x)&~(y))
#define MASKREG32(x, y, z)  OUTREG32(x, (INREG32(x)&~(y))|(z))

//------------------------------------------------------------------------------

#define IOCTL_OAL_READBUSDATA           0x03
#define IOCTL_OAL_WRITEBUSDATA          0x04
#define IOCTL_OAL_TRANSBUSADDRESS       0x05
#define IOCTL_OAL_TRANSSYSADDRESS       0x06
#define IOCTL_OAL_BUSPOWEROFF           0x07
#define IOCTL_OAL_BUSPOWERON            0x08

typedef struct {
    UINT32 function;
    UINT32 rc;
    union {
        struct {
            DEVICE_LOCATION devLoc;
            UINT32 offset;
            UINT32 length;
            VOID *pBuffer;
        } busData;
        struct {
            INTERFACE_TYPE ifcType;
            UINT32 busNumber;
            UINT32 space;
            UINT64 address;
        } transAddress;
        struct {
            INTERFACE_TYPE ifcType;
            UINT32 busNumber;
        } busPower;
    };
} OAL_DDK_PARAMS;

//------------------------------------------------------------------------------
//
//  Function:  OALIoCtrlHalDdkCall
//
//  This function is called form OEMIoControl for IOCTL_HAL_DDK_CALL.
//
BOOL OALIoCtlHalDdkCall(UINT32, VOID*, UINT32, VOID*, UINT32, UINT32*);

//------------------------------------------------------------------------------
//
//  Function:  OALIoReadBusData
//
//  This function reads data from device configuration space. On most platform
//  this function will support only PCI bus based devices.
//
UINT32 OALIoReadBusData(
    DEVICE_LOCATION *pDevLoc, UINT32 address, UINT32 size, VOID *pData
);

//------------------------------------------------------------------------------
//
//  Function:  OALIoWriteBusData
//
//  This function write data to device configuration space. On most platform
//  this function will support only PCI bus based devices.
//
UINT32 OALIoWriteBusData(
    DEVICE_LOCATION *pDevLoc, UINT32 address, UINT32 size, VOID *pData
);

//------------------------------------------------------------------------------
//
//  Function:  OALIoTransBusAddress
//
//  This function translate bus relative address to system address which can
//  be used for I/O operations.
//
BOOL OALIoTransBusAddress(
    INTERFACE_TYPE ifcType, UINT32 busNumber, UINT64 busAddress,
    UINT32 *pAddressSpace, UINT64 *pSystemAddress
);

//------------------------------------------------------------------------------
//
//  Function:  OALIoTransSystemAddress
//
//  This function translate system address to bus-relative address which can
//  be used for DMA operations. This function isn't reverse to
//  OALIoTransBusAddress. It maps system RAM address to bus relative RAM
//  address.
//
BOOL OALIoTransSystemAddress(
    INTERFACE_TYPE ifcType, UINT32 busNumber, UINT64 systemAddress,
    UINT64 *pBusAddress
);

//------------------------------------------------------------------------------
//
//  Function:  OALIoBusPowerOff
//
//  This function is called to put bus driver to power off mode. It should
//  save bus driver state if required and switch power off on bus driver.
//
BOOL OALIoBusPowerOff(INTERFACE_TYPE ifcType, UINT32 busId);

//------------------------------------------------------------------------------
//
//  Function:  OALIoBusPowerOn
//
//  This function is called to put bus driver back to power on mode. It should
//  power on bus driver and restore its state if required.
//
BOOL OALIoBusPowerOn(INTERFACE_TYPE ifcType, UINT32 busId);

//------------------------------------------------------------------------------

#if __cplusplus
}
#endif

#endif
