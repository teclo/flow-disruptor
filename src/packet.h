/* -*- mode: c++; c-basic-offset: 4 indent-tabs-mode: nil -*- */
/*
 * Copyright 2012 Teclo Networks AG
 */

#ifndef _PACKET_H_
#define _PACKET_H_

#include <stdbool.h>
#include <stdint.h>

#include "iface.h"
#include "pkt_in.h"

enum buffer_owner {
    BUFFER_OWNER_UNKNOWN,
    BUFFER_OWNER_BACKEND,
    BUFFER_OWNER_APPLICATION,
};

// Collection of pointers that make up a TCP packet
class Packet {
public:
    Packet() : ethh_(NULL) {
    }

    ~Packet();

    bool init(uint8_t* frame, size_t length, IoInterface* from_iface);

    // Size of packet buffer.
    size_t length_;

    // Interface this packet was received from.
    IoInterface* from_iface_;

    // Ethernet header (coincides with start of packet buffer).
    pkt_eth_t* ethh_;

    // IPv4 header
    pkt_ip_t* iph_;

    // IPv6 header
    pkt_ip6_t* ip6h_;

    // TCP header
    pkt_tcp_t* tcph_;

    // The checksum should be recomputed before transmit.
    bool needs_checksum_;

    // Which part of the application owns the memory?
    enum buffer_owner owner_;

    // Pointers into the IPvN header at the addresses
    int addr_bytes_;
    uint32_t* ip_saddr_;
    uint32_t* ip_daddr_;

    // Store this when parsing the packet so we don't have to compute
    // it every time in tcp_length()
    int ipv6_options_length_;
    // Likewise store whether this is a fragmented packet at the IP
    // level.
    bool is_ip_fragment_;

private:
    bool parse_ipv4(uint8_t* frame, size_t length);
    bool parse_ipv6(uint8_t* frame, size_t length);
};

#endif // PACKET_H
