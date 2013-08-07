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
#include "boot_args.h"
#include "eboot.h"
#include "nand.h"
#pragma warning(push)
#pragma warning(disable: 4201 4115)
#include <blcommon.h>
#pragma warning(pop)
#include "oalex.h"
//------------------------------------------------------------------------------
//  Global variables

UINT32  g_ulFlashBase = BSP_NAND_REGS_PA;
//------------------------------------------------------------------------------
//  Static variables

static UINT8 g_bpartBuffer[NAND_BPART_BUFFER_SIZE];

//------------------------------------------------------------------------------
// Global variables
//
UINT32 g_ulFlashLengthBytes = sizeof(UINT16);
UINT32 g_ulBPartBase        = (UINT32)g_bpartBuffer;    
UINT32 g_ulBPartLengthBytes = sizeof(g_bpartBuffer);

#ifndef BSP_NO_NAND_IN_SDBOOT

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
BOOL
WriteFlashLogo(
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

#ifdef IMGMULTIXIP
static
BOOL
WriteFlashEXT(
    UINT32 address,
    UINT32 size
    );
#endif
#endif
//------------------------------------------------------------------------------
//  Local Variables

struct {
    HANDLE hFlash;
    BOOL failure;
    ULONG address;
    ULONG size;
} s_binDio;

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
    UINT32 rc = (UINT32) BL_ERROR;

    UNREFERENCED_PARAMETER(pBootDevices);
#ifndef BSP_NO_NAND_IN_SDBOOT
    // We have do device initialization for some devices
    switch (pConfig->bootDevLoc.IfcType)
        {
        case Internal:
            switch (pConfig->bootDevLoc.LogicalLoc)
                {
                case BSP_NAND_REGS_PA + 0x20:
                rc = ReadFlashNK();
                break;
                }
            break;
        }
#else
    UNREFERENCED_PARAMETER(pConfig);
#endif
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
    BOOL rc;
    
    UNREFERENCED_PARAMETER(address);
    UNREFERENCED_PARAMETER(size);
    
#ifndef BSP_NO_NAND_IN_SDBOOT
    rc = TRUE;

    OALMSG(OAL_FUNC, (
        L"+OEMStartEraseFlash(0x%08x, 0x%08x)\r\n", address, size
        ));
    
    OALMSG(OAL_FUNC, (L"-OEMStartEraseFlash(rc = %d)\r\n", rc));   
    
#else   
    rc = FALSE;    
#endif	

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
#ifndef BSP_NO_NAND_IN_SDBOOT
    BOOL rc;

    switch (g_eboot.type)
        {
        case DOWNLOAD_TYPE_XLDR:
            rc = WriteFlashXLDR(address, size);
            break;
        case DOWNLOAD_TYPE_EBOOT:
            rc = WriteFlashEBOOT(address, size);
            break;
		case DOWNLOAD_TYPE_LOGO:
            rc = WriteFlashLogo(address, size);
			break;
        case DOWNLOAD_TYPE_FLASHNAND:
            rc = WriteFlashNK(address, size);
            break;
#ifdef IMGMULTIXIP
        case DOWNLOAD_TYPE_EXT:
            rc = WriteFlashEXT(address, size);
            break;
#endif			
        default:
            rc = FALSE;
            break;
        }
    return rc;
	
#else
    UNREFERENCED_PARAMETER(address);
    UNREFERENCED_PARAMETER(size);
    return FALSE;
#endif
}

//------------------------------------------------------------------------------

BOOL
WriteFlashNK(
    UINT32 address,
    UINT32 size
    )
{
#ifndef BSP_NO_NAND_IN_SDBOOT
    BOOL rc = FALSE;
    BOOL bExt = FALSE;
    HANDLE hPartition;
    UCHAR *pData;
    PCI_REG_INFO regInfo;

    memset(&regInfo, 0, sizeof(PCI_REG_INFO));
    regInfo.MemBase.Num    = 1;
    regInfo.MemLen.Num     = 1;
    regInfo.MemBase.Reg[0] = g_ulFlashBase;
    regInfo.MemLen.Reg[0]  = g_ulFlashLengthBytes;


    // Get data location
    pData = OEMMapMemAddr(address, address);
#ifdef IMGMULTIXIP
    // Checking if this is an EXT region
    if((address == (IMAGE_WINCE_EXT_CA + NAND_ROMOFFSET)) || (address == IMAGE_WINCE_EXT_CA) )
    {
        bExt = TRUE;
        OALLog(L"\r\nWriting EXT image to OS partition, address=%x, SIZE=%x\r\n", address, size);
    }
    else if ((address == (IMAGE_WINCE_CODE_CA + NAND_ROMOFFSET) )|| (address == IMAGE_WINCE_CODE_CA))
    {
        OALLog(L"\r\nWriting NK image to OS partition address=%x, SIZE=%x\r\n", address, size);
    }
#endif

    // Verify that we get CE image.
    if (!VerifyImage(pData, NULL))
        {
        OALMSG(OAL_ERROR, (L"ERROR: OEMWriteFlash: "
            L"NK image signature not found\r\n"
            ));
        rc = TRUE;
        goto cleanUp;
        }

    // Initialize boot partition library
    if (!BP_Init((LPBYTE)g_ulBPartBase, g_ulBPartLengthBytes, NULL, &regInfo, NULL))
    {
        OALLog(L"WriteFlashNK: Error initializing bootpart library!!\r\n");
        goto cleanUp;
    }
    
    // Open partition based on region type 
    hPartition = BP_OpenPartition((DWORD)NEXT_FREE_LOC, (DWORD)USE_REMAINING_SPACE, bExt ? PART_BINFS : PART_BOOTSECTION, FALSE, PART_OPEN_EXISTING);
    if (hPartition == INVALID_HANDLE_VALUE)
    {
        OALMSG(OAL_ERROR, (L"ERROR: OS partition not found!\r\n"));
        goto cleanUp;
    }
    
    // Check length against size of partition
    if (!BP_SetDataPointer(hPartition, size))
    {
        OALMSG(OAL_ERROR, (L"ERROR: OS partition too small!  Aborting...\r\n"));
        goto cleanUp;
    }
    
    // Write image to partition
    BP_SetDataPointer(hPartition, 0);
    if (!BP_WriteData(hPartition, pData, size))
    {
        OALMSG(OAL_ERROR, (L"ERROR: Failed writing to OS partition!\r\n"));
        goto cleanUp;
    }
    
    //OALLog(L"%s image written\r\n", bExt ? L"EXT" :L"nk");

    // Change boot device to NAND
    g_bootCfg.bootDevLoc.IfcType = Internal;
    g_bootCfg.bootDevLoc.LogicalLoc = BSP_NAND_REGS_PA + 0x20;

    // Done
    rc = TRUE;

cleanUp:
    return rc;
#else
    UNREFERENCED_PARAMETER(address);
    UNREFERENCED_PARAMETER(size);
    return TRUE;
#endif
}

#ifdef IMGMULTIXIP
//------------------------------------------------------------------------------

BOOL
WriteFlashEXT(
    UINT32 address,
    UINT32 size
    )
{
#ifndef BSP_NO_NAND_IN_SDBOOT
    BOOL rc = FALSE;
    HANDLE hPartition;
    UCHAR *pData;
    PCI_REG_INFO regInfo;

    memset(&regInfo, 0, sizeof(PCI_REG_INFO));
    regInfo.MemBase.Num    = 1;
    regInfo.MemLen.Num     = 1;
    regInfo.MemBase.Reg[0] = g_ulFlashBase;
    regInfo.MemLen.Reg[0]  = g_ulFlashLengthBytes;


    // Get data location
    pData = OEMMapMemAddr(address, address);

    // Verify that we get CE image.
    if (!VerifyImage(pData, NULL))
        {
        OALMSG(OAL_ERROR, (L"ERROR: OEMWriteFlash: "
            L"EXT image signature not found\r\n"
            ));
        rc = TRUE;
        goto cleanUp;
        }

    // Initialize boot partition library
    if (!BP_Init((LPBYTE)g_ulBPartBase, g_ulBPartLengthBytes, NULL, &regInfo, NULL))
    {
        OALLog(L"WriteFlashEXT: Error initializing bootpart library!!\r\n");
        goto cleanUp;
    }
    
    // Open partition 
    hPartition = BP_OpenPartition((DWORD)NEXT_FREE_LOC, (DWORD)USE_REMAINING_SPACE, PART_BINFS,  FALSE, PART_OPEN_EXISTING);
    if (hPartition == INVALID_HANDLE_VALUE)
    {
        OALMSG(OAL_ERROR, (L"ERROR: OS partition not found!\r\n"));
        goto cleanUp;
    }
    
    // Check length against size of partition
    if (!BP_SetDataPointer(hPartition, size))
    {
        OALMSG(OAL_ERROR, (L"ERROR: OS partition too small!  Aborting...\r\n"));
        goto cleanUp;
    }
    
    // Write image to partition
    BP_SetDataPointer(hPartition, 0);
    if (!BP_WriteData(hPartition, pData, size))
    {
        OALMSG(OAL_ERROR, (L"ERROR: Failed writing to OS partition!\r\n"));
        goto cleanUp;
    }
    
    OALLog(L"EXT image written, spin for ever!\r\n");
    OALLog(L"EXT image written, spin for ever!\r\n");
    for(;;);

    // Done
    //rc = TRUE;

cleanUp:
    return rc;
#else
    UNREFERENCED_PARAMETER(address);
    UNREFERENCED_PARAMETER(size);
    return TRUE;
#endif
}
#endif

#ifndef BSP_NO_NAND_IN_SDBOOT
//------------------------------------------------------------------------------

static
UINT32
ReadFlashNK(
    )
{
    UINT32 rc = (UINT32) BL_ERROR;
    HANDLE hPartition;
    ROMHDR *pTOC;
    ULONG offset, size;
    UCHAR *pData;
    DWORD *pInfo;
    PCI_REG_INFO regInfo;

    memset(&regInfo, 0, sizeof(PCI_REG_INFO));
    regInfo.MemBase.Num    = 1;
    regInfo.MemLen.Num     = 1;
    regInfo.MemBase.Reg[0] = g_ulFlashBase;
    regInfo.MemLen.Reg[0]  = g_ulFlashLengthBytes;


    // Check if there is a valid image
    OALMSG(OAL_INFO, (L"\r\nLoad NK image from flash memory\r\n"));

    // Initialize boot partition library
    if (!BP_Init((LPBYTE)g_ulBPartBase, g_ulBPartLengthBytes, NULL, &regInfo, NULL))
    {
        OALLog(L"ReadFlashNK: Error initializing bootpart library!!\r\n");
        goto cleanUp;
    }
    
    // Open OS boot partition
    hPartition = BP_OpenPartition((DWORD)NEXT_FREE_LOC, (DWORD)USE_REMAINING_SPACE, PART_BOOTSECTION, FALSE, PART_OPEN_EXISTING);
    if (hPartition == INVALID_HANDLE_VALUE)
    {
        OALMSG(OAL_ERROR, (L"ERROR: OS partition not found!\r\n"));
        goto cleanUp;
    }
    
    BP_SetDataPointer(hPartition, 0);
    
    // Set address where to place image
    pData = (UCHAR*)IMAGE_WINCE_CODE_CA;


    // First read 4kB with pointer to TOC
    offset = 0;
    size = 4096;
    if (!BP_ReadData(hPartition, pData + offset, size))
    {
        OALMSG(OAL_ERROR, (L"ERROR: Error reading OS partition!\r\n"));
        goto cleanUp;
    }

    // Verify that we get CE image
    pInfo = (DWORD*)(pData + ROM_SIGNATURE_OFFSET);
    if (*pInfo != ROM_SIGNATURE)
        {
        OALMSG(OAL_ERROR, (L"ERROR: "
            L"Image signature not found\r\n"
            ));
        goto cleanUp;
        }

    // Read image up through actual TOC
    offset = size;
    size = pInfo[2] - size + sizeof(ROMHDR);
    if (!BP_ReadData(hPartition, pData + offset, size))
    {
        OALMSG(OAL_ERROR, (L"ERROR: "
            L"BP_ReadData call failed!\r\n"
            ));
        goto cleanUp;
     }

    // Verify image
    if (!VerifyImage(pData, &pTOC))
        {
        OALMSG(OAL_ERROR, (L"ERROR: "
            L"NK image doesn't have ROM signature\r\n"
            ));
        goto cleanUp;
        }

    // Read remainder of image
    offset += size;
    size = pTOC->physlast - pTOC->physfirst - offset;
    if (!BP_ReadData(hPartition, pData + offset, size))
    {
        OALMSG(OAL_ERROR, (L"ERROR: "
            L"BP_ReadData call failed!\r\n"
            ));
        goto cleanUp;
    }

    OALMSG(OAL_INFO, (L"NK Image Loaded\r\n"));

    // Done
    g_eboot.launchAddress = OALVAtoPA((UCHAR*)IMAGE_WINCE_CODE_CA);
    rc = BL_JUMP;

cleanUp:
    return rc;
}

//------------------------------------------------------------------------------

BOOL
WriteFlashXLDR(
    UINT32 address,
    UINT32 size
    )
{
    BOOL rc = FALSE;
    HANDLE hFlash = NULL;
    ROMHDR *pTOC;
    UINT32 offset, xldrSize, blocknum, blocksize;
    UINT8 *pData;

	UNREFERENCED_PARAMETER(size);

    OALMSG(OAL_INFO, (L"\r\nWriting XLDR image to flash memory\r\n"));

    // Open flash storage
    hFlash = OALFlashStoreOpen(g_ulFlashBase);
    if (hFlash == NULL)
        {
        OALMSG(OAL_ERROR, (L"ERROR: OEMWriteFlash: "
            L"OALFlashStoreOpen call failed!\r\n"
            ));
        goto cleanUp;
        }

    // Get data location
    pData = OEMMapMemAddr(address, address);

    // Verify image
    if (!VerifyImage(pData, &pTOC))
        {
        OALMSG(OAL_ERROR, (L"ERROR: OEMWriteFlash: "
            L"XLDR image signature not found\r\n"
            ));
        }

    // Verify that this is XLDR image
    if (pTOC->numfiles > 0 || pTOC->nummods > 1)
        {
        OALMSG(OAL_ERROR, (L"ERROR: OEMWriteFlash: "
            L"XLDR image must have only one module and no file\r\n"
            ));
        goto cleanUp;
        }

    // Check for maximal XLRD size
    xldrSize = pTOC->physlast - pTOC->physfirst;
    if (xldrSize > (IMAGE_XLDR_CODE_SIZE - 2*sizeof(DWORD)) )
        {
        OALMSG(OAL_ERROR, (L"ERROR: OEMWriteFlash: "
            L"XLDR image size 0x%04x doesn't fit to limit 0x%04x\r\n", size, IMAGE_XLDR_CODE_SIZE - 2*sizeof(DWORD)
            ));
        goto cleanUp;
        }
        
    blocksize = OALFlashStoreBlockSize(hFlash);
    
    if (blocksize < IMAGE_XLDR_CODE_SIZE)
        {
        OALMSG(OAL_ERROR, (L"ERROR: OEMWriteFlash: "
            L"XLDR image size 0x%04x doesn't fit to flash block size 0x%04x\r\n", IMAGE_XLDR_CODE_SIZE, blocksize
            ));
        goto cleanUp;
        }

    // First we have to offset image by 2 DWORDS to insert BootROM header 
    memmove(pData + 2*sizeof(DWORD), pData, xldrSize);
    
    // Insert BootROM header
    ((DWORD*)pData)[0] = IMAGE_XLDR_CODE_SIZE - 2*sizeof(DWORD);    // Max size of image
    ((DWORD*)pData)[1] = IMAGE_XLDR_CODE_PA;                        // Load address

    // Now copy into first four blocks per boot ROM requirement.
    // Internal boot ROM expects the loader to be duplicated in the first 4 blocks for
    // data redundancy. Note that block size of memory used on this platform is larger 
    // than xldr size so there will be a gap in each block.
    offset = 0;
    for (blocknum = 0; blocknum < 4; blocknum++)
        {
        if (!OALFlashStoreWrite(
                hFlash, offset, pData, IMAGE_XLDR_CODE_SIZE, FALSE, FALSE
                ))
            {
            OALMSG(OAL_ERROR, (L"ERROR: OEMWriteFlash: "
                L"OALFlashStoreWrite at relative address 0x%08x failed\r\n",
                offset
                ));
            }
        offset += blocksize;
        }

    OALMSG(OAL_INFO, (L"XLDR image written\r\n"));

    // Done
    rc = TRUE;

cleanUp:
    if (hFlash != NULL) OALFlashStoreClose(hFlash);
    return rc;
}

//------------------------------------------------------------------------------

BOOL
WriteFlashEBOOT(
    UINT32 address,
    UINT32 size
    )
{
    BOOL rc = FALSE;
    HANDLE hFlash = NULL;
    UINT8 *pData;
    UINT32 offset;


    OALMSG(OAL_INFO, (L"\r\nWriting EBOOT image to flash memory\r\n"));

    // Open flash storage
    hFlash = OALFlashStoreOpen(g_ulFlashBase);
    if (hFlash == NULL)
        {
        OALMSG(OAL_ERROR, (L"ERROR: OEMWriteFlash: "
            L"OALFlashStoreOpen call failed!\r\n"
            ));
        goto cleanUp;
        }

    // Check if image fit (last sector used for configuration)
    if (size > (IMAGE_EBOOT_CODE_SIZE - OALFlashStoreSectorSize(hFlash)))
        {
        OALMSG(OAL_ERROR, (L"ERROR: OEMWriteFlash: "
            L"EBOOT image too big (size 0x%08x, maximum size 0x%08x)\r\n",
            size, IMAGE_EBOOT_CODE_SIZE - OALFlashStoreBlockSize(hFlash)
            ));
        goto cleanUp;
        }

    // Get data location
    pData = OEMMapMemAddr(address, address);

    // Verify that we get CE image
    if (!VerifyImage(pData, NULL))
        {
        OALMSG(OAL_ERROR, (L"ERROR: OEMWriteFlash: "
            L"EBOOT image signature not found\r\n"
            ));
        goto cleanUp;
        }

    // Fill unused space with 0xFF
    if (size < IMAGE_EBOOT_CODE_SIZE)
        {
        memset(pData + size, 0xFF, IMAGE_EBOOT_CODE_SIZE - size);
        }

    offset = IMAGE_XLDR_BOOTSEC_NAND_SIZE;
    if (!OALFlashStoreWrite(
            hFlash, offset, pData, IMAGE_EBOOT_CODE_SIZE, FALSE, FALSE
            ))
        {
        OALMSG(OAL_ERROR, (L"ERROR: OEMWriteFlash: "
            L"OALFlashStoreWrite at relative address 0x%08x failed\r\n", offset
            ));
        goto cleanUp;
        }

    OALMSG(OAL_INFO, (L"EBOOT image written\r\n"));

    // Done
    rc = TRUE;

cleanUp:
    if (hFlash != NULL) OALFlashStoreClose(hFlash);
    return rc;
}

//------------------------------------------------------------------------------

BOOL
WriteFlashLogo(
    UINT32 address,
    UINT32 size
    )
{
    BOOL rc = FALSE;
    HANDLE hFlash = NULL;
    UINT8 *pData;
    UINT32 offset;

    OALMSG(OAL_INFO, (L"\r\nWriting Splashcreen image to flash memory\r\n"));

    // Open flash storage
    hFlash = OALFlashStoreOpen(g_ulFlashBase);
    if (hFlash == NULL)
        {
        OALMSG(OAL_ERROR, (L"ERROR: OEMWriteFlash: "
            L"OALFlashStoreOpen call failed!\r\n"
            ));
        goto cleanUp;
        }

    // Check if image fit (last sector used for configuration)
    if (size > (IMAGE_BOOTLOADER_BITMAP_SIZE - OALFlashStoreSectorSize(hFlash)))
        {
        OALMSG(OAL_ERROR, (L"ERROR: OEMWriteFlash: "
            L"Logo image too big (size 0x%08x, maximum size 0x%08x)\r\n",
            size, IMAGE_BOOTLOADER_BITMAP_SIZE - OALFlashStoreBlockSize(hFlash)
            ));
        goto cleanUp;
        }

    // Get data location
    pData = OEMMapMemAddr(address, address);

    // Fill unused space with 0xFF
    if (size < IMAGE_BOOTLOADER_BITMAP_SIZE)
        {
        memset(pData + size, 0xFF, IMAGE_BOOTLOADER_BITMAP_SIZE - size);
        }

    offset = IMAGE_XLDR_BOOTSEC_NAND_SIZE + IMAGE_EBOOT_BOOTSEC_NAND_SIZE;
    if (!OALFlashStoreWrite(
            hFlash, offset, pData, IMAGE_BOOTLOADER_BITMAP_SIZE, FALSE, FALSE
            ))
        {
        OALMSG(OAL_ERROR, (L"ERROR: OEMWriteFlash: "
            L"OALFlashStoreWrite at relative address 0x%08x failed\r\n", offset
            ));
        goto cleanUp;
        }

    OALMSG(OAL_INFO, (L"Splashcreen image written\r\n"));

    // Done
    rc = TRUE;

cleanUp:
    if (hFlash != NULL) OALFlashStoreClose(hFlash);
    return rc;
}

//------------------------------------------------------------------------------

VOID
DumpTOC(
    ROMHDR *pTOC
    )
{
	UNREFERENCED_PARAMETER(pTOC);

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

#endif

//------------------------------------------------------------------------------

BOOL BLConfigureFlashPartitions(BOOL bForceEnable)
{
    BOOL rc = FALSE;
	
#ifndef BSP_NO_NAND_IN_SDBOOT
	
    HANDLE hFMD;
    PCI_REG_INFO regInfo;
    FlashInfo flashInfo;
    HANDLE hPartition;
    PPARTENTRY pPartitionEntry;
    DWORD dwBootPartitionSectorCount;

#ifdef IMGMULTIXIP
    // Variable for sector count for the new BinFS region (EXT) 
    DWORD dwExtPartitionSectorCount;
#endif

    memset(&regInfo, 0, sizeof(PCI_REG_INFO));
    regInfo.MemBase.Num    = 1;
    regInfo.MemLen.Num     = 1;
    regInfo.MemBase.Reg[0] = g_ulFlashBase;
    
    // Get flash info
    hFMD = FMD_Init(NULL, &regInfo, NULL);
    if (hFMD == NULL)
        goto cleanUp;

    if (!FMD_GetInfo(&flashInfo))
        goto cleanUp;
    
    FMD_Deinit(hFMD);
    
    // Initialize boot partition library
    if (!BP_Init((LPBYTE)g_ulBPartBase, g_ulBPartLengthBytes, NULL, &regInfo, NULL))
    {
        OALLog(L"BLConfigureFlashPartitions: Error initializing bootpart library!!\r\n");
        goto cleanUp;
    }
    
    // Get boot partition size
    // Ensure boot partition uses entire blocks with no space left over
    // Round up to an even block size
    dwBootPartitionSectorCount = ((g_bootCfg.osPartitionSize + (flashInfo.dwBytesPerBlock - 1))/ flashInfo.dwBytesPerBlock) * flashInfo.wSectorsPerBlock;

    // Reduce by one to account for MBR, which will be the first sector in non reserved area.
    // This causes boot partition to end on a block boundary
    dwBootPartitionSectorCount -= 1;

#ifdef IMGMULTIXIP
    
    // Calculation of the size for the EXT region
    dwExtPartitionSectorCount = ((IMAGE_WINCE_EXT_SIZE + (flashInfo.dwBytesPerBlock - 1))/ flashInfo.dwBytesPerBlock) * flashInfo.wSectorsPerBlock;

    dwExtPartitionSectorCount -= 1;
#endif

    // Check for existence and size of OS boot partition
    hPartition = BP_OpenPartition((DWORD)NEXT_FREE_LOC, (DWORD)USE_REMAINING_SPACE, PART_BOOTSECTION, FALSE, PART_OPEN_EXISTING);
    if (hPartition == INVALID_HANDLE_VALUE)
        OALLog(L"OS partition does not exist!!\r\n");
    else
    {
        pPartitionEntry = BP_GetPartitionInfo(hPartition);
        if (dwBootPartitionSectorCount != pPartitionEntry->Part_TotalSectors)
        {
            OALLog(L"OS partition does not match configured size!!  Sector count expected: 0x%x, actual 0x%x\r\n", dwBootPartitionSectorCount, pPartitionEntry->Part_TotalSectors);
            // Mark handle invalid to kick us into formatting code
            hPartition = INVALID_HANDLE_VALUE;
        }
    }
        
    if ((hPartition == INVALID_HANDLE_VALUE) || (bForceEnable == TRUE))
    {
        // OS binary partition either does not exist or does not match configured size
        OALLog(L"Formatting flash...\r\n");
        // Create a new partion
        // Can't just call BP_OpenPartition with PART_OPEN_ALWAYS because it will erase reserved 
        // blocks (bootloader) if MBR doesn't exist.  Also, we want to ensure the boot partition
        // is actually the first partition on the flash.  So do low level format here (note that 
        // this destroys all other partitions on the device)
        // Note, we're skipping the block check for speed reasons. Might not want this in a production device...
        BP_LowLevelFormat (0, flashInfo.dwNumBlocks, FORMAT_SKIP_RESERVED|FORMAT_SKIP_BLOCK_CHECK);
        // Create the OS partition
        hPartition = BP_OpenPartition((DWORD)NEXT_FREE_LOC, dwBootPartitionSectorCount, PART_BOOTSECTION, FALSE, PART_CREATE_NEW);
        if (hPartition == INVALID_HANDLE_VALUE)
        {
            OALLog(L"Error creating OS partition!!\r\n");
            goto cleanUp;
        }
        OALLog(L"NK partition created\r\n");
		
#ifdef IMGMULTIXIP

       // Creation of the BinFS partition
       hPartition = BP_OpenPartition((DWORD)NEXT_FREE_LOC, dwExtPartitionSectorCount, PART_BINFS, FALSE, PART_CREATE_NEW);
       if (hPartition == INVALID_HANDLE_VALUE)
       {
           OALLog(L"Error creating OS partition!!\r\n");
           goto cleanUp;
       }
       OALLog(L"EXT partition created\r\n");
#endif

        // Create FAT partition on remaining flash (can be automatically mounted)
        hPartition = BP_OpenPartition((DWORD)NEXT_FREE_LOC, (DWORD)USE_REMAINING_SPACE, PART_DOS32, FALSE, PART_CREATE_NEW);
        if (hPartition == INVALID_HANDLE_VALUE)
        {
            OALLog(L"Error creating file partition!!\r\n");
            goto cleanUp;
        }
        OALLog(L"Flash format complete!\r\n");
    }
    
    // Done
    rc = TRUE;

cleanUp:
#else
    UNREFERENCED_PARAMETER(bForceEnable);
#endif
    return rc;
}

