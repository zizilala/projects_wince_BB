// All rights reserved ADENEO EMBEDDED 2010
// Copyright (c) 2007, 2008 BSQUARE Corporation. All rights reserved.

/*
================================================================================
*             Texas Instruments OMAP(TM) Platform Software
* (c) Copyright Texas Instruments, Incorporated. All Rights Reserved.
*
* Use of this software is controlled by the terms and conditions found
* in the license agreement under which this software has been supplied.
*
================================================================================
*/
//
//  File:  flash.c
//
//  This file implements boot loader functions related to image flash.
//
#include "bsp.h"
#include <bootpart.h>
#include "eboot.h"
#include "oalex.h"

//------------------------------------------------------------------------------
//  Global variables
UINT32  g_ulFlashBase = BSP_NOR_REGS_PA;

//------------------------------------------------------------------------------
//  Local Functions

static
UINT32
ReadFlashNK(
    );

static
BOOL
WriteFlashXLDR(
    UINT32 address,
    UINT32 size
    );

static
BOOL
WriteFlashEBOOT(
    UINT32 address,
    UINT32 size
    );

BOOL
WriteFlashNK(
    UINT32 address,
    UINT32 size
    );

static
VOID
DumpTOC(
    ROMHDR *pTOC
    );

static
BOOL
VerifyImage(
    UCHAR *pData,
    ROMHDR **ppTOC
    );

//------------------------------------------------------------------------------
//
//  Function:  BLFlashDownload
//
//  This function download image from flash memory to RAM.
//
UINT32
BLFlashDownload(
    BOOT_CFG *pConfig,
    OAL_KITL_DEVICE *pBootDevices
    )
{
    UINT32 rc = (UINT32)BL_ERROR;

    UNREFERENCED_PARAMETER(pBootDevices);

    // We have do device initialization for some devices
    switch (pConfig->bootDevLoc.IfcType)
        {
        case Internal:
            switch (pConfig->bootDevLoc.LogicalLoc)
                {
                case BSP_NOR_REGS_PA:
					rc = ReadFlashNK();
					break;
                }
            break;
        }

    return rc;
}

//------------------------------------------------------------------------------

static
UINT32
ReadFlashNK(
    )
{
	UINT32 rc = (UINT32)BL_ERROR;
    ROMHDR *pTOC;
    ULONG offset, size;
    UCHAR *pData;
	DWORD *pInfo;
	UCHAR *pBase = OALPAtoVA(g_ulFlashBase, FALSE);
	UCHAR *pStart = (void *)((UINT32)pBase + IMAGE_WINCE_NOR_OFFSET);

	OALMSG(OAL_INFO, (L"\r\nLoad NK image from flash memory\r\n"));

	// Set address where to place image
	pData = (UCHAR*)IMAGE_WINCE_CODE_CA;

	// First read 4kB with pointer to TOC
	offset = 0;
	size = 4096;
	memcpy(pData + offset, pStart, size);

    // Verify that we get CE image
	pInfo = (DWORD *)(pData + offset + ROM_SIGNATURE_OFFSET);
	if (*pInfo != ROM_SIGNATURE)
	{
		OALMSG(OAL_ERROR, (L"ERROR: Image signature not found\r\n"));
		goto cleanUp;
	}

    // Read image up through actual TOC
    offset = size;
    size = pInfo[2] - size + sizeof(ROMHDR);
	memcpy(pData + offset, pStart + offset, size);

    // Verify image
    if (!VerifyImage(pData, &pTOC))
	{
		OALMSG(OAL_ERROR, (L"ERROR: NK image doesn't have ROM signature\r\n"));
		goto cleanUp;
	}

    // Read remainder of image
    offset += size;
    size = pTOC->physlast - pTOC->physfirst - offset;
    memcpy(pData + offset, pStart + offset, size);

	OALMSG(OAL_INFO, (L"NK Image Loaded\r\n"));

    // Done
    g_eboot.launchAddress = OALVAtoPA((UCHAR*)IMAGE_WINCE_CODE_CA);
    rc = BL_JUMP;

cleanUp:
    return rc;
}

//------------------------------------------------------------------------------
//
//  Function:  OEMStartEraseFlash
//
//  This function is called by the bootloader to initiate the flash memory
//  erasing process.
//
BOOL
OEMStartEraseFlash(
    ULONG address,
    ULONG size
    )
{
    BOOL rc = TRUE;

    OALMSG(OAL_FUNC, (
        L"+OEMStartEraseFlash(0x%08x, 0x%08x)\r\n", address, size
        ));
    
    OALMSG(OAL_FUNC, (L"-OEMStartEraseFlash(rc = %d)\r\n", rc));
    return rc;
}


//------------------------------------------------------------------------------
//
//  Function:  OEMContinueEraseFlash
//
//  This function is called by the bootloader to during download to provide
//  ability to continue the flash block erasing operation.
//
VOID
OEMContinueEraseFlash(
    )
{
}


//------------------------------------------------------------------------------
//
//  Function:  OEMFinishEraseFlash
//
//  This function is called by the bootloader to finish flash erase before
//  it will call OEMWriteFlash.
//
BOOL
OEMFinishEraseFlash(
    )
{
    BOOL rc = TRUE;

    return rc;
}

//------------------------------------------------------------------------------
//
//  Function:  OEMWriteFlash
//
//  This function is called by the bootloader to write the image that may
//  be stored in a RAM file cache area to flash memory. This function is
//  called once per each downloaded region.
//
BOOL
OEMWriteFlash(
    ULONG address,
    ULONG size
    )
{
    BOOL rc;

    switch (g_eboot.type)
        {
        case DOWNLOAD_TYPE_XLDR:
            rc = WriteFlashXLDR(address, size);
            break;
        case DOWNLOAD_TYPE_EBOOT:
            rc = WriteFlashEBOOT(address, size);
            break;
		case DOWNLOAD_TYPE_FLASHNOR:
            rc = WriteFlashNK(address, size);
            break;
        default:
            rc = FALSE;
            break;
        }

    return rc;
}

//------------------------------------------------------------------------------

BOOL
WriteFlashXLDR(
    UINT32 address,
    UINT32 size
    )
{
	ROMHDR *pTOC;
	UINT8 *pData;
	void *pBase = OALPAtoVA(g_ulFlashBase, FALSE);
	void *pStart = (void *)((UINT32)pBase + IMAGE_XLDR_NOR_OFFSET);
	UINT32 xldrSize;
	BOOL rc = FALSE;

	OALMSG(OAL_INFO, (L"\r\nWriting XLDR image to flash memory\r\n"));

	// Get data location
	pData = OEMMapMemAddr(address, address);

	// Verify image
	if (!VerifyImage(pData, &pTOC))
	{
		OALMSG(OAL_ERROR, (L"ERROR: OEMWriteFlash: XLDR image signature not found\r\n"));
		goto cleanUp;
	}

	// Verify that this is XLDR image
	if (pTOC->numfiles > 0 || pTOC->nummods > 1)
	{
		OALMSG(OAL_ERROR, (L"ERROR: OEMWriteFlash: XLDR image must have only one module and no file\r\n"));
		goto cleanUp;
	}

	// Check for maximal XLRD size
	xldrSize = pTOC->physlast - pTOC->physfirst;
	if (xldrSize > IMAGE_XLDR_CODE_SIZE)
	{
		OALMSG(OAL_ERROR, (L"ERROR: OEMWriteFlash: XLDR image size 0x%04x doesn't fit to limit 0x%04x\r\n", size, IMAGE_XLDR_CODE_SIZE));
		goto cleanUp;
	}

	// Unlock blocks
	if (!OALFlashLock(pBase, pStart, size, FALSE))
	{
		OALMSG(OAL_ERROR, (L"ERROR: OEMWriteFlash: XLDR blocks could not be unlocked\r\n"));
		goto cleanUp;
	}

	// Erase blocks
	if (!OALFlashErase(pBase, pStart, size))
	{
		OALMSG(OAL_ERROR, (L"ERROR: OEMWriteFlash: XLDR blocks could not be erased\r\n"));
		goto cleanUp;
	}

	// Write blocks
	if (!OALFlashWrite(pBase, pStart, size, pData))
	{
		OALMSG(OAL_ERROR, (L"ERROR: OEMWriteFlash: XLDR blocks could not be written\r\n"));
		goto cleanUp;
	}

    // Done
    rc = TRUE;

cleanUp:
    return rc;
}

//------------------------------------------------------------------------------

BOOL
WriteFlashEBOOT(
    UINT32 address,
    UINT32 size
    )
{
	void *pData;
	void *pBase = OALPAtoVA(g_ulFlashBase, FALSE);
	void *pStart = (void *)((UINT32)pBase + IMAGE_EBOOT_NOR_OFFSET);
	BOOL rc = FALSE;

    OALMSG(OAL_INFO, (L"\r\nWriting EBOOT image to flash memory\r\n"));

    // Check if image fit (last sector used for configuration)
    if (size > IMAGE_EBOOT_CODE_SIZE)
	{
		OALMSG(OAL_ERROR, (L"ERROR: OEMWriteFlash: EBOOT image too big (size 0x%08x, maximum size 0x%08x)\r\n", size, IMAGE_EBOOT_CODE_SIZE));
		goto cleanUp;
	}

	// Get data location
	pData = OEMMapMemAddr(address, address);

	// Verify that we get CE image
	if (!VerifyImage(pData, NULL))
	{
		OALMSG(OAL_ERROR, (L"ERROR: OEMWriteFlash: EBOOT image signature not found\r\n"));
		goto cleanUp;
	}

	// Fill unused space with 0xFF
	if (size < IMAGE_EBOOT_CODE_SIZE)
	{
		memset((void *)((UINT32)pData + size), 0xFF, IMAGE_EBOOT_CODE_SIZE - size);
	}

	// Unlock blocks
	if (!OALFlashLock(pBase, pStart, size, FALSE))
	{
		OALMSG(OAL_ERROR, (L"ERROR: OEMWriteFlash: EBOOT blocks could not be unlocked\r\n"));
		goto cleanUp;
	}

	// Erase blocks
	if (!OALFlashErase(pBase, pStart, size))
	{
		OALMSG(OAL_ERROR, (L"ERROR: OEMWriteFlash: EBOOT blocks could not be erased\r\n"));
		goto cleanUp;
	}

	// Write blocks
	if (!OALFlashWrite(pBase, pStart, size, pData))
	{
		OALMSG(OAL_ERROR, (L"ERROR: OEMWriteFlash: EBOOT blocks could not be written\r\n"));
		goto cleanUp;
	}

	OALMSG(OAL_INFO, (L"EBOOT image written\r\n"));

    // Done
    rc = TRUE;

cleanUp:
    return rc;
}

//------------------------------------------------------------------------------

BOOL
WriteFlashNK(
    UINT32 address,
    UINT32 size
    )
{
	BOOL rc = FALSE;
	UCHAR *pData;
	void *pBase = OALPAtoVA(g_ulFlashBase, FALSE);
	void *pStart = (void *)((UINT32)pBase + IMAGE_WINCE_NOR_OFFSET);

	OALLog(L"\r\nWriting NK image to flash memory\r\n");

	// Get data location
	pData = OEMMapMemAddr(address, address);

	// Verify that we get CE image
	if (!VerifyImage(pData, NULL))
	{
		OALMSG(OAL_ERROR, (L"ERROR: OEMWriteFlash: NK image signature not found\r\n"));
		goto cleanUp;
	}

	// Unlock blocks
	if (!OALFlashLock(pBase, pStart, size, FALSE))
	{
		OALMSG(OAL_ERROR, (L"ERROR: OEMWriteFlash: NK blocks could not be unlocked\r\n"));
		goto cleanUp;
	}

	// Erase blocks
	if (!OALFlashErase(pBase, pStart, size))
	{
		OALMSG(OAL_ERROR, (L"ERROR: OEMWriteFlash: NK blocks could not be erased\r\n"));
		goto cleanUp;
	}

	// Write blocks
	if (!OALFlashWrite(pBase, pStart, size, pData))
	{
		OALMSG(OAL_ERROR, (L"ERROR: OEMWriteFlash: NK blocks could not be written\r\n"));
		goto cleanUp;
	}

	OALLog(L"NK image written\r\n");

	// Change boot device to NOR
	g_bootCfg.bootDevLoc.IfcType = Internal;
	g_bootCfg.bootDevLoc.LogicalLoc = BSP_NOR_REGS_PA;

	// Done
	rc = TRUE;

cleanUp:
	return rc;
}

//------------------------------------------------------------------------------

VOID
DumpTOC(
    ROMHDR *pTOC
    )
{
    // Print out ROMHDR information
    OALMSG(OAL_INFO, (L"\r\n"));
    OALMSG(OAL_INFO, (L"ROMHDR (pTOC = 0x%08x) ---------------------\r\n", pTOC));
    OALMSG(OAL_INFO, (L"  DLL First           : 0x%08x\r\n", pTOC->dllfirst));
    OALMSG(OAL_INFO, (L"  DLL Last            : 0x%08x\r\n", pTOC->dlllast));
    OALMSG(OAL_INFO, (L"  Physical First      : 0x%08x\r\n", pTOC->physfirst));
    OALMSG(OAL_INFO, (L"  Physical Last       : 0x%08x\r\n", pTOC->physlast));
    OALMSG(OAL_INFO, (L"  Num Modules         : %10d\r\n",   pTOC->nummods));
    OALMSG(OAL_INFO, (L"  RAM Start           : 0x%08x\r\n", pTOC->ulRAMStart));
    OALMSG(OAL_INFO, (L"  RAM Free            : 0x%08x\r\n", pTOC->ulRAMFree));
    OALMSG(OAL_INFO, (L"  RAM End             : 0x%08x\r\n", pTOC->ulRAMEnd));
    OALMSG(OAL_INFO, (L"  Num Copy Entries    : %10d\r\n",   pTOC->ulCopyEntries));
    OALMSG(OAL_INFO, (L"  Copy Entries Offset : 0x%08x\r\n", pTOC->ulCopyOffset));
    OALMSG(OAL_INFO, (L"  Prof Symbol Length  : 0x%08x\r\n", pTOC->ulProfileLen));
    OALMSG(OAL_INFO, (L"  Prof Symbol Offset  : 0x%08x\r\n", pTOC->ulProfileOffset));
    OALMSG(OAL_INFO, (L"  Num Files           : %10d\r\n",   pTOC->numfiles));
    OALMSG(OAL_INFO, (L"  Kernel Flags        : 0x%08x\r\n", pTOC->ulKernelFlags));
    OALMSG(OAL_INFO, (L"  FileSys RAM Percent : 0x%08x\r\n", pTOC->ulFSRamPercent));
    OALMSG(OAL_INFO, (L"  Driver Glob Start   : 0x%08x\r\n", pTOC->ulDrivglobStart));
    OALMSG(OAL_INFO, (L"  Driver Glob Length  : 0x%08x\r\n", pTOC->ulDrivglobLen));
    OALMSG(OAL_INFO, (L"  CPU                 :     0x%04x\r\n", pTOC->usCPUType));
    OALMSG(OAL_INFO, (L"  MiscFlags           :     0x%04x\r\n", pTOC->usMiscFlags));
    OALMSG(OAL_INFO, (L"  Extensions          : 0x%08x\r\n", pTOC->pExtensions));
    OALMSG(OAL_INFO, (L"  Tracking Mem Start  : 0x%08x\r\n", pTOC->ulTrackingStart));
    OALMSG(OAL_INFO, (L"  Tracking Mem Length : 0x%08x\r\n", pTOC->ulTrackingLen));
    OALMSG(OAL_INFO, (L"------------------------------------------------\r\n"));
    OALMSG(OAL_INFO, (L"\r\n"));
}

//------------------------------------------------------------------------------

BOOL
VerifyImage(
    UCHAR *pData,
    ROMHDR **ppTOC
    )
{
    BOOL rc = FALSE;
    UINT32 *pInfo;
    ROMHDR *pTOC;

    // Verify that we get CE image.
    pInfo = (UINT32*)(pData + ROM_SIGNATURE_OFFSET);
    if (*pInfo != ROM_SIGNATURE) goto cleanUp;

    // We are on correct location....
    pTOC = (ROMHDR*)(pData + pInfo[2]);

    // Let see
    DumpTOC(pTOC);

    // Return pTOC if pointer isn't NULL
    if (ppTOC != NULL) *ppTOC = pTOC;

    // Done
    rc = TRUE;

cleanUp:
    return rc;
}

//------------------------------------------------------------------------------

BOOL BLConfigureFlashPartitions(BOOL bForceEnable)
{
    BOOL rc = FALSE;
    UNREFERENCED_PARAMETER(bForceEnable);
    return rc;
}