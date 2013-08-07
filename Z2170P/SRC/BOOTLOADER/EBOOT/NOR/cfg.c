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
//  File:  cfg.c
//
//  This file implements functions used to load/save EBOOT configuration info.
//  The EBOOT configuration is located on last block reserved for EBOOT image
//  on NOR flash memory.
//
#include <eboot.h>
#include <bsp_logo.h>

//------------------------------------------------------------------------------

BOOL
BLReadBootCfg(
    BOOT_CFG *pBootCfg
    )
{
	void *pBase = OALPAtoVA(g_ulFlashBase, FALSE);
	void *pStart = (void *)((UINT32)pBase + IMAGE_EBOOT_CFG_NOR_OFFSET);

	memcpy((void *)pBootCfg, pStart, sizeof(BOOT_CFG));
    
    return TRUE;
}

//------------------------------------------------------------------------------

BOOL
BLWriteBootCfg(
    BOOT_CFG *pBootCfg
    )
{
    BOOL rc = FALSE;

	void *pBase = OALPAtoVA(g_ulFlashBase, FALSE);
	void *pStart = (void *)((UINT32)pBase + IMAGE_EBOOT_CFG_NOR_OFFSET);

	// Unlock blocks
	if (!OALFlashLock(pBase, pStart, sizeof(BOOT_CFG), FALSE))
	{
		OALMSG(OAL_ERROR, (L"ERROR: BLWriteBootCfg: configuration blocks could not be unlocked\r\n"));
		goto cleanUp;
	}

	// Erase blocks
	if (!OALFlashErase(pBase, pStart, sizeof(BOOT_CFG)))
	{
		OALMSG(OAL_ERROR, (L"ERROR: BLWriteBootCfg: configuration blocks could not be erased\r\n"));
		goto cleanUp;
	}

	// Write blocks
	if (!OALFlashWrite(pBase, pStart, sizeof(BOOT_CFG), (void *)pBootCfg))
	{
		OALMSG(OAL_ERROR, (L"ERROR: BLWriteBootCfg: configuration blocks could not be written\r\n"));
		goto cleanUp;
	}

	// Done
	rc = TRUE;

cleanUp:
    return rc;
}

//------------------------------------------------------------------------------

BOOL
BLReserveBootBlocks(
    BOOT_CFG *pBootCfg
    )
{
    UNREFERENCED_PARAMETER(pBootCfg);
    // Nothing to do...
    return TRUE;
}

//------------------------------------------------------------------------------

BOOL
BLShowLogo(
    )
{
    //  Show the bootloader splashscreen, use -1 for flashaddr to force default display
    ShowLogo((UINT32)-1, 0);

    return TRUE;
}

//------------------------------------------------------------------------------

