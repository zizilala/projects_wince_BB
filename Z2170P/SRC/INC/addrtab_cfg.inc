; All rights reserved ADENEO EMBEDDED 2010
;==============================================================================
;             Texas Instruments OMAP(TM) Platform Software
; (c) Copyright Texas Instruments, Incorporated. All Rights Reserved.
;
; Use of this software is controlled by the terms and conditions found
; in the license agreement under which this software has been supplied.
;
;==============================================================================
;
;------------------------------------------------------------------------------
;
;  File:  memory_cfg.inc
;
;  This file is used to define g_oalAddressTable. This table is passed to
;  KernelStart to estabilish physical to virtual memory mapping. This table
;  is used also in memory OAL module to map between physical and virtual
;  memory addresses via OALPAtoVA/OALVAtoPA functions.
;
;  The config.bib file defines image memory layout ant it contains virtual
;  cached memory addresses which must be synchronized with g_oalAddressTable.
;  With each table change make sure that those constant are still valid.
;
;------------------------------------------------------------------------------
;  Export Definition

        EXPORT  g_oalAddressTable[DATA]
        EXPORT  g_oalAddressTableHynix[DATA]
        
;------------------------------------------------------------------------------
;  Table format: cached address, physical address, size

g_oalAddressTable

    DCD     0x80000000, 0x80000000, 128     ; SDRAM  
    IF BSP_SDRAM_BANK1_ENABLE
        DCD     0x88000000, 0xA0000000, 128     ; SDRAM
    ENDIF
    DCD     0x95000000, 0x15000000,  16     ; CS5, LAN9115
    DCD     0x96000000, 0x48000000,  16     ; L4 Core/Wakeup registers
    DCD     0x97000000, 0x49000000,   1     ; L4 Peripheral
    DCD     0x97100000, 0x68000000,  16     ; L3 registers
    DCD     0x98100000, 0x6C000000,  16     ; SMS registers
    DCD     0x99100000, 0x6D000000,  16     ; SDRC registers
    DCD     0x9a100000, 0x6E000000,  16     ; GPMC registers
    DCD     0x9b100000, 0x40200000,   1     ; 64KB SRAM
    DCD     0x9b200000, 0x5C000000,  16     ; IPSS interconnect
    DCD     0x9C200000, 0x00000000,   1     ; ROM
    DCD     0x9C300000, 0x08000000,   1     ; NAND Registers (FIFO)
    DCD     0x00000000, 0x00000000,   0     ; end of table

g_oalAddressTableHynix


    DCD     0x80000000, 0x80000000, 256     ; SDRAM           
    DCD     0x94000000, 0x11000000,  16     ; CS2, NOR flash 
    DCD     0x95000000, 0x15000000,  16     ; CS5, LAN9115
    DCD     0x96000000, 0x48000000,  16     ; L4 Core/Wakeup registers
    DCD     0x97000000, 0x49000000,   1     ; L4 Peripheral
    DCD     0x97100000, 0x68000000,  16     ; L3 registers
    DCD     0x98100000, 0x6C000000,  16     ; SMS registers
    DCD     0x99100000, 0x6D000000,  16     ; SDRC registers
    DCD     0x9a100000, 0x6E000000,  16     ; GPMC registers
    DCD     0x9b100000, 0x40200000,   1     ; 64KB SRAM
    DCD     0x9b200000, 0x5C000000,  16     ; IPSS interconnect
    DCD     0x9C200000, 0x00000000,   1     ; ROM
    DCD     0x9C300000, 0x08000000,   1     ; NAND Registers (FIFO)
    DCD     0x00000000, 0x00000000,   0     ; end of table

;------------------------------------------------------------------------------

    END       
