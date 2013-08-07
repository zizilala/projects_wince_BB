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
//  File:  kitlserial.c
//
#include <windows.h>
#include <nkintr.h>
#include <kitl.h>
#include <oal.h>

//------------------------------------------------------------------------------

#define OAL_KITL_SERIAL_PACKET      0xAA

static const UINT8 g_kitlSign[] = { 'k', 'I', 'T', 'L' };

static struct {
    OAL_KITL_SERIAL_DRIVER *pDriver;
    KITL_SERIAL_INFO info;
    UINT32 recvBuffer[KITL_MTU/sizeof(UINT32)];
    UINT16 recvCount;
    UINT16 recvPayload;
} g_kitlSerialState;

#include <pshpack1.h>
typedef struct {
    UINT32 signature;
    UCHAR  packetType;
    UCHAR  reserved;
    USHORT payloadSize; // packet size, not including this header
    UCHAR  crcData;
    UCHAR  crcHeader;
} OAL_KITL_SERIAL_HEADER;
#include <poppack.h>

// header checksum size == size of the header minus the checksum itself
#define HEADER_CHKSUM_SIZE      (sizeof (OAL_KITL_SERIAL_HEADER) - sizeof (UCHAR))

//------------------------------------------------------------------------------
//
//  Function:  ChkSum
//
//  Calculate 8-bit check sum
//
static UINT8 ChkSum(UINT8 *pBuffer, UINT16 length)
{
    UINT8 sum = 0;
    int   nLen = length;

    while (nLen -- > 0) sum += *pBuffer++;
    return sum;
}

//------------------------------------------------------------------------------
//
//  Function:  SerialSend
//
//  This function is called by KITL to send packet over transport layer
//
static BOOL SerialSend(UINT8 *pData, UINT16 length)
{
    UINT16 bestSize = (UINT16)g_kitlSerialState.info.bestSize;
    UINT16 total = length;
    UINT16 toSend, sent;

    KITL_RETAILMSG(ZONE_SEND, (
        "+SerialSend(0x%08x, %d)\r\n", pData, length
    ));

    // block until send is complete; no timeout
    while (total > 0) {
        toSend = (total > bestSize) ? bestSize : total;
        sent = g_kitlSerialState.pDriver->pfnSend(pData, toSend);
        total -= sent;
        pData += sent;
    }

    // notify driver packet end if function exist (USB serial would need this)
    if (g_kitlSerialState.pDriver->pfnSendComplete != NULL) {
        g_kitlSerialState.pDriver->pfnSendComplete(length);
    }
    
        KITL_RETAILMSG(ZONE_SEND, ("-SerialSend(rc = 1)\r\n"));
    return TRUE;
}

//
// We'll retry a few times when there is no data on the wire before toggling the 
// flow control. Some desktop machines would gets interrupt-stormed if we toggle
// the control line too fast.
//
#define RECV_RETRY  200

//------------------------------------------------------------------------------
//
//  Function:  SerialRecv
//
//  This function is called by KITL to receive packet from transport layer
//
static BOOL SerialRecv(UINT8 *pData, UINT16 *pLength)
{
    UINT8 *pBuffer = (UINT8*)g_kitlSerialState.recvBuffer;
    OAL_KITL_SERIAL_HEADER *pHeader = (OAL_KITL_SERIAL_HEADER*)pBuffer;
    UINT16 bestSize = (UINT16)g_kitlSerialState.info.bestSize;
    UINT16 recvCount = g_kitlSerialState.recvCount;
    UINT16 recvPayload = g_kitlSerialState.recvPayload;
    UINT16 count, rc = 0;
    DWORD  dwRetry = 0;
    UINT8  cSum;

   
    KITL_RETAILMSG(ZONE_RECV, (
        "+SerialRecv(0x%08x, %d)\r\n", pData, *pLength
    ));


    if (g_kitlSerialState.pDriver->pfnFlowControl) {
        g_kitlSerialState.pDriver->pfnFlowControl (TRUE);
    }

    // Loop till there are data or we received a full packet
    while (recvCount < (sizeof(OAL_KITL_SERIAL_HEADER) + recvPayload)) {

        // Call driver read function, with best size
        count = g_kitlSerialState.pDriver->pfnRecv(
            &pBuffer[recvCount], bestSize
        );

        // Break loop when no are avaiable
        if (count == 0) {

            if (g_kitlSerialState.pDriver->pfnFlowControl
                && (dwRetry ++ < RECV_RETRY)) {
                continue;
            }
            break;
        }

        // Update amount of data received
        recvCount += count;

        // When we already know packet payload we can try next read
        if (recvPayload != 0) continue;
        
        // If we don't get full header just verify header
        while (recvCount >= sizeof(OAL_KITL_SERIAL_HEADER)) {
            // When we get full signature break loop and continue
            if (memcmp(g_kitlSign, pBuffer, sizeof(g_kitlSign)) == 0) break;
            // Shift buffer by one byte and try again
            for (count = 1; count < recvCount;  count++) {
                pBuffer[count - 1] = pBuffer[count];
            }
            recvCount--;
        }            

        // Did we received header?
        if (recvCount >= sizeof(OAL_KITL_SERIAL_HEADER)) {
            // Validate header checksum, when we just received the full header

            if ((cSum = ChkSum(pBuffer, HEADER_CHKSUM_SIZE)) != pHeader->crcHeader)
            {
                // Checksum failure, discard the packet
                    KITL_RETAILMSG(ZONE_WARNING, ("WARN: SerialRecv: "
                    "Header checksum error, Rec = 0x%02x, Calc'd = 0x%02x, discarded...\r\n",
                    pHeader->crcHeader,cSum
                ));
                recvCount = 0;
                continue;
            }
            else if (OAL_KITL_SERIAL_PACKET != pHeader->packetType)
            {
                // Checksum failure, discard the packet
                    KITL_RETAILMSG(ZONE_WARNING, ("WARN: SerialRecv: "
                    "Packet type wrong, Rec = 0x%02x, Desired = 0x%02x, discarded...\r\n",
                    pHeader->packetType,OAL_KITL_SERIAL_PACKET
                ));
                recvCount = 0;
                continue;
            }
            else if (KITL_MTU - sizeof(OAL_KITL_SERIAL_HEADER) < pHeader->payloadSize)
            {
                // Checksum failure, discard the packet
                    KITL_RETAILMSG(ZONE_WARNING, ("WARN: SerialRecv: "
                    "payload size error, Rec = %d > Max Desired = %d, discarded...\r\n",
                    pHeader->payloadSize, KITL_MTU - sizeof(OAL_KITL_SERIAL_HEADER)
                ));
                recvCount = 0;
                continue;
            }
            // Now we know how large payload should be
            recvPayload = pHeader->payloadSize;
        }
    }

    // Did we receive full packet? If no leave for now...
    if (
        recvPayload == 0 ||
        recvCount < (sizeof(OAL_KITL_SERIAL_HEADER) + recvPayload)
    ) goto cleanUp;


    // Received the full packet, calculate checksum
    if (ChkSum(
        pBuffer + sizeof(OAL_KITL_SERIAL_HEADER), recvPayload
    ) != pHeader->crcData) {
            KITL_RETAILMSG(ZONE_WARNING, ("WARN: SerialRecv: "
            "Packet checksum error, discarded...\r\n"
        ));
        recvCount = recvPayload = 0;
        goto cleanUp;
    }

    // Valid packet received, check length requested
    if (*pLength < sizeof(OAL_KITL_SERIAL_HEADER) + recvPayload) {
        KITL_RETAILMSG(ZONE_WARNING, ("ERROR: SerialRecv: "
            "Received packet too large, discarded...\r\n"
        ));
        recvCount = recvPayload = 0;
        goto cleanUp;
    }        

    // We have complete packet, indicate it up...
    memcpy(pData, pBuffer, sizeof(OAL_KITL_SERIAL_HEADER) + recvPayload);
    rc = sizeof(OAL_KITL_SERIAL_HEADER) + recvPayload;
    recvCount = recvPayload = 0;

cleanUp:

    // Save state variable
    g_kitlSerialState.recvCount = recvCount;
    g_kitlSerialState.recvPayload = recvPayload;
    *pLength = rc;

    if (g_kitlSerialState.pDriver->pfnFlowControl) {
        g_kitlSerialState.pDriver->pfnFlowControl (FALSE);
    }

        KITL_RETAILMSG(ZONE_RECV, ("-SerialRecv(*pLength = %d)\r\n", rc));
    return rc != 0;
}

//------------------------------------------------------------------------------
//
//  Function:  SerialEndcode
//
//  This function encode packet for serial transport. It simply add header
//  structure at the start of frame.
//
static BOOL SerialEncode(UINT8 *pFrame, UINT16 size)
{
    OAL_KITL_SERIAL_HEADER *pHeader = (OAL_KITL_SERIAL_HEADER*)pFrame;

    memcpy(&pHeader->signature, g_kitlSign, sizeof(pHeader->signature));
    pHeader->packetType = OAL_KITL_SERIAL_PACKET;
    pHeader->payloadSize = size;
    pHeader->crcData = ChkSum(pFrame + sizeof(OAL_KITL_SERIAL_HEADER), size);
    pHeader->crcHeader = ChkSum(pFrame, HEADER_CHKSUM_SIZE);
    return TRUE;
}

//------------------------------------------------------------------------------
//
//  Function:  SerialDecode
//
//  This function decode packet from serial transport. It verified packet
//  header and return pointer to packet body plus body size.
//
static UINT8* SerialDecode(UINT8 *pFrame, UINT16 *pSize)
{
    UINT8 *pData;
    
    *pSize -= sizeof(OAL_KITL_SERIAL_HEADER);
    pData = pFrame + sizeof(OAL_KITL_SERIAL_HEADER);
    return pData;
}

//------------------------------------------------------------------------------
//
//  Function:  SerialEnableInt
//
//  This function is called by KITL to enable and disable the KITL transport
//  interrupt if the transport is interrupt-based
//
static void SerialEnableInt(BOOL enable)
{
    if (enable) {
        g_kitlSerialState.pDriver->pfnEnableInts();
    } else {
        g_kitlSerialState.pDriver->pfnDisableInts();
    }        
}

//------------------------------------------------------------------------------
//
//  Function:  SerialGetDevCfg
//
//  This function is called by KITL to get information about transport layer.
//  There is nothing useful for serial.
//
static BOOL SerialGetDevCfg(UINT8 *pBuffer, UINT16 *pSize)
{
    *pSize = 0;
    return TRUE;
}

//------------------------------------------------------------------------------
//
//  Function:  SerialGetDevCfg
//
//  This function is called by KITL to with information sent by
//  host in negotiation. There is nothing useful for serial.
//
static BOOL SerialSetHostCfg(UINT8 *pData, UINT16 size)
{
     return TRUE;
}

//------------------------------------------------------------------------------
//
//  Function:  OALKitlSerialInit
//
//  This function initialize serial transport layer. It is called from
//  OEMKitlInit for serial class KITL devices.
//
BOOL OALKitlSerialInit(
    LPSTR deviceId, OAL_KITL_DEVICE *pDevice, OAL_KITL_ARGS *pArgs, 
    KITLTRANSPORT *pKitl
) {
    BOOL rc = FALSE;
    OAL_KITL_SERIAL_DRIVER *pDriver;
    KITL_SERIAL_INFO *pInfo = &g_kitlSerialState.info;
    UINT32 irq, sysIntr;

    KITL_RETAILMSG(ZONE_KITL_OAL, (
        "+OALKitlSerialInit('%S', '%s', 0x%08x, 0x%08x)\r\n",
        deviceId, pDevice->name, pArgs, pKitl
    ));

    // Cast driver config parameter
    pDriver = (OAL_KITL_SERIAL_DRIVER*)pDevice->pDriver;
    if (pDriver == NULL) {
        KITL_RETAILMSG(ZONE_ERROR, ("ERROR: KITL device driver is NULL\r\n"));
        goto cleanUp;
    }        
    // initialize serial kitl information
    pInfo->pAddress = (UINT8*)pArgs->devLoc.PhysicalLoc;
    pInfo->baudRate = pArgs->baudRate;
    pInfo->dataBits = pArgs->dataBits;
    pInfo->stopBits = pArgs->stopBits;
    pInfo->parity   = pArgs->parity;
    pInfo->bestSize = 1;

    // Call pfnInit
    if (!pDriver->pfnInit(pInfo)) {
        KITL_RETAILMSG(ZONE_ERROR, ("ERROR: KITL call to pfnInit failed\r\n"));                
        goto cleanUp;
    }

    // Best size can't be larger than MTU
    if (pInfo->bestSize > KITL_MTU) pInfo->bestSize = KITL_MTU;

    // Map and enable interrupt
    if ((pArgs->flags & OAL_KITL_FLAGS_POLL) != 0) {
        sysIntr = KITL_SYSINTR_NOINTR;
    } else {
        // Get IRQ, when interface is undefined use Pin as IRQ
        if (pArgs->devLoc.IfcType == InterfaceTypeUndefined) {
            irq = pArgs->devLoc.Pin;
        } else {
            if (!OEMIoControl(
                IOCTL_HAL_REQUEST_IRQ, &pArgs->devLoc, sizeof(pArgs->devLoc),
                &irq, sizeof(irq), NULL
            )) {                
                KITL_RETAILMSG(ZONE_WARNING, (
                    "WARN: KITL can't obtain IRQ for KITL device\r\n"
                ));
                irq = OAL_INTR_IRQ_UNDEFINED;
            }
        }
        // Get SYSINTR for IRQ
        if (irq != OAL_INTR_IRQ_UNDEFINED) {
            UINT32 aIrqs[3];
        
            aIrqs[0] = -1;
            aIrqs[1] = OAL_INTR_FORCE_STATIC;
            aIrqs[2] = irq;
            if (
                OEMIoControl(
                    IOCTL_HAL_REQUEST_SYSINTR, aIrqs, sizeof(aIrqs), &sysIntr,
                    sizeof(sysIntr), NULL
                ) && sysIntr != SYSINTR_UNDEFINED
            ) {                
                OEMInterruptEnable(sysIntr, NULL, 0);
            } else {
                KITL_RETAILMSG(ZONE_WARNING, (
                    "WARN: KITL can't obtain SYSINTR for IRQ %d\r\n", irq
                ));
                sysIntr = KITL_SYSINTR_NOINTR;
            }
        } else {
            sysIntr = KITL_SYSINTR_NOINTR;
        }
    }

    if(sysIntr == KITL_SYSINTR_NOINTR) {
        KITL_RETAILMSG(ZONE_WARNING, (
            "WARN: KITL will run in polling mode\r\n"
        ));
    }
    //-----------------------------------------------------------------------
    // Initalize KITL transport structure
    //-----------------------------------------------------------------------

    memcpy(pKitl->szName, deviceId, sizeof(pKitl->szName));
    pKitl->Interrupt     = (UCHAR)sysIntr; 
    pKitl->WindowSize    = OAL_KITL_WINDOW_SIZE;
    pKitl->FrmHdrSize    = sizeof(OAL_KITL_SERIAL_HEADER);
    pKitl->dwPhysBuffer  = 0;
    pKitl->dwPhysBufLen  = 0;
    pKitl->pfnEncode     = SerialEncode;
    pKitl->pfnDecode     = SerialDecode;
    pKitl->pfnSend       = SerialSend;
    pKitl->pfnRecv       = SerialRecv;
    pKitl->pfnEnableInt  = SerialEnableInt;
    pKitl->pfnGetDevCfg  = SerialGetDevCfg;
    pKitl->pfnSetHostCfg = SerialSetHostCfg;

    //-----------------------------------------------------------------------
    // Initalize state structure
    //-----------------------------------------------------------------------

    g_kitlSerialState.pDriver = pDriver;

    // Done
    rc = TRUE;
    
cleanUp:
    KITL_RETAILMSG(ZONE_KITL_OAL, ("-OALKitlSerialInit(rc = %d)\r\n", rc));
    return rc;
}

//------------------------------------------------------------------------------
