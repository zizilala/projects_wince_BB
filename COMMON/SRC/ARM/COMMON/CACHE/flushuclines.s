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
;-------------------------------------------------------------------------------
;
;  File:  flushuclines.s
;
;
        INCLUDE kxarm.h
        INCLUDE armmacros.s
        INCLUDE oal_cache.inc

        IMPORT g_oalCacheInfo

        TEXTAREA

;-------------------------------------------------------------------------------
;
;  Function:  OALFlushUCacheLines
;
        LEAF_ENTRY OALFlushUCacheLines

        ldr     r2, =g_oalCacheInfo
        ldr     r3, [r2, #L1DLineSize]

10      mcr     p15, 0, r0, c7, c15, 1          ; clean and invalidate entry
        add     r0, r0, r3                      ; move to next
        subs    r1, r1, r3
        bgt     %b10                            ; loop while > 0 bytes left

        RETURN

        END
