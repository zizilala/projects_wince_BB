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
//  File:  kitleth.c
//
#include <windows.h>
#include <ceddk.h>
#include <nkintr.h>
#include <kitl.h>
#include <ethdbg.h>
#include <oal.h>
#include "kitleth.h"

//------------------------------------------------------------------------------
//  Local variable

static struct {

    OAL_KITL_ETH_DRIVER *pDriver;
    UINT32 flags;

    UINT8  deviceMAC[6];
    UINT32 deviceIP;

    UINT8  kitlServerMAC[6];
    UINT32 kitlServerIP;
    UINT16 kitlServerPort;

    UINT32 dhcpState;
    UINT32 dhcpServerIP;
    UINT32 dhcpXId;

    PFN_SENDFRAME pfnSend;
    PFN_RECVFRAME pfnRecv;

    BOOL updateFilter;
    UINT32 deviceFilter;
    
    BOOL updateMCast;
    UINT32 deviceMCastSize;
    UINT8 deviceMCast[32 * 6];
    
    UINT8 packet[1516];
} g_kitlEthState;

//------------------------------------------------------------------------------

static UINT8* KitlEthDecode(UINT8 *pFrame, UINT16 *pLength);


//------------------------------------------------------------------------------

static UINT16 Sum(UINT16 seed, VOID *pData, UINT32 size)
{
    UINT32 sum;
    UINT16 *p;

    // There is no need to swap for network ordering during calculation
    // because of the end around carry used in 1's complement addition
    sum = seed;
    p = (UINT16*)pData;
    while (size >= sizeof(UINT16)) {
        sum += *p++;
        size -= sizeof(UINT16);
    }
    if (size > 0) sum += *((UINT8*)p);

    while ((sum & 0xFFFF0000) != 0) {
        sum = (sum & 0x0000FFFF) + (sum >> 16);
    }
    return (UINT16)sum;
}

//------------------------------------------------------------------------------

static VOID SendARP(UINT16 op, UINT8 mac[], UINT32 ip)
{
    UINT8 *pFrame = g_kitlEthState.packet;
    ETH_HEADER *pEth = (ETH_HEADER*)pFrame;
    ARP_MESSAGE *pARP = (ARP_MESSAGE*)((UINT8*)pEth + sizeof(ETH_HEADER));

    memcpy(pEth->destmac, mac, sizeof(pEth->destmac));
    memcpy(pEth->srcmac, g_kitlEthState.deviceMAC, sizeof(pEth->srcmac));
    pEth->ftype = htons(ARP_FRAME);

    pARP->htype = htons(1);         // Ethernet
    pARP->ptype = htons(IP_FRAME);  // IP4 addresses
    pARP->hsize = 6;                // MAC address is 6 bytes long
    pARP->psize = 4;                // IP4 addresses are 4 bytes long
    pARP->op = htons(op);           // Specify an ARP op

    // Fill in the source side information
    memcpy(pARP->srcmac, g_kitlEthState.deviceMAC, sizeof(pARP->srcmac));
    pARP->srcip = g_kitlEthState.deviceIP;

    // Fill in the destination information
    memcpy(pARP->destmac, mac, sizeof(pARP->destmac));
    pARP->destip = ip;

    // Send packet
    g_kitlEthState.pfnSend(pFrame, sizeof(ETH_HEADER) + sizeof(ARP_MESSAGE));
}

//------------------------------------------------------------------------------

static UINT16 GetOpARP(UINT8 *pFrame, UINT16 length)
{
    ETH_HEADER *pEth = (ETH_HEADER*)pFrame;
    ARP_MESSAGE *pARP = (ARP_MESSAGE*)((UINT8*)pEth + sizeof(ETH_HEADER));

    if (pEth->ftype != htons(ARP_FRAME)) return 0;
    return htons(pARP->op);
}

//------------------------------------------------------------------------------

static VOID DecodeARP(UINT8 *pFrame, UINT16 length, BOOL *pUsed)
{
    ETH_HEADER *pEth = (ETH_HEADER*)pFrame;
    ARP_MESSAGE *pARP = (ARP_MESSAGE*)((UINT8*)pEth + sizeof(ETH_HEADER));


    // Check to see that they were requesting the ARP response from us,
    // send reply to ARP request, but ignore other ops
    if (
        pARP->destip == g_kitlEthState.deviceIP &&
        pARP->op == htons(ARP_REQUEST)
    ) {
        SendARP(ARP_RESPONSE, pARP->srcmac, pARP->srcip);
        *pUsed = TRUE;
    } else {
        *pUsed = FALSE;
    }
}

//------------------------------------------------------------------------------

static UINT16 EncodeIP(
    UINT8 *pFrame, UINT16 length, UINT8 *pDestMAC, UINT32 srcIP, UINT32 destIP,
    UINT8 protocol
) {
    static UINT16 ipId = 0;
    ETH_HEADER *pEth = (ETH_HEADER*)pFrame;
    IP4_HEADER *pIP = (IP4_HEADER*)((UINT8*)pEth + sizeof(ETH_HEADER));
    UINT16 ipLength, ethLength;

    // Get final length
    ipLength = sizeof(IP4_HEADER) + length;
    ethLength = sizeof(ETH_HEADER) + ipLength;

    // Fill in Ethernet header
    if (pDestMAC != NULL) {
        memcpy(pEth->destmac, pDestMAC, sizeof(pEth->destmac));
    } else {
        memset(pEth->destmac, 0xFF, sizeof(pEth->destmac));
    }
    memcpy(pEth->srcmac, g_kitlEthState.deviceMAC, sizeof(pEth->srcmac));
    pEth->ftype = htons(IP_FRAME);

    // Fill in IP4 header
    pIP->verlen = 0x45;
    pIP->tos = 0;
    pIP->length = htons(ipLength);
    pIP->id = ipId++;
    pIP->fragment = 0;
    pIP->ttl = 64;
    pIP->protocol = protocol;
    pIP->sum = 0;
    pIP->srcip = srcIP;
    pIP->destip = destIP;

    // Compute IP4 header checksum
    pIP->sum = ~ Sum(0, pIP, sizeof(IP4_HEADER));

    // We are done
    return ethLength;
}

//------------------------------------------------------------------------------

static UINT16 EncodeUDP(
    UINT8 *pFrame, UINT16 length, UINT8 *pDestMAC, UINT32 srcIP, UINT32 destIP,
    UINT16 srcPort, UINT16 destPort
) {
    UDP_HEADER *pUDP;
    UINT16 xsum, udpLength;

    // First calculate UDP length
    udpLength = sizeof(UDP_HEADER) + length;

    // Fill in UDP header
    pUDP = (UDP_HEADER*)(pFrame + sizeof(ETH_HEADER) + sizeof(IP4_HEADER));
    pUDP->srcPort = srcPort;
    pUDP->dstPort = destPort;
    pUDP->length = htons(udpLength);
    pUDP->sum = 0;

    // Compute UDP header checksum
    xsum = htons(UDP_PROTOCOL);
    xsum = Sum(xsum, &srcIP, sizeof(UINT32));
    xsum = Sum(xsum, &destIP, sizeof(UINT32));
    xsum = Sum(xsum, &pUDP->length, sizeof(UINT32));
    xsum = Sum(xsum, pUDP, udpLength);
    pUDP->sum = ~xsum;

    // And now do IP encode
    return EncodeIP(pFrame, udpLength, pDestMAC, srcIP, destIP, UDP_PROTOCOL);
}

//------------------------------------------------------------------------------

static UINT8* FindDHCPOption(BOOTP_MESSAGE *pDHCP, DHCP_OPTIONS option)
{
    UINT8 *p;

    p = &pDHCP->options[4];
    while (*p != DHCP_END) {
        if (*p == option) break;
        p += p[1] + 2;
    }
    if (*p == DHCP_END) p = NULL;
    return p;
}

//------------------------------------------------------------------------------

static VOID AddDHCPOption(
    BOOTP_MESSAGE *pDHCP, DHCP_OPTIONS option, UINT32 *pOffset,
    VOID *pData, UINT32 size
) {
    pDHCP->options[(*pOffset)++] = option;
    pDHCP->options[(*pOffset)++] = size;
    memcpy(&pDHCP->options[*pOffset], pData, size);
    *pOffset += size;
}

//------------------------------------------------------------------------------

static VOID SendDHCP(DHCP_MESSAGE_TYPE msgType, UINT32 requestIP)
{
    UINT8 *pFrame = g_kitlEthState.packet;
    BOOTP_MESSAGE *pDHCP;
    UINT32 offset;
    UINT16 size;
    UINT8 clientId[7];

    // Get pointer to BOOTP/DHCP message
    pDHCP = (BOOTP_MESSAGE*)(
        pFrame + sizeof(ETH_HEADER) + sizeof(IP4_HEADER) + sizeof(UDP_HEADER)
    );

    // Clear all fields
    memset(pDHCP, 0, sizeof(BOOTP_MESSAGE));

    // BOOTP header
    pDHCP->op = 1;                              // BOOTREQUEST
    pDHCP->htype = 1;                           // 10 Mbps Ethernet
    pDHCP->hlen = 6;                            // Ethernet address size
    pDHCP->secs = 0;                            // Who care?
    pDHCP->xid = g_kitlEthState.dhcpXId;

    // Device MAC address
    memcpy(
        pDHCP->chaddr, g_kitlEthState.deviceMAC,
        sizeof(g_kitlEthState.deviceMAC)
    );

    // Start with options
    offset = 0;

    // DHCP cookie
    pDHCP->options[offset++] = 0x63;
    pDHCP->options[offset++] = 0x82;
    pDHCP->options[offset++] = 0x53;
    pDHCP->options[offset++] = 0x63;

    // DHCP message type
    AddDHCPOption(pDHCP, DHCP_MSGTYPE, &offset, &msgType, 1);

    // Add client id
    clientId[0] = 1;
    memcpy(&clientId[1], g_kitlEthState.deviceMAC, 6);
    AddDHCPOption(pDHCP, DHCP_CLIENT_ID, &offset, clientId, 7);

    switch (msgType) {
    case DHCP_REQUEST:
        if (requestIP != 0) {
            AddDHCPOption(
                pDHCP, DHCP_IP_ADDR_REQ, &offset, &requestIP, sizeof(requestIP)
            );
            AddDHCPOption(
                pDHCP, DHCP_SERVER_ID, &offset, &g_kitlEthState.dhcpServerIP,
                sizeof(g_kitlEthState.dhcpServerIP)
            );
        } else {
            pDHCP->ciaddr = g_kitlEthState.deviceIP;
        }
        break;
    case DHCP_DECLINE:
        AddDHCPOption(
            pDHCP, DHCP_IP_ADDR_REQ, &offset, &requestIP, sizeof(requestIP)
        );
        AddDHCPOption(
            pDHCP, DHCP_SERVER_ID, &offset, &g_kitlEthState.dhcpServerIP,
            sizeof(g_kitlEthState.dhcpServerIP)
        );
        break;
    }

    // DHCP message end
    pDHCP->options[offset++] = DHCP_END;

    // Add MAC/IP/UDP header and checksum
    size = EncodeUDP(
        pFrame, sizeof(BOOTP_MESSAGE) + offset, NULL, requestIP, 0xFFFFFFFF,
        htons(DHCP_CLIENT_PORT), htons(DHCP_SERVER_PORT)
    );

    // Send packet on wire
    g_kitlEthState.pfnSend(pFrame, size);
}

//------------------------------------------------------------------------------

static VOID RenewDHCP(VOID *pParam)
{
    KITL_RETAILMSG(ZONE_WARNING, ("KITL: Start DHCP RENEW process\r\n"));

    // Change transaction id (ok, not too random)
    g_kitlEthState.dhcpXId += 0x01080217;
    // Send request
    SendDHCP(DHCP_REQUEST, 0);
    g_kitlEthState.dhcpState = DHCP_REQUESTING;
}

//------------------------------------------------------------------------------

static VOID GetAddressDHCP(UINT32 ip)
{
    UINT32 startTime, attempts;
    UINT16 size;
    UINT8 mac[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};


    // If there is already IP adress, try first renew it
    if (ip != 0) {
        // Send request
        SendDHCP(DHCP_REQUEST, 0);

        g_kitlEthState.dhcpState = DHCP_REQUESTING;
        // Wait until DHCP gets address or timeout
        startTime = OALGetTickCount();
        while ((OALGetTickCount() - startTime) < DHCP_TIMEOUT) {
            size = sizeof(g_kitlEthState.packet);
            if (!g_kitlEthState.pfnRecv(g_kitlEthState.packet, &size)) continue;
            KitlEthDecode(g_kitlEthState.packet, &size);
            if (g_kitlEthState.dhcpState == DHCP_BOUND) break;
        }
        // If we end in BOUND state we renew it...
        if (g_kitlEthState.dhcpState == DHCP_BOUND) goto cleanUp;
    }

    // Allocate new IP address
    attempts = 0;
    while (attempts++ < 3) {

        // Reset device and server IP
        g_kitlEthState.deviceIP = 0;
        g_kitlEthState.dhcpServerIP = 0;

        // Send DHCP discover message
        SendDHCP(DHCP_DISCOVER, 0);
        g_kitlEthState.dhcpState = DHCP_SELECTING;

        // Wait until DHCP gets address or timeout
        startTime = OALGetTickCount();
        while ((OALGetTickCount() - startTime) < DHCP_TIMEOUT) {
            size = sizeof(g_kitlEthState.packet);
            if (!g_kitlEthState.pfnRecv(g_kitlEthState.packet, &size)) continue;
            KitlEthDecode(g_kitlEthState.packet, &size);
            if (g_kitlEthState.dhcpState == DHCP_BOUND) break;
        }

        // If there was timeout try start DHCP againg with new transaction id
        if (g_kitlEthState.dhcpState != DHCP_BOUND) {
            g_kitlEthState.dhcpXId += 0x00080000;
            continue;
        }

        // We get address, verify if it isn't used by somebody else...
        SendARP(ARP_REQUEST, mac, g_kitlEthState.deviceIP);
        startTime = OALGetTickCount();
        while ((OALGetTickCount() - startTime) < ARP_TIMEOUT) {
            size = sizeof(g_kitlEthState.packet);
            if (g_kitlEthState.pfnRecv(g_kitlEthState.packet, &size)) {
                if (GetOpARP(g_kitlEthState.packet, size) == ARP_RESPONSE) {
                    // Oops, somebody is using assigned address, decline...
                    SendDHCP(DHCP_DECLINE, g_kitlEthState.deviceIP);
                    g_kitlEthState.deviceIP = 0;
                    break;
                }
            }
        }

        // If we get with valid address we are done
        if (g_kitlEthState.deviceIP != 0) break;

        // Let try new DHCP transaction
        g_kitlEthState.dhcpXId += 0x01080000;

    }

cleanUp:
    if(g_kitlEthState.deviceIP != 0)
    KITL_RETAILMSG(ZONE_INIT, ("KITL: DHCP get/renew device IP: %s\r\n",
        OALKitlIPtoString(g_kitlEthState.deviceIP)
    ));
}

//------------------------------------------------------------------------------

static VOID DecodeDHCP(UINT8* pFrame, UINT16 *pSize, BOOL *pUsed)
{
    BOOTP_MESSAGE *pDHCP;
    UINT8 *pOption, msgType;
    UINT32 value;
    BOOL rc;

    // Be pesimistic
    *pUsed = FALSE;

    // Get pointers to BOOTP/DHCP message
    pDHCP = (BOOTP_MESSAGE*)(
        pFrame + sizeof(ETH_HEADER) + sizeof(IP4_HEADER) + sizeof(UDP_HEADER)
    );

    // Check magic DHCP cookie & transaction id
    if (
        pDHCP->options[0] != 0x63 || pDHCP->options[1] != 0x82 ||
        pDHCP->options[2] != 0x53 || pDHCP->options[3] != 0x63 ||
        pDHCP->xid != g_kitlEthState.dhcpXId
    ) goto cleanUp;

    // This is our DHCP transaction
    *pUsed = TRUE;

    // Then find DHCP message type
    pOption = FindDHCPOption(pDHCP, DHCP_MSGTYPE);
    if (pOption == NULL) return;
    msgType = pOption[2];

    // Message processing depend on DHCP client state
    switch (g_kitlEthState.dhcpState) {
    case DHCP_SELECTING:
        // Ignore anything other then offer
        if (msgType != DHCP_OFFER) break;
        // Find server IP address
        pOption = FindDHCPOption(pDHCP, DHCP_SERVER_ID);
        if (pOption == NULL) break;
        memcpy(&g_kitlEthState.dhcpServerIP, &pOption[2], sizeof(UINT32));
        // Request offered IP address
        SendDHCP(DHCP_REQUEST, pDHCP->yiaddr);
        // We moved to new state
        g_kitlEthState.dhcpState = DHCP_REQUESTING;
        break;
    case DHCP_REQUESTING:
        if (msgType == DHCP_ACK) {
            // Set assigned address
            g_kitlEthState.deviceIP = pDHCP->yiaddr;
            // Find renew period & set timer callback
            pOption = FindDHCPOption(pDHCP, DHCP_RENEW_TIME);
            if (pOption != NULL) {
                memcpy(&value, &pOption[2], sizeof(UINT32));
                rc = KitlSetTimerCallback(ntohl(value), RenewDHCP, NULL);
            } else {
                // If there isn't renew period select 30 minutes
                KitlSetTimerCallback(30*60, RenewDHCP, NULL);
            }
            // We get address, let check it...
            g_kitlEthState.dhcpState = DHCP_BOUND;
        } else if (msgType == DHCP_NAK) {
            g_kitlEthState.deviceIP = 0;
            // Discover DHCP servers
            SendDHCP(DHCP_DISCOVER, 0);
            // Start with discover again
            g_kitlEthState.dhcpState = DHCP_SELECTING;
        }
        break;
    }

cleanUp:
    return;
}

//------------------------------------------------------------------------------

static VOID DecodeICMP(UINT8 *pFrame, UINT16 length, BOOL *pUsed)
{
    ETH_HEADER *pEth = (ETH_HEADER*)pFrame;
    IP4_HEADER *pIP = (IP4_HEADER*)((UINT8*)pEth + sizeof(ETH_HEADER));
    ICMP_HEADER *pICMP = (ICMP_HEADER*)((UINT8*)pIP + sizeof(IP4_HEADER));
    UINT16 icmpLength;

    // Be pesimistic
    *pUsed = FALSE;

    // Get ICMP message size & verify checksum
    icmpLength = ntohs(pIP->length) - sizeof(IP4_HEADER);
    if (Sum(0, pICMP, icmpLength) != 0xFFFF) goto cleanUp;

    // Reply to ping only
    if (pICMP->op != ICMP_ECHOREQ) goto cleanUp;

    // We now know that packet is used
    *pUsed = TRUE;

    // Encode reply & do checksum
    pICMP->op = 0;
    pICMP->code = 0;
    pICMP->sum = 0;
    pICMP->sum = ~ Sum(0, pICMP, icmpLength);

    // Add IP header
    EncodeIP(
        pFrame, icmpLength, pEth->srcmac, pIP->destip, pIP->srcip, ICMP_PROTOCOL
    );

    // Send packet
    g_kitlEthState.pfnSend(pFrame, length);

cleanUp:
    return;
}

//------------------------------------------------------------------------------

static UINT8* DecodeUDP(UINT8 *pFrame, UINT16 *pLength, BOOL *pUsed)
{
    UINT8 *pData = NULL;
    ETH_HEADER *pEth = (ETH_HEADER*)pFrame;
    IP4_HEADER *pIP = (IP4_HEADER*)((UINT8*)pEth + sizeof(ETH_HEADER));
    UDP_HEADER *pUDP = (UDP_HEADER*)((UINT8*)pIP + sizeof(IP4_HEADER));
    UINT16 xsum;
    UINT32 ipLength, udpLength;

    // Be pesimistic
    *pUsed = FALSE;

    // Get IP and UDP lengths from packet
    ipLength = ntohs(pIP->length);
    udpLength = ntohs(pUDP->length);

    // UPD length must be in sync with IP length
    if (ipLength < sizeof(IP4_HEADER) + udpLength) goto cleanUp;

    // Verify UDP header checksum
    if (pUDP->sum != 0) {    
       xsum = htons(UDP_PROTOCOL);
       xsum = Sum(xsum, &pIP->srcip, sizeof(UINT32));
       xsum = Sum(xsum, &pIP->destip, sizeof(UINT32));
       xsum = Sum(xsum, &pUDP->length, sizeof(UINT32));
       xsum = Sum(xsum, pUDP, udpLength);
       if (xsum != pUDP->sum) goto cleanUp;
    }

    switch (ntohs(pUDP->dstPort)) {
    case DHCP_CLIENT_PORT:
        DecodeDHCP(pFrame, pLength, pUsed);
        break;
    case KITL_CLIENT_PORT:
        // Message must be for us and client must have an address
        if (
            g_kitlEthState.deviceIP != pIP->destip ||
            g_kitlEthState.deviceIP == 0
        ) break;
        // Initialize KITL server address
        if (g_kitlEthState.kitlServerIP == 0xFFFFFFFF) {
            memcpy(
                g_kitlEthState.kitlServerMAC, pEth->srcmac,
                sizeof(g_kitlEthState.kitlServerMAC)
            );
            g_kitlEthState.kitlServerIP = pIP->srcip;
            g_kitlEthState.kitlServerPort = pUDP->srcPort;
            KITL_RETAILMSG(ZONE_INIT, (
                "KITL: Connected host  IP: %s  Port: %d\r\n",
                OALKitlIPtoString(g_kitlEthState.kitlServerIP),
                ntohs(g_kitlEthState.kitlServerPort)
            ));
        }
        *pUsed = TRUE;
        pData = (UINT8*)pUDP + sizeof(UDP_HEADER);
        *pLength = udpLength - sizeof(UDP_HEADER);
        break;
    }

cleanUp:
    return pData;
}

//------------------------------------------------------------------------------

UINT8* DecodeIP(UINT8 *pFrame, UINT16 *pLength, BOOL *pUsed)
{
    UINT8 *pData = NULL;
    ETH_HEADER *pEth = (ETH_HEADER*)pFrame;
    IP4_HEADER *pIP = (IP4_HEADER*)((UINT8*)pEth + sizeof(ETH_HEADER));

    // Be pesimistic
    *pUsed = FALSE;

    // We support only IP4 with standard IP4 header (no options...)
    if (pIP->verlen != 0x45) goto cleanUp;

    // First check if packet is for us
    if (
        pIP->destip != g_kitlEthState.deviceIP &&
        pIP->destip != 0xFFFFFFFF &&
        g_kitlEthState.deviceIP != 0
    ) goto cleanUp;

    // Verify IP4 header checksum
    if (Sum(0, pIP, sizeof(IP4_HEADER)) != 0xFFFF) goto cleanUp;

    // Then decode protocol
    switch (pIP->protocol) {
    case UDP_PROTOCOL:
        pData = DecodeUDP(pFrame, pLength, pUsed);
        break;
    case ICMP_PROTOCOL:
        DecodeICMP(pFrame, *pLength, pUsed);
        break;
    }

cleanUp:
    return pData;
}


//------------------------------------------------------------------------------
//
//  Function:  KitlEthDecode
//
//  This function is called by KITL to decode data for transport. For KITL
//  ethernet it means process packet if it belongs to any supporting protocol
//  (currently ARP, ICMP and DHCP) and send any reply packet. For KITL packets
//  function returns (after checksums pass) pointer to data and its size.
//  For any other packet function return NULL.
//
static UINT8* KitlEthDecode(UINT8 *pFrame, UINT16 *pLength)
{
    ETH_HEADER *pEth = (ETH_HEADER*)pFrame;
    UINT8 *pData = NULL;
    BOOL used = FALSE;


    KITL_RETAILMSG(ZONE_RECV, (
        "+KitlEthDecode(0x%08x, 0x%08x->%d)\r\n", pFrame, pLength, *pLength
    ));

    // Process received packet
    switch (ntohs(pEth->ftype)) {
    case ARP_FRAME:
        DecodeARP(pFrame, *pLength, &used);
        break;
    case IP_FRAME:
        pData = DecodeIP(pFrame, pLength, &used);
        break;
    }

    // If packet wasn't used, indicate it to VMINI
    if (!used && (g_kitlEthState.flags & OAL_KITL_FLAGS_VMINI) != 0) {
        VBridgeKIndicateOneRxBuffer(pFrame, *pLength, FALSE, &used);
    }

    KITL_RETAILMSG(ZONE_RECV, (
        "-KitlEthDecode(pData = 0x%08x, length = %d)\r\n", pData, *pLength
    ));
    return pData;
}

//------------------------------------------------------------------------------
//
//  Function:  KitlEthEncode
//
//  This function is called by KITL to encode data for transport. For KITL
//  ethernet it means adding MAC/IP4/UDP headers and checksums.
//
static BOOL KitlEthEncode(UINT8 *pFrame, UINT16 length)
{
    KITL_RETAILMSG(ZONE_SEND, (
        "+KitlEthEncode(%08x, %d)\r\n", pFrame, length
    ));

    // Encode packets
    EncodeUDP(
        pFrame, length,
        g_kitlEthState.kitlServerMAC, g_kitlEthState.deviceIP,
        g_kitlEthState.kitlServerIP, htons(KITL_CLIENT_PORT),
        g_kitlEthState.kitlServerPort
    );

    KITL_RETAILMSG(ZONE_SEND, ("-KitlEthEncode(rc = 1)\r\n"));
    return TRUE;
}

//------------------------------------------------------------------------------
//
//  Function:  KitlEthGetFrameHdrSize
//
//  This function is called by KITL to get information about transport header
//  size. It allows to allocate packet structure with room for it.
//
static UCHAR KitlEthGetFrameHdrSize()
{
    return sizeof(ETH_HEADER) + sizeof(IP4_HEADER) + sizeof(UDP_HEADER);
}

//------------------------------------------------------------------------------
//
//  Function:  KitlEthSetHostCfg
//
//  This function is called by KITL to with information sent by host in
//  negotiation. There is nothing useful for KITL on Ethernet.
//
static BOOL KitlEthSetHostCfg(UINT8 *pData, UINT16 size)
{
     return TRUE;
}

//------------------------------------------------------------------------------
//
//  Function:  KitlEthGetDevCfg
//
//  This function is called by KITL to get information about transport layer.
//  For ethernet device should return its IP address, MAC address and UDP port.
//
static BOOL KitlEthGetDevCfg(UINT8 *pData, UINT16 *pSize)
{
    BOOL rc = FALSE;
    UINT16 port = htons(KITL_CLIENT_PORT);

    KITL_RETAILMSG(ZONE_KITL_OAL,  (
        "+KitlEthGetDevCfg(0x%08x, 0x%08x->%d)\r\n", pData, pSize, *pSize
    ));

    // Is there space in buffer?
    if (*pSize < (sizeof(UINT32) + 4 * sizeof(UINT16))) goto cleanUp;

    *pSize = 0;
    memcpy(pData, &g_kitlEthState.deviceIP, sizeof(UINT32));
    pData += sizeof(UINT32);
    *pSize += sizeof(UINT32);
    memcpy(pData, g_kitlEthState.deviceMAC, 3*sizeof(UINT16));
    pData += 3*sizeof(UINT16);
    *pSize += 3*sizeof(UINT16);
    memcpy(pData, &port, sizeof(UINT16));
    pData += sizeof(UINT16);
    *pSize += sizeof(UINT16);

    // We are done
    rc = TRUE;

cleanUp:
    KITL_RETAILMSG(ZONE_KITL_OAL, ("-KitlEthGetDevCfg(rc = %d)\r\n", rc));
    return rc;
}

//------------------------------------------------------------------------------
//
//  Function:  KitlEthSend
//
//  This function is called by KITL to send packet over transport layer
//
static BOOL KitlEthSend(UCHAR *pData, USHORT length)
{
    UCHAR *pSendData;
    ULONG sendLength;
    USHORT code;
    int   nRetries = 0;

    KITL_RETAILMSG(ZONE_SEND,  (
        "+KitlEthSend(0x%08x, %d)\r\n", pData, length
    ));

    // Update multicast addresses
    if (g_kitlEthState.updateMCast) {
        if (g_kitlEthState.pDriver->pfnMulticastList)
            g_kitlEthState.pDriver->pfnMulticastList(
                g_kitlEthState.deviceMCast, g_kitlEthState.deviceMCastSize
            );
        g_kitlEthState.updateMCast = FALSE;
    }

    // Update filter 
    if (g_kitlEthState.updateFilter) {
        // Update filter
        g_kitlEthState.pDriver->pfnCurrentPacketFilter(
            g_kitlEthState.deviceFilter
        );
        // Tell VBRIDGE that there is new filter
        VBridgeUCurrentPacketFilter(&g_kitlEthState.deviceFilter);
        g_kitlEthState.updateFilter = FALSE;
    }

    do {
        code = g_kitlEthState.pDriver->pfnSendFrame(pData, length);
        if (code == 0) {
            if (g_kitlEthState.flags & OAL_KITL_FLAGS_VMINI) {
                // Consume all the client packets.
                while (VBridgeKGetOneTxBuffer(&pSendData, &sendLength)) {
                    g_kitlEthState.pDriver->pfnSendFrame(pSendData, (USHORT)sendLength);
                    VBridgeKGetOneTxBufferComplete(pSendData);
                }
            }
            break;
        }
        KITL_RETAILMSG(ZONE_ERROR, ("!Driver Send failure, retry %u\n",nRetries));
    } while (nRetries ++ < 8);

    KITL_RETAILMSG(ZONE_SEND,  (
        "-KitlEthSend(rc = %d)\r\n", code == 0));
    return code == 0;
}

//------------------------------------------------------------------------------
//
//  Function:  KitlEthRecv
//
//  This function is called by KITL to receive packet from transport layer
//
static BOOL KitlEthRecv(UCHAR *pData, USHORT *pLength)
{
    USHORT code;
    UCHAR *pSendData;
    ULONG sendLength;

    KITL_RETAILMSG(ZONE_RECV,  (
        "+KitlEthRecv(0x%08x, 0x%08x->%d)\r\n", pData, pLength, *pLength
    ));

    // Update multicast addresses
    if (g_kitlEthState.updateMCast) {
        if (g_kitlEthState.pDriver->pfnMulticastList)
            g_kitlEthState.pDriver->pfnMulticastList(
                g_kitlEthState.deviceMCast, g_kitlEthState.deviceMCastSize
            );
        g_kitlEthState.updateMCast = FALSE;
    }

    // Update filter 
    if (g_kitlEthState.updateFilter) {
        // Update filter
        g_kitlEthState.pDriver->pfnCurrentPacketFilter(
            g_kitlEthState.deviceFilter
        );
        // Tell VBRIDGE that there is new filter
        VBridgeUCurrentPacketFilter(&g_kitlEthState.deviceFilter);
        g_kitlEthState.updateFilter = FALSE;
    }

    // When VMINI is active send all queued VMINI packets
    if ((g_kitlEthState.flags & OAL_KITL_FLAGS_VMINI) != 0) {
        // First send all VMINI packets
        while (VBridgeKGetOneTxBuffer(&pSendData, &sendLength)) {
            g_kitlEthState.pDriver->pfnSendFrame(pSendData, (USHORT)sendLength);
            VBridgeKGetOneTxBufferComplete(pSendData);
        }
    }

    // Now get packet
    code = g_kitlEthState.pDriver->pfnGetFrame(pData, pLength);

    KITL_RETAILMSG(ZONE_RECV, ("-KitlEthRecv(rc = %d)\r\n", code > 0));
    return code > 0;
}


//------------------------------------------------------------------------------
//
//  Function:  KitlEthEnableInt
//
//  This function is called by KITL to enable and disable the KITL transport
//  interrupt if the transport is interrupt-based
//
//
static VOID KitlEthEnableInt(BOOL enable)
{
    KITL_RETAILMSG(ZONE_KITL_OAL, ("+KitlEthEnableInt(%d)\r\n", enable));
    if (enable) {
        g_kitlEthState.pDriver->pfnEnableInts();
    } else {
        g_kitlEthState.pDriver->pfnDisableInts();
    }
    KITL_RETAILMSG(ZONE_KITL_OAL, ("-KitlEthEnableInt\r\n"));
}

//------------------------------------------------------------------------------
//
//  Function:  OALIoCtlVBridge
//
//  This function implements OEMKitlIoctl calls, e.g. VMINI related IOCTL codes.
//
BOOL OALIoCtlVBridge(
    UINT32 code, VOID *pInBuffer, UINT32 inSize, VOID *pOutBuffer, 
    UINT32 outSize, UINT32 *pOutSize
) {
    BOOL rc = TRUE;

    KITL_RETAILMSG(ZONE_SEND|ZONE_RECV, ("+OALIoCtlVBridge(0x%08x,...)\r\n", code));

    switch (code) {
    case IOCTL_VBRIDGE_GET_TX_PACKET:
        rc = VBridgeUGetOneTxPacket((UINT8**)pOutBuffer, inSize);
        break;

    case IOCTL_VBRIDGE_GET_TX_PACKET_COMPLETE:
        VBridgeUGetOneTxPacketComplete((UINT8*)pInBuffer, inSize);
        break;

    case IOCTL_VBRIDGE_GET_RX_PACKET:
        rc = VBridgeUGetOneRxPacket((UINT8**)pOutBuffer, pOutSize);
        break;

    case IOCTL_VBRIDGE_GET_RX_PACKET_COMPLETE:
        VBridgeUGetOneRxPacketComplete((UINT8*)pInBuffer);
        break;

    case IOCTL_VBRIDGE_GET_ETHERNET_MAC:
        VBridgeUGetEDBGMac((UINT8*)pOutBuffer);
        break;

    case IOCTL_VBRIDGE_CURRENT_PACKET_FILTER:
        g_kitlEthState.deviceFilter = *(UINT32*)pInBuffer;
        g_kitlEthState.updateFilter = TRUE;
        break;

    case IOCTL_VBRIDGE_802_3_MULTICAST_LIST:
        if (inSize * 6 <= sizeof(g_kitlEthState.deviceMCast)) {
            memcpy(g_kitlEthState.deviceMCast, pInBuffer, inSize * 6);
            g_kitlEthState.deviceMCastSize = inSize;
            g_kitlEthState.updateMCast = TRUE;
        } else {
            rc = FALSE;
        }
        break;

    case IOCTL_VBRIDGE_WILD_CARD:
        rc = VBridgeUWildCard(
            pInBuffer, inSize, pOutBuffer, outSize, pOutSize
        );
        break;

    case IOCTL_VBRIDGE_SHARED_ETHERNET:
        break;

    default:
        NKSetLastError (ERROR_NOT_SUPPORTED);
        rc = FALSE;
    }

    KITL_RETAILMSG(ZONE_SEND|ZONE_RECV, ("-OALIoCtlVBridge(rc = %d)\r\n", rc));
    return rc;
}

//------------------------------------------------------------------------------
//
//  Function:  OALKitlEthInit
//
//  This function is called by OAL to initialize KITL Ethernet/IP4 protocol
//  driver. It first initialize KITL device, fill KITLTRANSPORT structure
//  and if OAL_KITL_FLAGS_DHCP flag is set it gets or renews IP4 address
//  from DHCP server.
//
BOOL OALKitlEthInit(
    LPSTR deviceId, OAL_KITL_DEVICE *pDevice, OAL_KITL_ARGS *pArgs,
    KITLTRANSPORT *pKitl
) {
    BOOL rc = FALSE;
    OAL_KITL_ETH_DRIVER *pDriver;
    UINT32 irq, sysIntr;

    KITL_RETAILMSG(ZONE_KITL_OAL, (
        "+OALKitlEthInit('%S', '%s', 0x%08x, 0x%08x)\r\n",
        deviceId, pDevice->name, pArgs, pKitl
    ));

    // Cast driver config parameter
    pDriver = (OAL_KITL_ETH_DRIVER*)pDevice->pDriver;
    if (pDriver == NULL) {
        KITL_RETAILMSG(ZONE_ERROR, ("ERROR: KITL device driver is NULL\r\n"));
        goto cleanUp;
    }

    // Call InitDmaBuffer if there is any
    if (pDriver->pfnInitDmaBuffer != NULL) {
        if (!pDriver->pfnInitDmaBuffer(
            (DWORD)g_oalKitlBuffer, sizeof(g_oalKitlBuffer)
        )) {
                KITL_RETAILMSG(ZONE_ERROR, (
                "ERROR: KITL call to pfnInitDmaBuffer failed\r\n"
            ));
            goto cleanUp;
        }
    }

    // Call pfnInit
    if (!pDriver->pfnInit(
        (UCHAR*)pArgs->devLoc.PhysicalLoc, pArgs->devLoc.LogicalLoc, pArgs->mac
    )) {
        KITL_RETAILMSG(ZONE_ERROR, ("ERROR: KITL call to pfnInit failed\r\n"));
        goto cleanUp;
    }

    // Extend name if flag is set
    if ((pArgs->flags & OAL_KITL_FLAGS_EXTNAME) != 0) {
        OALKitlCreateName(deviceId, pArgs->mac, deviceId);
    }

    // Now we know final name, print it
    KITL_RETAILMSG(ZONE_INIT, ("KITL: *** Device Name %s ***\r\n", deviceId));

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
            aIrqs[1] = (pArgs->devLoc.IfcType == InterfaceTypeUndefined)
                        ? OAL_INTR_TRANSLATE
                        : OAL_INTR_FORCE_STATIC;
            aIrqs[2] = irq;
            if (
                OEMIoControl(
                    IOCTL_HAL_REQUEST_SYSINTR, aIrqs, sizeof(aIrqs), &sysIntr,
                    sizeof(sysIntr), NULL
                ) && sysIntr != SYSINTR_UNDEFINED
            ) {                
                KITL_RETAILMSG(ZONE_INIT, ("KITL: using sysintr 0x%x\r\n", sysIntr));
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


    if (sysIntr == KITL_SYSINTR_NOINTR) {
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
    pKitl->FrmHdrSize    = KitlEthGetFrameHdrSize();
    pKitl->dwPhysBuffer  = 0;
    pKitl->dwPhysBufLen  = 0;
    pKitl->pfnEncode     = KitlEthEncode;
    pKitl->pfnDecode     = KitlEthDecode;
    pKitl->pfnSend       = KitlEthSend;
    pKitl->pfnRecv       = KitlEthRecv;
    pKitl->pfnEnableInt  = KitlEthEnableInt;
    pKitl->pfnGetDevCfg  = KitlEthGetDevCfg;
    pKitl->pfnSetHostCfg = KitlEthSetHostCfg;

    //-----------------------------------------------------------------------
    // Initalize KITL IP4 state structure
    //-----------------------------------------------------------------------

    g_kitlEthState.pDriver = pDriver;
    g_kitlEthState.flags = pArgs->flags;
    g_kitlEthState.deviceMAC[0] = (UINT8)pArgs->mac[0];
    g_kitlEthState.deviceMAC[1] = (UINT8)(pArgs->mac[0] >> 8);
    g_kitlEthState.deviceMAC[2] = (UINT8)pArgs->mac[1];
    g_kitlEthState.deviceMAC[3] = (UINT8)(pArgs->mac[1] >> 8);
    g_kitlEthState.deviceMAC[4] = (UINT8)pArgs->mac[2];
    g_kitlEthState.deviceMAC[5] = (UINT8)(pArgs->mac[2] >> 8);
    g_kitlEthState.deviceIP = pArgs->ipAddress;

    g_kitlEthState.kitlServerMAC[0] = 0xFF;
    g_kitlEthState.kitlServerMAC[1] = 0xFF;
    g_kitlEthState.kitlServerMAC[2] = 0xFF;
    g_kitlEthState.kitlServerMAC[3] = 0xFF;
    g_kitlEthState.kitlServerMAC[4] = 0xFF;
    g_kitlEthState.kitlServerMAC[5] = 0xFF;
    g_kitlEthState.kitlServerIP = 0xFFFFFFFF;
    g_kitlEthState.kitlServerPort = htons(KITL_SERVER_PORT);

    g_kitlEthState.dhcpXId = pArgs->mac[2] | 0x17016414;
    g_kitlEthState.dhcpState = DHCP_BOUND;

    // Get or renew DHCP address
    if ((pArgs->flags & OAL_KITL_FLAGS_DHCP) != 0) {
        // KITL isn't running let use direct functions for send/receive
        g_kitlEthState.pfnSend = pKitl->pfnSend;
        g_kitlEthState.pfnRecv = pKitl->pfnRecv;
        // Get or renew address from DHCP server
        GetAddressDHCP(pArgs->ipAddress);
    }

    // When KITL is running we should call sync function for send
    g_kitlEthState.pfnSend = KitlSendRawData;

    // There should not be direct receive
    g_kitlEthState.pfnRecv = NULL;

#ifdef KITL_ETHER     
    // Activate VMINI bridge...
    if ((pArgs->flags & OAL_KITL_FLAGS_VMINI) != 0) {
        VBridgeInit();
        VBridgeKSetLocalMacAddress((char*)pArgs->mac);
    }
#endif

    // Result depends on fact if we get IP address
    rc = (g_kitlEthState.deviceIP != 0);
    

cleanUp:
    KITL_RETAILMSG(ZONE_KITL_OAL, ("-OALKitlEthInit(rc = %d)\r\n", rc));
    return rc;
}

//------------------------------------------------------------------------------
