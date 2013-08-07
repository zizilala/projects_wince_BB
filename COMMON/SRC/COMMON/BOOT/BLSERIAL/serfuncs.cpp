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
//  File:  serfuncs.cpp
//
//  Common RS232/USB Serial routines for bootloaders.
//
#include <oal_blserial.h>

__declspec(align(4)) BYTE g_buffer[KITL_MTU];

//------------------------------------------------------------------------------
//
//  Function:  SerialReadData
//
//  Keep getting peices of the image as the download progresses.
//
extern "C" BOOL SerialReadData (DWORD cbData, LPBYTE pbData)
{
    static DWORD dwBlockNumber = 0;
    static USHORT cbDataBuffer = 0;
    static BYTE dataBuffer[KITL_MTU] = {0};

    // the first DWORD in the local buffer is the block header which contains
    // the sequence number of the block received
    static PSERIAL_BLOCK_HEADER pBlockHeader = (PSERIAL_BLOCK_HEADER)dataBuffer;
    static PBYTE pBlock = dataBuffer + sizeof(PSERIAL_BLOCK_HEADER);

    SERIAL_PACKET_HEADER header;
    USHORT cbPacket;

    LPBYTE pOutBuffer = pbData;

    while(cbData)
    {
        // if there is no data in the local buffer, read some
        //
        while(0 == cbDataBuffer)
        {
             // read from port
             cbPacket = sizeof(dataBuffer);
             if(RecvPacket(&header, dataBuffer, &cbPacket, TRUE))
             {
                // ignore non-download packet types
                if(KS_PKT_DLPKT == header.pktType)
                {
                    // make sure we received the correct block in the sequence
                    if(dwBlockNumber == pBlockHeader->uBlockNum)
                    {
                        cbDataBuffer = header.payloadSize - sizeof(SERIAL_BLOCK_HEADER);
                        dwBlockNumber++;
                    }
                    else
                    {
                        KITLOutputDebugString("Received out of sequence block %u\r\n", pBlockHeader->uBlockNum);
                        KITLOutputDebugString("Expected block %u\r\n", dwBlockNumber);
                    }

                    // ack, or re-ack the sender
                    if(dwBlockNumber > 0)
                    {
                        // block number has already been incremented when appropriate
                        SerialSendBlockAck(dwBlockNumber - 1);
                    }
                }
                else
                {
                    //Error received non-download packet type
                    KITLOutputDebugString("received non-download packet type 0x%X\r\n", header.pktType);
                    return FALSE;
                }
             }
        }

        // copy from local buffer into output buffer
        //

        // if there are more than the requested bytes, copy and shift
        // the local data buffer
        if(cbDataBuffer > cbData)
        {
            // copy requested bytes from local buffer into output buffer
            memcpy(pOutBuffer, pBlock, cbData);
            cbDataBuffer -= (USHORT)cbData;

            // shift the local buffer accordingly because not all data was used
            memmove(pBlock, pBlock + cbData, cbDataBuffer);
            cbData = 0;
        }
        else // cbDataBuffer <= cbData
        {
            // copy all bytes in local buffer to output buffer
            memcpy(pOutBuffer, pBlock, cbDataBuffer);
            cbData -= cbDataBuffer;
            pOutBuffer += cbDataBuffer;
            cbDataBuffer = 0;
        }
    }

    return TRUE;
}

//------------------------------------------------------------------------------
//
//  Function:  CalcChksum
//
//  Calculate checksum to insert into a block before we send. Also check what we receive.
//
UCHAR CalcChksum(PUCHAR pBuf, USHORT len)
{
    USHORT s = 0;
    UCHAR csum = 0;

    for(s = 0; s < len; s++)
        csum += *(pBuf + s);

    return csum;
}
//------------------------------------------------------------------------------
//
//  Function:  SerialSendBootRequest
//
//  Send a BOOTME to the desktop to establish a connection.
//
extern "C" BOOL SerialSendBootRequest(const char * platformString)
{
    BYTE buffer[sizeof(SERIAL_PACKET_HEADER) + sizeof(SERIAL_BOOT_REQUEST)] = {0};
    PSERIAL_PACKET_HEADER pHeader = (PSERIAL_PACKET_HEADER)buffer;
    PSERIAL_BOOT_REQUEST pBootReq = (PSERIAL_BOOT_REQUEST)(buffer + sizeof(SERIAL_PACKET_HEADER));

    // create boot request
    strncpy((char *)pBootReq->PlatformId, platformString, sizeof(pBootReq->PlatformId));
    pBootReq->PlatformId[ sizeof(pBootReq->PlatformId) - 1 ] = '\0';

    // create header
    memcpy(pHeader->headerSig, packetHeaderSig, HEADER_SIG_BYTES);
    pHeader->pktType = KS_PKT_DLREQ;
    pHeader->payloadSize = sizeof(SERIAL_BOOT_REQUEST);
    pHeader->crcData = CalcChksum((PBYTE)pBootReq, sizeof(SERIAL_BOOT_REQUEST));
    pHeader->crcHdr = CalcChksum((PBYTE)pHeader,
        sizeof(SERIAL_PACKET_HEADER) - sizeof(pHeader->crcHdr));

    OEMSerialSendRaw(buffer, sizeof(SERIAL_PACKET_HEADER) + sizeof(SERIAL_BOOT_REQUEST));

    return TRUE;
}

//------------------------------------------------------------------------------
//
//  Function:  SerialSendBlockAck
//
//  Acknowledge a block has been received from the desktop.
//
extern "C" BOOL SerialSendBlockAck(DWORD uBlockNumber)
{
    BYTE buffer[sizeof(SERIAL_PACKET_HEADER) + sizeof(SERIAL_BLOCK_HEADER)];
    PSERIAL_PACKET_HEADER pHeader = (PSERIAL_PACKET_HEADER)buffer;
    PBYTE pBlockAck = (buffer + sizeof(SERIAL_PACKET_HEADER));

    // create block ack
    pBlockAck[0] = (BYTE)(uBlockNumber >> 0) & 0xFF;
    pBlockAck[1] = (BYTE)(uBlockNumber >> 8) & 0xFF;
    pBlockAck[2] = (BYTE)(uBlockNumber >> 16) & 0xFF;
    pBlockAck[3] = (BYTE)(uBlockNumber >> 24) & 0xFF;

    // create header
    memcpy(pHeader->headerSig, packetHeaderSig, HEADER_SIG_BYTES);
    pHeader->pktType = KS_PKT_DLACK;
    pHeader->payloadSize = sizeof(SERIAL_BLOCK_HEADER);
    pHeader->crcData = CalcChksum((PBYTE)pBlockAck, sizeof(SERIAL_BLOCK_HEADER));
    pHeader->crcHdr = CalcChksum((PBYTE)pHeader,
        sizeof(SERIAL_PACKET_HEADER) - sizeof(pHeader->crcHdr));

    OEMSerialSendRaw(buffer, sizeof(SERIAL_PACKET_HEADER) + sizeof(SERIAL_BLOCK_HEADER));

    return TRUE;
}

//------------------------------------------------------------------------------
//
//  Function:  SerialWaitForBootAck
//
//  Attempt to receive the serial bootme ack from the desktop.
//  This function will fail if a boot ack is not received in a timely manner
//  so that another boot request can be sent
//
extern "C" BOOL SerialWaitForBootAck(BOOL *pfJump)
{
    BOOL fRet = FALSE;
    USHORT cbBuffer = KITL_MTU;
    SERIAL_PACKET_HEADER header = {0};
    PSERIAL_BOOT_ACK pBootAck = (PSERIAL_BOOT_ACK)g_buffer;

    KITLOutputDebugString("Waiting for boot ack...\r\n");
    if(RecvPacket(&header, g_buffer, &cbBuffer, FALSE))
    {
        // header checksum already verified
        if(KS_PKT_DLACK == header.pktType &&
            sizeof(SERIAL_BOOT_ACK) == header.payloadSize)
        {
            *pfJump = pBootAck->fJumping;
            fRet = TRUE;
        }
    }

    return fRet;
}

//------------------------------------------------------------------------------
//
//  Function:  SerialWaitForJump
//
//  When the download is complete PB will instruct us to jump to the image.
//  PB will specify what KITL transport to use for debugging (may not be the 
//  same as the download transport).
//  Wait indefinately for the jump command.
//
extern "C" DWORD SerialWaitForJump(VOID)
{
    USHORT cbBuffer = KITL_MTU;
    SERIAL_PACKET_HEADER header = {0};
    PSERIAL_JUMP_REQUEST pJumpReq = (PSERIAL_JUMP_REQUEST)g_buffer;

    // wait indefinitely for a jump request
    while(1)
    {
        if(RecvPacket(&header, g_buffer, &cbBuffer, TRUE))
        {
            // header & checksum already verified
            if(KS_PKT_JUMP == header.pktType &&
                sizeof(SERIAL_JUMP_REQUEST) == header.payloadSize)
            {
                SerialSendBlockAck(0);
                KITLOutputDebugString("Got jump request, KITL transport = 0x%x\r\n",
                    pJumpReq->dwKitlTransport);
                return pJumpReq->dwKitlTransport;
            }
        }
    }

    // never reached
    return KTS_NONE;
}


//------------------------------------------------------------------------------
//
//  Function:  RecvPacket
//
//  SerialReadData helper function. Eventually cals USBSerialRecvRaw.
//
BOOL RecvPacket(PSERIAL_PACKET_HEADER pHeader, PBYTE pbFrame, PUSHORT pcbFrame, BOOLEAN bWaitInfinite)
{
    // receive header
    if(!RecvHeader(pHeader, bWaitInfinite))
    {
        KITLOutputDebugString("failed to receive header\r\n");
        return FALSE;
    }

    // verify packet checksum
    if(pHeader->crcHdr != CalcChksum((PBYTE)pHeader,
        sizeof(SERIAL_PACKET_HEADER) - sizeof(pHeader->crcHdr)))
    {
        KITLOutputDebugString("header checksum failure\r\n");
        return FALSE;
    }

    // make sure sufficient buffer is provided
    if(*pcbFrame < pHeader->payloadSize)
    {
        KITLOutputDebugString("insufficient buffer size; ignoring packet\r\n");
        return FALSE;
    }

    // receive data
    *pcbFrame = pHeader->payloadSize;
    if(!OEMSerialRecvRaw(pbFrame, pcbFrame, bWaitInfinite))
    {
        KITLOutputDebugString("failed to read packet data\r\n");
        return FALSE;
    }

    // verify data checksum
    if(pHeader->crcData != CalcChksum(pbFrame, *pcbFrame))
    {
        KITLOutputDebugString("data checksum failure. expected 0x%X, received 0x%X\r\n",pHeader->crcData,CalcChksum(pbFrame,*pcbFrame));
        return FALSE;
    }

    // verify packet type -- don't return any packet that is not
    // a type the bootloader expects to receive
    if(KS_PKT_DLPKT != pHeader->pktType &&
       KS_PKT_DLACK != pHeader->pktType &&
       KS_PKT_JUMP != pHeader->pktType)
    {
        KITLOutputDebugString("received non-download packet type 0x%X\r\n", pHeader->pktType);
        return FALSE;
    }

    return TRUE;
}


//------------------------------------------------------------------------------
//
//  Function:  RecvHeader
//
//  SerialReadData helper function. Eventually cals USBSerialRecvRaw.
//
BOOL RecvHeader(PSERIAL_PACKET_HEADER pHeader, BOOLEAN bWaitInfinite)
{
    USHORT cbRead;
    UINT i = 0;
    cbRead = sizeof(UCHAR);

    // read the header bytes
    while(i < HEADER_SIG_BYTES)
    {
        if(!OEMSerialRecvRaw((PBYTE)&(pHeader->headerSig[i]), &cbRead, bWaitInfinite) || sizeof(UCHAR) != cbRead)
        {
            KITLOutputDebugString("failed to receive header signature\r\n");
            return FALSE;
        }

        if(pHeader->headerSig[i] == packetHeaderSig[i])
        {
            i++;
        }

        else
        {
            i = 0;
        }
    }

    // read the remaining header
    cbRead = sizeof(SERIAL_PACKET_HEADER) - HEADER_SIG_BYTES;
    if(!OEMSerialRecvRaw((PUCHAR)pHeader + HEADER_SIG_BYTES, &cbRead, bWaitInfinite) ||
        sizeof(SERIAL_PACKET_HEADER) - HEADER_SIG_BYTES != cbRead)
    {
        KITLOutputDebugString("failed to receive header data\r\n");
        return FALSE;
    }

    // verify the header checksum
    if(pHeader->crcHdr != CalcChksum((PUCHAR)pHeader,
        sizeof(SERIAL_PACKET_HEADER) - sizeof(pHeader->crcHdr)))
    {
        KITLOutputDebugString("header checksum fail\r\n");
        return FALSE;
    }

    return TRUE;
}





