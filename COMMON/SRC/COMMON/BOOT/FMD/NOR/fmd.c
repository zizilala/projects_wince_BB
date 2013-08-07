//
// Copyright (c) Microsoft Corporation.  All rights reserved.
//
//
// Use of this sample source code is subject to the terms of the Microsoft
// license agreement under which you licensed this sample source code. If
// you did not accept the terms of the license agreement, you are not
// authorized to use this sample source code. For the terms of the license,
// please see the license agreement between you and Microsoft or, if applicable,
// see the LICENSE.RTF on your install media or the root of your tools installation.
// THE SAMPLE SOURCE CODE IS PROVIDED "AS IS", WITH NO WARRANTIES.
//
//------------------------------------------------------------------------------
//
//  This file implements FMD for NOR flash memory. It uses abstract NOR_xxxx
//  functions.
//
//  This is read-only version intended to be used as part of IPL.
//
#include <windows.h>
#include <oal.h>
#include <fmd.h>
#include <fls.h>

//------------------------------------------------------------------------------
//
//     The FAL expects the Flash media to be broken up into Flash blocks
//     which are then subdivided into physical sectors.  Some types of
//     Flash memory (i.e. NAND Flash) already have this layout.  NOR Flash,
//     on the other hand, DOES NOT breakup each Flash block into physical
//     sectors.  Instead, each byte is individually addressable (like RAM).
//     Despite this characteristic, NOR Flash can still logically be broken
//     up into discrete sectors as follows:
//
//                             NOR Flash
//
//                          Sector Data     SectorInfo
//                  ---------------------------------
//                  |Sector(0)              |       |
//                  |Sector(1)              |       |
//                  |Sector(2)              |       |       Block 0
//                  |...                    |       |
//                  |Sector(k)              |       |
//                  |                       XXXXXXXXX
//                   -------------------------------
//                  |Sector(k+1)            |       |
//                  |Sector(k+2)            |       |
//                  |Sector(k+3)            |       |       Block 1
//                  |...                    |       |
//                  |Sector(2k)             |       |
//                  |                       XXXXXXXXX
//                   -------------------------------
//                  |                       |       |
//                  |                       |       |
//                  |                       |       |       Block 2
//                  |                       |       |
//                  |                       |       |
//                  |                       XXXXXXXXX
//                   -------------------------------        ...
//                  |                       |       |
//                  |                       |       |
//                  |                       |       |       Block N
//                  |                       |       |
//                  |                       |       |
//                  |                       XXXXXXXXX
//                  ---------------------------------
//
//     That is, each NOR Flash block is logically subdivided into a "page",
//     where each page contains space for sector data and SectorInfo metadata.
//     Most often, Flash blocks are a power of 2 in size but the size
//     of a page is not a power of 2 (i.e. 512 + 8 = 520 bytes).
//     Thus, each Flash block DOES NOT evenly divide into an integral number
//     of pages and some bytes in a block are left unused.  These unused
//     bytes are denoted above by XXXXX's.
//
//     To help clarify how this process works, consider the following
//     example: suppose you have a NOR Flash memory device that contains
//     256 Flash blocks each 256K in size. From these size characteristics,
//     we find:
//
//         (256K / (512+8)) ==> 504 sectors + 64 unused bytes
//
//     Therefore, each Flash block can map 504 sectors (including SectorInfo
//     metadata) and leave 64 unused bytes per block.  Notice that 8 bytes
//     is used for the SectorInfo metadata although the SectorInfo structure
//     is currently only 6 bytes.  The reason for this is to make sure that
//     all sector addresses are DWORD aligned (i.e. 520 divides by 4 evenly
//     while 518 doesn't divide by 4 evenly).  Furthemore, we see that
//     this NOR Flash memory can map (504*256)==>129,024 physical sectors.
//
//     -------
//
//     Two other points are worth mentioning:
//
//     1) NOR Flash memory is guaranteed by manufacturing to ship
//        with no bad Flash blocks.
//     2) NOR Flash memory doesn't suffer from electrical leakage
//        currents (like NAND Flash) and it does not require error-correction
//        codes (ECC) to insure data integrity.
//
//------------------------------------------------------------------------------

#ifndef dimof
#define dimof(x)    (sizeof(x)/sizeof(x[0]))
#endif

//------------------------------------------------------------------------------

typedef struct {

    DWORD memBase;
    DWORD memLen;
    DWORD sectorSize;
    DWORD blockSize;

    UCHAR *pBase;
    ULONG blocks;

    ULONG reservedRegions;
    ReservedEntry reservedRegion[16];
    ULONG regions;
    FlashRegion region[16];
    ULONG firstSector[17];

} FmdInfo_t;

//------------------------------------------------------------------------------

static FmdInfo_t s_fmd;

//------------------------------------------------------------------------------

static DWORD s_blockSign[] =
    { 0xC1552106, 0xDF9C29D5, 0xBAB8EAB8, 0x82D3F9F3,
      0x3B438A47, 0xA9D92AE6, 0x09396731, 0x12BF6753 };

//------------------------------------------------------------------------------

static DWORD
FindBlockSize(
    VOID *pBase,
    ULONG size
    )
{
    OAL_FLASH_INFO flashInfo;
    ULONG blockSize = 0;
    ULONG region;
    ULONG flashSize;

    while (size > 0)
        {
        if (!OALFlashInfo(pBase, &flashInfo))
            {
            blockSize = 0;
            goto cleanUp;
            }
        for (region = 0; region < flashInfo.regions; region++)
            {
            if (flashInfo.aBlockSize[region] > blockSize)
                {
                    if ( (blockSize > 0) &&
                        ((flashInfo.aBlockSize[region] % blockSize) != 0) )
                        {
                        blockSize = 0;
                        goto cleanUp;
                        }
                    blockSize = flashInfo.aBlockSize[region];
                }
            else if (blockSize > 0)
                {
                if ((blockSize % flashInfo.aBlockSize[region]) != 0)
                    {
                    blockSize = 0;
                    goto cleanUp;
                    }
                }
            }
        flashSize = flashInfo.size * flashInfo.parallel;
        pBase = (UCHAR*)pBase + flashSize;
        if (flashSize > size) break;
        size -= flashSize;
        }

cleanUp:
    return blockSize;
}

//------------------------------------------------------------------------------

static BOOL
VerifyBlockSignature(
    BLOCK_ID block
    )
{
    int rc;
    UCHAR *pBlock;

    // There is no block signature on reserved blocks
    if ((FMD_GetBlockStatus(block) & BLOCK_STATUS_RESERVED) != 0)
        {
        rc = 0;
        goto cleanUp;
        }

    // Get block end address
    pBlock = s_fmd.pBase + (block + 1) * s_fmd.blockSize;

    // Compare signature
    rc = memcmp(pBlock - sizeof(s_blockSign), s_blockSign, sizeof(s_blockSign));

cleanUp:
    return (rc == 0);
}

//------------------------------------------------------------------------------

static BOOL
BuildLayoutInfo(
    UCHAR *pLayoutSector
    )
{
    BOOL rc = FALSE;
    FlashLayoutSector *pSector = (FlashLayoutSector*)pLayoutSector;
    UCHAR *pInfo = (UCHAR*)&pSector[1];
    FlashRegion *pRegion;
    ULONG sector;
    ULONG region;

    s_fmd.reservedRegions = pSector->cbReservedEntries/sizeof(ReservedEntry);
    if (s_fmd.reservedRegions > dimof(s_fmd.reservedRegion))
        {
        OALMSG(OAL_ERROR, (L"ERROR: FMD_Init!BuildLayoutInfo: "
            L"To many reserved regions (%d)\r\n", s_fmd.reservedRegions
            ));
        goto cleanUp;
        }
    if (s_fmd.reservedRegions > 0)
        {
        memcpy(
            s_fmd.reservedRegion, pInfo,
            s_fmd.reservedRegions * sizeof(ReservedEntry)
            );
        }

    s_fmd.regions = pSector->cbRegionEntries/sizeof(FlashRegion);
    if (s_fmd.regions > dimof(s_fmd.region))
        {
        OALMSG(OAL_ERROR, (L"ERROR: FMD_Init!BuildLayoutInfo: "
            L"To many regions (%d)\r\n", s_fmd.regions
            ));
        goto cleanUp;
        }

    if (s_fmd.regions > 0)
       {
        memcpy(
            s_fmd.region, pInfo + pSector->cbReservedEntries,
            s_fmd.regions * sizeof(FlashRegion)
            );
        }

    // Build region first sector table
    if ((s_fmd.regions + 1) > dimof(s_fmd.firstSector))
        {
        OALMSG(OAL_ERROR, (L"ERROR: FMD_Init!BuildLayoutInfo: "
            L"To many regions (%d)\r\n", s_fmd.regions
            ));
        goto cleanUp;
        }
    
    pRegion = &s_fmd.region[0];
    sector = pRegion->dwStartPhysBlock * pRegion->dwSectorsPerBlock;
    for (region = 0; region < s_fmd.regions; region++)
        {
        s_fmd.firstSector[region] = sector;
        sector += pRegion->dwNumPhysBlocks * pRegion->dwSectorsPerBlock;
        pRegion++;
        }
    s_fmd.firstSector[region] = sector;

    // Verify block signatures
    pRegion = &s_fmd.region[0];
    for (region = 0; region < s_fmd.regions; region++, pRegion++)
        {
        ULONG block, count;

        if (pRegion->regionType == XIP) continue;

        block = pRegion->dwStartPhysBlock;
        count = pRegion->dwNumPhysBlocks;
        while (count-- > 0)
            {
            if (!VerifyBlockSignature(block))
                {
                OALMSG(OAL_ERROR, (L"ERROR: FMD_Init!BuildLayoutInfo: "
                    L"Failed verify block signature (block %d)\r\n", block
                    ));
                //goto cleanUp;
                }
            block++;
            }
        }

    rc = TRUE;

cleanUp:
    return rc;
}

//------------------------------------------------------------------------------

static VOID GetSectorAddresses(
    SECTOR_ADDR sector, VOID **ppSector, VOID **ppSectorInfo
    )
{
    UCHAR *pSector = NULL;
    UCHAR *pInfo = NULL;
    UCHAR *pBlock;
    ULONG region;
    FlashRegion *pRegion;
    ULONG sectorInRegion;
    ULONG sectorInBlock;
    ULONG block;


    for (region = 0; region < s_fmd.regions; region++)
        {
        // Try next region in sector isn't in range
        if ( (sector < s_fmd.firstSector[region]) ||
             (sector >= s_fmd.firstSector[region + 1]) )
            {
            continue;
            }
        pRegion = &s_fmd.region[region];

        // Calculate block, sector position in region and inblock
        sectorInRegion = sector - s_fmd.firstSector[region];
        block = sectorInRegion / pRegion->dwSectorsPerBlock;
        block += pRegion->dwStartPhysBlock;
        sectorInBlock = sectorInRegion % pRegion->dwSectorsPerBlock;
        // Block address
        pBlock = s_fmd.pBase + block * s_fmd.blockSize;
        // Sector address
        pSector = pBlock + sectorInBlock * s_fmd.sectorSize;
        // If this is a region with SectorInfo, return the address
        // of the start of sector info, which is located immediatly
        // after the sector data for all the sectors in the block.
        if (pRegion->regionType != XIP)
            {
            pInfo = pBlock + pRegion->dwSectorsPerBlock * s_fmd.sectorSize;
            pInfo += sectorInBlock * sizeof(SectorInfo);
            }
        break;
        }

    *ppSector = pSector;
    *ppSectorInfo = pInfo;
}

//------------------------------------------------------------------------------

VOID*
FMD_Init(
    LPCTSTR activePath,
    PCI_REG_INFO *pRegIn,
    PCI_REG_INFO *pRegOut
    )
{
    HANDLE hFMD = NULL;
    UCHAR *pLayoutSector;
    UCHAR *pAddress;
    ULONG sectors;


    memset(&s_fmd, 0, sizeof(s_fmd));

    // This must not be NULL
    if (pRegIn == NULL) goto cleanUp;
    
    // Map NAND registers
    s_fmd.memBase = pRegIn->MemBase.Reg[0];
    s_fmd.memLen = pRegIn->MemLen.Reg[0];

    // Map base address    
    s_fmd.pBase = OALPAtoUA(s_fmd.memBase);

    // Find block size
    s_fmd.blockSize = FindBlockSize(s_fmd.pBase, s_fmd.memLen);

    // Calculate number of blocks on device
    s_fmd.blocks = s_fmd.memLen / s_fmd.blockSize;

    //----------------------------------------------------------------------
    //  Get volume/device layout
    //----------------------------------------------------------------------

    s_fmd.reservedRegions = 0;
    s_fmd.regions = 0;
    pLayoutSector = NULL;

    // Set sector size to 512 bytes, we can modify it later
    s_fmd.sectorSize = 512;

    // Now we have to try find boot sector & flash partition table
    pAddress = s_fmd.pBase;
    sectors = s_fmd.memLen / s_fmd.sectorSize;

    while (sectors-- > 0)
        {
        UINT32 sectorSize;
        
        // Check for boot sector
        if (!IS_VALID_BOOTSEC(pAddress))
            {
            // Move to next sector
            pAddress += s_fmd.sectorSize;
            continue;
            }

        // Look for flash layout
        sectorSize = 512;
        while (sectorSize <= s_fmd.blockSize)
            {
            if (IS_VALID_FLS(pAddress + sectorSize))
                {
                pLayoutSector = pAddress;
                break;
                }
            // Multiply by two
            sectorSize <<= 1;
            }

        // Did we find flash layout sector?
        if (pLayoutSector != NULL)
            {
            // Update sector size
            s_fmd.sectorSize = sectorSize;
            break;
            }
        
        // Move to next sector and check for region table
        pAddress += s_fmd.sectorSize;

        }

    if (pLayoutSector == 0)
        {
        OALMSG(OAL_ERROR, (L"ERROR: FMD_Init: "
            L"Failed find flash layout sector\r\n"
            ));
        }
    
    // Read flash layout sector and make local copy...
    if (!BuildLayoutInfo(pLayoutSector))
        {
        OALMSG(OAL_ERROR, (L"ERROR: FMD_Init: "
            L"Failed build flash layout info\r\n"
            ));
        goto cleanUp;
        }

    // Done
    hFMD = &s_fmd;

cleanUp:
    if (hFMD == NULL) FMD_Deinit(&s_fmd);
    return hFMD;
}

//------------------------------------------------------------------------------
//
//  Function:  FMD_Deinit
//
BOOL
FMD_Deinit(
    VOID *pContext
    )
{
    return TRUE;
}

//------------------------------------------------------------------------------
//
//  Function:  FMD_GetInfo
//
BOOL
FMD_GetInfo(
    FlashInfo *pInfo
    )
{
    pInfo->flashType           = NOR;
    pInfo->dwNumBlocks         = s_fmd.blocks;
    pInfo->dwBytesPerBlock     = s_fmd.blockSize;
    pInfo->wDataBytesPerSector = (WORD)s_fmd.sectorSize;
    pInfo->wSectorsPerBlock    = (WORD)s_fmd.region[0].dwSectorsPerBlock;

    return TRUE;
}

//------------------------------------------------------------------------------
//
//  Function:  FMD_GetInfoEx
//
BOOL
FMD_GetInfoEx(
    FlashInfoEx *pInfo,
    DWORD *pRegions
    )
{
    BOOL rc = FALSE;

    if (pRegions == NULL) goto cleanUp;

    if (pInfo == NULL)
        {
        // Return required buffer size to caller
        *pRegions = s_fmd.regions;
        rc = TRUE;
        goto cleanUp;
        }

    if (*pRegions < s_fmd.regions)
        {
        goto cleanUp;
        }

    // Copy region table
    PREFAST_SUPPRESS(386, "Warning is result of API");
    memcpy(pInfo->region, s_fmd.region, s_fmd.regions * sizeof(FlashRegion));
    *pRegions = s_fmd.regions;

    pInfo->cbSize               = sizeof(FlashInfoEx);
    pInfo->flashType            = NOR;
    pInfo->dwNumBlocks          = s_fmd.blocks;
    pInfo->dwDataBytesPerSector = (WORD)s_fmd.sectorSize;
    pInfo->dwNumRegions         = s_fmd.regions;

    rc = TRUE;

cleanUp:
    return rc;
}

//------------------------------------------------------------------------------
//
//  Function:  FMD_GetPhysSectorAddr
//
VOID
FMD_GetPhysSectorAddr(
    SECTOR_ADDR sector,
    DWORD *pSector
    )
{
    VOID *pInfo;

    GetSectorAddresses(sector, &pSector, &pInfo);
}

//------------------------------------------------------------------------------
//
//  Function:  FMD_ReadSector
//
BOOL
FMD_ReadSector(
    SECTOR_ADDR sector,
    UCHAR *pBuffer,
    SectorInfo *pSectorInfo,
    DWORD sectors
    )
{
    BOOL rc = FALSE;
    VOID *pSector;
    VOID *pInfo;


    if (sectors == 0 || ((pBuffer == NULL) &&  (pSectorInfo == NULL)))
        {
        OALMSG(OAL_ERROR, (L"ERROR: FMD_ReadSector: "
            L"Invalid parameters (%d, 0x%08x, 0x%08x, %d)\r\n",
            sector, pBuffer, pSectorInfo, sectors
            ));
        goto cleanUp;
        }

    while (sectors-- > 0)
        {
        // Get sector and info location, NULL for sector means error
        GetSectorAddresses(sector, &pSector, &pInfo);
        if (pSector == NULL)
            {
            OALMSG(OAL_ERROR, (L"ERROR: FMD_ReadSector: "
                L"Invalid sector number (%d)\r\n", sector
                ));
            goto cleanUp;
            }

        // Read sector data
        if (pBuffer != NULL)
            {
            memcpy(pBuffer, pSector, s_fmd.sectorSize);
            pBuffer += s_fmd.sectorSize;
            }

        // Read sector info
        if (pSectorInfo != NULL)
            {
            if (pInfo != NULL)
                {
                memcpy(pSectorInfo, pInfo, sizeof(SectorInfo));
                }
            else
                {
                memset(pSectorInfo, 0xff, sizeof(SectorInfo));
                pSectorInfo->dwReserved1 = sector;
                }
            pSectorInfo++;
        }

        sector++;
    }

    rc = TRUE;

cleanUp:
    ASSERT(rc);
    return rc;
}

//------------------------------------------------------------------------------
//
//  Function:  FMD_WriteSector
//
BOOL
FMD_WriteSector(
    SECTOR_ADDR sector,
    UCHAR *pBuffer,
    SectorInfo *pSectorInfo,
    DWORD sectors
    )
{
    return FALSE;
}

//------------------------------------------------------------------------------
//
//  Function:  FMD_EraseBlock
//
BOOL
FMD_EraseBlock(
    BLOCK_ID block
    )
{
    return FALSE;
}

//------------------------------------------------------------------------------
//
//  Function: FMD_GetBlockStatus
//
DWORD FMD_GetBlockStatus(
    BLOCK_ID block
    )
{
    DWORD status = 0;
    ULONG region;
    SECTOR_ADDR sector;
    SectorInfo sectorInfo;


    // First check for reserved block
    for (region = 0; region < s_fmd.reservedRegions; region++)
        {
        ReservedEntry *pRegion = &s_fmd.reservedRegion[region];
        if ( (block >= pRegion->dwStartBlock) &&
             (block < (pRegion->dwStartBlock +  pRegion->dwNumBlocks)) )
            {
            status = BLOCK_STATUS_RESERVED;
            goto cleanUp;
            }
        }


    // Now find block region
    for (region = 0; region < s_fmd.regions; region++)
        {
        FlashRegion *pRegion = &s_fmd.region[region];
        if ( (block >= pRegion->dwStartPhysBlock) &&
             (block < (pRegion->dwStartPhysBlock +  pRegion->dwNumPhysBlocks)) )
            {
            break;
            }
        }

    // If we don't find it return error
    if (region >= s_fmd.regions)
        {
        status = BLOCK_STATUS_UNKNOWN;
        goto cleanUp;
        }

    // Get first block sector number
    sector = s_fmd.firstSector[region] +
        (block - s_fmd.region[region].dwStartPhysBlock) *
        s_fmd.region[region].dwSectorsPerBlock;

    // Read sector info
    if (!FMD_ReadSector(sector, NULL, &sectorInfo, 1))
        {
        status = BLOCK_STATUS_UNKNOWN;
        goto cleanUp;
        }

    // Note that we invert status bit
    if ((sectorInfo.bOEMReserved & OEM_BLOCK_RESERVED) == 0)
        {
        status |= BLOCK_STATUS_RESERVED;
        }

cleanUp:
    return status;
}

//------------------------------------------------------------------------------

BOOL
FMD_SetBlockStatus(
    BLOCK_ID block, DWORD status
    )
{
    return FALSE;
}

//------------------------------------------------------------------------------
//
//  Function:  FMD_PowerUp
//
VOID
FMD_PowerUp(
    VOID
    )
{
}

//------------------------------------------------------------------------------
//
//  Function:  FMD_PowerDown
//
VOID
FMD_PowerDown(
    VOID
    )
{
}

//------------------------------------------------------------------------------

BOOL
FMD_OEMIoControl(
    DWORD code,
    UCHAR *pInBuffer,
    DWORD inSize,
    UCHAR *pOutBuffer,
    DWORD outSize,
    DWORD *pOutSize
    )
{
    BOOL rc = TRUE;

    switch(code)
        {
        case IOCTL_FMD_GET_INTERFACE:
            {
            FMDInterface *pInterface = (FMDInterface*)pOutBuffer;
            if (pOutSize != NULL) *pOutSize = sizeof(FMDInterface);
            if ((pOutBuffer == NULL) || (outSize < sizeof(FMDInterface)))
                {
                rc = FALSE;
                break;
                }
            pInterface->cbSize = sizeof(FMDInterface);
            pInterface->pInit = FMD_Init;
            pInterface->pDeInit = FMD_Deinit;
            pInterface->pGetInfo = FMD_GetInfo;
            pInterface->pGetInfoEx = FMD_GetInfoEx;
            pInterface->pGetBlockStatus = FMD_GetBlockStatus;
            pInterface->pSetBlockStatus = FMD_SetBlockStatus;
            pInterface->pReadSector = FMD_ReadSector;
            pInterface->pWriteSector = FMD_WriteSector;
            pInterface->pEraseBlock = FMD_EraseBlock;
            pInterface->pPowerUp = FMD_PowerUp;
            pInterface->pPowerDown = FMD_PowerDown;
            pInterface->pGetPhysSectorAddr = FMD_GetPhysSectorAddr;
            pInterface->pOEMIoControl = FMD_OEMIoControl;
            }
            break;

        case IOCTL_FMD_GET_RESERVED_TABLE:
            {
            ULONG regions = s_fmd.reservedRegions;
            ULONG size;
            if (regions > dimof(s_fmd.reservedRegion))
                {
                regions = dimof(s_fmd.reservedRegion);
                }
            size = regions * sizeof(ReservedEntry);
            if (pOutSize != NULL) *pOutSize = size;
            if ((pOutBuffer == NULL) || (outSize < size))
                {
                rc = FALSE;
                break;
                }
            memcpy(pOutBuffer, s_fmd.reservedRegion, size);
            }
            break;

        case IOCTL_FMD_GET_RAW_BLOCK_SIZE:
            if (pOutSize != NULL) *pOutSize = sizeof(DWORD);
            if ((pOutBuffer == NULL) || (outSize < sizeof(DWORD)))
                {
                rc = FALSE;
                break;
                }
            *((DWORD*)pOutBuffer) = s_fmd.blockSize;
            break;

        case IOCTL_FMD_GET_INFO:
            {
            FMDInfo *pInfo = (FMDInfo*)pOutBuffer;
            if ((pOutBuffer == NULL) || (outSize < sizeof(FMDInfo)))
                {
                rc = FALSE;
                break;
                }
            pInfo->flashType = NOR;
            pInfo->dwBaseAddress = s_fmd.memBase;
            pInfo->dwNumRegions = s_fmd.regions;
            pInfo->dwNumReserved = s_fmd.reservedRegions;
            }
            break;

        default:
            rc = FALSE;
            break;
    }

    return rc;
}

//------------------------------------------------------------------------------
