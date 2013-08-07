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
//  File: lan91c.c
//
//  This file implements ethernet debug driver for SMSC LAN91Cxxx network chip.
//
#include <windows.h>
#include <halether.h>
#include <oal.h>

//------------------------------------------------------------------------------
// Types

typedef struct {
    union {
        struct {            // Bank 0
            UINT16 TCR;     // 0000
            UINT16 EPHSR;   // 0002
            UINT16 RCR;     // 0004
            UINT16 ECR;     // 0006
            UINT16 MIR;     // 0008
            UINT16 MCRPCR;  // 000A
        };
        struct {            // Bank 1
            UINT16 CR;      // 0000
            UINT16 BAR;     // 0002
            UINT16 IAR0;    // 0004
            UINT16 IAR1;    // 0006
            UINT16 IAR2;    // 0008
            UINT16 GPR;     // 000A
            UINT16 CTR;     // 000C
        };
        struct {            // Bank 2
            UINT16 MMUCR;   // 0000
            UINT16 PNRARR;  // 0002
            UINT16 FIFO;    // 0004
            UINT16 PTR;     // 0006
            UINT16 DATA;    // 0008
            UINT16 DATAEX;  // 000A
            UINT16 INTR;    // 000C
        };
        struct {            // Bank 3
            UINT16 MT[4];   // 0000
            UINT16 MGMT;    // 0008
            UINT16 REV;     // 000A
            UINT16 ERDV;    // 000C
        };
    };
    UINT16 BANKSEL;         // 000E
} LAN91C_REGS;

//------------------------------------------------------------------------------
// Defines 
#define TIMEOUT_VALUE           30000       // 30 seconds.

#define CTR_STORE               (1 << 0)
#define CTR_RELOAD              (1 << 1)
#define CTR_TEEN                (1 << 5)
#define CTR_CREN                (1 << 6)
#define CTR_LEEN                (1 << 7)
#define CTR_BIT8                (1 << 8)
#define CTR_AR                  (1 << 11)

#define CR_SETSQLCH             (1 << 9)
#define CR_NOWAIT               (1 << 12)

#define TCR_TXEN                (1 << 0)
#define TCR_PADEN               (1 << 7)
#define TCR_FDSE                (1 << 15)

#define RCR_PRMS                (1 << 1)
#define RCR_ALMUL               (1 << 2)
#define RCR_RXEN                (1 << 8)
#define RCR_STRIP_CRC           (1 << 9)

#define INTR_RX                 (1 << 0)
#define INTR_TX                 (1 << 1)
#define INTR_TX_EMPTY           (1 << 2)
#define INTR_ALLOC              (1 << 3)
#define INTR_RX_OVRN            (1 << 4)
#define INTR_EPH                (1 << 5)
#define INTR_ERCV               (1 << 6)
#define INTR_TX_IDLE            (1 << 7)
#define INTR_RX_MASK            (1 << 8)
#define INTR_TX_MASK            (1 << 9)
#define INTR_TX_EMPTY_MASK      (1 << 10)
#define INTR_ALLOC_MASK         (1 << 11)
#define INTR_RX_OVRN_MASK       (1 << 12)
#define INTR_EPH_MASK           (1 << 13)
#define INTR_ERCV_MASK          (1 << 14)
#define INTR_TX_IDLE_MASK       (1 << 15)

#define EPH_STAT_TXSUC          (1 << 0)
#define EPH_STAT_SNGLCOL        (1 << 1)
#define EPH_STAT_MULTCOL        (1 << 2)
#define EPH_STAT_LTXMULT        (1 << 3)
#define EPH_STAT_16COL          (1 << 4)
#define EPH_STAT_SQET           (1 << 5)
#define EPH_STAT_LTXBRD         (1 << 6)
#define EPH_STAT_TXDEFR         (1 << 7)
#define EPH_STAT_LATCOL         (1 << 9)
#define EPH_STAT_LOSTCARR       (1 << 10)
#define EPH_STAT_EXCDEV         (1 << 11)
#define EPH_STAT_CTRROLL        (1 << 12)
#define EPH_STAT_LINKOK         (1 << 14)
#define EPH_STAT_TXUNRN         (1 << 15)

#define PTR_RCV                 (1 << 15)
#define PTR_AUTOINC             (1 << 14)
#define PTR_READ                (1 << 13)

#define MMUCR_BUSY              (1 << 0)    // MMU busy, don't modify PNR
#define MMUCR_NOP               (0 << 4)    // No-Op command
#define MMUCR_ALLOC             (2 << 4)    // Allocate memory
#define MMUCR_RESET             (4 << 4)    // Reset MMU to initial state
#define MMUCR_REM_TOP           (6 << 4)    // Remove frame from top of RX fifo
#define MMUCR_REM_REL_TOP       (8 << 4)    // Remove and release top of RX fifo
#define MMUCR_REL_SPEC          (10 << 4)   // Release specific packet
#define MMUCR_ENQ_TX            (12 << 4)   // Enqueue to xmit fifo
#define MMUCR_ENQ_RX            (14 << 4)   // Reset xmit fifos

#define MMUCR_111_ALLOC_TX      (1 << 5)    // Allocate Tx memory
#define MMUCR_111_RESET_MMU     (2 << 5)    // Reset the MMU
#define MMUCR_111_REMOVE_RX     (3 << 5)    // Delete top Rx FIFO buffer
#define MMUCR_111_REM_REL_RX    (4 << 5)    // Delete and release Rx FIFO buffer
#define MMUCR_111_RELEASE_RX    (5 << 5)    // Release Rx buffer memory
#define MMUCR_111_RELEASE_TX    (5 << 5)    // Release Tx buffer memory
#define MMUCR_111_ENQUEUE       (6 << 5)    // Queue Tx buffer
#define MMUCR_111_RESET_TX      (7 << 5)    // 

#define STAT_ALGNERR            (1 << 15)
#define STAT_BADCRC             (1 << 13)
#define STAT_LONG               (1 << 11)
#define STAT_SHORT              (1 << 10)

#define CTRL_ODD                (1 << 13)
#define CTRL_CRC                (1 << 12)

#define MGMT_MDO                (1 << 0)
#define MGMT_MDI                (1 << 1)
#define MGMT_MCLK               (1 << 2)
#define MGMT_MDOE               (1 << 3)
#define MGMT_MSK_CRS100         (3 << 14)

#define EPHSR_LINKOK            (1 << 14)

// PHY register definitions.
#define CONTROL_MII_DIS         0x3000
#define CONTROL_LPBK            0x7000

#define GET_CHIP_ID(a)          ((a >> 4) & 0xF)
#define GET_REV_ID(a)           (a & 0xF)

#define CHIP_ID_LAN91C90        3
#define CHIP_ID_LAN91C92        3
#define CHIP_ID_LAN91C94        4
#define CHIP_ID_LAN91C95        5
#define CHIP_ID_LAN91C96        4           // revision ID starts at 6
#define CHIP_ID_LAN91C100       7
#define CHIP_ID_LAN91C100FD     8
#define CHIP_ID_LAN91C110       9
#define CHIP_ID_LAN91C111       9

//------------------------------------------------------------------------------
// Local Variables 

static LAN91C_REGS *g_pLAN91C;
static UINT16 g_chipRevision;

//------------------------------------------------------------------------------
// Local Functions 

static UINT32 Crc(UINT8 *pAddress);
static VOID PhyWrite(UINT8 address, UINT8 reg, UINT16 data);

//------------------------------------------------------------------------------

BOOL LAN91CInit(UINT8 *pAddress, UINT32 offset, UINT16 mac[3])
{
    BOOL rc = FALSE;

    OALMSGS(OAL_ETHER&&OAL_FUNC, (
        L"+LAN91CInit(0x%08x, 0x%08x, 0x%08x)\r\n", pAddress, offset, mac
    ));

    // Save address
    g_pLAN91C = (LAN91C_REGS*)pAddress;

    // Chip settle time.
    OALStall(750);

    // Verify that network chip can be detected
    if ((INPORT16(&g_pLAN91C->BANKSEL) & 0xFF00) != 0x3300) {

        OALMSGS(OAL_ERROR, (
            L"ERROR: LAN91CInit: Network Chip not found at 0x%08x\r\n", pAddress
        ));
        goto cleanUp;
    }

    // Select bank 3 and read the chip ID and revision.
    OUTPORT16(&g_pLAN91C->BANKSEL, 3);
    g_chipRevision = INPORT16(&g_pLAN91C->REV);
    OALMSGS(TRUE, (
        L"LAN91Cxxx: Chip Id %d Revision %d\r\n", 
        GET_CHIP_ID(g_chipRevision), GET_REV_ID(g_chipRevision)
    ));
           
    // Select bank 1    
    OUTPORT16(&g_pLAN91C->BANKSEL, 1);

    // Wait until reset & EEPROM load is done
    OUTPORT16(&g_pLAN91C->CTR, CTR_RELOAD);
    while ((INPORT16(&g_pLAN91C->CTR) & (CTR_RELOAD|CTR_STORE)) != 0);

    // Get MAC address from chip
    mac[0] = INPORT16(&g_pLAN91C->IAR0);
    mac[1] = INPORT16(&g_pLAN91C->IAR1);
    mac[2] = INPORT16(&g_pLAN91C->IAR2);

    // Initialize the control register
    switch (GET_CHIP_ID(g_chipRevision)) {
    case CHIP_ID_LAN91C111:
        OUTPORT16(&g_pLAN91C->CTR, CTR_TEEN);
        // The LAN91C111's internal PHY is disabled at boot time - enable it.
        PhyWrite(0, 0, CONTROL_MII_DIS);
        // Enable auto-negotiation of link speed.
        OUTPORT16(&g_pLAN91C->BANKSEL, 0);
        OUTPORT16(&g_pLAN91C->MCRPCR, 0x0800);
        break;
    default:
        // Set SQUELCH & NO WAIT bits
        OUTPORT16(&g_pLAN91C->BANKSEL, 1);
        SETPORT16(&g_pLAN91C->CR, CR_SETSQLCH|CR_NOWAIT);
        OUTPORT16(&g_pLAN91C->CTR, CTR_BIT8|CTR_TEEN);
        // Memory configuration register value.
        OUTPORT16(&g_pLAN91C->BANKSEL, 0);
        OUTPORT16(&g_pLAN91C->MCRPCR, 0x0006);
    }

    // Initialize transmit control register
    OUTPORT16(&g_pLAN91C->BANKSEL, 0);
    OUTPORT16(&g_pLAN91C->TCR, TCR_PADEN|TCR_TXEN);

    // Initialize interrupt mask register (all ints disabled to start)
    OUTPORT16(&g_pLAN91C->BANKSEL, 2);
    OUTPORT16(&g_pLAN91C->INTR, 0);

    // Initialize the Receive Control Register
    OUTPORT16(&g_pLAN91C->BANKSEL, 0);
    OUTPORT16(&g_pLAN91C->RCR, RCR_RXEN|RCR_STRIP_CRC);

    // We are done
    rc = TRUE;

cleanUp:
    OALMSGS(OAL_ETHER&&OAL_FUNC, (
        L"-LAN91CInit(mac = %02x:%02x:%02x:%02x:%02x:%02x, rc = %d)\r\n",
        mac[0]&0xFF, mac[0]>>8, mac[1]&0xFF, mac[1]>>8, mac[2]&0xFF, mac[2]>>8,
        rc
    ));

    return rc;
}


//------------------------------------------------------------------------------

UINT16 LAN91CSendFrame(UINT8 *pBuffer, UINT32 length)
{
    UINT16 rc = 0;
    UINT16 bufferSize, frameHandle;
    UINT16 packetNumber;
    UINT32 startTime;
    static BOOLEAN bAllocRequest = FALSE;

    // Calculate the amount of memory needed (must be an even number)
    bufferSize = 2 + 2 + (UINT16)length + 1;
    if ((bufferSize & 1) != 0) bufferSize++;

    switch (GET_CHIP_ID(g_chipRevision)) {
    case CHIP_ID_LAN91C111:
        // Make sure there's enough free Tx memory.
        OUTPORT16(&g_pLAN91C->BANKSEL, 0);
        if ((INPORT16(&g_pLAN91C->MIR) >> 8) == 0) {
            // No memory?  Reset the MMU.
            OUTPORT16(&g_pLAN91C->BANKSEL, 2);
            OUTPORT16(&g_pLAN91C->MMUCR, MMUCR_111_RESET_MMU);
            while ((INPORT16(&g_pLAN91C->MMUCR) & MMUCR_BUSY) != 0);

            bAllocRequest = FALSE;
        }
        // Allocate memory in the buffer for the frame
        OUTPORT16(&g_pLAN91C->BANKSEL, 2);
        if (!bAllocRequest)
        {
            OUTPORT16(&g_pLAN91C->MMUCR, MMUCR_111_ALLOC_TX);
            bAllocRequest = TRUE;
        }
        break;
    default:
        // Allocate memory in the buffer for the frame
        OUTPORT16(&g_pLAN91C->BANKSEL, 2);
        OUTPORT16(&g_pLAN91C->MMUCR, MMUCR_ALLOC|(bufferSize >> 8));
    }

    // Loop until the request is satisfied
    startTime = OALGetTickCount();
    while ((OALGetTickCount() - startTime) < TIMEOUT_VALUE)
    {
        if (INPORT16(&g_pLAN91C->INTR) & INTR_ALLOC)
        {
            break;
        }
    }
    // If we couldn't satisfy the allocation, abort - we'll give the
    // receive side time to free up some space in the MMU, then we'll
    // re-check whether the allocation succeeded on the next call.
    if ((INPORT16(&g_pLAN91C->INTR) & INTR_ALLOC) == 0)
    {
        OALMSGS(OAL_ERROR, (L"ERROR: LAN91CSendFrame: Timed out allocating frame.\r\n"));
        return(1);
    }

    bAllocRequest = FALSE;

    // Make sure the allocation didn't fail.
    if (INPORT16(&g_pLAN91C->PNRARR) & 0x8000)
    {
        OALMSGS(OAL_ERROR, (L"ERROR: LAN91CSendFrame: Failed to allocate frame.\r\n"));
        return(1);
    }

    // Get frame handle
    frameHandle = (0x3f00 & INPORT16(&g_pLAN91C->PNRARR)) >> 8;

    // Now write the frame into the buffer
    OUTPORT16(&g_pLAN91C->PNRARR, frameHandle);
    OUTPORT16(&g_pLAN91C->PTR, PTR_AUTOINC);

    // Write status word
    OUTPORT16(&g_pLAN91C->DATA, 0);
    // Write the buffer size
    OUTPORT16(&g_pLAN91C->DATA, bufferSize);

    // Now write all except possibly the last data byte
    while (length > 1) {
        OUTPORT16(&g_pLAN91C->DATA, *(UINT16*)pBuffer);
        pBuffer += sizeof(UINT16);
        length -= sizeof(UINT16);
    }

    if (length > 0) {
        // If length was odd we can put that just before the control byte
        OUTPORT16(&g_pLAN91C->DATA, *pBuffer|CTRL_ODD|CTRL_CRC);
    } else {
        // Otherwise just pad the last byte with 0
        OUTPORT16(&g_pLAN91C->DATA, CTRL_CRC);
    }        

    // Enqueue Frame number into TX FIFO
    switch (GET_CHIP_ID(g_chipRevision)) {
    case CHIP_ID_LAN91C111:
        OUTPORT16(&g_pLAN91C->MMUCR, MMUCR_111_ENQUEUE);
        break;
    default:        
        OUTPORT16(&g_pLAN91C->MMUCR, MMUCR_ENQ_TX);
    }

    // Wait until it is sent or an error is generated.
    startTime = OALGetTickCount();
    while ((OALGetTickCount() - startTime) < TIMEOUT_VALUE)
    {
        if (INPORT16(&g_pLAN91C->INTR) & INTR_TX)
        {
            break;
        }
    }

    if ((INPORT16(&g_pLAN91C->INTR) & INTR_TX) == 0)
    {
        OALMSGS(OAL_ERROR, (L"ERROR: LAN91CSendFrame: Timed out waiting for the transfer to complete.\r\n"));
        return(1);
    }

	// Read TXDONE Pkt# from FIFO Port Register
    OUTPORT16(&g_pLAN91C->BANKSEL, 2);
    packetNumber = (INPORT16(&g_pLAN91C->FIFO) & 0x3F);

	// Write to Packet Number Register
    OUTPORT16(&g_pLAN91C->PNRARR, packetNumber);

	// Retrieve packet status
    OUTPORT16(&g_pLAN91C->PTR, (PTR_AUTOINC | PTR_READ));
    OALStall(100);

    rc = INPORT16(&g_pLAN91C->DATA);
    // SQET bit always set on lan91c111 and lan100FD (so mask it) on all plats
    rc &= ~EPH_STAT_SQET;

	// Release the packet
    switch (GET_CHIP_ID(g_chipRevision)) {
    case CHIP_ID_LAN91C111:
        OUTPORT16(&g_pLAN91C->MMUCR, MMUCR_111_RELEASE_TX);
        break;
    default:            
        OUTPORT16(&g_pLAN91C->MMUCR, MMUCR_REL_SPEC);
    }
    while ((INPORT16(&g_pLAN91C->MMUCR) & MMUCR_BUSY) != 0);

	// Clear the tx interrupt status
    SETPORT16(&g_pLAN91C->INTR, INTR_TX);

    // Tx error?
    if (rc & (EPH_STAT_TXUNRN | EPH_STAT_SQET | EPH_STAT_LOSTCARR | EPH_STAT_LATCOL | EPH_STAT_16COL))
    {
        // Display error status.
        OALMSGS(OAL_ERROR, (L"ERROR: LAN91CSendFrame: status = ( "));
        if (rc & EPH_STAT_TXUNRN)
        {
            OALMSGS(OAL_ERROR, (L"TXUNRN "));
        }
        if (rc & EPH_STAT_SQET)
        {
            OALMSGS(OAL_ERROR, (L"SQET "));
        }
        if (rc & EPH_STAT_LOSTCARR)
        {
            OALMSGS(OAL_ERROR, (L"LOSTCARR "));
        }
        if (rc & EPH_STAT_LATCOL)
        {
            OALMSGS(OAL_ERROR, (L"LATCOL "));
        }
        if (rc & EPH_STAT_16COL)
        {
            OALMSGS(OAL_ERROR, (L"16COL "));
        }
        OALMSGS(OAL_ERROR, (L")\r\n"));
            
        // Re-enable TXENA
        OUTPORT16(&g_pLAN91C->BANKSEL, 0);
        OUTPORT16(&g_pLAN91C->TCR, TCR_PADEN|TCR_TXEN);

        // Failure.
        rc = 1;
    }
    else
    {
        // Success.
        rc = 0;
    }

    // Clear the statistics registers
    OUTPORT16(&g_pLAN91C->BANKSEL, 0);
    INPORT16(&g_pLAN91C->ECR);

    // Set back bank 2
    OUTPORT16(&g_pLAN91C->BANKSEL, 2);

    return rc;
}

//------------------------------------------------------------------------------

UINT16 LAN91CGetFrame(UINT8 *pBuffer, UINT16 *pLength)
{
    UINT8 *pos = pBuffer;
    UINT16 code, pointer;
    UINT32 length, count;
    BOOL   bErr = FALSE; 

    // Make sure that bank 2 is actual
    OUTPORT16(&g_pLAN91C->BANKSEL, 2);

    length = 0;
    while ((INPORT16(&g_pLAN91C->INTR) & INTR_RX) != 0) {

        // Setup pointer address register
        pointer = PTR_RCV | PTR_READ;

        // Read status
        OUTPORT16(&g_pLAN91C->PTR, pointer);
        code = INPORT16(&g_pLAN91C->DATA);
        pointer += sizeof(UINT16);

        if ((code & (STAT_ALGNERR|STAT_BADCRC|STAT_LONG|STAT_SHORT)) == 0) {
        

            // Get packet size
            OUTPORT16(&g_pLAN91C->PTR, pointer);
            length = (INPORT16(&g_pLAN91C->DATA) & 0x07FF) - 6;
            pointer += sizeof(UINT16);

            //  Check packet size
            if( length <= *pLength )
            {
                // Copy packet
                count = length;
                while (count > 1) {
                    OUTPORT16(&g_pLAN91C->PTR, pointer);
                    *(UINT16*)pos = INPORT16(&g_pLAN91C->DATA);
                    pointer += sizeof(UINT16);
                    pos += sizeof(UINT16);
                    count -= sizeof(UINT16);
                }


                // Get control word (which can contain last byte)
                OUTPORT16(&g_pLAN91C->PTR, pointer);
                code = INPORT16(&g_pLAN91C->DATA);
                pointer += sizeof(UINT16);
                if ((code & CTRL_ODD) != 0) {
                    length++;
                    *pos = (UINT8)code;
                }
            }
            else
            {
                //  Error getting the packet size
                OALMSGS(OAL_ERROR, (L"ERROR: LAN91CGetFrame: packet size (%d) > than buf len (%d)\r\n", length, *pLength ));
                length = 0;
                bErr = TRUE;
            }
        }

        // Release the memory for the received frame
        switch (GET_CHIP_ID(g_chipRevision)) {

            case CHIP_ID_LAN91C111:
                OUTPORT16(&g_pLAN91C->MMUCR, MMUCR_111_REM_REL_RX);
                break;

            default:            
                OUTPORT16(&g_pLAN91C->MMUCR, MMUCR_REM_REL_TOP);
        }

        while ((INPORT16(&g_pLAN91C->MMUCR) & MMUCR_BUSY) != 0);


        // If error, break
        if( bErr ) break;

        // If length is non zero we get a packet
        if (length > 0) break;

    }        

    *pLength = (UINT16)length;
    return (*pLength);
}


//------------------------------------------------------------------------------

VOID LAN91CEnableInts()
{
    OALMSGS(OAL_ETHER&&OAL_FUNC, (L"+LAN91CEnableInts\r\n"));

    // Only enable receive interrupts (we poll for Tx completion)
    OUTPORT16(&g_pLAN91C->BANKSEL, 2);
    OUTPORT16(&g_pLAN91C->INTR, INTR_RX_MASK);

    OALMSGS(OAL_ETHER&&OAL_FUNC, (L"-LAN91CEnableInts\r\n"));
}

//------------------------------------------------------------------------------

VOID LAN91CDisableInts()
{
    OALMSGS(OAL_ETHER&&OAL_FUNC, (L"+LAN91CDisableInts\r\n"));

    // Disable all interrupts
    OUTPORT16(&g_pLAN91C->BANKSEL, 2);
    OUTPORT16(&g_pLAN91C->INTR, 0);

    OALMSGS(OAL_ETHER&&OAL_FUNC, (L"-LAN91CDisableInts\r\n"));
}

//------------------------------------------------------------------------------

VOID LAN91CCurrentPacketFilter(UINT32 filter)
{
    UINT16 rcr = RCR_RXEN|RCR_STRIP_CRC;

    OALMSGS(OAL_ETHER&&OAL_FUNC, (
        L"+LAN91CCurrentPacketFilter(0x%08x)\r\n", filter
    ));

    if ((filter & PACKET_TYPE_ALL_MULTICAST) != 0) rcr |= RCR_ALMUL;
    if ((filter & PACKET_TYPE_PROMISCUOUS) != 0) rcr |= RCR_PRMS;

    OUTPORT16(&g_pLAN91C->BANKSEL, 0);
    OUTPORT16(&g_pLAN91C->RCR, rcr);

    OALMSGS(OAL_ETHER&&OAL_FUNC, (L"-LAN91CCurrentPacketFilter\r\n"));
}

//------------------------------------------------------------------------------

BOOL LAN91CMulticastList(UINT8 *pAddresses, UINT32 count)
{
    UINT32 crc, i;
    UINT16 h[4];

    OALMSGS(OAL_ETHER&&OAL_FUNC, (
        L"+LAN91CMulticastList(0x%08x, %d)\r\n", pAddresses, count
    ));

    // Calculate hash bits       
    h[0] = h[1] = h[2] = h[3] = 0;
    for (i = 0; i < count; i++) {
        crc = Crc(pAddresses);
        h[crc >> 30] |= 1 << ((crc >> 26) & 0x0F);
        pAddresses += 6;
    }

    // Write it to hardware
    OUTPORT16(&g_pLAN91C->BANKSEL, 3);
    OUTPORT16(&g_pLAN91C->MT[0], h[0]);
    OUTPORT16(&g_pLAN91C->MT[1], h[1]);
    OUTPORT16(&g_pLAN91C->MT[2], h[2]);
    OUTPORT16(&g_pLAN91C->MT[3], h[3]);

    OALMSGS(OAL_ETHER&&OAL_FUNC, (L"-LAN91CMulticastList(rc = 1)\r\n"));
    return TRUE;
}

//------------------------------------------------------------------------------

UINT32 Crc(UINT8 *pAddress)
{
    UINT32 crc, carry;
    UINT32 i, j;
    UINT8 uc;

    crc = 0xFFFFFFFF;
    for (i = 0; i < 6; i++) {
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

static VOID PhyWrite(UINT8 PHYaddr, UINT8 PHYreg, UINT16 PHYdata)
{
    INT32 i;
    UINT16 mask;
    UINT16 mii_reg;
    UINT8 bits[65];
    INT32 clk_idx = 0;

    // 32 consecutive ones on MDO to establish sync.
    //
    for (i = 0; i < 32; ++i)
    {
        bits[clk_idx++] = MGMT_MDOE | MGMT_MDO;
    }

    // Start code <01>.
    //
    bits[clk_idx++] = MGMT_MDOE;
    bits[clk_idx++] = MGMT_MDOE | MGMT_MDO;

    // Write command <01>.
    //
    bits[clk_idx++] = MGMT_MDOE;
    bits[clk_idx++] = MGMT_MDOE | MGMT_MDO;

    // Output the PHY address, msb first.
    //
    mask = (UINT8)0x10;
    for (i = 0; i < 5; ++i)
    {
        if (PHYaddr & mask)
            bits[clk_idx++] = MGMT_MDOE | MGMT_MDO;
        else
            bits[clk_idx++] = MGMT_MDOE;

        // Shift to next lowest bit.
        mask >>= 1;
    }

    // Output the PHY register number, msb first.
    //
    mask = (UINT8)0x10;
    for (i = 0; i < 5; ++i)
    {
        if (PHYreg & mask)
            bits[clk_idx++] = MGMT_MDOE | MGMT_MDO;
        else
            bits[clk_idx++] = MGMT_MDOE;

        // Shift to next lowest bit.
        mask >>= 1;
    }

    // Tristate and turnaround (2 bit times).
    //
    bits[clk_idx++] = 0;
    bits[clk_idx++] = 0;

    // Write out 16 bits of data, msb first.
    //
    mask = 0x8000;
    for (i = 0; i < 16; ++i)
    {
        if (PHYdata & mask)
            bits[clk_idx++] = MGMT_MDOE | MGMT_MDO;
        else
            bits[clk_idx++] = MGMT_MDOE;

        // Shift to next lowest bit.
        mask >>= 1;
    }

    // Final clock bit (tristate).
    //
    bits[clk_idx++] = 0;

    // Select bank 3.
    //
    OUTPORT16(&g_pLAN91C->BANKSEL, 3);

    // Get the current MII register value.
    //
    mii_reg = INPORT16(&g_pLAN91C->MGMT);

    // Turn off all MII Interface bits.
    //
    mii_reg &= ~(MGMT_MDOE | MGMT_MCLK | 
                 MGMT_MDI  | MGMT_MDO);

    // Clock all cycles.
    //
    for (i = 0; i < sizeof bits; ++i)
    {
        // Clock Low - output data.
        //
        OUTPORT16(&g_pLAN91C->MGMT, mii_reg | bits[i]);
        OALStall(50);

        // Clock Hi - input data.
        //
        OUTPORT16(&g_pLAN91C->MGMT, (UINT16)(mii_reg | bits[i] | MGMT_MCLK));
        OALStall(50);

        bits[i] |= INPORT16(&g_pLAN91C->MGMT) & MGMT_MDI;
    }

    // Return to idle state.  Set clock to low, data to low, and output tristated.
    //
    OUTPORT16(&g_pLAN91C->MGMT, mii_reg);
    OALStall(50);

}

//------------------------------------------------------------------------------

