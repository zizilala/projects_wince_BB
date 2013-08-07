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
//  File:  rtl8139.c
//
#include <windows.h>
#include <ceddk.h>
#include <ethdbg.h>
#include <oal_log.h>
#include <oal_memory.h>
#include <oal_io.h>
#include <oal_timer.h>
#include <nkexport.h>

//------------------------------------------------------------------------------

#define MAC_TX_BUFFER_SIZE          1536
#define MAC_TX_BUFFERS              4

//------------------------------------------------------------------------------

#define RTL_IDR0                    0x00
#define RTL_IDR4                    0x04

#define RTL_MAR0                    0x08
#define RTL_MAR4                    0x0c

#define RTL_TSD0                    0x10
#define RTL_TSAD0                   0x20

#define RTL_RBSTART                 0x30
#define RTL_CR                      0x37
#define RTL_CAPR                    0x38
#define RTL_CBR                     0x3a
#define RTL_IMR                     0x3c
#define RTL_ISR                     0x3e

#define RTL_TCR                     0x40
#define RTL_RCR                     0x44

#define RTL_MSR                     0x58

//------------------------------------------------------------------------------

#define CR_RST                      (1 << 4)
#define CR_RE                       (1 << 3)
#define CR_TE                       (1 << 2)
#define CR_BUFE                     (1 << 0)

//------------------------------------------------------------------------------

#define IR_TER                      (1 << 3)
#define IR_TOK                      (1 << 2)
#define IR_RER                      (1 << 1)
#define IR_ROK                      (1 << 0)

//------------------------------------------------------------------------------

#define RCR_RXFTH16                 (0 << 13)
#define RCR_RBLEN64                 (3 << 11)
#define RCR_RBLEN32                 (2 << 11)
#define RCR_RBLEN16                 (1 << 11)
#define RCR_RBLEN8                  (0 << 11)
#define RCR_MXDMA16                 (0 << 8)
#define RCR_MXDMA1024               (6 << 8)
#define RCR_MXDMA2048               (7 << 8)
#define RCR_AR                      (1 << 4)
#define RCR_AB                      (1 << 3)
#define RCR_AM                      (1 << 2)
#define RCR_APM                     (1 << 1)
#define RCR_AAP                     (1 << 0)

//------------------------------------------------------------------------------

#define RSR_ROK                     (1 << 0)

//------------------------------------------------------------------------------

#define TCR_IFG_STD                 (3 << 24)
#define TCR_MXDMA1024               (6 << 8)
#define TCR_MXDMA2048               (7 << 8)

//------------------------------------------------------------------------------

#define TSR_OWN                     (1 << 13)

//------------------------------------------------------------------------------

#define MSR_LINKB                   (1 << 2)

//------------------------------------------------------------------------------

typedef struct {
    UINT32 address;
    UINT32 txBuffer;
    UINT32 txSize;
    UINT32 txPos;
    UINT32 rxBuffer;
    UINT32 rxSize;
    UINT32 filter;    
    UINT32 hash[2];
} RTL8139;

static RTL8139 g_rtl8139;

//------------------------------------------------------------------------------

static UINT32 ComputeCRC(UINT8 *pBuffer, UINT32 length)
{
    UINT32 crc, carry, i, j;
    UINT8 byte;

    crc = 0xffffffff;

    for (i = 0; i < length; i++) {
        byte = pBuffer[i];
        for (j = 0; j < 8; j++) {
            carry = ((crc & 0x80000000) ? 1 : 0) ^ (byte & 0x01);
            crc <<= 1;
            byte >>= 1;
            if (carry) crc = (crc ^ 0x04c11db6) | carry;
        }
    }
    return crc;

}

//------------------------------------------------------------------------------
//
//  Function:  RTL8139InitDMABuffer
//
//  This function is used to inform this library on where the buffer for
//  Tx/Rx DMA is. Driver needs  at least:
//    4 TX buffers (4 * ONE_BUFFER_SIZE) ~ 6k
//    8K + 16 bytes of RX buffers (64K max)
//    ** Note ** that ONE_BUFFER_SIZE is 1536 bytes which is in UINT32 boundary
//
//
BOOL RTL8139InitDMABuffer(UINT32 address, UINT32 size)
{
    BOOL rc = FALSE;
    UINT32 ph;

    OALMSGS(OAL_ETHER&&OAL_FUNC, (
        L"+RTL8139InitDMABuffer(0x%08x, 0x%08x)\r\n", address, size
    ));

    ph = OALVAtoPA((VOID*)address);
    if ((ph & 0x03) != 0) {
        size -= 4 - (ph & 0x03);
        ph = (ph + 3) & ~0x03;
    }

    g_rtl8139.txBuffer = (UINT32)OALPAtoUA(ph);
    g_rtl8139.txSize = MAC_TX_BUFFERS * MAC_TX_BUFFER_SIZE;
    if (g_rtl8139.txSize > size) {
        OALMSGS(OAL_ERROR, (
            L"ERROR: RTL8139InitDMABuffer: Buffer too small\r\n"
        ));
        goto cleanUp;
    }
    size -= g_rtl8139.txSize;
    ph += g_rtl8139.txSize;

    g_rtl8139.rxBuffer = (UINT32)OALPAtoUA(ph);
    if (size >= (64 * 1024 + 16)) {
        g_rtl8139.rxSize = 64 * 1024;
    } else if (size >= (32 * 1024 + 16)) {
        g_rtl8139.rxSize = 32 * 1024;
    } else if (size >= (16 * 1024 + 16)) {
        g_rtl8139.rxSize = 16 * 1024;
    } else if (size >= (8 * 1024 + 16)) {
        g_rtl8139.rxSize = 8 * 1024;
    } else {
        OALMSGS(OAL_ERROR, (
            L"ERROR: RTL8139InitDMABuffer: Buffer too small\r\n"
        ));
        goto cleanUp;
    }

    rc = TRUE;

cleanUp:
    OALMSGS(OAL_ETHER&&OAL_FUNC, (L"-RTL8139InitDMABuffer(rc = %d)\r\n", rc));
    return rc;
}

//------------------------------------------------------------------------------
//
//  Function:  RTL8139InitDMABuffer
//
BOOL RTL8139Init(UINT8 *pAddress, UINT32 offset, UINT16 mac[3])
{
    BOOL rc = FALSE;
    UINT32 u32, address, i, start;
    

    OALMSGS(OAL_ETHER&&OAL_FUNC, (
        L"+RTL8139Init(0x%08x, 0x%08x, 0x%08x)\r\n", pAddress, offset, mac
    ));

    // Get virtual uncached address
    g_rtl8139.address = (UINT32)pAddress;

    // Reset card
    OUTPORT8((UINT8*)(g_rtl8139.address + RTL_CR), CR_RST);
    while ((INPORT8((UINT8*)(g_rtl8139.address + RTL_CR)) & CR_RST) != 0);

    // Wait for link
    OALMSGS(OAL_WARN, (L"RTL8139: Wait for link..."));
    start = OALGetTickCount();
    while ((INPORT8((UINT8*)(g_rtl8139.address + RTL_MSR)) & MSR_LINKB) != 0) {
        if ((OALGetTickCount() - start) > 5000) {
            OALMSGS(OAL_WARN, (L" Wait timouted, exiting\r\n"));
            goto clean;
        }            
    }        
    OALMSGS(OAL_WARN, (L" Link detected\r\n"));

    // Enable TX/RX
    OUTPORT8((UINT8*)(g_rtl8139.address + RTL_CR), CR_TE | CR_RE);

    // Read MAC address
    u32 = INPORT32((UINT32*)(g_rtl8139.address + RTL_IDR0));
    mac[0] = (UINT16)(u32);
    mac[1] = (UINT16)(u32 >> 16);
    u32 = INPORT32((UINT32*)(g_rtl8139.address + RTL_IDR4));
    mac[2] = (UINT16)(u32);

    // Set TX descriptors
    address = OALVAtoPA((VOID*)g_rtl8139.txBuffer);
    offset = RTL_TSAD0;
    for (i = 0; i < MAC_TX_BUFFERS; i++) {
        OUTPORT32((UINT32*)(g_rtl8139.address + offset), address);
        address += MAC_TX_BUFFER_SIZE;
        offset += 4;
    }
    g_rtl8139.txPos = 0;

    // Set TX DMA burst size per TX DMA burst & standard interframe gap
    OUTPORT32((UINT32*)(g_rtl8139.address + RTL_TCR), TCR_IFG_STD|TCR_MXDMA1024);

    // Clear multicast hash registers
    g_rtl8139.hash[0] = g_rtl8139.hash[1] = 0;
    OUTPORT32((UINT32*)(g_rtl8139.address + RTL_MAR0), 0);
    OUTPORT32((UINT32*)(g_rtl8139.address + RTL_MAR4), 0);

    // Configure RX
    if (g_rtl8139.rxSize >= 64 * 1024) {
        u32 = RCR_RBLEN64;
    } else if (g_rtl8139.rxSize >= 32 * 1024) {
        u32 = RCR_RBLEN32;
    } else if (g_rtl8139.rxSize >= 16 * 1024) {
        u32 = RCR_RBLEN16;
    } else {
        u32 = RCR_RBLEN8;
    }
    u32 |= RCR_RXFTH16 | RCR_MXDMA16 | RCR_AR | RCR_AB | RCR_APM;
    OUTPORT32((UINT32*)(g_rtl8139.address + RTL_RCR),  u32);
    OUTPORT32(
        (UINT32*)(g_rtl8139.address + RTL_RBSTART),
        OALVAtoPA((VOID*)g_rtl8139.rxBuffer)
    );

    // Actual filter 
    g_rtl8139.filter = PACKET_TYPE_DIRECTED | PACKET_TYPE_BROADCAST;
    
    // Clear all interrupts
    OUTPORT16((UINT16*)(g_rtl8139.address + RTL_ISR), 0xFFFF);

    // Done
    rc = TRUE;

clean:
    OALMSGS(OAL_ETHER&&OAL_FUNC, (L"-RTL8139Init(rc = %d)\r\n", rc));
    return rc;
}

//------------------------------------------------------------------------------
//
//  Function:  RTL8139SendFrame
//
UINT16 RTL8139SendFrame(UINT8 *pData, UINT32 length)
{
    UINT32 *pTsd;
    UINT32 buffer, start;
    

    OALMSGS(OAL_ETHER&&OAL_VERBOSE, (
        L"+RTL8139SendFrame(0x%08x, %d)\r\n", pData, length
    ));

    // Wait until buffer is released by hardware
    pTsd = (UINT32*)(g_rtl8139.address + RTL_TSD0 + (g_rtl8139.txPos << 2));
    start = OALGetTickCount();
    while ((INPORT32(pTsd) & TSR_OWN) == 0) {
        if ((OALGetTickCount() - start) > 2000) {
            OALMSGS(OAL_ERROR, (L"ERROR: RTL8139SendFrame: Send timeout\r\n"));
            return 1;
        }
    }

    // Copy data to buffer
    buffer = g_rtl8139.txBuffer + g_rtl8139.txPos * MAC_TX_BUFFER_SIZE;
    memcpy((VOID*)buffer, pData, length);
    if (length < 60) {
        memset((VOID*)(buffer + length), 0, 60 - length);
        length = 60;
    }

    // Write TSD register to start send
    OUTPORT32(pTsd, length);

    // Move to next position
    if (++g_rtl8139.txPos >= MAC_TX_BUFFERS) g_rtl8139.txPos = 0;

    OALMSGS(OAL_ETHER&&OAL_VERBOSE, (L"-RTL8139SendFrame(rc = 0)\r\n"));
    return 0;
}

//------------------------------------------------------------------------------
//
//  Function:  RTL8139GetFrame
//
UINT16 RTL8139GetFrame(UINT8 *pData, UINT16 *pLength)
{
    UINT16 offset, length, status, len1;
    BOOL ok;
    
    OALMSGS(OAL_ETHER&&OAL_VERBOSE, (
        L"+RTL8139GetFrame(0x%08x, %d)\r\n", pData, *pLength
    ));

    length = 0;
    ok = FALSE;
    while ((INPORT8((UINT8*)(g_rtl8139.address + RTL_CR)) & CR_BUFE) == 0) {

        // Get read offset
        offset = INPORT16((UINT16*)(g_rtl8139.address + RTL_CAPR)) + 0x10;

        // Read packet status and length
        status = *(UINT16*)(g_rtl8139.rxBuffer + offset);
        length = *(UINT16*)(g_rtl8139.rxBuffer + offset + 2) - 4;

        offset += 4;
        if (offset >= g_rtl8139.rxSize) offset = 0;

        // When packet is OK copy it to buffer
        if ((status & RSR_ROK) != 0 && length <= *pLength) {
            len1 = g_rtl8139.rxSize - offset;
            if (length <= len1) {
                memcpy(pData, (VOID*)(g_rtl8139.rxBuffer + offset), length);
            } else {
                memcpy(pData, (VOID*)(g_rtl8139.rxBuffer + offset), len1);
                memcpy(pData + len1, (VOID*)g_rtl8139.rxBuffer, length - len1);
            }
            ok = TRUE;
        }

        // Update read offset
        offset += (length + 7) & ~3;
        if (offset >= g_rtl8139.rxSize) offset -= g_rtl8139.rxSize;
        OUTPORT16((UINT16*)(g_rtl8139.address + RTL_CAPR), offset - 0x10);

        // If packet is ok, we have all we need
        if (ok) break;

        length = 0;
    }

    // If there isn't a packet ack the interrupt
    if (length == 0) OUTPORT16((UINT16*)(g_rtl8139.address + RTL_ISR), 0xFFFF);

    // Return packet size
    *pLength = length;

    OALMSGS(OAL_ETHER&&OAL_VERBOSE, (
        L"-RTL8139GetFrame(length = %d)\r\n", length
    ));
    return length;
}

//------------------------------------------------------------------------------
//
//  Function:  RTL8139EnableInts
//
VOID RTL8139EnableInts()
{
    OALMSGS(OAL_ETHER&&OAL_FUNC, (L"+RTL8139EnableInts\r\n"));
    // We are only interested in RX interrupts
    OUTPORT16((UINT16*)(g_rtl8139.address + RTL_IMR), IR_ROK);
    OALMSGS(OAL_ETHER&&OAL_FUNC, (L"-RTL8139EnableInts\r\n"));
}


//------------------------------------------------------------------------------
//
//  Function:  RTL8139DisableInts
//
VOID RTL8139DisableInts()
{
    OALMSGS(OAL_ETHER&&OAL_FUNC, (L"+RTL8139DisableInts\r\n"));
    OUTPORT16((UINT16*)(g_rtl8139.address + RTL_IMR), 0);
    OALMSGS(OAL_ETHER&&OAL_FUNC, (L"-RTL8139DisableInts\r\n"));
}

//------------------------------------------------------------------------------
//
//  Function:  RTL8139CurrentPacketFiler
//

VOID RTL8139CurrentPacketFilter(UINT32 filter)
{
    UINT32 ctrl;

    OALMSGS(OAL_ETHER&&OAL_FUNC, (
        L"+RTL8139CurrentPacketFilter(0x%08x)\r\n", filter
    ));

    ctrl = INPORT32((UINT32*)(g_rtl8139.address + RTL_RCR));

    // First set hash functions
    if (
        (filter & PACKET_TYPE_PROMISCUOUS) != 0 ||
        (filter & PACKET_TYPE_ALL_MULTICAST) != 0
    ) {
        OUTPORT32((UINT32*)(g_rtl8139.address + RTL_MAR0), 0xFFFFFFFF);
        OUTPORT32((UINT32*)(g_rtl8139.address + RTL_MAR4), 0xFFFFFFFF);
    } else {
        OUTPORT32((UINT32*)(g_rtl8139.address + RTL_MAR0), g_rtl8139.hash[0]);
        OUTPORT32((UINT32*)(g_rtl8139.address + RTL_MAR4), g_rtl8139.hash[1]);
    }

    // Then global 
    if (
        (filter & PACKET_TYPE_PROMISCUOUS) != 0 ||
        (filter & PACKET_TYPE_ALL_MULTICAST) != 0 ||
        (filter & PACKET_TYPE_MULTICAST) != 0
    ) {
        ctrl |= RCR_AM;
    } else {
        ctrl &= ~RCR_AM;
    }
    
    if ((filter & PACKET_TYPE_PROMISCUOUS) != 0) {
        ctrl |= RCR_AAP;
    } else {
        ctrl &= ~RCR_AAP;
    }

    // Set filter to hardware
    OUTPORT32((UINT32*)(g_rtl8139.address + RTL_RCR), ctrl);

    // Save actual filter
    g_rtl8139.filter = filter;

    OALMSGS(OAL_ETHER&&OAL_FUNC, (L"-RTL8139CurrentPacketFilter\r\n"));
}

//------------------------------------------------------------------------------
//
//  Function:  RTL8139MulticastList
//
BOOL RTL8139MulticastList(UINT8 *pAddresses, UINT32 count)
{
    UINT32 i, crc, bit;
    UINT8 hash[8];

    OALMSGS(OAL_ETHER&&OAL_FUNC, (
        L"+RTL8139MulticastList(0x%08x, %d)\r\n", pAddresses, count
    ));

    memset(hash, 0, sizeof(hash));
    for (i = 0; i < count; i++) {
        crc = ComputeCRC(pAddresses, 6);
        bit = (crc >> 26) & 0x3f;
        hash[bit >> 3] |= 1 << (bit & 0x07);
        pAddresses += 6;
    }
    g_rtl8139.hash[0] = (hash[3] << 24)|(hash[2] << 16)|(hash[1] << 8)|hash[0];
    g_rtl8139.hash[1] = (hash[7] << 24)|(hash[6] << 16)|(hash[5] << 8)|hash[4];

    // But update only if all multicast and promiscuous mode aren't active
    if (
        (g_rtl8139.filter & PACKET_TYPE_ALL_MULTICAST) == 0 &&
        (g_rtl8139.filter & PACKET_TYPE_PROMISCUOUS) == 0
    ) {        
        OUTPORT32((UINT32*)(g_rtl8139.address + RTL_MAR0), g_rtl8139.hash[0]);
        OUTPORT32((UINT32*)(g_rtl8139.address + RTL_MAR4), g_rtl8139.hash[1]);
    }

    OALMSGS(OAL_ETHER&&OAL_FUNC, (L"-RTL8139MulticastList(rc = 1)\r\n"));
    return TRUE;
}

//------------------------------------------------------------------------------

