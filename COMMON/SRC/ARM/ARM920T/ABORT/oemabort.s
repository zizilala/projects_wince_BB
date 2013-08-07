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
; oemabort.s - header file for the data abort veneer
;
; This file selects options suitable for Windows CE's use of
; the data abort veneer.
;

        OPT     2       ; disable listing
        INCLUDE kxarm.h
        OPT     1       ; reenable listing
		
        TEXTAREA
        IMPORT DataAbortHandler
	
        LEAF_ENTRY OEMDataAbortHandler
        b       DataAbortHandler
        ENTRY_END

        END

; EOF oemabort.s
