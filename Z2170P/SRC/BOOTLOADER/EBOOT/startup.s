;
;================================================================================
;             Texas Instruments OMAP(TM) Platform Software
; (c) Copyright Texas Instruments, Incorporated. All Rights Reserved.
;
; Use of this software is controlled by the terms and conditions found
; in the license agreement under which this software has been supplied.
;
;================================================================================
;
;   File:  startup.s
;
;   Boot startup routine for OMAP35xx boards.
;

        INCLUDE kxarm.h
        INCLUDE bsp.inc
        
        IMPORT  main

        STARTUPTEXT
        
;-------------------------------------------------------------------------------
;
;  Function:  StartUp
;
;  This function is entry point to Windows CE EBOOT. It should be called
;  in state with deactivated MMU and disabled caches.
;
;  Note that g_oalAddressTable is needed for OEMMapMemAddr because
;  downloaded image is placed on virtual addresses (but EBOOT runs without
;  MMU).
;
        LEAF_ENTRY StartUp

        ;---------------------------------------------------------------
        ; Jump to BootMain
        ;---------------------------------------------------------------

BUILDTTB
        ; find out CHIP id, use chip id to determine what DDR is used
        ; This approach has following assumptions:
        ; 1. OMAP35xx uses Micron DDR that has 128M connected to CS0 
        ;     and another 128M connected to CS1, if BSP_SDRAM_BANK1_ENABLE
        ;     is enabled.
        ; 2. 37xx uses Hynix DDR that has 256M continuous memory connected to CS0
        ;
        ldr     r0, =OMAP_IDCODE_REGS_PA
        ldr     r1, [r0]
        ldr     r2, =0x0FFFF000
        ldr     r3, =0x0B891000
        and		r1, r1, r2
        cmp		r1, r3
        addne     r11, pc, #g_oalAddressTable-(.+8)    ; Pointer to OEMAddressTable.
        addeq     r11, pc, #g_oalAddressTableHynix - (. + 8)   ; Pointer to OEMAddressTable for Hynix device

    ; Set the TTB.
    ;
    ldr     r9, =BSP_PTES_PA
    ldr     r0, =0xFFFFC000                   ;
    and     r9, r9, r0                        ; Mask off TTB to be on 16K boundary.
    mcr     p15, 0, r9, c2, c0, 0             ; Set the TTB.

    ; ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    ; ~~~~~~~~~~ MAP CACHED and BUFFERED SECTION DESCRIPTORS ~~~~~~~~~~~~~~~~~~
    ; ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    mov     r0, #0x0A                         ; Section (1MB) descriptor; (C=1 B=0: cached write-through).
    orr     r0, r0, #0x400                    ; Set AP.
20  mov     r1, r11                           ; Pointer to OEMAddressTable.

    ; Start Crunching through the OEMAddressTable[]:
    ;
    ; r2 temporarily holds OEMAddressTable[VA]
    ; r3 temporarily holds OEMAddressTable[PHY]
    ; r4 temporarily holds OEMAddressTable[#MB]
    ;
25  ldr     r2, [r1], #4                       ; Virtual (cached) address to map physical address to.
    ldr     r3, [r1], #4                       ; Physical address to map from.
    ldr     r4, [r1], #4                       ; Number of MB to map.

    cmp     r4, #0                             ; End of table?
    beq     %F29

    ; r2 holds the descriptor address (virtual address)
    ; r0 holds the actual section descriptor
    ;

    ; Create descriptor address.
    ;
    ldr     r6, =0xFFF00000
    and     r2, r2, r6             ; Only VA[31:20] are valid.
    orr     r2, r9, r2, LSR #18    ; Build the descriptor address:  r2 = (TTB[31:14} | VA[31:20] >> 18)

    ; Create the descriptor.
    ;
    ldr     r6, =0xFFF00000
    and     r3, r3, r6             ; Only PA[31:20] are valid for the descriptor and the rest will be static.
    orr     r0, r3, r0             ; Build the descriptor: r0 = (PA[31:20] | the rest of the descriptor)

    ; Store the descriptor at the proper (physical) address
    ;
28  str     r0, [r2], #4
    add     r0, r0, #0x00100000    ; Section descriptor for the next 1MB mapping (just add 1MB).
    sub     r4, r4, #1             ; Decrement number of MB left.
    cmp     r4, #0                 ; Done?
    bne     %B28                   ; No - map next MB.

    bic     r0, r0, #0xF0000000    ; Clear section base address field.
    bic     r0, r0, #0x0FF00000    ; Clear section base address field.
    b       %B25                   ; Get and process the next OEMAddressTable element.

    ; ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    ; ~~~~~~~~~~ MAP UNCACHED and UNBUFFERED SECTION DESCRIPTORS ~~~~~~~~~~~~~~
    ; ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

29  tst     r0, #8                 ; Test for 'C' bit set (means we just used 
                                   ; above loop structure to map cached and buffered space).
    bic     r0, r0, #0x0C          ; Clear cached and buffered bits in the descriptor (clear C&B bits).
    add     r9, r9, #0x0800        ; Pointer to the first PTE for "unmapped uncached space" (0x2000 0000 + V_U_Adx).
    bne     %B20                   ; Repeat the descriptor setup for uncached space (map C=B=0 space).

ACTIVATEMMU
    ; The 1st Level Section Descriptors are setup. Initialize the MMU and turn it on.
    ;
    mov     r1, #1
    mcr     p15, 0, r1, c3, c0, 0   ; Set up access to domain 0.
    mov     r0, #0
    mcr     p15, 0, r0, c8, c7, 0   ; Flush the instruction and data TLBs.
    mcr     p15, 0, r1, c7, c10, 4  ; Drain the write and fill buffers.

	mrc     p15, 0, r1, c1, c0, 0   
    orr     r1, r1, #0x1            ; Enable MMU.
    orr     r1, r1, #0x1000         ; Enable IC.
    orr     r1, r1, #0x4            ; Enable DC.
    ldr     r2, =VirtualStart       ; Get virtual address of 'VirtualStart' label.
    cmp     r2, #0                  ; Make sure no stall on "mov pc,r2" below.

    ; Enable the MMU.
    ;
    mcr     p15, 0, r1, c1, c0, 0   ; MMU ON:  All memory accesses are now virtual.

    ; Jump to the virtual address of the 'VirtualStart' label.
    ;
    mov     pc, r2                  ;
    nop
    nop
    nop

    ; *************************************************************************
    ; *************************************************************************
    ; The MMU and caches are now enabled and we're running in a virtual
    ; address space.
    ;
    
    ALIGN
    
VirtualStart

    ;  Set up a supervisor mode stack.
    ;
    ; NOTE: These values must match the OEMAddressTable and .bib file entries for
    ; the bootloader.
    ;
    ldr     sp, =(IMAGE_EBOOT_STACK_CA + IMAGE_EBOOT_STACK_SIZE)

	
	
    ; Jump to the C entrypoint.
    ;
    bl      main					; Jump to main.c::main(), never to return...

    
        ENTRY_END 

        ; Include memory configuration file with g_oalAddressTable
        INCLUDE addrtab_cfg.inc

        END

;------------------------------------------------------------------------------
