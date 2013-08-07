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
//  Header: oal_keyvalue.h
//
//  Defines the interface to the OAL Key-Value component. The Key-value 
//  component provides a framework to specify bootloader and CE boot time
//  parameters from a text file. The text file is included in the image
//  using the BIB file.
//
#ifndef __oal_keyvalue_h__
#define __oal_keyvalue_h__

#if __cplusplus
    extern "C" 
    {
#endif

// Include bootloader defines for ROMHDR 

#include "blcommon.h"

//------------------------------------------------------------------------------
//  Define: OAL_KEYVALUE_MAX_XXX
//
//  INI File Limits. 
//
// 	The max number of chars in a line is 256.
//	The max number of chars in a key is 64.
// 	The max number of chars in a Value is 128.
//

#define     OAL_KEYVALUE_MAX_LINE   	(256)
#define     OAL_KEYVALUE_MAX_KEY   		(64)
#define     OAL_KEYVALUE_MAX_VALUE   	(128)


//------------------------------------------------------------------------------
//  Define: OAL_KEYVALUE_SIGNATURE
//
//  Defines the signature used to determine if the component has been 
//  initialized.
//

#define OAL_KEYVALUE_SIGNATURE        (0x93547431)

//------------------------------------------------------------------------------
//  Enumeration: OAL_KEYVALUE_STATUS
//
//  Defines the possible status codes for key-value operations.
//

typedef enum
{
    OAL_KEYVALUE_SUCCESS,                   // operation succeeded
    OAL_KEYVALUE_INIT,                      // not initialized
    OAL_KEYVALUE_ARG,                       // bad argument
    OAL_KEYVALUE_SPACE,                     // insufficient space for add
    OAL_KEYVALUE_USED,                      // key is already used
    OAL_KEYVALUE_KEY,                       // key not found
    OAL_KEYVALUE_TOKEN,                     // bad key or literal token
    OAL_KEYVALUE_SIZE,                      // buffer too small for value
    OAL_KEYVALUE_LITERAL,                   // bad literal string
    OAL_KEYVALUE_FILE                       // file not found
}
OAL_KEYVALUE_STATUS;

//------------------------------------------------------------------------------
//  Type: OAL_KEYVALUE_PAIR    
//
//  The KEYVALUE pair is used to store a key-value entry in the table. This
//  type is inserted at the top of the table. The offsets point into the table 
//  to the actual text strings.
//

typedef struct  
{
    UINT16  KeyOffset;
    UINT16  KeyLength;
    UINT16  ValueOffset;
    UINT16  ValueLength;
}
OAL_KEYVALUE_PAIR, *POAL_KEYVALUE_PAIR;

//------------------------------------------------------------------------------
//  Type: OAL_KEYVALUE    
//
//  Key-value control block.
//

typedef struct  
{
    UINT32  Signature;
    UINT32  Count;                          // # of key-value pairs in table
    UINT32  Space;                          // space left in table
    UINT32  TableSize;                      // size of table
    UINT8   *pEnd;                          // pointer to end of table
    UINT8   *pTable;                        // pointer to table
}
OAL_KEYVALUE, *POAL_KEYVALUE;


// Interface

BOOL                OALKeyValueInit   ( POAL_KEYVALUE pKeyValue,
                                        UINT8 *pTable,
                                        UINT32 TableSize );

OAL_KEYVALUE_STATUS OALKeyValueAdd    ( POAL_KEYVALUE pKeyValue,
                                        char *Key,
                                        char *Value );

OAL_KEYVALUE_STATUS OALKeyValueAddFile( POAL_KEYVALUE pKeyValue,
                                        ROMHDR *pTOC,
                                        char *FileName );

OAL_KEYVALUE_STATUS OALKeyValueGet    ( POAL_KEYVALUE pKeyValue,
                                        char *Key,
                                        char *Value,
                                        UINT32 *pValueLen );
// Test Support

BOOL 	OALKeyValueTestMain( VOID );


#if __cplusplus
    }
#endif

#endif 
