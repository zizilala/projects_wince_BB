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
// THE SAMPLE SOURCE CODE IS PROVIDED "AS IS", WITH NO WARRANTIES OR INDEMNITIES.
//
//------------------------------------------------------------------------------
//
//  File: PerReg.c
//
//  This file implements persistent registry module for NOR flash memory.
//
#include <windows.h>
#include <oal_log.h>
#include <oal_memory.h>
#include <oal_flash.h>
#include <oal_perreg.h>
#include <oemglobal.h>


//------------------------------------------------------------------------------
//
//  Define: OAL_PERREG_MAX_REGIONS
//
//  This constant defines maximal supported number of regions. It is solved
//  in this way to avoid memory allocation which isn't supported at time
//  when OALPerRegInit is usally called.
//
#define OAL_PERREG_MAX_REGIONS          8

//------------------------------------------------------------------------------
//
//  Define: OAL_PERREG_SIGN
//
//  This constant is used as persistent registry signature in registry storage
//  header.
//
#define OAL_PERREG_SIGN                 'PREG'

//------------------------------------------------------------------------------
//
//  Type: OAL_PERREG_HEADER
//
//  This type defines registry storage header. Its size must be 32-bit word
//  allignment and it must be smaller than buffer size in OAL_PERREG_STATE
//  structure.
//
typedef struct {
    UINT32 signature;
    UINT32 size;
    UINT32 checkSum;
} OAL_PERREG_HEADER;

//------------------------------------------------------------------------------
//
//  Type: OAL_PERREG_STATE
//
//  The type encapsulating implementation internal status variables.
//
typedef struct {
    UINT32 regions;
    OAL_PERREG_REGION region[OAL_PERREG_MAX_REGIONS];
    
    UINT32 size;
    UINT32 checkSum;        
    
    UINT32 regionIdx;
    UINT32 regionPos;    
    
    UINT8  buffer[1024];
    UINT32 bufferPos;
} OAL_PERREG_STATE;

//------------------------------------------------------------------------------
//
//  Static: g_perRegState
//
//  Persistent registry implementation internal state.
//
static OAL_PERREG_STATE g_perRegState;

//------------------------------------------------------------------------------
// Local Functions 

static UINT32 CheckSum(VOID *pStart, UINT32 size);
static BOOL WriteBufferToFlash(BOOL skipHeader);
static BOOL ReadBufferFromFlash();
static BOOL EraseFlash();
static VOID ResetBuffer(BOOL write);
static BOOL CopyFromBuffer(VOID *pBuffer, UINT32 count);
static BOOL CopyToBuffer(VOID *pBuffer, UINT32 count);
static BOOL FlushBuffer(BOOL skipHeader);

//------------------------------------------------------------------------------
//
//  Function: OALPerRegInit
//
//  This function should be called from OAL to initalize persistent registry
//  functionality. When clean flag is set it also erase all regions. Function
//  make all regions flash memory block alignment.
//
BOOL OALPerRegInit(BOOL clean, OAL_PERREG_REGION aRegions[])
{
    UINT32 rc = FALSE;
    OAL_PERREG_REGION *pRegion;
    UINT32 i = 0;
    UINT32 offset; //, blockStart, blockSize;

    OALMSG(OAL_FLASH&&OAL_FUNC, (L"+OALPerRegInit(0x%08x)\r\n", aRegions));
    
    // Copy regions info to state variable
    g_perRegState.regions = 0;
    pRegion = &g_perRegState.region[0];
    while (aRegions[i].base != 0) {

#if 0
        // Verify, that there is flash memory
        if (!OALFlashBlockInfo(
            aRegions[i].pBase, aRegions[i].pStart, &blockStart, &blockSize
        )) {
            OALMSG(OAL_ERROR, (
                L"ERROR: There isn't flash memory at 0x%08x\r\n", 
                aRegions[i].pBase
            ));
            i++;
            continue;
        }

        // Get region offset in block
        offset = OALVAtoPA(aRegions[i].pStart) - blockStart;
        if (offset >= pRegion->size) {
            OALMSG(OAL_WARN, (
                L"WARN: Region %08x - %08x - %08x too small, ignored\r\n",
                aRegions[i].pBase, aRegions[i].pStart, aRegions[i].size
            ));
            i++;
            continue;
        }            
#else
        offset = 0;
#endif

        // Save region to state variable
        pRegion->base  = aRegions[i].base;
        pRegion->start = aRegions[i].start + offset;
        pRegion->size  = aRegions[i].size - offset;
        pRegion++;
        if (++g_perRegState.regions >= OAL_PERREG_MAX_REGIONS) break;
        
        i++;
    }

    // We was succesfull when at least one region was verified
    if (rc = (g_perRegState.regions > 0)) {
        g_pOemGlobal->pfnWriteRegistry = (PFN_WriteRegistry) OALPerRegWrite;
        if (!clean) g_pOemGlobal->pfnReadRegistry = (PFN_ReadRegistry) OALPerRegRead;
    }        
    
    OALMSG(OAL_FLASH&&OAL_FUNC, (L"-OALPerRegInit(rc = %d)\r\n", rc));
    return rc;        
}

//------------------------------------------------------------------------------
//
//  Function: OALPerRegWrite
//
//  This function implements pfnWriteRegistry write function.
//
BOOL OALPerRegWrite(UINT32 flags, UINT8 *pData, UINT32 size)
{
    BOOL rc = FALSE;
    OAL_PERREG_HEADER header;


    OALMSG(OAL_FLASH&&OAL_FUNC, (
        L"+OALPerRegWrite(0x%08x, 0x%08x, 0x%08x)\r\n", flags, pData, size
    ));
    
    // Check if it is write start
    if ((flags & REG_WRITE_BYTES_START) != 0) {

        // First check if there are regions defined
        if (g_perRegState.regions <= 0) {
            OALMSG(OAL_ERROR, (
                L"ERROR: OALPerRegWrite: No valid flash regions defined\r\n"
            ));
            goto cleanUp;
        }

        RETAILMSG(TRUE, (L"INFO: OALPerRegWrite: Registry write start\r\n"));

        // Erase flash memory
        if (!EraseFlash()) goto cleanUp;
        
        // Initialize local state variables
        g_perRegState.size = 0;
        g_perRegState.checkSum = 0;
        memset(&header, 0xFF, sizeof(header));

        // Reset buffer & write temporary header
        ResetBuffer(TRUE);
        CopyToBuffer(&header, sizeof(header));
       
    }

    // While there are some data && place where to write
    if (size > 0) {

        if (!CopyToBuffer(pData, size)) {
            OALMSG(OAL_ERROR, (
                L"ERROR: OALPerRegWrite: CopyToBuffer failed\r\n"
            ));
            goto cleanUp;
        }
        g_perRegState.size += size;
        
    } else {

        // Flush buffer
        FlushBuffer(TRUE);

        // Prepare and write header
        header.signature = OAL_PERREG_SIGN;
        header.size = g_perRegState.size;
        header.checkSum = g_perRegState.checkSum;
        ResetBuffer(TRUE);
        CopyToBuffer(&header, sizeof(header));
        FlushBuffer(FALSE);

        RETAILMSG(TRUE, (L"INFO: OALPerRegWrite: Registry write done\r\n"));
    }
    
    rc = TRUE;
    
cleanUp:
    OALMSG(OAL_FLASH&&OAL_FUNC, (L"-OALPerRegWrite(rc = %d)", rc));
    return rc;
}

//------------------------------------------------------------------------------
//
//  Function: OALPerRegRead
//
//  This function implements pfnReadRegistry read function.
//
UINT32 OALPerRegRead(UINT32 flags, UINT8 *pData, UINT32 dataSize)
{
    UINT32 rc = 0;
    OAL_PERREG_HEADER header;
    UINT32 count, checkSum, chunk, buffer[32];
    UINT8 *pBase = pData;

    OALMSG(OAL_FLASH&&OAL_FUNC, (
        L"+OALPerRegRead(0x%08x, 0x%08x, 0x%08x)\r\n", flags, pData, dataSize
    ));

    if (flags & REG_READ_BYTES_START) {

        if (g_perRegState.regions <= 0) {
            OALMSG(OAL_ERROR, (
                L"ERROR: OALPerRegRead: No valid flash regions defined\r\n"
            ));
            rc = -1;
            goto cleanUp;
        }

        RETAILMSG(TRUE, (L"INFO: OALPerRegRead: Registry read start\r\n"));

        // Reset buffer and read header
        ResetBuffer(FALSE);
        CopyFromBuffer(&header, sizeof(header));

        if (header.signature != OAL_PERREG_SIGN) {
            OALMSG(OAL_WARN, (L"WARN: ReadRegistryFromOEM: Bad signature\r\n"));
            rc = -1;
            goto cleanUp;
        }

        // Calculate checksum
        count = (header.size + 3) & ~0x3;
        checkSum = 0;
        while (count > 0) {
            chunk = sizeof(buffer) < count ? sizeof(buffer) : count;
            CopyFromBuffer(buffer, chunk);
            checkSum += CheckSum(buffer, chunk);
            count -= chunk;
        }

        if (header.checkSum != checkSum) {
            OALMSG(OAL_WARN, (
                L"WARN: ReadRegistryFromOEM: Invalid checksum\r\n"
            ));
            rc = -1;
            goto cleanUp;
        }            

        // Reset buffer and read header
        ResetBuffer(FALSE);
        CopyFromBuffer(&header, sizeof(header));
        g_perRegState.size = header.size;
        
    }

    if (g_perRegState.size >= dataSize) {
        CopyFromBuffer(pData, dataSize);
        g_perRegState.size -= dataSize;
        rc = dataSize;
    }

    RETAILMSG(rc == 0, (
        L"INFO: OALPerRegRead: Registry read done (%d)\r\n", g_perRegState.size
    ));

cleanUp:
    OALMSG(OAL_FLASH&&OAL_FUNC, (L"-OALPerRegRead(rc = %d)", rc));
    return rc;
}

//------------------------------------------------------------------------------

static UINT32 CheckSum(VOID *pStart, UINT32 size)
{
    UINT32 sum = 0;
    UINT32 *p;

    for (p = pStart; size > sizeof(UINT32) - 1; size -= sizeof(UINT32)) {
        sum += *p++;
    }
    return sum;
}

//------------------------------------------------------------------------------

static BOOL EraseFlash()
{
    BOOL rc = FALSE;
    OAL_PERREG_REGION *pRegion;
    UINT32 i;
    
    // Erase all regions...
    RETAILMSG(TRUE, (L"INFO: OALPerRegWrite: Registry store erase start\r\n"));
    pRegion = g_perRegState.region;
    for (i = 0; i < g_perRegState.regions; i++) {
        if (!OALFlashErase(
            OALPAtoUA(pRegion->base), OALPAtoUA(pRegion->start), 
            pRegion->size
        )) {
            OALMSG(OAL_ERROR, (
                L"ERROR: OALPerRegWrite: Flash erase at 0x%08x failed\r\n",
                pRegion->start
            ));
            goto cleanUp;
        }
        pRegion++;
    }
    rc = TRUE;
    RETAILMSG(TRUE, (L"INFO: OALPerRegWrite: Registry store erase done\r\n"));

cleanUp:
    return rc;
}

//------------------------------------------------------------------------------

static BOOL WriteBufferToFlash(BOOL skipHeader)
{
    BOOL rc;
    OAL_PERREG_REGION *pRegion;
    UINT32 pos, size;
    UINT8 *pBuffer;
    static BOOL fWriteFailure = FALSE;

    // Sanity check: do we have a valid region to write to?
    if (g_perRegState.regionIdx >= g_perRegState.regions) {
        if (!fWriteFailure)
        {
            OALMSG(OAL_ERROR, (
                L"ERROR: OALPerRegWrite: no free space; write skipped! (additional errors suppressed)\r\n"
            ));
            fWriteFailure = TRUE;
        }

        rc = FALSE;
        goto cleanUp;
    }


    // Get write position    
    pRegion = &g_perRegState.region[g_perRegState.regionIdx];
    pos = pRegion->start + g_perRegState.regionPos;
    pBuffer = g_perRegState.buffer;
    size = g_perRegState.bufferPos;

    // Don't write header part until say so...
    if (
        skipHeader &&
        g_perRegState.regionIdx == 0 && 
        g_perRegState.regionPos < sizeof(OAL_PERREG_HEADER)
    ) {
        pos += sizeof(OAL_PERREG_HEADER) - g_perRegState.regionPos;
        pBuffer += sizeof(OAL_PERREG_HEADER) - g_perRegState.regionPos;
        size -= sizeof(OAL_PERREG_HEADER);
    }

    // Round size to 4 bytes (32 bits)
    if ((size & 0x03) != 0) {
        memset(pBuffer + (size & ~0x03), 0xFF, 4 - (size & 0x03));
        size += 4 - (size & 0x03);
    }

    // Add buffer check sum
    g_perRegState.checkSum += CheckSum(pBuffer, size);

    // Write buffer to flash
    if (!(rc = OALFlashWrite(
        OALPAtoUA(pRegion->base), OALPAtoUA(pos), size, pBuffer
    ))) OALMSG(OAL_ERROR, (
        L"ERROR: OALPerRegWrite: Flash write at 0x%08x failed\r\n", pos
    ));

    // Move write pointer
    g_perRegState.regionPos += g_perRegState.bufferPos;
    g_perRegState.bufferPos = 0;
    if (g_perRegState.regionPos >= pRegion->size) {
        g_perRegState.regionPos = 0;
        g_perRegState.regionIdx++;

        if (g_perRegState.regionIdx >= g_perRegState.regions) {
            OALMSG(OAL_ERROR, (
                L"ERROR: OALPerRegWrite: out of space; set aside larger backing store for persistent registry!\r\n"
            ));
        }
    }

cleanUp:
    return rc;
}

//------------------------------------------------------------------------------

static BOOL ReadBufferFromFlash()
{
    OAL_PERREG_REGION *pRegion;
    UINT32 count, pos, chunk, remain;
    UINT8 *pBuffer;

    // Get possition    
    count = sizeof(g_perRegState.buffer);
    pBuffer = g_perRegState.buffer;
    
    while (count > 0 && g_perRegState.regionIdx < g_perRegState.regions) {

        pRegion = &g_perRegState.region[g_perRegState.regionIdx];
        pos = pRegion->start + g_perRegState.regionPos;

        remain = pRegion->size - g_perRegState.regionPos;
        chunk = (remain < count) ? remain : count;

        // Copy data chunk
        memcpy(pBuffer, OALPAtoUA(pos), chunk);
    
        // Move pointer
        g_perRegState.regionPos += chunk;
        if (g_perRegState.regionPos >= pRegion->size) {
            g_perRegState.regionPos = 0;
            g_perRegState.regionIdx++;
        }

        // We already read this amount of data
        count -= chunk;
    }

    // Read should start from buffer start
    g_perRegState.bufferPos = 0;

    // Return true when we read all data
    return (count == 0);
}

//------------------------------------------------------------------------------

static BOOL CopyFromBuffer(VOID *pBuffer, UINT32 count)
{
    BOOL rc = FALSE;
    UINT32 chunk, remain;

    while (count > 0) {

        // Get data chunk size
        remain = sizeof(g_perRegState.buffer) - g_perRegState.bufferPos;
        chunk = (remain < count) ? remain : count;
        
        // Copy data chunk
        memcpy(pBuffer, &g_perRegState.buffer[g_perRegState.bufferPos], chunk);

        g_perRegState.bufferPos += chunk;
        (UINT8*)pBuffer += chunk;
        if (g_perRegState.bufferPos >= sizeof(g_perRegState.buffer)) {
            if (!ReadBufferFromFlash()) goto cleanUp;
        }

        // How much we still need to get
        count -= chunk;
    }        

    rc = TRUE;
    
cleanUp:
    return rc;
}

//------------------------------------------------------------------------------

static VOID ResetBuffer(BOOL write)
{
    // Move at beginning of flash regions
    g_perRegState.regionIdx = 0;
    g_perRegState.regionPos = 0;
    g_perRegState.bufferPos = 0;
    if (!write) ReadBufferFromFlash();
}

//------------------------------------------------------------------------------

BOOL CopyToBuffer(VOID *pBuffer, UINT32 count)
{
    BOOL rc = FALSE;
    UINT32 chunk, remain;

    while (count > 0) {

        // Get data chunk size
        remain = sizeof(g_perRegState.buffer) - g_perRegState.bufferPos;
        chunk = (remain < count) ? remain : count;
        
        // Copy data chunk
        memcpy(&g_perRegState.buffer[g_perRegState.bufferPos], pBuffer, chunk);

        // Move buffer pointer and write buffer to flash when it is full
        g_perRegState.bufferPos += chunk;
        (UINT8*)pBuffer += chunk;
        if (g_perRegState.bufferPos >= sizeof(g_perRegState.buffer)) {
            if (!WriteBufferToFlash(TRUE)) goto cleanUp;
        }

        // How much we still need to copy
        count -= chunk;
    }        

    rc = TRUE;

cleanUp:
    return rc;
}

//------------------------------------------------------------------------------

BOOL FlushBuffer(BOOL skipHeader)
{
    BOOL rc = FALSE;

    // If there are data in buffer write it to flash
    if (g_perRegState.bufferPos > 0) {
        if (!WriteBufferToFlash(skipHeader)) goto cleanUp;
    }

    rc = TRUE;

cleanUp:
    return rc;
}

//------------------------------------------------------------------------------

#if 0
typedef struct {
    BYTE     parent;    // which predefined parent is this under
    BYTE     _bPad;
    BYTE     bNameLen;  // length of name including null in WCHARs
    BYTE     bClassLen; // length of name including null in WCHARs (0 if not present)
    WCHAR    name[];    // inlined string, null terminated
    // WCHAR class[];     // inlined string, null terminated
} REGKEY;

typedef struct {
    WORD     wType;
    BYTE     bNameLen;  // length of name including null in WCHARs
    BYTE     _bPad;
    WORD     wDataLen;  // length of data in bytes
    WCHAR    name[];    // inlined string, null terminated
    // BYTE     data[];   // inlined data
} REGVALUE;

static UINT8 g_perRegBuffer[4096];

VOID* XCopyToBuffer(UINT32 size, UINT32 *pIdx, UINT32 *pPos)
{
    OAL_PERREG_REGION *pRegion;
    UINT32 pos, chunk, remain;

    if (size > sizeof(g_perRegBuffer)) size = sizeof(g_perRegBuffer);
    
    pos = 0;
    while (pos < size) {
        pRegion = &g_perRegState.region[*pIdx];
        chunk = size - pos;
        remain = pRegion->size - *pPos;
        if (chunk > remain) chunk = remain;
        memcpy(
            &g_perRegBuffer[pos], (UINT8*)OALPAtoCA(pRegion->start) + *pPos, 
            chunk
        );
        pos += chunk;
        if (pos < pRegion->size) {
            *pPos += chunk;
        } else {
            (*pIdx)++;
            *pPos = 0;
        }
    }
    return g_perRegBuffer;            
}

//------------------------------------------------------------------------------

VOID OALLogDumpBuffer(VOID *pData, UINT32 size)
{
    WCHAR buffer1[49], buffer2[17];
    UINT8 *p = pData;
    UINT32 i, j;

    j = 0;
    for (i = 0; i < size; i++) {
        OALLogPrintf(
            buffer1 + j, sizeof(buffer1) - sizeof(WCHAR) * j, L" %02x", p[i]
        );
        j += 3;
        if (p[i] >= ' ' && p[i] < 127) {
            buffer2[i & 0xF] = (WCHAR)p[i];
        } else {
            buffer2[i & 0xF] = L'.';
        }
        if ((i & 0xF) == 0xF) {
            buffer2[0x10] = L'\0';
            OALLog(L"%04x %-48s  %s\r\n", i & ~0xF, buffer1, buffer2);
            j = 0;
        }
    }        
    if ((i & 0xF) != 0) {
        buffer2[i & 0xF] = L'\0';
        OALLog(L"%04x %-48s  %s\r\n", i & ~0xF, buffer1, buffer2);
    }
}

//------------------------------------------------------------------------------

void OALPerRegDump()
{
    UINT32 region, pos, count, size, info, prevRegion, prevPos;
    OAL_PERREG_HEADER *pHeader;
    UINT8 *pData;
    REGKEY *pKey;
    REGVALUE *pValue;

    pHeader = OALPAtoCA(g_perRegState.region[0].start);
    region = 0;
    pos = sizeof(OAL_PERREG_HEADER);
    count = pHeader->size;

    OALLog(L"Registry size %08x\r\n", count);
    while (count > 0) {
        prevRegion = region;
        prevPos = pos;
        info = *(UINT32*)XCopyToBuffer(sizeof(UINT32), &region, &pos);
        OALLog(
            L"Type %04x size %04x (region %d pos %08x count %08x)\r\n",
            info >> 16, info & 0xFFFF, prevRegion, prevPos, count
        );
        pData = XCopyToBuffer(info & 0xFFFF, &region, &pos);
        switch (info >> 16) {
        case 1:
            pKey = (REGKEY*)pData;
            size = sizeof(REGKEY);
            size += sizeof(WCHAR) * (pKey->bNameLen + pKey->bClassLen);
            if (size != (info & 0xFFFF)) OALLog(
                L"***** %d versus %d", size, (info & 0xFFFF)
            );
            OALLog(
                L"Name '%s' (length %d), parent %02x, class '%s' (length %d)\r\n", 
                pKey->name, pKey->bNameLen, pKey->parent,
                pKey->bClassLen > 0 ? pKey->name + pKey->bNameLen : L"*none*", 
                pKey->bClassLen
            );
            break;
        case 2:
            pValue = (REGVALUE*)pData;
            size = sizeof(REGVALUE); 
            size += sizeof(WCHAR) * pValue->bNameLen + pValue->wDataLen;
            if (size != (info & 0xFFFF)) OALLog(
                L"***** %d versus %d", size, (info & 0xFFFF)
            );
            OALLog(
                L"Name '%s' (length %d), type %d, size %d\r\n", pValue->name, 
                pValue->bNameLen, pValue->wType, pValue->wDataLen
            );
            OALLogDumpBuffer(pValue->name + pValue->bNameLen, pValue->wDataLen);
            break;    
        default:
            OALLog(L"Unknown type...\r\n");
            OALLogDumpBuffer(pData, info & 0xFFFF);
            
        }
        count -= 4 + (info & 0xFFFF);
    }
    
}

//------------------------------------------------------------------------------

#endif
