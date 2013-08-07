// All rights reserved ADENEO EMBEDDED 2010
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
//  This file implements CFI NOR flash memory erase and write functions.
//
#pragma warning(push)
#pragma warning(disable: 4214 4201 4115)
#include <windows.h>
#include <oal.h>
#pragma warning(pop)

//------------------------------------------------------------------------------

static struct {
    BOOL pending;
    OAL_FLASH_INFO info;
    UINT32 base;
    UINT32 end;
    UINT32 pos;
    UINT32 region;
    UINT32 block;
} s_erase;

//------------------------------------------------------------------------------

static UCHAR
BitsOr(
    OAL_FLASH_INFO *pInfo,
    DWORD base, 
    ULONG offset
    )
{
    UCHAR bits = 0;
    DWORD data;    
    ULONG width = pInfo->width;
    ULONG parallel = pInfo->parallel;

    switch (width)
        {
        case 4:
            data = INREG32((UINT32*)(base + (offset << 2)));
            switch (parallel)
                {
                case 4:
                    bits  = (UCHAR)(((data >> 24)&0xFF) | ((data >> 16)&0xFF));
                    bits |= (UCHAR)(((data >>  8)&0xFF) | ((data >>  0)&0xFF));
                    break;
                case 2:
                    bits  = (UCHAR)(((data >> 16)&0xFF) | ((data >> 0)&0xFF));
                    break;                    
                case 1:
                    bits = (UCHAR)(data&0xFF);
                    break;
                }
            break;
        case 2:
            data = INREG16((UINT16*)(base + (offset << 1)));
            switch (parallel)
                {
                case 2:
                    bits = (UCHAR)(((data >> 8)&0xFF) | ((data >> 0)&0xFF));
                    break;                    
                case 1:
                    bits = (UCHAR)(data&0xFF);
                    break;
                }
            break;
        case 1:
            bits = INREG8((UINT8*)(base + offset));
            break;
        }            

    return bits;   
}

//------------------------------------------------------------------------------

static UCHAR
BitsAnd(
    OAL_FLASH_INFO *pInfo,
    DWORD base,
    ULONG offset
    )
{
    UINT8 bits = 0;
    UINT32 data;    
    ULONG width = pInfo->width;
    ULONG parallel = pInfo->parallel;

    switch (width)
        {
        case 4:
            data = INREG32((UINT32*)(base + (offset << 2)));
            switch (parallel)
                {
                case 4:
                    bits  = (UCHAR)(((data >> 24)&0xFF) & ((data >> 16)&0xFF));
                    bits &= (UCHAR)(((data >>  8)&0xFF) & ((data >>  0)&0xFF));
                    break;
                case 2:
                    bits = (UCHAR)(((data >> 16)&0xFF) & ((data >> 0)&0xFF));
                    break;                    
                case 1:
                    bits = (UCHAR)(data&0xFF);
                    break;
                }
            break;
        case 2:
            data = INREG16((UINT16*)base);
            switch (parallel)
                {
                case 2:
                    bits  = (UCHAR)(((data >> 8)&0xFF) & ((data >> 0) & 0xFF));
                    break;                    
                case 1:
                    bits = (UCHAR)(data&0xFF);
                    break;
                }
            break;
        case 1:
            bits = INREG8((UINT8*)base);
            break;
        }            

    return bits;   
}

//------------------------------------------------------------------------------

static UINT32
ReadInfo(
    OAL_FLASH_INFO *pInfo,
    DWORD base, 
    ULONG offset, 
    ULONG shift,
    ULONG size
    )
{
    DWORD data = 0;
    ULONG count = (INT32)size;
    
    switch (pInfo->width)
        {
        case 4:        
            if (shift >= 4) shift = 0;
            while (count-- > 0)
                {
                data <<= 8;
                data |= INREG32(
                        (UINT32*)(base + ((offset + count) << 2) + shift)
                    ) & 0xFF;
                }
            break;
        case 2:        
            if (shift >= 2) shift = 0;
            while (count-- > 0)
                {
                data <<= 8;
                data |= INREG16(
                        (UINT16*)(base + ((offset + count) << 1) + shift)
                    ) & 0xFF;
                }
            break;
        case 1:
            while (count-- > 0)
                {
                data <<= 8;
                data |= INREG8((UINT8*)(base + offset + count));
                }
            break;
        }

    return data;            
}

//------------------------------------------------------------------------------

static VOID
WriteCommand(
    OAL_FLASH_INFO *pInfo,
    DWORD base, 
    ULONG offset,
    UCHAR cmd
    )
{
    DWORD code;
    ULONG width = pInfo->width;
    ULONG parallel = pInfo->parallel;
        
    switch (width)
        {
        case 4:
            if (parallel == 4)
                {
                code = cmd | (cmd << 8) | (cmd << 16) | (cmd << 24);
                }
            else if (parallel == 2) 
                {
                code = cmd | (cmd << 16);
                }
            else if (parallel == 1)
                {
                code = cmd;
                }
            else
                {
                break;
                }
            OUTREG32((UINT32*)(base + (offset << 2)), code);
            break;
        case 2:
            if (parallel == 2)
                {
                code = cmd | (cmd << 8);
                }
            else if (parallel == 1)
                {
                code = cmd;
                }
            else
                {
                break;
                }
            OUTREG16((UINT16*)(base + (offset << 1)), code);
            break;
        case 1:
            if (parallel != 1) break;
            code = cmd;
            OUTREG8((UINT8*)(base + offset), code);
            break;
        }
}

//------------------------------------------------------------------------------
//  Command Set 1 support (Intel)
//------------------------------------------------------------------------------

#ifndef OAL_FLASH_NO_COMMAND_SET_1

//------------------------------------------------------------------------------

static BOOL
LockBlock1(
    OAL_FLASH_INFO *pInfo,
    DWORD base, 
    DWORD block, 
    BOOL lock
    )
{
    UCHAR command;
    UCHAR bits;

	UNREFERENCED_PARAMETER(base);

    command = lock ? 0x01 : 0xD0;
    
    // Set block lock
    WriteCommand(pInfo, block, 0, 0x60);
    WriteCommand(pInfo, block, 0, command);

    // Verify it is correct
    WriteCommand(pInfo, block, 0x02, 0x90);
    if (lock)
        {
        bits = BitsAnd(pInfo, block, 0x02);
        }
    else
        {
        bits = BitsOr(pInfo, block, 0x02);
        }
    
    // Reset memory back to normal state
    WriteCommand(pInfo, block, 0, 0xFF);

    return (((bits & (1 << 0)) != 0) == lock);
}

//------------------------------------------------------------------------------

static BOOL
LockDownBlock1(
    OAL_FLASH_INFO *pInfo,
    DWORD base, 
    DWORD block
    )
{
    UCHAR bits;

	UNREFERENCED_PARAMETER(base);

    // Set block lock-down
    WriteCommand(pInfo, block, 0, 0x60);
    WriteCommand(pInfo, block, 0, 0x2F);

    // Verify it is correct
    WriteCommand(pInfo, block, 0x02, 0x90);
    bits = BitsAnd(pInfo, block, 0x02);
    
    // Reset memory back to normal state
    WriteCommand(pInfo, block, 0, 0xFF);

    return ((bits & (1 << 1)) != 0);
}

//------------------------------------------------------------------------------

static BOOL
EraseBlock1(
    OAL_FLASH_INFO *pInfo,
    DWORD base, 
    DWORD block
    )
{
    BOOL rc;
    UCHAR bits;

    // Start block reset
    WriteCommand(pInfo, base, 0, 0x50);
    WriteCommand(pInfo, block, 0, 0x20);
    WriteCommand(pInfo, block, 0, 0xD0);

    // Wait until it is done
    do {
        WriteCommand(pInfo, base, 0, 0x70);
        bits = BitsAnd(pInfo, base, 0);
       }
    while ((bits & (1 << 7)) == 0);

    // Reset memory back to normal state
    WriteCommand(pInfo, base, 0, 0xFF);

    // Bit 5 is zero if erase succeeded
    WriteCommand(pInfo, base, 0, 0x70);
    bits = BitsOr(pInfo, base, 0);
    rc = (bits & (1 << 5)) == 0;
    
    // Switch back to read mode
    WriteCommand(pInfo, base, 0, 0xFF);

    return rc;
}

//------------------------------------------------------------------------------

static BOOL
StartEraseBlock1(
    OAL_FLASH_INFO *pInfo,
    DWORD base, 
    DWORD block
    )
{
	UNREFERENCED_PARAMETER(base);

    // Erase status register
    WriteCommand(pInfo, block, 0, 0x50);

    // Start block reset
    WriteCommand(pInfo, block, 0, 0x20);
    WriteCommand(pInfo, block, 0, 0xD0);

    return TRUE;
}

//------------------------------------------------------------------------------

static UINT32
ContinueEraseBlock1(
    OAL_FLASH_INFO *pInfo,
    DWORD base, 
    DWORD block
    )
{
    UINT32 rc = OAL_FLASH_ERASE_PENDING;
    UINT8 bits;

	UNREFERENCED_PARAMETER(base);

    // Check if erase is done
    WriteCommand(pInfo, block, 0, 0x70);
    bits = BitsAnd(pInfo, block, 0);
    if ((bits & (1 << 7)) == 0) goto cleanUp;
        
    // Bit 5 is zero if erase succeeded
    rc = (bits & (1 << 5)) == 0 ? OAL_FLASH_ERASE_DONE : OAL_FLASH_ERASE_FAILED;
    
cleanUp:
    // Switch back to read mode
    WriteCommand(pInfo, block, 0, 0xFF);
    return rc;
}

//------------------------------------------------------------------------------

static UINT32
WriteData1(
    OAL_FLASH_INFO *pInfo,
    ULONG base, 
    ULONG pos, 
    VOID *pBuffer
    )
{
    UINT32 size = 0;
    UINT8 bits;

    // Issue write command
    WriteCommand(pInfo, base, 0, 0x40);
   
    // Now write info and wait until it is done
    switch (pInfo->width)
        {
        case 4:
            OUTREG32((UINT32*)pos, *(UINT32*)pBuffer);
            break;
        case 2:
            OUTREG16((UINT16*)pos, *(UINT16*)pBuffer);
            break;
        case 1:
            OUTREG8((UINT8*)pos, *(UINT8*)pBuffer);
            break;
        }

    // Wait until write is done
    WriteCommand(pInfo, base, 0, 0x70);
    bits = BitsAnd(pInfo, pos, 0);
    while ((bits & (1 << 7)) == 0)
        {
        WriteCommand(pInfo, base, 0, 0x70);
        bits = BitsAnd(pInfo, pos, 0);
        }
    
    // Bit 4 is zero if write succeeded
    if ((bits & (1 << 4)) == 0) size = pInfo->width;

    // Reset memory back to normal state
    WriteCommand(pInfo, base, 0, 0xFF);

    // Return result
    return size;
}

#endif OAL_FLASH_NO_COMMAND_SET_1

//------------------------------------------------------------------------------
//  Command Set 2 support (AMD)
//------------------------------------------------------------------------------

#ifndef OAL_FLASH_NO_COMMAND_SET_2

//------------------------------------------------------------------------------

static BOOL
LockBlock2(
    OAL_FLASH_INFO *pInfo,
    UINT32 base, 
    UINT32 block, 
    BOOL lock
    )
{
	UNREFERENCED_PARAMETER(pInfo);
	UNREFERENCED_PARAMETER(base);
	UNREFERENCED_PARAMETER(block);
	UNREFERENCED_PARAMETER(lock);

    return TRUE;
}

//------------------------------------------------------------------------------

static BOOL
LockDownBlock2(
    OAL_FLASH_INFO *pInfo,
    UINT32 base, 
    UINT32 block
    )
{
	UNREFERENCED_PARAMETER(pInfo);
	UNREFERENCED_PARAMETER(base);
	UNREFERENCED_PARAMETER(block);

    return TRUE;
}

//------------------------------------------------------------------------------

static BOOL
EraseBlock2(
    OAL_FLASH_INFO *pInfo,
    UINT32 base, 
    UINT32 block
    )
{
    UINT32 code;
    UINT8 bits;
   
    // Start block reset
    WriteCommand(pInfo, base, 0x0555, 0xAA);
    WriteCommand(pInfo, base, 0x02AA, 0x55);
    WriteCommand(pInfo, base, 0x0555, 0x80);
    WriteCommand(pInfo, base, 0x0555, 0xAA);
    WriteCommand(pInfo, base, 0x02AA, 0x55);
    WriteCommand(pInfo, block, 0, 0x30);
   
    // Wait until it is done
    for (;;)
    {
        bits = BitsAnd(pInfo, block, 0);
        if ((bits & (1 << 5)) == 0)
			continue;
        code = INREG32((UINT32*)block);
        break;
    }

    // Switch back to read mode
    WriteCommand(pInfo, base, 0, 0xF0);

    return (code == 0xFFFFFFFF);
}

//------------------------------------------------------------------------------

static BOOL
StartEraseBlock2(
    OAL_FLASH_INFO *pInfo,
    DWORD base, 
    DWORD block
    )
{
	UNREFERENCED_PARAMETER(pInfo);
	UNREFERENCED_PARAMETER(base);
	UNREFERENCED_PARAMETER(block);

    return FALSE;
}

//------------------------------------------------------------------------------

static UINT32
ContinueEraseBlock2(
    OAL_FLASH_INFO *pInfo,
    DWORD base, 
    DWORD block
    )
{
	UNREFERENCED_PARAMETER(pInfo);
	UNREFERENCED_PARAMETER(base);
	UNREFERENCED_PARAMETER(block);

    return (UINT32)OAL_FLASH_ERASE_FAILED;
}

//------------------------------------------------------------------------------

static UINT32
WriteData2(
    OAL_FLASH_INFO *pInfo, 
    UINT32 base, 
    UINT32 pos, 
    VOID *pBuffer
    )
{
    UINT32 size = 0;
    UINT32 code;
    UINT8 bits;

    // Set flash memory to write mode
    WriteCommand(pInfo, base, 0x555, 0xAA);
    WriteCommand(pInfo, base, 0x2AA, 0x55);
    WriteCommand(pInfo, base, 0x555, 0xA0);

    // Now write info and wait until it is done
    switch (pInfo->width)
        {
        case 4:
            OUTREG32((UINT32*)pos, *(UINT32*)pBuffer);
            for (;;)
            {
                bits = BitsAnd(pInfo, pos, 0);
                if ((bits & (1 << 5)) == 0)
					continue;
                code = INREG32((UINT32*)pos);
                break;
            }
            if (code == *(UINT32*)pBuffer) size = sizeof(UINT32);
            break;
        case 2:
            OUTREG16((UINT16*)pos, *(UINT16*)pBuffer);
            for (;;)
            {
                bits = BitsAnd(pInfo, pos, 0);
                if ((bits & (1 << 5)) == 0)
					continue;
                code = INREG16((UINT16*)pos);
                break;
            }
            if (code == *(UINT16*)pBuffer) size = sizeof(UINT16);
            break;
        case 1:
            OUTREG8((UINT8*)pos, *(UINT8*)pBuffer);
            for (;;)
            {
                bits = BitsAnd(pInfo, pos, 0);
                if ((bits & (1 << 5)) == 0)
					continue;
                code = INREG8((UINT8*)pos);
                break;
            }
            if (code == *(UINT8*)pBuffer) size = sizeof(UINT8);
            break;
        }

    return size;
}

#endif OAL_FLASH_NO_COMMAND_SET_2

//------------------------------------------------------------------------------
//  Common code
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
//
//  Function:  OALFlashInfo
//
//  This function read NOR flash memory layout via CFI. 
//
//  Note: please pass through Uncached Address (pBase)!
//
BOOL
OALFlashInfo(
    VOID *pBase, 
    OAL_FLASH_INFO *pInfo
    )
{
    BOOL rc = FALSE;
    UINT32 base = (UINT32)pBase;
    UINT32 code1, code2, code3;
    UINT32 i;
    
    OALMSG(OAL_FUNC, (L"+OALFlashInfo(0x%08x, ...)\r\n", pBase));

    // Just to avoid possible problems
    memset(pInfo, 0, sizeof(*pInfo));

    // Try 32-bit geometry
    OUTREG32((UINT32*)(base + 4 * 0x55), 0x98989898);
    code1 = INREG32((UINT32*)(base + 4 * 0x10));
    code2 = INREG32((UINT32*)(base + 4 * 0x11));
    code3 = INREG32((UINT32*)(base + 4 * 0x12));
    if (code1 == 'QQQQ' && code2 == 'RRRR' && code3 == 'YYYY')
        {
        pInfo->width = 4;
        pInfo->parallel = 4;
        }
    else if (code1 == 'Q\0Q\0' && code2 == 'R\0R\0' && code3 == 'Y\0Y\0')
        {
        pInfo->width = 4;
        pInfo->parallel = 2;
        }
    else if (code1 == 'Q\0\0\0' && code2 == 'R\0\0\0' && code3 == 'Y\0\0\0')
        {
        pInfo->width = 4;
        pInfo->parallel = 1;
        }
    else 
        {
        // Now try luck with 16-bit geometry
        OUTREG16((UINT16*)(base + 2 * 0x55), 0x0098);
        code1 = INREG16((UINT16*)(base + 2 * 0x10));
        code2 = INREG16((UINT16*)(base + 2 * 0x11));
        code3 = INREG16((UINT16*)(base + 2 * 0x12));
        if (code1 == 'QQ' && code2 == 'RR' && code3 == 'YY')
            {
            pInfo->width = 2;
            pInfo->parallel = 2;
            }
        else if (code1 == '\0Q' && code2 == '\0R' && code3 == '\0Y')
            {
            pInfo->width = 2;
            pInfo->parallel = 1;
            }
        else 
            {
            // So last opportunity is 8-bit mode
            OUTREG8((UINT8*)(base + 0x55), 0x98);
            code1 = INREG8((UINT8*)(base + 0x10));
            code2 = INREG8((UINT8*)(base + 0x11));
            code3 = INREG8((UINT8*)(base + 0x12));
            if (code1 == 'Q' && code2 == 'R' && code3 == 'Y')
                {
                pInfo->width = 1;
                pInfo->parallel = 1;
                }
            else 
                {
                goto cleanUp;
                }                
            }
        }        

    // Read primary command set, size, burst size and number of regions
    pInfo->set = ReadInfo(pInfo, base, 0x13, 0, 1);
    pInfo->size = 1 << ReadInfo(pInfo, base, 0x27, 0, 1);
    pInfo->burst = 1 << ReadInfo(pInfo, base, 0x2A, 0, 1);
    pInfo->regions = ReadInfo(pInfo, base, 0x2C, 0, 1);

    // Verify that parallel chips are same
    for (i = 1; i < pInfo->parallel; i++)
        {
            if ( (pInfo->set != ReadInfo(pInfo, base, 0x13, i, 1)) ||
                 (pInfo->size != (UINT32)(1 << ReadInfo(pInfo, base, 0x27, 0, 1))) ||
                 (pInfo->burst != (UINT32)(1 << ReadInfo(pInfo, base, 0x2A, 0, 1))) ||
                 (pInfo->regions != ReadInfo(pInfo, base, 0x2C, 0, 1)) )
                {
                    goto cleanUp;
                }
            
        }
    
    // If there is more regions than expected
    if (pInfo->regions > 8) goto cleanUp;
   
    // Read region info
    for (i = 0; i < pInfo->regions; i++)
        {
        code1 = ReadInfo(pInfo, base, 0x2d + (i << 2), 0, 4);
        pInfo->aBlocks[i] = (code1 & 0xFFFF) + 1;
        pInfo->aBlockSize[i] = (code1 >> 8) & 0x00FFFF00;
        if (pInfo->aBlockSize[i] == 0) pInfo->aBlockSize[i] = 128;
        }

    // Switch back to read mode
    switch (pInfo->set)
        {
        case 1:  // Intel/Sharp
        case 3:
            WriteCommand(pInfo, base, 0, 0xFF);
            break;
        case 2:  // AMD/Fujitsu
            WriteCommand(pInfo, base, 0, 0xF0);
            break;
        }      

    rc = TRUE;

cleanUp:
    OALMSG(OAL_FUNC, (L"-OALFlashInfo(rc = %d)\r\n", rc));
    return rc;
}

//------------------------------------------------------------------------------

BOOL
OALFlashTiming(
    VOID *pBase, 
    OAL_FLASH_TIMING *pTimingInfo
    )
{
    BOOL rc = FALSE;
    UINT32 base = (UINT32)pBase;
    OAL_FLASH_INFO info;

    // Ther read first chip info
    if (!OALFlashInfo((VOID*)base, &info)) goto cleanUp;

    // Switch to CFI Query
    WriteCommand(&info, base, 0x55, 0x98);
    
    // Read timeout parameters
    pTimingInfo->writeDelay    = ReadInfo(&info, base, 0x1F, 0, 1);
    pTimingInfo->writeTimeout  = ReadInfo(&info, base, 0x23, 0, 1);
    pTimingInfo->bufferDelay   = ReadInfo(&info, base, 0x20, 0, 1);
    pTimingInfo->bufferTimeout = ReadInfo(&info, base, 0x24, 0, 1);
    pTimingInfo->eraseDelay    = ReadInfo(&info, base, 0x21, 0, 1);
    pTimingInfo->eraseTimeout  = ReadInfo(&info, base, 0x25, 0, 1);

    // Switch back to read mode
    switch (info.set)
        {
        case 1:  // Intel/Sharp
        case 3:
            WriteCommand(&info, base, 0, 0xFF);
            break;
        case 2:  // AMD/Fujitsu
            WriteCommand(&info, base, 0, 0xF0);
            break;
        }      

    rc = TRUE;
    
cleanUp:
    return rc;
}

//------------------------------------------------------------------------------
//
//  Function:  OALFlashLock
//
//  This function locks/unlocks region on NOR flash memory. 
//
BOOL 
OALFlashLock(
    VOID *pBase,
    VOID *pStart,
    DWORD size,
    BOOL lock
    )
{
    BOOL rc = FALSE;
    OAL_FLASH_INFO info;
    UINT32 base = (UINT32)pBase;
    UINT32 start = (UINT32)pStart;
    UINT32 end, chip, blockStart, blockEnd;
    UINT32 region, block;


    // First get end addresss
    end = start + size;
   
    // Ther read first chip info
    if (!OALFlashInfo((VOID*)base, &info))
        {
            goto cleanUp;
        }

    region = block = 0;
    blockStart = chip = base;
    while (blockStart < end)
        {

        // Block end
        blockEnd = blockStart + info.aBlockSize[region] * info.parallel;

        // Should block be erased?
        if (start < blockEnd && end >= blockStart)
            {
            switch (info.set)
                {

#ifndef OAL_FLASH_NO_COMMAND_SET_1                
                case 1: // Intel/Sharp
                case 3:
                    if (!LockBlock1(&info, chip, blockStart, lock))
                        {
                        goto cleanUp;
                        }
                    break;
#endif OAL_FLASH_NO_COMMAND_SET_1                
                    
#ifndef OAL_FLASH_NO_COMMAND_SET_2
                case 2: // AMD
                    if (!LockBlock2(&info, chip, blockStart, lock))
                        {
                        goto cleanUp;
                        }                    
                    break;
#endif OAL_FLASH_NO_COMMAND_SET_2
                default:
                    goto cleanUp;
                }
            }         

        // Move to next block
        blockStart = blockEnd;
        if (blockStart >= end) break;
        if (++block >= info.aBlocks[region])
            {
            block = 0;
            if (++region >= info.regions)
                {
                // Try read next chip layout
                if (!OALFlashInfo((VOID*)block, &info)) break;
                region = 0;
                chip = block;
                }
            }
        }

    rc = TRUE;
   
cleanUp:
    return rc;
}

//------------------------------------------------------------------------------
//
// Function: OALFlashErase
//
// Note: please pass through Uncached Addresses (pBase, pStart)!
//
BOOL OALFlashErase(
    VOID *pBase,
    VOID *pStart,
    UINT32 size
    )
{
    BOOL rc = FALSE;
    OAL_FLASH_INFO info;
    UINT32 base = (UINT32)pBase;
    UINT32 start = (UINT32)pStart;
    UINT32 end = start + size;
    UINT32 chip, blockStart, blockEnd;
    UINT32 region, block;

    OALMSG(OAL_FUNC, (
        L"+OALFlashErase(0x%08x, 0x%08x, 0x%08x)\r\n", pBase, pStart, size
    ));

    // Read first chip layout
    if (!OALFlashInfo((VOID*)base, &info))
    {
        OALMSG(OAL_ERROR, (
            L"ERROR: OALFlashErase failed get flash memory info\r\n"
        ));
            goto cleanUp;
    }

    region = block = 0;
    blockStart = chip = base;
    while (blockStart < end)
        {

        // Block end (+1)
        blockEnd = blockStart + info.aBlockSize[region] * info.parallel;

        // Should block be erased?
        if (start < blockEnd && end >= blockStart)
            {
            switch (info.set)
                {

#ifndef OAL_FLASH_NO_COMMAND_SET_1                
                case 1: // Intel/Sharp
                case 3:
                    if (!EraseBlock1(&info, chip, blockStart))
                        {
                        goto cleanUp;
                        }
                    break;
#endif OAL_FLASH_NO_COMMAND_SET_1                

#ifndef OAL_FLASH_NO_COMMAND_SET_2
                case 2: // AMD
                    if (!EraseBlock2(&info, chip, blockStart))
                        {
                        goto cleanUp;
                        }                    
                    break;
#endif OAL_FLASH_NO_COMMAND_SET_2

                default:
                OALMSG(OAL_ERROR, (
                    L"ERROR: Flash type %d isn't supported\r\n", info.set
                ));
                    goto cleanUp;
                }
            }         

        // Move to next block
        blockStart = blockEnd;
        if (blockStart >= end) break;
        if (++block >= info.aBlocks[region])
            {
            block = 0;
            if (++region >= info.regions)
                {
                // Try read next chip info
                if (!OALFlashInfo((VOID*)blockStart, &info)) break;
                region = 0;
                chip = blockStart;
                }
            }
        }

    rc = TRUE;
   
cleanUp:
    OALMSG(OAL_FUNC, (L"-OALFlashErase(rc = %d)\r\n", rc));
    return rc;
}

//------------------------------------------------------------------------------

BOOL 
OALFlashEraseStart(
    VOID *pBase,
    VOID *pStart,
    UINT32 size
    )
{
    BOOL rc = FALSE;
    OAL_FLASH_INFO info;
    UINT32 base = (UINT32)pBase;
    UINT32 start = (UINT32)pStart;
    UINT32 end = start + size;
    UINT32 chip, blockStart, blockEnd;
    UINT32 region, block;

    // There can be only one pending erase
    if (s_erase.pending) goto cleanUp;

    // First detect flash at base
    if (!OALFlashInfo((VOID*)base, &info))
        {
        goto cleanUp;
        }

    // Find first block to be erased
    region = block = 0;
    blockStart = chip = base;
    while (blockStart < end)
        {

        // Block end (+1)
        blockEnd  = blockStart + info.aBlockSize[region] * info.parallel;

        // Should block be erased?
        if (start < blockEnd && end >= blockStart) break;

        // This should not happen...
        if (blockEnd >= end) goto cleanUp;

        // Move to next block
        blockStart = blockEnd;
        if (++block >= info.aBlocks[region])
            {
            block = 0;
            if (++region >= info.regions)
                {
                // Try read next chip info
                if (!OALFlashInfo((VOID*)block, &info)) goto cleanUp;
                region = 0;
                chip = block;
                }
            }
        }

    // Start erase
    switch (info.set)
        {

#ifndef OAL_FLASH_NO_COMMAND_SET_1                
        case 1: // Intel/Sharp
        case 3:
            if (!StartEraseBlock1(&info, chip, blockStart))
                {
                goto cleanUp;
                }
            break;
#endif OAL_FLASH_NO_COMMAND_SET_1                

#ifndef OAL_FLASH_NO_COMMAND_SET_2
        case 2: // AMD
            if (!StartEraseBlock2(&info, chip, blockStart))
                {
                goto cleanUp;
                }                    
            break;
#endif OAL_FLASH_NO_COMMAND_SET_2

        default:
            goto cleanUp;
        }


    // Save context for continue...
    s_erase.info = info;
    s_erase.base = chip;
    s_erase.end = end;
    s_erase.pos = blockStart;
    s_erase.region = region;
    s_erase.block = block;
    s_erase.pending = TRUE;
    
    rc = TRUE;

cleanUp:
    return rc;
}

//------------------------------------------------------------------------------

UINT32 
OALFlashEraseContinue(
    )
{
    UINT32 rc = (UINT32)OAL_FLASH_ERASE_FAILED;
    OAL_FLASH_INFO *pInfo = &s_erase.info;
    
    // There must be pending erase
    if (!s_erase.pending) goto cleanUp;

    // Look if erase is done
    switch (s_erase.info.set)
        {

#ifndef OAL_FLASH_NO_COMMAND_SET_1                
        case 1: // Intel/Sharp
        case 3:
            rc = ContinueEraseBlock1(&s_erase.info, s_erase.base, s_erase.pos);
            break;
#endif OAL_FLASH_NO_COMMAND_SET_1                

#ifndef OAL_FLASH_NO_COMMAND_SET_2
        case 2: // AMD
            rc = ContinueEraseBlock2(&s_erase.info, s_erase.base, s_erase.pos);
            break;
#endif OAL_FLASH_NO_COMMAND_SET_2

        default:
            goto cleanUp;
        }

    // If erase is pending or failed we are done
    if (rc != OAL_FLASH_ERASE_DONE) goto cleanUp;

    // Move to next block
    s_erase.pos += pInfo->aBlockSize[s_erase.region] * pInfo->parallel;
    // Are we done?
    if (s_erase.pos >= s_erase.end) goto cleanUp;

    // Is is next chip?
    if (++s_erase.block >= pInfo->aBlocks[s_erase.region])
        {
        s_erase.block = 0;
        if (++s_erase.region >= pInfo->regions)
            {
            s_erase.base = s_erase.pos;
            // Try read next chip info
            if (!OALFlashInfo((VOID*)s_erase.base, pInfo)) goto cleanUp;
            s_erase.region = 0;
            }
        }
    
cleanUp:
    if (rc != OAL_FLASH_ERASE_PENDING) s_erase.pending = FALSE;    
    return rc;
}

//------------------------------------------------------------------------------
//
// Function: OALFlashWrite
//
// Note: please pass through Uncached Address (pBase, pStart)!
//
BOOL
OALFlashWrite(
    VOID *pBase, 
    VOID *pStart, 
    UINT32 size, 
    VOID *pBuffer
    )
{
    BOOL rc = FALSE;
    OAL_FLASH_INFO info;
    DWORD base = (DWORD)pBase;
    DWORD start = (DWORD)pStart;
    DWORD end = start + size;
    DWORD chipStart;
    DWORD chipEnd;
    DWORD pos, count;
    UCHAR *pPos;

    OALMSG(OAL_FUNC, (
        L"+OALFlashWrite(0x%08x, 0x%08x, 0x%08x, 0x%08x)\r\n", 
        pBase, pStart, size, pBuffer
    ));

    chipStart = base;
    pPos = (UCHAR*)pBuffer;

    // First read first chip info
    for (;;)
    {
        if (!OALFlashInfo((VOID*)chipStart, &info))
        {
			OALMSG(OAL_ERROR, (
                L"ERROR: OALFlashWrite - failed get flash info at 0x%08x\r\n",
                chipStart
            ));
            goto cleanUp;
        }
        chipEnd = chipStart + info.size * info.parallel;
        // Is start address on this chip
        if (start >= chipStart && start < chipEnd) break;
        // Move to next chip
        chipStart = chipEnd;
    }
   
    pos = start;
    while (pos < end)
        {

        // How many data we can write
        if (end > chipEnd)
            {
            count = chipEnd - pos;
            }
        else
            {
            count = end - pos;
            }
        
        // Program data chunk
        switch (info.set)
            {

#ifndef OAL_FLASH_NO_COMMAND_SET_1                
            case 1:
            case 3:
                count = WriteData1(&info, chipStart, pos, pPos);
                break;
#endif OAL_FLASH_NO_COMMAND_SET_1

#ifndef OAL_FLASH_NO_COMMAND_SET_2
            case 2:
                count = WriteData2(&info, chipStart, pos, pPos);
                break;
#endif OAL_FLASH_NO_COMMAND_SET_2

            default:
            OALMSG(OAL_ERROR, (
                L"ERROR: Flash type %d isn't supported\r\n", info.set
            ));
                goto cleanUp;
            }

        // If we write nothing something wrong happen
        if (count == 0)
            {
            rc = FALSE;
            OALMSG(OAL_ERROR, (
                L"ERROR: Flash write at 0x%08x failed\r\n", pos
            ));
            goto cleanUp;
            }
      
        // Move position
        pos += count;
        pPos += count;

        // Break when we are done
        if (pos >= end) break;
        
        // If we run out of chip move to next one
        if (pos > chipEnd)
            {
            switch(info.set) {
                case 1:
                case 3:
                    WriteCommand(&info, chipStart, 0, 0xFF);
                    break;
                case 2:
                    WriteCommand(&info, chipStart, 0, 0xF0);
                    break;
                }
            chipStart = chipEnd;
            if (!OALFlashInfo((VOID*)chipStart, &info)) break;
            chipEnd = chipStart + info.size * info.parallel;
            }         
        }

    // Done
    //rc = TRUE;
    switch (info.set) {
    case 1:
    case 3:
        WriteCommand(&info, chipStart, 0, 0xFF);
        break;
    case 2:
        WriteCommand(&info, chipStart, 0, 0xF0);
        break;
    }

    // Do final check
    pPos = (UINT8*)pBuffer;
    for (pos = start; pos < end - sizeof(UINT32) + 1; pos += sizeof(UINT32)) {
        if (*(UINT32*)pPos != *(UINT32*)pos) break;
        pPos += sizeof(UINT32);
    }

    // If we reach end, all is ok
    rc = (pos >= end - sizeof(UINT32) + 1);
    OALMSG(!rc&&OAL_ERROR, (
        L"ERROR: Flash failed at 0x%08x -- write 0x%08x, but read 0x%08x\r\n",
        pos, *(UINT32*)pPos, *(UINT32*)pos
    ));
    
cleanUp:
    OALMSG(OAL_FUNC, (L"-OALFlashWrite(rc = %d)", rc));
    return rc;
}

//------------------------------------------------------------------------------

BOOL 
OALFlashLockDown(
    VOID *pBase,
    VOID *pStart,
    DWORD size
    )
{
    BOOL rc = FALSE;
    OAL_FLASH_INFO info;
    UINT32 base = (UINT32)pBase;
    UINT32 start = (UINT32)pStart;
    UINT32 end = start + size;
    UINT32 chip, blockStart, blockEnd;
    UINT32 region, block;


    // Ther read first chip info
    if (!OALFlashInfo((VOID*)base, &info))
        {
            goto cleanUp;
        }

    region = block = 0;
    blockStart = chip = base;
    while (blockStart < end)
        {

        // Block end (+1)
        blockEnd = blockStart + info.aBlockSize[region] * info.parallel;

        // Should block be erased?
        if (start < blockEnd && end >= blockStart)
            {
            switch (info.set)
                {

#ifndef OAL_FLASH_NO_COMMAND_SET_1
                case 1: // Intel/Sharp
                case 3:
                    if (!LockDownBlock1(&info, chip, blockStart))
                        {
                        goto cleanUp;
                        }
                    break;
#endif OAL_FLASH_NO_COMMAND_SET_1

#ifndef OAL_FLASH_NO_COMMAND_SET_2
                case 2: // AMD
                    if (!LockDownBlock2(&info, chip, blockStart))
                        {
                        goto cleanUp;
                        }                    
                    break;
#endif OAL_FLASH_NO_COMMAND_SET_2

                default:
                    goto cleanUp;
                }
            }         

        // Move to next block
        blockStart = blockEnd;
        if (blockStart >= end) break;
        if (++block >= info.aBlocks[region])
            {
            block = 0;
            if (++region >= info.regions)
                {
                // Try read next chip info
                if (!OALFlashInfo((VOID*)block, &info)) break;
                region = 0;
                chip = block;
                }
            }
        }

    rc = TRUE;
   
cleanUp:
    return rc;
}

//------------------------------------------------------------------------------
