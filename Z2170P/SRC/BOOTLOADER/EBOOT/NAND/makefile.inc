# All rights reserved ADENEO EMBEDDED 2010
!if 0
;================================================================================
;             Texas Instruments OMAP(TM) Platform Software
; (c) Copyright Texas Instruments, Incorporated. All Rights Reserved.
;
; Use of this software is controlled by the terms and conditions found
; in the license agreement under which this software has been supplied.
;
;================================================================================
!endif

_FILES=$(_PLATFORMROOT)\$(_TGTPLAT)\files
_TGT=$(_RAWFILETGT)\$(WINCEDEBUG)

BootImage:
!IF "$(NOLINK)" == ""
    romimage $(ROMIMAGE_FLAGS) ebootnand.bib
!IF "$(WINCEREL)"=="1"
    xcopy /Q /I $(_TGT)\ebootnd.bin $(_FLATRELEASEDIR)
    xcopy /Q /I $(_TGT)\ebootnd.nb0 $(_FLATRELEASEDIR)
    copy $(_FLATRELEASEDIR)\ebootnd.nb0 $(_FLATRELEASEDIR)\ebootnd.raw
!ENDIF
!ENDIF        

