;
; Copyright (c) Microsoft Corporation.  All rights reserved.
;
;
; Use of this sample source code is subject to the terms of the Microsoft
; license agreement under which you licensed this sample source code. If
; you did not accept the terms of the license agreement, you are not
; authorized to use this sample source code. For the terms of the license,
; please see the license agreement between you and Microsoft or, if applicable,
; see the LICENSE.RTF on your install media or the root of your tools installation.
; THE SAMPLE SOURCE CODE IS PROVIDED "AS IS", WITH NO WARRANTIES.
;
;------------------------------------------------------------------------------
;
;  Header: oal_memory.inc
;
;  Defines the OAL memory interface for assembler files. This
;  file is a subset of the memory.h file.
;

;------------------------------------------------------------------------------
;
;  Define: MEM_UNCACHED_BIT
;
;  Defines the address bit that determines if an address is cached
;  or uncached, according to the ranges below:
;
;      0x80000000 - 0x9FFFFFFF ==> CACHED   address
;      0xA0000000 - 0xBFFFFFFF ==> UNCACHED address
;

MEM_UNCACHED_BIT        EQU     (0x20000000)


;------------------------------------------------------------------------------
;  Define: MEM_SIZE_X_KB
;
;  Utility defintions for memory sizes, in KB.
;

MEM_SIZE_4_KB          	EQU     (0x00001000) 
MEM_SIZE_8_KB         	EQU     (0x00002000) 
MEM_SIZE_16_KB         	EQU     (0x00004000) 
MEM_SIZE_32_KB         	EQU     (0x00008000) 
MEM_SIZE_64_KB          EQU     (0x00010000) 
MEM_SIZE_128_KB         EQU     (0x00020000) 
MEM_SIZE_256_KB         EQU     (0x00040000) 
MEM_SIZE_512_KB         EQU     (0x00080000) 

;------------------------------------------------------------------------------
;  Define: MEM_SIZE_X_MB
;
;  Utility defintions for memory sizes, in MB.
;

MEM_SIZE_1_MB           EQU     (0x00100000) 
MEM_SIZE_2_MB           EQU     (0x00200000) 
MEM_SIZE_4_MB           EQU     (0x00400000) 
MEM_SIZE_8_MB           EQU     (0x00800000) 
MEM_SIZE_16_MB          EQU     (0x01000000) 
MEM_SIZE_32_MB          EQU     (0x02000000) 
MEM_SIZE_64_MB          EQU     (0x04000000) 
MEM_SIZE_128_MB         EQU     (0x08000000) 
MEM_SIZE_256_MB         EQU     (0x10000000) 
MEM_SIZE_512_MB         EQU     (0x20000000) 
MEM_SIZE_1024_MB        EQU     (0x40000000) 
MEM_SIZE_2048_MB        EQU     (0x80000000) 

    END

