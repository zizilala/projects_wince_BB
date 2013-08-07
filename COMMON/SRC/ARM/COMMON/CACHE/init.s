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
;  File: init.s
;
;
        INCLUDE kxarm.h
        INCLUDE armmacros.s
        INCLUDE oal_cache.inc

        IMPORT g_oalCacheInfo

        TEXTAREA

        LEAF_ENTRY OALCacheGlobalsInit

        mrc     p15, 0, r0, c0, c0, 1
        ldr     r1, =g_oalCacheInfo

        ; First get flags
        mov     r2, #0

        ; Set write through flag
        mov     r3, r0, lsr #25
        ands    r3, r3, #15
        orreq   r2, r2, #2

        ; Set unified flag
        mov     r3, r0, lsr #24
        ands    r3, r3, #1
        orreq   r2, r2, #1

        ; Store flag
        str     r2, [r1, #L1Flags]

        ; Get I line size as (1 << (b[1..0] + 3))
        and     r3, r0, #3
        add     r3, r3, #3
        mov     r2, #1
        mov     r2, r2, lsl r3
        str     r2, [r1, #L1ILineSize]

        ; Get I associativity as ((2 + b[2]) << (b[5..3] - 1))
        mov     r3, r0, lsr #3
        ands    r3, r3, #7
        bne     %f10
        mov     r2, #1
        b       %f20
10      mov     r2, r0, lsr #2
        and     r2, r2, #1
        add     r2, r2, #2
        sub     r3, r3, #1
        mov     r2, r2, lsl r3
20      str     r2, [r1, #L1INumWays]

        ; Get I size as ((2 + b[2]) << (b[9..6] + 8))
        mov     r3, r0, lsr #2
        and     r3, r3, #1
        add     r2, r3, #2
        mov     r3, r0, lsr #6
        and     r3, r3, #15
        add     r3, r3, #8
        mov     r2, r2, lsl r3
        str     r2, [r1, #L1ISize]

        ; Get I sets as (1 << (b[9..6] + 6 - b[5..3] - b[1..0]))
        mov     r3, r0, lsr #6
        and     r3, r3, #15
        add     r2, r3, #6
        mov     r3, r0, lsr #3
        and     r3, r3, #7
        sub     r2, r2, r3
        and     r3, r0, #3
        sub     r2, r2, r3
        mov     r3, #1
        mov     r2, r3, lsl r2
        str     r2, [r1, #L1ISetsPerWay]

        ; Get D line size as (1 << (b[13..12] + 3))
        mov     r3, r0, lsr #12
        and     r3, r3, #3
        add     r3, r3, #3
        mov     r2, #1
        mov     r2, r2, lsl r3
        str     r2, [r1, #L1DLineSize]

        ; Get D associativity as ((2 + b[14]) << (b[17..15] - 1))
        mov     r3, r0, lsr #15
        ands    r3, r3, #7
        bne     %f30
        mov     r2, #1
        b       %f40
30      mov     r2, r0, lsr #14
        and     r2, r2, #1
        add     r2, r2, #2
        sub     r3, r3, #1
        mov     r2, r2, lsl r3
40      str     r2, [r1, #L1DNumWays]

        ; Get D size as ((2 + b[14]) << (b[21..18] + 8))
        mov     r3, r0, lsr #14
        and     r3, r3, #1
        add     r2, r3, #2
        mov     r3, r0, lsr #18
        and     r3, r3, #15
        add     r3, r3, #8
        mov     r2, r2, lsl r3
        str     r2, [r1, #L1DSize]

        ; Get # sets as (1 << (b[21..18] + 6 - b[17..15] - b[13..12]))
        mov     r3, r0, lsr #18
        and     r3, r3, #15
        add     r2, r3, #6
        mov     r3, r0, lsr #15
        and     r3, r3, #7
        sub     r2, r2, r3
        mov     r3, r0, lsr #12
        and     r3, r3, #3
        sub     r2, r2, r3
        mov     r3, #1
        mov     r2, r3, lsl r2
        str     r2, [r1, #L1DSetsPerWay]

        ; No secondary cache
        mov     r2, #0
        str     r2, [r1, #L2Flags]
        str     r2, [r1, #L2ISetsPerWay]
        str     r2, [r1, #L2INumWays]
        str     r2, [r1, #L2ILineSize]
        str     r2, [r1, #L2ISize]
        str     r2, [r1, #L2DSetsPerWay]
        str     r2, [r1, #L2DNumWays]
        str     r2, [r1, #L2DLineSize]
        str     r2, [r1, #L2DSize]
        
        RETURN

        END

