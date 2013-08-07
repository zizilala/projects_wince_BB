//
//  All rights reserved ADENEO EMBEDDED 2010
//
////////////////////////////////////////////////////////////////////////////////
//
//  Tux_PowerManagement TUX DLL
//
//  Module: platform.h
//          Header for platform specific function prototypes.
//
//  Revision History:
//
////////////////////////////////////////////////////////////////////////////////

#ifndef __PLATFORM_H__
#define __PLATFORM_H__

////////////////////////////////////////////////////////////////////////////////
// Included files

#include <windows.h>

////////////////////////////////////////////////////////////////////////////////
// Defines for default parameters

#define I2C_NOTSET_VARIABLE	255

#define I2C_BUFFER_SIZE		2
#define I2C_NB_THREADS		30
#define I2C_WRITES			20
#define I2C_ADDRESS			I2C_NOTSET_VARIABLE
#define I2C_PORT			2
#define I2C_BAUDRATE_INDEX	I2C_NOTSET_VARIABLE
#define I2C_SUBADDRESS_MODE	I2C_NOTSET_VARIABLE
#define I2C_SUBADDRESS		0x87
#define I2C_DUMMY_FILL		0xAA

// Here is a working set of variables to test TPS device over I2C
// ADDRESS  = 72
// I2C_PORT = 1
// BAUDRATE_INDEX = 0
// SUBADDRESS_MODE = 1
// SUBADDRESS = 0x87

////////////////////////////////////////////////////////////////////////////////
// Function prototypes

HANDLE Platform_Open();
void Platform_Close(HANDLE hI2C);
BOOL Platform_setSlaveAddress(HANDLE hI2C);
BOOL Platform_setBaudRateIndex(HANDLE hI2C);
BOOL Platform_setSubAddrMode(HANDLE hI2C);
UINT Platform_Write(HANDLE hI2C,LONG subAddr,void * buffer, UINT32 size);

#endif // __PLATFORM_H__
