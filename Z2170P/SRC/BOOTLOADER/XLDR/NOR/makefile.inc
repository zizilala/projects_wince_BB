# All rights reserved ADENEO EMBEDDED 2010
!if 0 
;  Copyright (c) 2007, 2008 BSQUARE Corporation. All rights reserved.

;================================================================================
;             Texas Instruments OMAP(TM) Platform Software
; (c) Copyright Texas Instruments, Incorporated. All Rights Reserved.
;
; Use of this software is controlled by the terms and conditions found
; in the license agreement under which this software has been supplied.
;
;================================================================================
!endif

_TGT=$(_RAWFILETGT)\$(WINCEDEBUG)

xldr_romimage_raw:
!if "$(NOLINK)" == ""
    romimage $(ROMIMAGE_FLAGS) xldrnor.bib
!if "$(WINCEREL)"=="1"
    xcopy /Q /I $(_TGT)\xldrnor.* $(_FLATRELEASEDIR)
!endif
!endif
