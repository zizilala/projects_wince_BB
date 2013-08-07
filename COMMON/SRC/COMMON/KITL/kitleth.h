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
//  File:  kitleth.h
//
#ifndef __KITLETH_H
#define __KITLETH_H

//------------------------------------------------------------------------------

#define DHCP_TIMEOUT        30000
#define ARP_TIMEOUT         2000

//------------------------------------------------------------------------------

#define ARP_FRAME           0x0806
#define IP_FRAME            0x0800

#define ICMP_PROTOCOL       1
#define UDP_PROTOCOL        17

#define DHCP_CLIENT_PORT    0x0044
#define DHCP_SERVER_PORT    0x0043
#define DHCP_COOKIE         0x82538263

#define KITL_CLIENT_PORT    981
#define KITL_SERVER_PORT    981

//------------------------------------------------------------------------------

#pragma pack( push, 1 )

typedef struct {
    UINT8  destmac[6];
    UINT8  srcmac[6];
    UINT16 ftype;
} ETH_HEADER;

typedef struct {
    UINT8  verlen;
    UINT8  tos;
    UINT16 length;
    UINT16 id;
    UINT16 fragment;
    UINT8  ttl;
    UINT8  protocol;
    UINT16 sum;
    UINT32 srcip;
    UINT32 destip;
} IP4_HEADER;

typedef struct  {
    UINT16 srcPort;
    UINT16 dstPort;
    UINT16 length;
    UINT16 sum;
} UDP_HEADER;

typedef struct {
    UINT16 htype;
    UINT16 ptype;
    UINT8  hsize;
    UINT8  psize;
    UINT16 op;
    UINT8  srcmac[6];
    UINT32 srcip;
    UINT8  destmac[6];
    UINT32 destip;
} ARP_MESSAGE;

typedef enum {
    ARP_REQUEST = 1,
    ARP_RESPONSE = 2
} ARP_MESSAGE_OP;

typedef struct {
    UINT8  op;
    UINT8  code;
    UINT16 sum;
    UINT8  data[];
} ICMP_HEADER;

typedef enum {
    ICMP_ECHOREPLY = 0,
    ICMP_ECHOREQ = 8
} ICMP_MESSAGE_OP;

typedef struct {
    UINT8  op;              // Op/message code
    UINT8  htype;           // Hardware address type
    UINT8  hlen;            // Hardware address length
    UINT8  hops;            // 
    UINT32 xid;             // Transaction ID
    UINT16 secs;            // Time since renewal start
    UINT16 flags;           // Flags
    UINT32 ciaddr;          // Existing client address
    UINT32 yiaddr;          // Address to use for the client
    UINT32 siaddr;          // Address of server to use for next step
    UINT32 giaddr;          // Address of relay agent being used, if any
    UINT8  chaddr[16];      // Client hardware address
    CHAR   sname[64];       // Optional server host name
    CHAR   file[128];       // Optional boot file name
    UINT8  options[0];      // Optional parameter fields
} BOOTP_MESSAGE;

typedef enum {
    DHCP_DISCOVER = 1,
    DHCP_OFFER = 2,
    DHCP_REQUEST = 3,
    DHCP_DECLINE = 4,
    DHCP_ACK = 5,
    DHCP_NAK = 6,
    DHCP_RELEASE = 7
} DHCP_MESSAGE_TYPE;

typedef enum {
    DHCP_SUBNET_MASK = 1,
    DHCP_HOSTNAME = 12,
    DHCP_IP_ADDR_REQ = 50,
    DHCP_LEASE_TIME = 51,
    DHCP_OPTION_OVERLOAD = 52,
    DHCP_MSGTYPE = 53,
    DHCP_SERVER_ID = 54,
    DHCP_RENEW_TIME = 58,
    DHCP_REBIND_TIME = 59,
    DHCP_CLIENT_ID = 61,
    DHCP_END = 255
} DHCP_OPTIONS;

typedef enum {
    DHCP_BOUND,
    DHCP_SELECTING,
    DHCP_REQUESTING
} DHCP_STATE;

#pragma pack( pop )

//------------------------------------------------------------------------------

#endif
