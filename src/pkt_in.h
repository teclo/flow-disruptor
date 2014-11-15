/* -*- mode: c; c-basic-offset: 4 indent-tabs-mode: nil -*- */
/*
 * Copyright 2011 Teclo Networks AG
 */

// Packet definitions

#ifndef __PKT_IN_H__
#define __PKT_IN_H__

#include <stdio.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <memory.h>
#include <assert.h>

#include <arpa/inet.h>

#include <endian.h>

#define PKT_ETHER_ADDR_LEN    6
#define PKT_ETHER_HEADER_LEN  14	      // ether header len
#define PKT_VETHER_HEADER_LEN 18	      // vlan ether header len

typedef struct _pkt_eth_addr_t
{
    uint8_t addr[PKT_ETHER_ADDR_LEN];
} pkt_eth_addr_t;

typedef struct
{
    pkt_eth_addr_t h_dest;
    pkt_eth_addr_t h_source;
    uint16_t	h_proto;		      // ether type PKT_ETHER_TYPE_*
} __attribute__((packed)) pkt_eth_t;

// VLAN tagged ethernet frame
typedef struct
{
    pkt_eth_addr_t h_dest;
    pkt_eth_addr_t h_source;
    uint16_t	h_tpid;		              // fixed to PKT_ETHER_VLAN
    uint16_t    h_tci;                        // pcp:3, cfi:1, vid:12
    uint16_t	h_proto;		      // ether type PKT_ETHER_TYPE_*
} __attribute__((packed)) pkt_veth_t;


#define PKT_ETHER_TYPE_LOOP	0x0060		// loopback packet
#define PKT_ETHER_TYPE_IP	0x0800		// IPv4
#define PKT_ETHER_TYPE_ARP	0x0806		// Address Resolution packet
#define PKT_ETHER_TYPE_IPV6	0x86DD		// IPv6
#define PKT_ETHER_VLAN          0x8100          // 802.1Q frame format

// Minimum packet size, ethernet header + payload. This is the same
// with or without VLANs.
#define PKT_ETHER_MIN_SIZE      60

// Address resolution protocol
// RFC 826   -- 1982
// RFC 2390  -- 1998
//
typedef struct
{
    uint16_t hrd;   // format of hardware address
    uint16_t pro;   // format of protocol address
    uint8_t  hln;   // length of hardware address
    uint8_t  pln;   // length of protocol address
    uint16_t op;    // arp operation
} __attribute__((packed)) pkt_arp_t;

#define PKT_ARP_HRD_ETHER 	    1    // ethernet hardware format
#define PKT_ARP_HRD_IEEE802	    6    // token-ring hardware format
#define PKT_ARP_HRD_FRELAY 	    15   // frame relay hardware format
#define PKT_ARP_HRD_IEEE1394	    24   // IEEE1394 hardware address
#define PKT_ARP_NRD_IEEE1394_EUI64  27   // IEEE1394 EUI-64

#define	PKT_ARP_OP_REQUEST	1  // request to resolve address
#define	PKT_ARP_OP_REPLY	2  // response to previous request
#define	PKT_ARP_OP_REVREQUEST   3  // request protocol address given hardware
#define	PKT_ARP_OP_REVREPLY	4  // response giving protocol address
#define PKT_ARP_OP_INVREQUEST   8  // request to identify peer
#define PKT_ARP_OP_INVREPLY	9  // response identifying peer

typedef struct _pkt_ip_addr_t
{
    union {
	uint8_t  addr[4];
	uint32_t saddr;
    };
} __attribute__((packed)) pkt_ip_addr_t;

typedef struct
{
#if BYTE_ORDER == LITTLE_ENDIAN
    uint8_t ihl:4, version:4;
#elif BYTE_ORDER == BIG_ENDIAN
    uint8_t version:4, ihl:4;
#else
#error	"Please fix endian.h"
#endif
    uint8_t	tos;
    uint16_t	tot_len;
    uint16_t	id;
    uint16_t	frag_off;
    uint8_t	ttl;
    uint8_t	protocol;
    uint16_t	check;
    uint32_t	saddr;
    uint32_t	daddr;
} __attribute__((packed)) pkt_ip_t;

typedef struct
{
    uint8_t     version_class_flow[4];
    uint16_t	payload_len;
    uint8_t	next_header;
    uint8_t	hop_limit;
    uint8_t	saddr[16];
    uint8_t     daddr[16];
} __attribute__((packed)) pkt_ip6_t;

#define IPV4_ADDR_LEN 4
#define IPV6_ADDR_LEN 16

#define PKT_IP_MAX_SIZE    65535

#define PKT_IP_PROTO_IP    0
#define PKT_IP_PROTO_ICMP  1
#define PKT_IP_PROTO_IPV4  4  // IPv4 encapsulation
#define PKT_IP_PROTO_TCP   6
#define PKT_IP_PROTO_UDP   17

/* IPv6 */
#define PKT_IP_PROTO_HOPOPTS 0
#define PKT_IP_PROTO_ROUTING 43
#define PKT_IP_PROTO_FRAGMENT 44
#define PKT_IP_PROTO_DSTOPTS 60

#define PKT_IP_FRAG_RES  0x8000    // reserved fragment flag
#define PKT_IP_FRAG_DONT 0x4000    // dont fragment flag
#define PKT_IP_FRAG_MORE 0x2000    // more fragments flag
#define PKT_IP_FRAG_MASK 0x1fff    // mask for fragmenting bits

#define	IP_OPTION_COPIED(o)		((o)&0x80)
#define	IP_OPTION_CLASS(o)		((o)&0x60)
#define	IP_OPTION_NUMBER(o)		((o)&0x1f)

#define	PKT_IP_OPTION_CONTROL	0x00
#define	PKT_IP_OPTION_RESERVED1	0x20
#define	PKT_IP_OPTION_DEBMEAS	0x40
#define	PKT_IP_OPTION_RESERVED2	0x60

#define	PKT_IP_OPTION_EOL	0	   // end of option list
#define	PKT_IP_OPTION_NOP	1	   // no operation

#define	PKT_IP_OPTION_RR	7	   // record packet route
#define	PKT_IP_OPTION_TS	68	   // timestamp
#define	PKT_IP_OPTION_SECURITY  130	   // provide s,c,h,tcc
#define	PKT_IP_OPTION_LSRR	131        // loose source route
#define	PKT_IP_OPTION_SATID	136	   // satnet id
#define	PKT_IP_OPTION_SSRR	137	   // strict source route
#define	PKT_IP_OPTION_RA	148	   // router alert


// RFC 792 - 1981
//
typedef struct {
    uint8_t type;	// type of message
    uint8_t code;	// sub code
    uint16_t check;	// IP checksum over pkt_icmp_t + data
} __attribute__((packed)) pkt_icmp_t;

#define	PKT_ICMP_TYPE_ECHOREPLY	        0
#define	PKT_ICMP_TYPE_UNREACH	        3
#define	PKT_ICMP_TYPE_SOURCEQUENCH	4
#define	PKT_ICMP_TYPE_REDIRECT		5
#define	PKT_ICMP_TYPE_ECHO		8
#define	PKT_ICMP_TYPE_TIMXCEED		11
#define	PKT_ICMP_TYPE_TIMESTAMP		13
#define	PKT_ICMP_TYPE_TIMESTAMP_REPLY	14
#define	PKT_ICMP_TYPE_INFO_REQUEST	15
#define	PKT_ICMP_TYPE_INFO_REPLY	16

typedef struct {
    uint16_t	source;   // source port
    uint16_t	dest;     // destination port
    uint16_t    len;      // packet length
    uint16_t    check;    // udp checksum
} __attribute__((packed)) pkt_udp_t;

typedef struct {
    uint16_t	source;
    uint16_t	dest;
    uint32_t	seq;
    uint32_t	ack_seq;
#if BYTE_ORDER == LITTLE_ENDIAN
    uint8_t res1:4, doff:4;
    union {
	uint8_t flags:8;
	struct {
	uint8_t fin:1,syn:1,rst:1,psh:1,ack:1,urg:1,ece:1,cwr:1;
	};
    };
#elif BYTE_ORDER == BIG_ENDIAN
    uint8_t doff:4, res1:4;
    union {
	uint8_t flags:8;
	struct {
	uint8_t cwr:1,ece:1,urg:1,ack:1,psh:1,rst:1,syn:1,fin:1;
	};
    };
#else
#error	"Please fix endian.h"
#endif
    uint16_t	window;
    uint16_t	check;
    uint16_t	urg_ptr;
} __attribute__((packed)) pkt_tcp_t;

#define TCP_FLAG_FIN  0x01
#define TCP_FLAG_SYN  0x02
#define TCP_FLAG_RST  0x04
#define TCP_FLAG_PUSH 0x08
#define TCP_FLAG_ACK  0x10
#define TCP_FLAG_URG  0x20
#define TCP_FLAG_ECE  0x40
#define TCP_FLAG_CWR  0x80

// Relevant TCP options
#define TCP_OPTION_EOL            0
#define TCP_OPTION_NOP            1
#define TCP_OPTION_MSS            2  // max segment size
#define TCP_OPTION_WINDOW_SCALE   3
#define TCP_OPTION_SACK_PERMITTED 4
#define TCP_OPTION_SACK           5
#define TCP_OPTION_TIMESTAMP      8

// TCP_OPTION_SACK data
struct tcp_sack_block {
    uint32_t start_seq;
    uint32_t end_seq;
} __attribute__((packed));

// RFC 879
#define TCP_DEFAULT_MSS 536

extern pkt_eth_addr_t pkt_eth_broadcast;
extern pkt_eth_addr_t pkt_eth_zero;
static inline int pkt_eth_addr_cmp(pkt_eth_addr_t* a, pkt_eth_addr_t* b) {
    return memcmp(a->addr, b->addr, PKT_ETHER_ADDR_LEN);
}

#endif
