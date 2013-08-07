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
;  File: oal_cache.inc
;
;  ARM assembly version of partial contents of oal_cache.h
;

;-------------------------------------------------------------------------------
;
;  Defines: L1xxx/L2xxx
;
;  Following defines are used in ARM assember to access OAL_CACHE_INFO
;  structure members. It must be synchronized with OAL_CACHE_INFO definition.
;

L1Flags		EQU	0
L1ISetsPerWay   EQU    	4
L1INumWays      EQU     8
L1ILineSize     EQU     12
L1ISize         EQU     16
L1DSetsPerWay   EQU     20
L1DNumWays      EQU     24
L1DLineSize     EQU     28
L1DSize         EQU     32

L2Flags         EQU     36
L2ISetsPerWay   EQU     40
L2INumWays      EQU     44
L2ILineSize     EQU     48
L2ISize         EQU     52
L2DSetsPerWay   EQU     56
L2DNumWays      EQU     60
L2DLineSize     EQU     64
L2DSize         EQU     68

;-------------------------------------------------------------------------------

        	END

