/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- */
/*
 * Copyright 2010 Teclo Networks AG
 */

#include "io-backend.h"
#include "log.h"
#include "packet.h"

Packet::~Packet() {
    if (ethh_) {
        from_iface_->io()->release_packet(this);
    }
}

bool Packet::init(uint8_t* frame, size_t length, IoInterface* from_iface) {
    const int network_order_ether_vlan = htons(PKT_ETHER_VLAN);
    const int network_order_ether_ip = htons(PKT_ETHER_TYPE_IP);
    const int network_order_ether_ip6 = htons(PKT_ETHER_TYPE_IPV6);

    if (ethh_ != NULL) {
        warn("Packet already in use\n");
        return false;
    }

    pkt_eth_t* ethh = (pkt_eth_t*) frame;

    length_ = length;
    tcph_ = NULL;
    is_ip_fragment_ = false;
    needs_checksum_ = false;

    if (length < PKT_ETHER_HEADER_LEN) {
        return false;
    } else {
        // ethh ok.
        ethh_ = ethh;
    }

    uint16_t proto = ethh->h_proto;

    if (proto == network_order_ether_vlan) {
        pkt_veth_t *vethh = (pkt_veth_t *)ethh;
        proto = vethh->h_proto;
        frame += sizeof(pkt_veth_t);
        length -= sizeof(pkt_veth_t);
    } else {
        frame += sizeof(pkt_eth_t);
        length -= sizeof(pkt_eth_t);
    }

    if (proto == network_order_ether_ip) {
        return parse_ipv4(frame, length);
    } else if (proto == network_order_ether_ip6) {
        return parse_ipv6(frame, length);
    } else {
        // Non-IP.
        return true;
    }
}

bool Packet::parse_ipv4(uint8_t* frame, size_t length) {
    {
        pkt_ip_t* iph = reinterpret_cast<pkt_ip_t*>(frame);

        if ((length < sizeof(pkt_ip_t)) ||
            (iph->ihl < 5) ||
            (iph->version != 4) ||
            (length < iph->ihl*4U)) {
            return false;
        }

        iph_ = iph;
    }

    addr_bytes_ = 4;
    ip_saddr_ = &iph_->saddr;
    ip_daddr_ = &iph_->daddr;
    is_ip_fragment_ = iph_->frag_off & ~htons(1 << 14);
    length -= iph_->ihl * 4;

    if ((iph_->protocol != PKT_IP_PROTO_TCP)) {
        return true;
    }

    if ((ntohs(iph_->frag_off) & 0x1fff) != 0 ||
        length < 20) {
        return false;
    }

    tcph_ = reinterpret_cast<pkt_tcp_t*>(frame + iph_->ihl*4);

    return true;
}

bool Packet::parse_ipv6(uint8_t* frame, size_t length) {
    {
        pkt_ip6_t* ip6h = reinterpret_cast<pkt_ip6_t*>(frame);

        if ((length < sizeof(pkt_ip6_t)) ||
            ((ip6h->version_class_flow[0] >> 4) != 6) ||
            (length < ntohs(ip6h->payload_len) + sizeof(pkt_ip6_t))) {
            return false;
        }

        ip6h_ = ip6h;
    }

    addr_bytes_ = 16;
    ip_saddr_ = (uint32_t *) &ip6h_->saddr;
    ip_daddr_ = (uint32_t *) &ip6h_->daddr;
    length -= sizeof(pkt_ip6_t);

    size_t options_length = 0;
    unsigned char *next_header = &ip6h_->next_header;
    unsigned char *next_next_header = (frame + 40);

    bool done = false;
    while (!done) {
        if (length < 20 + options_length) {
            return false;
        }

        switch (*next_header) {
        case PKT_IP_PROTO_HOPOPTS:
        case PKT_IP_PROTO_ROUTING:
        case PKT_IP_PROTO_DSTOPTS:
            next_header = next_next_header;
            next_next_header = next_header + 8 + 8 * *(next_header + 1);
            options_length += 8 + 8 * *(next_header + 1);
            done = true;
            break;

        case PKT_IP_PROTO_FRAGMENT: {
            is_ip_fragment_ = true;
            uint32_t fragment_offset = (*(next_next_header + 2) << 5) + (*(next_next_header + 3) >> 3);

            if (fragment_offset == 0) {
                next_header = next_next_header;
                next_next_header = next_header + 8;
                options_length += 8;
                done = true;
            } else {
                return false;
            }
            break;
        }

        case PKT_IP_PROTO_TCP:
            break;

        default:
            return false;
        }
    }

    tcph_ = reinterpret_cast<pkt_tcp_t*>(frame + 40 + options_length);
    ipv6_options_length_ = options_length;

    return true;
}
