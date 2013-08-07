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
//  File:  am79c973.c
//
#include <windows.h>
#include <halether.h>
#include <oal.h>

//------------------------------------------------------------------------------

#define BUFFER_SIZE         0x0600
#define DESC_SIZE           0x10
#define INIT_SIZE           0x20
#define ADDR_SIZE           6

#define TIMEOUT             2

#define RX_BUFFERS          32
#define TX_BUFFERS          4

//------------------------------------------------------------------------------

#define APROM               0x00
#define RDP                 0x10
#define RAP                 0x14
#define RESET               0x18
#define BDP                 0x1C

//------------------------------------------------------------------------------

#define RMD1_OWN            0x80000000
#define RMD1_ERR            0x40000000
#define RMD1_FRAM           0x20000000
#define RMD1_OFLO           0x10000000
#define RMD1_CRC            0x08000000
#define RMD1_BUFF           0x04000000
#define RMD1_STP            0x02000000
#define RMD1_ENP            0x01000000
#define RMD1_BPE            0x00800000
#define RMD1_ONES           0x0000F000

#define TMD0_BUFF           0x80000000
#define TMD0_UFLO           0x40000000
#define TMD0_EXDEF          0x20000000
#define TMD0_LCOL           0x10000000
#define TMD0_LCAR           0x08000000
#define TMD0_RTRY           0x04000000

#define TMD1_OWN            0x80000000
#define TMD1_ERR            0x40000000
#define TMD1_ADD_FCS        0x20000000
#define TMD1_MORE           0x10000000
#define TMD1_LTINT          0x10000000
#define TMD1_ONE            0x08000000
#define TMD1_DEF            0x04000000
#define TMD1_STP            0x02000000
#define TMD1_ENP            0x01000000
#define TMD1_BPE            0x00800000
#define TMD1_ONES           0x0000F000

#define MODE_PROM           (1 << 15)
#define MODE_DRCVBC         (1 << 14)

//------------------------------------------------------------------------------

#define MAC_PHY_BMCR                0     // Basic Mode Control Register
#define MAC_PHY_BMSR                1     // Basic Mode Status Register
#define MAC_PHY_PHYIDR1             2     // PHY Identifier Register #1
#define MAC_PHY_PHYIDR2             3     // PHY Identifier Register #2
#define MAC_PHY_ANAR                4     // AutoNeg Advertisement Register
#define MAC_PHY_ANLPAR              5     // AutoNeg Link Partner Ability Reg
#define MAC_PHY_ANER                6     // AutoNeg Expansion Register

#define MAC_PHY_BMCR_RST            (1 << 15)
#define MAC_PHY_BMCR_SS             (1 << 13)
#define MAC_PHY_BMCR_ANE            (1 << 12)
#define MAC_PHY_BMCR_RAN            (1 << 9)

#define MAC_PHY_BMSR_ANC            (1 << 5)
#define MAC_PHY_BMSR_LINK           (1 << 2)

#define MAC_PHY_AN_100FD            (1 << 8)
#define MAC_PHY_AN_100HD            (1 << 7)
#define MAC_PHY_AN_10FD             (1 << 6)
#define MAC_PHY_AN_10HD             (1 << 5)

//------------------------------------------------------------------------------

static UINT32 g_base = 0;               // The chip base address
static UINT32 g_dmaAddress = 0;         // DMA buffer address
static UINT32 g_dmaSize = 0;            // DMA buffer size

static UINT32 *g_pInit;
static UINT32 *g_pRxRing;
static UINT32 *g_pTxRing;
static UINT8  *g_pRxBuffer;
static UINT8  *g_pTxBuffer;

static UINT32 g_rxPos;
static UINT32 g_txPos;

//------------------------------------------------------------------------------

static UINT32 ReadCSR(UINT32 address)
{
   OUTREG32((UINT32*)(g_base + RAP), address);
   return INREG32((UINT32*)(g_base + RDP));
}

//------------------------------------------------------------------------------

static VOID WriteCSR(UINT32 address, UINT32 data)
{
   OUTREG32((UINT32*)(g_base + RAP), address);
   OUTREG32((UINT32*)(g_base + RDP), data);
}

//------------------------------------------------------------------------------

static UINT32 ReadBCR(UINT32 address)
{
   OUTREG32((UINT32*)(g_base + RAP), address);
   return INREG32((UINT32*)(g_base + BDP));
}

//------------------------------------------------------------------------------

static VOID WriteBCR(UINT32 address, UINT32 data)
{
   OUTREG32((UINT32*)(g_base + RAP), address);
   OUTREG32((UINT32*)(g_base + BDP), data);
}

//------------------------------------------------------------------------------

static UINT16 ReadPhy(UINT16 id, UINT16 reg)
{
    UINT32 address;
    
    OUTREG32((UINT32*)(g_base + RAP), 33);
    address = INREG32((UINT32*)(g_base + BDP));
    OUTREG32((UINT32*)(g_base + BDP), (UINT16)((id << 5)|(reg & 0x1F)));
    OUTREG32((UINT32*)(g_base + RAP), 34);
    return (UINT16)INREG32((UINT32*)(g_base + BDP));
}

//------------------------------------------------------------------------------

static VOID WritePhy(UINT16 id, UINT16 reg, UINT16 data)
{
    UINT32 address;

    OUTREG32((UINT32*)(g_base + RAP), 33);
    address = INREG32((UINT32*)(g_base + BDP));
    OUTREG32((UINT32*)(g_base + BDP), (UINT16)((id << 5)|(reg & 0x1F)));
    OUTREG32((UINT32*)(g_base + RAP), 34);
    OUTREG32((UINT32*)(g_base + BDP), data);
}

//------------------------------------------------------------------------------

static DWORD HashAddress(UCHAR* pAddress)
{
   ULONG crc, carry;
   UINT i, j;
   UCHAR uc;
   
   crc = 0xFFFFFFFF;
   for (i = 0; i < ADDR_SIZE; i++) {
      uc = pAddress[i];
      for (j = 0; j < 8; j++) {
         carry = ((crc & 0x80000000) ? 1 : 0) ^ (uc & 0x01);
         crc <<= 1;
         uc >>= 1;
         if (carry) crc = (crc ^ 0x04c11db6) | carry;
      }
   }
   return crc;
}

//------------------------------------------------------------------------------

static BOOL HWInit()
{
    UINT32 i, pos;

    // Wait for while...
    OALStall(1000000);

    // Switch to DWIO mode
    OUTREG32((UINT32*)(g_base + RDP), 0);

    // Reset    
    INREG32((UINT32*)(g_base + RESET));
    
    // Wait 2ms
    OALStall(2000);
    
    // Set software style to 3 (32bit software structure)
    WriteBCR(20, 0x0503);

    // Stop adapter
    WriteCSR(0, 0x0004);

    // Divide DMA buffer
    pos = g_dmaAddress;
    g_pInit = (UINT32*)pos;
    pos += INIT_SIZE;
    g_pRxRing = (UINT32*)pos;
    pos += RX_BUFFERS * DESC_SIZE;
    g_pTxRing = (UINT32*)pos;
    pos += TX_BUFFERS * DESC_SIZE;
    g_pRxBuffer = (UINT8*)pos;
    pos += RX_BUFFERS * BUFFER_SIZE;
    g_pTxBuffer = (UINT8*)pos;

    // Prepare initialization block
    g_pInit[0] = 0x20500180;
    g_pInit[1] = INREG32((UINT32*)g_base);
    g_pInit[2] = INREG32((UINT32*)(g_base + 4));
    g_pInit[3] = 0;
    g_pInit[4] = 0;
    g_pInit[5] = OALVAtoPA(g_pRxRing);
    g_pInit[6] = OALVAtoPA(g_pTxRing);

    // Initialize RX ring   
    for (i = 0; i < RX_BUFFERS; i++) {
        g_pRxRing[4 * i + 0] = 0;
        g_pRxRing[4 * i + 1] = RMD1_OWN | RMD1_ONES | (4096 - BUFFER_SIZE);
        g_pRxRing[4 * i + 2] = OALVAtoPA(g_pRxBuffer + i * BUFFER_SIZE);
        g_pRxRing[4 * i + 3] = (UINT32)(g_pRxBuffer + i * BUFFER_SIZE);
    }
    g_rxPos = 0;
   
    // Initialize TX ring   
    for (i = 0; i < TX_BUFFERS; i++) {
        g_pTxRing[4 * i + 0] = 0;
        g_pTxRing[4 * i + 1] = 0;
        g_pTxRing[4 * i + 2] = OALVAtoPA(g_pTxBuffer + i * BUFFER_SIZE);
        g_pTxRing[4 * i + 3] = (UINT32)(g_pTxBuffer + i * BUFFER_SIZE);
    }
    g_txPos = 0;
   
    // Set initialization block address
    pos = OALVAtoPA(g_pInit);
    WriteCSR(1, pos & 0xFFFF);
    WriteCSR(2, pos >> 16);

    // Mask everything
    WriteCSR(3, 0x1F40); // Enable DXSUFLO to let it recover from underflow
    WriteCSR(4, 0x0914);

    // Start initialization
    WriteCSR(0, 0x0001);

    // Wait for initialization complete
    while ((ReadCSR(0) & 0x0100) == 0) OALStall(10);

    // Wait for link
    OALMSGS(OAL_WARN, (L"Am79c973: Wait for link..."));
    // First we must be out of reset
    while ((ReadPhy(0x1E, MAC_PHY_BMCR) & MAC_PHY_BMCR_RST) != 0);
    // Link status is lock low bit, so read it first time...
    ReadPhy(0x1E, MAC_PHY_BMSR);
    while ((ReadPhy(0x1E, MAC_PHY_BMSR) & MAC_PHY_BMSR_LINK) == 0);
    OALMSGS(OAL_WARN, (L" Link detected\r\n"));

    // Wait for while...
    OALStall(1000000);

    // Enable Tx/Rx
    WriteCSR(0, 0x0002);

    // Done
    return TRUE;
}

//------------------------------------------------------------------------------

BOOL AM79C973InitDMABuffer(UINT32 address, UINT32 size)
{
    BOOL rc = FALSE;
    UINT32 offset, buffers;

    OALMSGS(OAL_ETHER&&OAL_FUNC, (
        L"+AM79C973InitDMABuffer(0x%08x, 0x%08x)\r\n", address, size
    ));

    // Buffers must be aligned to 32 bytes boundary
    offset = address & 0x1F;
    if (offset != 0) {
        address = address + 0x20 - offset;
        size = size + 0x20 - offset;
    }

    // Check if buffer is big enough to accomodate all
    buffers = TX_BUFFERS + RX_BUFFERS;
    if (size < ((BUFFER_SIZE + DESC_SIZE) * buffers + INIT_SIZE)) {
        OALMSGS(OAL_ERROR, (
            L"ERROR: AM79C973InitDMABuffer: Buffer too small\r\n"
        ));
        goto cleanUp;
    }

    // Store address and size
    g_dmaAddress = (UINT32)OALCAtoUA(address);
    g_dmaSize = size;

    // Done
    rc = TRUE;

cleanUp:    
    OALMSGS(OAL_ETHER&&OAL_FUNC, (L"-AM79C973InitDMABuffer(rc = %d)\r\n", rc));
    return rc;
}

//------------------------------------------------------------------------------

BOOL AM79C973Init(UINT8 *pAddress, UINT32 offset, UINT16 mac[3])
{
    BOOL rc = FALSE;

    OALMSGS(OAL_ETHER&&OAL_FUNC, (
        L"+AM79C973Init(0x%08x, 0x%08x, 0x%08x)\r\n", pAddress, offset, mac
    ));
   
    g_base = (UINT32)pAddress;

    if (!HWInit()) goto cleanUp;
    
    // Set mac parameters 
    mac[0] = g_pInit[1] & 0xFFFF;
    mac[1] = g_pInit[1] >> 16;
    mac[2] = g_pInit[2] & 0xFFFF;

    // Done
    rc = TRUE;

cleanUp:    
    OALMSGS(OAL_ETHER&&OAL_FUNC, (L"-AM79C973Init(rc = %d)\r\n", rc));
    return rc;
}

//------------------------------------------------------------------------------

VOID AM79C973PowerOff()
{
    UINT32 exCtrl;

    // First we must go to suspend mode
    exCtrl = ReadCSR(5);
    exCtrl |= 0x0001;
    WriteCSR(5, exCtrl);
    
    // Wait until we get there
    while ((ReadCSR(5) & 0x0001) == 0) OALStall(10);

    // Res
}

//------------------------------------------------------------------------------

VOID AM79C973PowerOn()
{
    HWInit();
}

//------------------------------------------------------------------------------

UINT16 AM79C973SendFrame(UINT8 *pData, UINT32 length)
{
    UINT32 start;
    volatile UINT32 *pos;

    OALMSGS(OAL_ETHER&&OAL_VERBOSE, (
        L"+AM79C973SendFrame(0x%08x, %d)\r\n", pData, length
    ));

    // Check if packet fit to buffer    
    if (length > BUFFER_SIZE) return 1;

    // Wait until buffer is done
    pos = (volatile UINT32*)&g_pTxRing[g_txPos << 2];

    // Wait for transmit buffer available
    start = OALGetTickCount();
    while ((pos[1] & TMD1_OWN) != 0) {
        if ((OALGetTickCount() - start) > 2000) {
            OALMSGS(OAL_ERROR, (L"ERROR: AM79C973SendFrame: Send timeout\r\n"));
            return 1;
        }
    }

    // Copy data to buffer
    memcpy((VOID*)pos[3], pData, length);
    pos[0] = 0;
    pos[1] = TMD1_OWN|TMD1_STP|TMD1_ENP|TMD1_ONES|(4096 - length);

    // Force controller to read tx descriptor
    WriteCSR(0, (ReadCSR(0) & 0x0040) | 0x0008);

    // Move to next possition
    if (++g_txPos == TX_BUFFERS) g_txPos = 0;

    OALMSGS(OAL_ETHER&&OAL_VERBOSE, (L"-RAM79C973SendFrame(rc = 0)\r\n"));
    return 0;
}

//------------------------------------------------------------------------------

UINT16 AM79C973GetFrame(UINT8 *pData, UINT16 *pLength)
{
    UINT32 rmd1, rmd2, length;
    volatile UINT32 *pos;

    OALMSGS(OAL_ETHER&&OAL_VERBOSE, (
        L"+AM79C973GetFrame(0x%08x, %d)\r\n", pData, *pLength
    ));

    pos = (volatile UINT32 *)&g_pRxRing[g_rxPos << 2];
    length = 0;

    // Check if there is received frame 
    if ((ReadCSR(0) & 0x0400) != 0) {
        
        // When packet is in buffer hardware doesn own descriptor
        while (((rmd1 = pos[1]) & RMD1_OWN) == 0) {
            rmd2 = pos[0];
            // Is packet received ok?
            length = rmd2 & 0x0FFF;
            if (length > 4) length -= 4; 
            if ((rmd1 & RMD1_ERR) == 0 && length < *pLength) {
                // Copy packet if there is no problem
                memcpy(pData, (VOID*)pos[3], length);
            } else {
                OALMSGS(OAL_WARN, (
                    L"AM79C973GetFrame - %X/%X %d\n", rmd1, rmd2, *pLength
                ));
                length = 0;
            }
            // Reinitialize descriptor
            pos[0] = 0;
            pos[1] = RMD1_OWN | RMD1_ONES | (4096 - BUFFER_SIZE);
            // Move to next possition
            if (++g_rxPos == RX_BUFFERS) g_rxPos = 0;
            // Calculate position
            pos = (volatile UINT32 *)&g_pRxRing[g_rxPos << 2];
            // If this descriptor is owned by hardware clear interrupt
            if ((pos[1] & RMD1_OWN) != 0) {
                WriteCSR (0, (ReadCSR(0) & 0x0040) | 0x0400);
            }         
            // If we get a packet break loop
            if (length > 0) break;
        }

    }

    // Return size
    *pLength = (USHORT)length;

    OALMSGS(OAL_ETHER&&OAL_VERBOSE, (
        L"-AM79C973GetFrame(length = %d)\r\n", length
    ));
    return *pLength;
}

//------------------------------------------------------------------------------

void AM79C973EnableInts()
{
    OALMSGS(OAL_ETHER&&OAL_FUNC, (L"+AM79C973EnableInts\r\n"));
    WriteCSR(3, ReadCSR(3) & 0xFBFF);  // clear RINT mask
    WriteCSR(0, 0x40);
    OALMSGS(OAL_ETHER&&OAL_FUNC, (L"-AM79C973EnableInts\r\n"));
}

//------------------------------------------------------------------------------

void AM79C973DisableInts()
{
    OALMSGS(OAL_ETHER&&OAL_FUNC, (L"+AM79C973DisableInts\r\n"));
    WriteCSR(0, 0);
    OALMSGS(OAL_ETHER&&OAL_FUNC, (L"-AM79C973DisableInts\r\n"));
}

//------------------------------------------------------------------------------

VOID AM79C973CurrentPacketFilter(UINT32 filter)
{
    UINT32 exCtrl, mode;

    OALMSGS(OAL_ETHER&&OAL_FUNC, (
       L"+AM79C973CurrentPacketFilter(0x%08x)\r\n", filter
    ));

    // First we must go to suspend mode
    exCtrl = ReadCSR(5);
    exCtrl |= 0x0001;
    WriteCSR(5, exCtrl);
    
    // Wait until we get there
    while ((ReadCSR(5) & 0x0001) == 0) OALStall(10);

    // Just assume that we always receive direct & broadcast packets
    if ((filter & PACKET_TYPE_ALL_MULTICAST) != 0) {
        WriteCSR(8, 0xFFFF);
        WriteCSR(9, 0xFFFF);
        WriteCSR(10, 0xFFFF);
        WriteCSR(11, 0xFFFF);
    } else if ((filter & PACKET_TYPE_MULTICAST) == 0) {
        WriteCSR(8, 0);
        WriteCSR(9, 0);
        WriteCSR(10, 0);
        WriteCSR(11, 0);
    }

    mode = ReadCSR(15);
    if ((filter & PACKET_TYPE_PROMISCUOUS) != 0) {
        mode |= MODE_PROM;
    } else {
        mode &= ~MODE_PROM;
    }
    WriteCSR(15, mode);
    
    // It is time to leave suspend mode
    exCtrl = ReadCSR(5);
    exCtrl &= ~0x0001;
    WriteCSR(5, exCtrl);

    // Wait until we get there
    while ((ReadCSR(5) & 0x0001) != 0) OALStall(10);

    OALMSGS(OAL_ETHER&&OAL_FUNC, (L"-AM79C973CurrentPacketFilter\r\n"));
}

//------------------------------------------------------------------------------

BOOL AM79C973MulticastList(UINT8 *pAddresses, UINT32 count)
{
   ULONG exCtrl, crc;
   ULONG i, j, bit;
   USHORT h[4];

   OALMSGS(OAL_ETHER&&OAL_FUNC, (
       L"+AM79C973MulticastList(0x%08x, %d)\r\n", pAddresses, count
   ));

   // Calculate hash bits       
   h[0] = h[1] = h[2] = h[3] = 0;
   for (i = 0; i < count; i++) {
      crc = HashAddress(pAddresses);
      bit = 0;
      for (j = 0; j < 6; j++) bit = (bit << 1) + ((crc >> j) & 0x01);
      h[bit >> 4] |= 1 << (bit & 0x0F);
      pAddresses += ADDR_SIZE;
   }

   // Go to suspend mode
   exCtrl = ReadCSR(5);
   exCtrl |= 0x0001;
   WriteCSR(5, exCtrl);

   // Wait until we get there
   while ((ReadCSR(5) & 0x0001) == 0) OALStall(10);

   // And set hardware   
   WriteCSR(8, h[0]);
   WriteCSR(9, h[1]);
   WriteCSR(10, h[2]);
   WriteCSR(11, h[3]);

   // Leave suspend mode
   exCtrl = ReadCSR(5);
   exCtrl &= ~0x0001;
   WriteCSR(5, exCtrl);

   // Wait until we get there
   while ((ReadCSR(5) & 0x0001) != 0) OALStall(10);
   
   OALMSGS(OAL_ETHER&&OAL_FUNC, (L"-AM79C973MulticastList(rc = 1)\r\n"));
   return TRUE;
}

//------------------------------------------------------------------------------
