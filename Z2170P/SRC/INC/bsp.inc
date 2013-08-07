; All rights reserved ADENEO EMBEDDED 2010
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
;  File:  bsp_cfg.inc
;



;------------------------------------------------------------------------------
;
;  Define:  BSP_STALL_DELAY
;
;  This constant is used to generate stall in assembler code. We assume
;  worst case when CPU clock is already at 330 MHz which give us
;  165 instruction cycles (divide by 2) per microseconds.
;
BSP_STALL_DELAY         EQU     165

;------------------------------------------------------------------------------
;
;  Define:  BSP_PTES_PA
;
;  This is the area of PTEs used by the MMU in EBOOT
;
BSP_PTES_PA             EQU     (0x87EE0000)


;------------------------------------------------------------------------------
;
;  Define:  IMAGE_EBOOT_STACK_xxxxx
;
;  This defines the stack size and loction for EBOOT (virtual address)
;

IMAGE_EBOOT_STACK_CA    EQU     (0x87EE4000)
IMAGE_EBOOT_STACK_SIZE  EQU     (0x0001C000)

;------------------------------------------------------------------------------
;
;  Define:  OMAP_IDCODE_REGS_PA
;
;  This defines the Register Physical address for chip id code
;
OMAP_IDCODE_REGS_PA  EQU    0x4830A204       

                    END

