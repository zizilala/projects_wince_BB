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
//  File:  main.c
//
//  This file contains X-Loader implementation for OMAP35XX
//
#include "bsp.h"
#pragma warning(push)
#pragma warning(disable: 4115 4201)
#include <blcommon.h>
#include <fmd.h>
#pragma warning(pop)

#include "sdk_gpio.h"
#include "oalex.h"
#define OAL
#include "oal_alloc.h"
#include "oal_clock.h"
#include "bsp_cfg.h"

// !!! IMPORTANT !!!
// Check the .bib, image_cfg.h and image_cfg.inc files
// to make sure the addr and size match.
#ifdef UART_BOOT
// UART_BOOT
#define NAND_MAX_BLOCKS_TO_WRITE   15
extern int XReceive(unsigned char *p_data_buff, int buff_size, unsigned int *p_receive_size);
#else
// Not UART_BOOT
#undef UART_BOOT
#undef UART_DNLD_EBOOT_TO_RAM
#undef UART_DNLD_RAW_TO_NAND
#endif  /* UART_BOOT */

#if defined(FMD_ONENAND) && defined(FMD_NAND)
    #error FMD_ONENAND and FMD_NAND cannot both be defined.
#endif


extern VOID OEMWriteDebugHex(unsigned long n, long depth);
extern void XGetStats
(
	int *p_xblock_cnt,
	int *p_xack_cnt,
	int *p_xnak_cnt,
	int *p_xcan_cnt,
	int *p_xothers_cnt,
	int *p_checksum_error_cnt,
	int *p_dup_pkt_cnt
);

//------------------------------------------------------------------------------

#ifdef SHIP_BUILD
#define XLDRMSGINIT
#define XLDRMSGDEINIT
#define XLDRMSG(msg)
#else
#define XLDRMSGINIT         {EnableDeviceClocks(BSPGetDebugUARTConfig()->dev,TRUE); OEMInitDebugSerial();}
#define XLDRMSGDEINIT       OEMDeinitDebugSerial()
#define XLDRMSG(msg)        OEMWriteDebugString(msg)
#define XLDRHEX(val, len)   OEMWriteDebugHex(val, len)
#endif


//------------------------------------------------------------------------------
// External Variables
extern DEVICE_IFC_GPIO Omap_Gpio;

//------------------------------------------------------------------------------
//  Global variables
ROMHDR * volatile const pTOC = (ROMHDR *)-1;

const volatile DWORD dwOEMHighSecurity      = OEM_HIGH_SECURITY_GP;
unsigned int  gCPU_family;
const volatile DWORD dwEbootECCtype = (DWORD)-1;
UCHAR g_ecctype =0;

//------------------------------------------------------------------------------
//  External Functions

extern VOID PlatformSetup();
extern VOID JumpTo();
extern VOID EnableCache_GP();
extern VOID EnableCache_HS();

//------------------------------------------------------------------------------
//  Local Functions

static BOOL SetupCopySection(ROMHDR *const pTableOfContents);

#ifdef UART_BOOT
//------------------------------------------------------------------------------
//
//  Function:  XLDRWriteHexString
//
//  Output a hex string to debug serial port
//
VOID XLDRWriteHexString(UINT8 *string, INT len)
{
	while (len--) 
	{
		OEMWriteDebugByte((UINT8)*string++);
	}
}

//------------------------------------------------------------------------------
//
//  Function:  XLDRWriteCharString
//
//  Output NULL ended char string to debug serial port
//
VOID XLDRWriteCharString(UINT8 *string)
{
    while (*string != '\0') OEMWriteDebugByte((UINT8)*string++);
}

//------------------------------------------------------------------------------
//
//  Function:  XLDRReadCharMaxTime
//
//  Try input char from debug serial port max num of times, returns error code {-1, 0, 1}
//
//#define XLDR_READS_PER_SEC 0xE6666
#define XLDR_READS_PER_SEC 0x17FFFF
INT XLDRReadCharMaxTime(UINT8 *uc, INT num_sec)
{
	INT c;
	UINT32 reads = num_sec * XLDR_READS_PER_SEC;

	do
	{
	    c = OEMReadDebugByte();
        if (c == OEM_DEBUG_READ_NODATA)
        {
            /* continue to read */
			c=0;
        }
        else if (c == OEM_DEBUG_COM_ERROR) 
        {
			c=-1;
            break;
        }
        else
        {
            /* received one byte */
			*uc = (UINT8)c;
			c = 1;
            break;
        }

	}
	while (--reads);

	return (c);
}

//------------------------------------------------------------------------------
//
//  Function:  XLDRWriteChar
//
//  Output char to debug serial port
//
VOID XLDRWriteChar(const UINT8 c)
{
	OEMWriteDebugByte(c);
}

//------------------------------------------------------------------------------
//
//  Function:  XLDRWriteChar
//
//  Output char to debug serial port
//
VOID XLDRPrintUint8(const UINT8 ui8, UINT32 num_of_times, UINT32 end_cr_lf)
{
	INT i, j;
	UINT8 msg_buff[24];
	UINT8 c;
	UINT32 n = 0;

	while (n++ < num_of_times)
	{
		j=0;
		msg_buff[j++]='0';
		msg_buff[j++]='x';

		for(i=0; i<2; i++)
		{
			// grab a half byte and move it to the far right
		    c = (UINT8)(((ui8 & (0xf0 >> i*4)) >> 4*(1-i)));
			if ((0 <= c) && (c <= 9))
			{
				msg_buff[j++] = c + '0';
			}
			else
			{
				msg_buff[j++] = (c - 10) + 'A';
			}
		}

		if (end_cr_lf)
		{
			msg_buff[j++]='\r';
			msg_buff[j++]='\n';
		}
		else
		{
			msg_buff[j++]=' '; /* end with a space */
		}
		msg_buff[j++]='\0';

        XLDRWriteCharString(&(msg_buff[0]));
	}
}

//------------------------------------------------------------------------------
//
//  Function:  XLDRPrintUlong
//
//  Output unsigned long to debug serial port
//
VOID XLDRPrintUlong(const UINT32 ulong_to_write, UINT32 num_of_times, UINT32 end_cr_lf)
{
	INT i, j;
	UINT32 n;
	UINT8 msg_buff[24];
	UINT8 c;

	n = 0;
	while (n++ < num_of_times)
	{
		j=0;
		msg_buff[j++]='0';
		msg_buff[j++]='x';

		for(i=0; i<8; i++)
		{
			// get a half byte and move it to the far right
		    c = (UINT8)(((ulong_to_write & (0xf0000000 >> i*4)) >> 4*(7-i)));
			if ((0 <= c) && (c <= 9))
			{
				msg_buff[j++] = c + '0';
			}
			else
			{
				msg_buff[j++] = (c - 10) + 'A';
			}
		}

		if (end_cr_lf)
		{
			msg_buff[j++]='\r';
			msg_buff[j++]='\n';
		}
		else
		{
			msg_buff[j++]=' '; /* end with a space */
		}
		msg_buff[j++]='\0';

        XLDRWriteCharString(&(msg_buff[0]));
	}
}
#endif /*XLDRPrintUlong*/

void BSPGpioInit()
{
   BSPInsertGpioDevice(0,&Omap_Gpio,NULL);
}

//------------------------------------------------------------------------------
//
//  Function:  XLDRMain
//
VOID XLDRMain()
{
#ifdef FMD_NAND
    HANDLE hFMD;
    PCI_REG_INFO regInfo;
    FlashInfo flashInfo;
    UINT32 size;  
#endif

#ifdef MEMORY_BOOT
    UINT32 count;
    SECTOR_ADDR ix;
#endif

#ifdef UART_BOOT
    UINT32 dnld_size, offset;

    INT image_block_cnt, nand_blocks_to_write;
    INT xret;

    INT xblock_cnt = 0;
    INT xack_cnt = 0;
    INT xnak_cnt = 0;
    INT xcan_cnt = 0;
    INT xothers_cnt = 0;

    INT checksum_error_cnt = 0;
    INT dup_pkt_cnt = 0;
#endif

    SECTOR_ADDR sector;
    BLOCK_ID block;
    UINT8 *pImage;
    SectorInfo sectorInfo;

    HANDLE hGpio;
    static UCHAR allocationPool[512];
    LPCWSTR ProcessorName   = L"3530";

    // Setup global variables
    if (!SetupCopySection(pTOC))
        goto cleanUp;

    OALLocalAllocInit(allocationPool,sizeof(allocationPool));

/*
    //  Enable cache based on device type
    if( dwOEMHighSecurity == OEM_HIGH_SECURITY_HS )
    {
        EnableCache_HS();
    }
    else
    {
        EnableCache_GP();
    }
*/
    EnableCache_GP();

    gCPU_family = CPU_FAMILY(Get_CPUVersion());
	
    if( gCPU_family == CPU_FAMILY_DM37XX)
    {
        ProcessorName = L"3730";
    }
    
    PlatformSetup();

    // Initialize debug serial output
    XLDRMSGINIT;

    // Print information...
#ifdef FMD_ONENAND
#ifdef MEMORY_BOOT
    XLDRMSG(
        TEXT("\r\nTexas Instruments Windows CE OneNAND X-Loader for EVM "));
    XLDRMSG( (UINT16 *)ProcessorName);
    XLDRMSG( 
        TEXT("\r\n")
        TEXT("Built ") TEXT(__DATE__) TEXT(" at ") TEXT(__TIME__) TEXT("\r\n")
        );
#endif
#endif
#ifdef FMD_NAND
#ifdef MEMORY_BOOT
    XLDRMSG( TEXT("\r\nTexas Instruments Windows CE NAND X-Loader for EVM "));
    XLDRMSG( (UINT16 *)ProcessorName);
    XLDRMSG( 
        TEXT("\r\n")
        TEXT("Built ") TEXT(__DATE__) TEXT(" at ") TEXT(__TIME__) TEXT("\r\n")
        );
#endif
#endif
    XLDRMSG(
        TEXT("Version ") BSP_VERSION_STRING TEXT("\r\n")
        );

    GPIOInit();
    hGpio = GPIOOpen();
    GPIOSetBit(hGpio,BSP_LCD_POWER_GPIO);
    GPIOSetMode(hGpio,BSP_LCD_POWER_GPIO,GPIO_DIR_OUTPUT);
  
    
#ifdef FMD_ONENAND
    // Open FMD to access ONENAND
    regInfo.MemBase.Reg[0] = BSP_ONENAND_REGS_PA;
    hFMD = FMD_Init(NULL, &regInfo, NULL);
    if (hFMD == NULL)
    {
        XLDRMSG(L"\r\nFMD_Init failed\r\n");
        goto cleanUp;
    }

    //  Set ONENAND XLDR bootsector size
    size = IMAGE_XLDR_BOOTSEC_ONENAND_SIZE;
#endif
#ifdef FMD_NAND
    // Open FMD to access NAND
    regInfo.MemBase.Reg[0] = BSP_NAND_REGS_PA;
    hFMD = FMD_Init(NULL, &regInfo, NULL);
    if (hFMD == NULL)
    {
        XLDRMSG(L"\r\nFMD_Init failed\r\n");
        goto cleanUp;
    }

    //  Set NAND XLDR bootsector size
    size = IMAGE_XLDR_BOOTSEC_NAND_SIZE;
#endif

    // Get flash info
    if (!FMD_GetInfo(&flashInfo))
    {
        XLDRMSG(L"\r\nFMD_GetInfo failed\r\n");
        goto cleanUp;
    }

#ifdef MEMORY_BOOT
    // Start from NAND start
    block  = 0;
    sector = 0;

    // First skip XLDR boot region.

    // NOTE - The bootrom will load the xldr from the first good block starting
    // at zero.  If an uncorrectable ECC error is encountered it will try the next
    // good block.  The last block attempted is the fourth physical block.  The first
    // block is guaranteed good when shipped from the factory, for the first 1000
    // erase/program cycles.

    // Our programming algorithm will place four copies of the xldr into the first
    // four *good* blocks.  If one or more of the first four physical blocks is marked
    // bad, the XLDR boot region will include the fifth physical block or beyond.  This
    // would result in a wasted block containing a copy of the XLDR that will never be
    // loaded by the bootrom, but it simplifies the flash management algorithms.
    count = 0;
    while (count < size)
        {
        if ((FMD_GetBlockStatus(block) & BLOCK_STATUS_BAD) == 0)
            count += flashInfo.dwBytesPerBlock;
        block++;
        sector += flashInfo.wSectorsPerBlock;
        }
    // get ECC type for EBOOT from FIXUP value
    g_ecctype = (UCHAR)dwEbootECCtype;
	
    FMD_Deinit(hFMD); 
    hFMD = FMD_Init(NULL, &regInfo, NULL);

    // Set address where to place image
    pImage = (UINT8*)IMAGE_STARTUP_IMAGE_PA;

    // Read image to memory
    count = 0;
    while ((count < IMAGE_STARTUP_IMAGE_SIZE) && (block < flashInfo.dwNumBlocks))
        {
        // Skip bad blocks
        if ((FMD_GetBlockStatus(block) & BLOCK_STATUS_BAD) != 0)
            {
            block++;
            sector += flashInfo.wSectorsPerBlock;
            XLDRMSG(L"#");
            continue;
            }

        // Read sectors in block
        ix = 0;
        while ((ix++ < flashInfo.wSectorsPerBlock) &&
                (count < IMAGE_STARTUP_IMAGE_SIZE))
            {
            // If a read fails, there is nothing we can do about it
            if (!FMD_ReadSector(sector, pImage, &sectorInfo, 1))
                {
                XLDRMSG(L"$");
                }

            // Move to next sector
            sector++;
            pImage += flashInfo.wDataBytesPerSector;
            count += flashInfo.wDataBytesPerSector;
            }

        XLDRMSG(L".");

        // Move to next block
        block++;
        }

    XLDRMSG(L"\r\nJumping to bootloader\r\n");

    // Wait for serial port
    XLDRMSGDEINIT;

    // Jump to image
    JumpTo((VOID*)IMAGE_STARTUP_IMAGE_PA);
#endif  /* MEMORY_BOOT */
#ifdef UART_BOOT

#if defined(UART_DNLD_EBOOT_TO_RAM) || defined(UART_DNLD_RAW_TO_NAND)

#ifdef UART_DNLD_EBOOT_TO_RAM
    XLDRMSG(L"\r\nDNLD EBOOTND.nb0 Image\r\n");
#endif

#ifdef UART_DNLD_RAW_TO_NAND
    XLDRMSG(L"\r\nDNLD TIEVM3530-nand.raw Image\r\n");
#endif

    // Set address where to place download image
    pImage = (UINT8*)IMAGE_STARTUP_IMAGE_PA;

	xret = XReceive(pImage, IMAGE_XLDR_BOOTSEC_NAND_SIZE+IMAGE_EBOOT_CODE_SIZE+IMAGE_BOOTLOADER_BITMAP_SIZE+8, &dnld_size);
	if(xret < 0)
	{
		goto cleanUp;
	}

#ifdef UART_DNLD_EBOOT_TO_RAM
    XLDRMSG(L"\r\nJumping to bootloader EBOOT\r\n");

	// Wait for serial port
    XLDRMSGDEINIT;

	// Jump to image
	JumpTo((VOID*)IMAGE_STARTUP_IMAGE_PA);
#endif  /* UART_DNLD_EBOOT_TO_RAM */

#ifdef UART_DNLD_RAW_TO_NAND
	// How many nand blocks to write
	if (dnld_size < flashInfo.dwBytesPerBlock)
	{
		nand_blocks_to_write=0;
	}
	else
	{
		for(nand_blocks_to_write=0; nand_blocks_to_write <= NAND_MAX_BLOCKS_TO_WRITE; nand_blocks_to_write++)
		{
			if ((nand_blocks_to_write * flashInfo.dwBytesPerBlock) >= dnld_size)
			{
				break;
			}
		}
	}

	if ((nand_blocks_to_write == 0) || (nand_blocks_to_write > NAND_MAX_BLOCKS_TO_WRITE))
	{
		goto cleanUp;
	}

    // Set address to where to copy from
    pImage = (UINT8*)IMAGE_STARTUP_IMAGE_PA;

	// Write dnld image, starting from first good block (4 xldr block and 2 eboot block)
	block = 0;
    image_block_cnt = 0;

	while (image_block_cnt < nand_blocks_to_write)
	{

            /* writing Eboot: need to change ECC mode*/ 
            if(image_block_cnt == 4)
            {
                 // get EBOOT ECC type from FIXUP value
                 g_ecctype = (UCHAR)dwEbootECCtype;
                 
                 FMD_Deinit(hFMD); 
                 hFMD = FMD_Init(NULL, &regInfo, NULL);
            }
		// Skip to a good block
        while (block < flashInfo.dwNumBlocks)
        {
            if ((FMD_GetBlockStatus(block) & BLOCK_STATUS_BAD) == 0)
    		{
    			// A good block
    			break;
    		}
            block++;
        }
    
    	if (block >= flashInfo.dwNumBlocks)
    	{
    		// No good block found!!
    		goto cleanUp;
    	}

		// Erase block first
		if (!FMD_EraseBlock(block))
		{
			for(;;) XLDRMSG(L"Erase FAILED\r\n");
		}

		// Calculate starting sector id of the good block
    	sector = block * flashInfo.wSectorsPerBlock;
		offset = 0;

		// Copy sectors in block
		while (offset < flashInfo.dwBytesPerBlock)
		{
        	memset(&sectorInfo, 0xFF, sizeof(sectorInfo));
	        sectorInfo.bOEMReserved &= ~(OEM_BLOCK_READONLY|OEM_BLOCK_RESERVED);
			sectorInfo.dwReserved1 = 0;
			sectorInfo.wReserved2 = 0;
    
    		// Write 1 sector
    		if (!FMD_WriteSector(sector, pImage + offset, &sectorInfo, 1))
			{
				for(;;) XLDRMSG(L"Write FAILED\r\n");
				goto cleanUp;
			}

			// Next sector
			sector++;
			offset += flashInfo.wDataBytesPerSector;
		}
    
		// Written 1 block
		++image_block_cnt;

		// Start from next block id and next block of data
		++block;
		pImage += offset;
	}

	XGetStats ( &xblock_cnt, &xack_cnt, &xnak_cnt,
		        &xcan_cnt, &xothers_cnt, &checksum_error_cnt, 
				&dup_pkt_cnt);

	XLDRMSG(L"\r\nReceive return code "); XLDRPrintUint8((UINT8)xret, 1, 1);
	XLDRMSG(L"blocks written ");          XLDRPrintUint8((UINT8)image_block_cnt, 1, 1);
	XLDRMSG(L"bytes rx ");                XLDRPrintUlong((UINT32)dnld_size, 1, 1);
	XLDRMSG(L"pkts rx ");                 XLDRPrintUlong((UINT32)xblock_cnt, 1, 1);
	XLDRMSG(L"acks sent ");               XLDRPrintUlong((UINT32)xack_cnt, 1, 1);
	XLDRMSG(L"naks sent ");               XLDRPrintUlong((UINT32)xnak_cnt, 1, 1);
	XLDRMSG(L"can sent ");                XLDRPrintUlong((UINT32)xcan_cnt, 1, 1);
	XLDRMSG(L"others sent ");             XLDRPrintUlong((UINT32)xothers_cnt, 1, 1);
	XLDRMSG(L"chksum errs ");             XLDRPrintUlong((UINT32)checksum_error_cnt, 1, 1);
	XLDRMSG(L"dup pkts ");                XLDRPrintUlong((UINT32)dup_pkt_cnt, 1, 1);

	// Wait for serial port
    XLDRMSGDEINIT;

	// Done.
	return;
#endif  /* UART_DNLD_RAW_TO_NAND */
#endif  /* UART_DNLD_EBOOT_TO_RAM || UART_DNLD_RAW_TO_NAND */

#ifdef UART_BOOT_READ_SECTOR
    // Set address where to place data read
    pImage = (UINT8*)IMAGE_STARTUP_IMAGE_PA;

	sector = 0;
    if (!FMD_ReadSector(sector, pImage, &sectorInfo, 1))
    {
        for(;;) XLDRMSG(L"read failed\r\n");
    }            

    //print the first 16 bytes to debug port 
    pImage = (UINT8*)IMAGE_STARTUP_IMAGE_PA;

	for(;;)
	{
	    XLDRPrintUint8(sectorInfo.bOEMReserved, 1, 0);
	    XLDRPrintUint8(sectorInfo.bBadBlock, 1, 0);

		for (ix = 0; ix<16; ix++)
		{
		    XLDRPrintUint8(*(pImage+ix), 1, 0);
		}
   	    XLDRMSG(L"\r\n");
	}
#endif /* UART_BOOT_READ_SECTOR */

#ifdef UART_BOOT_WRITE_SECTOR

	if (!FMD_EraseBlock(0))
	{
		while(1) XLDRMSG(L"Erase FAILED\r\n");
	}

	// Set address where to place image
    pImage = (UINT8*)IMAGE_STARTUP_IMAGE_PA;

	for (uc = 0; uc<16; uc++)
		*(pImage+uc) = uc+'A';

	sector = 0;

   	memset(&sectorInfo, 0xFF, sizeof(sectorInfo));
    sectorInfo.bOEMReserved &= ~(OEM_BLOCK_READONLY|OEM_BLOCK_RESERVED);
	sectorInfo.dwReserved1 = 0;
	sectorInfo.wReserved2 = 0;
    
    // Write 1 sector
    if (!FMD_WriteSector(sector, pImage, &sectorInfo, 1))
	{
		while(1) XLDRMSG(L"Write FAILED\r\n");
		goto cleanUp;
	}

	XLDRPrintUint8('z', 10000, 1);
	XLDRMSG(L"That many times printed\r\n");

	// Wait for serial port
    XLDRMSGDEINIT;

	// Done.
	return;
#endif /* UART_BOOT_WRITE_SECTOR */

#endif  /* UART_BOOT */
    
cleanUp:
    XLDRMSG(L"\r\nHALT\r\n");
    for(;;);
}


//------------------------------------------------------------------------------
//
//  Function:  OALPAtoVA
//
VOID* OALPAtoVA(UINT32 address, BOOL cached)
{
    UNREFERENCED_PARAMETER(cached);
    return (VOID*)address;
}


//------------------------------------------------------------------------------
//
//  Function:  OALVAtoPA
//
UINT32 OALVAtoPA(VOID *pVA)
{
    return (UINT32)pVA;
}



//------------------------------------------------------------------------------
//
//  Function:  SetupCopySection
//
//  Copies image's copy section data (initialized globals) to the correct
//  fix-up location.  Once completed, initialized globals are valid. Optimized
//  memcpy is too big for X-Loader.
//
static BOOL SetupCopySection(ROMHDR *const pTableOfContents)
{
    BOOL rc = FALSE;
    UINT32 loop, count;
    COPYentry *pCopyEntry;
    UINT8 *pSrc, *pDst;

    if (pTableOfContents == (ROMHDR *const) -1) goto cleanUp;

    for (loop = 0; loop < pTableOfContents->ulCopyEntries; loop++)
        {
        pCopyEntry = (COPYentry *)(pTableOfContents->ulCopyOffset + loop*sizeof(COPYentry));

        count = pCopyEntry->ulCopyLen;
        pDst = (UINT8*)pCopyEntry->ulDest;
        pSrc = (UINT8*)pCopyEntry->ulSource;
        while (count-- > 0)
            *pDst++ = *pSrc++;
        count = pCopyEntry->ulDestLen - pCopyEntry->ulCopyLen;
        while (count-- > 0)
            *pDst++ = 0;
        }

    rc = TRUE;

cleanUp:
    return rc;
}

//------------------------------------------------------------------------------
//
//  Function:  NKDbgPrintfW
//
//
VOID NKDbgPrintfW(LPCWSTR pszFormat, ...)
{
    //  Stubbed out to shrink XLDR binary size
    UNREFERENCED_PARAMETER(pszFormat);
}

//------------------------------------------------------------------------------
//
//  Function:  NKvDbgPrintfW
//
//
VOID NKvDbgPrintfW(LPCWSTR pszFormat, va_list pArgList)
{
    //  Stubbed out to shrink XLDR binary size
    UNREFERENCED_PARAMETER(pszFormat);
    UNREFERENCED_PARAMETER(pArgList);

}


//------------------------------------------------------------------------------
