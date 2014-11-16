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
#include "PacketHeader.pb.h"

enum buffer_owner {
    BUFFER_OWNER_UNKNOWN,
    BUFFER_OWNER_BACKEND,
    BUFFER_OWNER_APPLICATION,
};

// Collection of pointers that make up a TCP packet
class Packet : public PacketHeader {
public:
    Packet() : ethh_(NULL) {
    }

    Packet(const Packet& other);

    ~Packet();

    bool init(uint8_t* frame, size_t length, IoInterface* from_iface);
    void release();

    // Size of packet buffer.
    size_t length_;

    // Interface this packet was received from.
    IoInterface* from_iface_;

    // Ethernet header (coincides with start of packet buffer).
    pkt_eth_t* ethh_;

private:
    bool parse_ipv4(uint8_t* frame, size_t length);
    bool parse_ipv6(uint8_t* frame, size_t length);
    bool parse_tcp(pkt_tcp_t* tcp, size_t length);
};

#endif // PACKET_H
