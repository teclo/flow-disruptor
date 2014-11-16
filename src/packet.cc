/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- */
/*
 * Copyright 2010 Teclo Networks AG
 */

#include "io-backend.h"
#include "log.h"
#include "packet.h"

Packet::Packet(const Packet& other)
    : PacketHeader(other),
      length_(other.length_),
      from_iface_(other.from_iface_),
      ethh_(NULL) {
    if (other.ethh_) {
        uint8_t* buf = (uint8_t*) malloc(length_);
        memcpy(buf, other.ethh_, length_);
        ethh_ = (pkt_eth_t*) buf;
    }
}

Packet::~Packet() {
    release();
}

void Packet::release() {
    if (ethh_) {
        free(ethh_);
        ethh_ = NULL;
    }
}

bool Packet::init(uint8_t* frame, size_t length, IoInterface* from_iface) {
    const int network_order_ether_vlan = htons(PKT_ETHER_VLAN);
    const int network_order_ether_ip = htons(PKT_ETHER_TYPE_IP);
    const int network_order_ether_ip6 = htons(PKT_ETHER_TYPE_IPV6);

    Clear();

    if (ethh_ != NULL) {
        warn("Packet already in use\n");
        return false;
    }

    {
        length_ = length;

        if (length < PKT_ETHER_HEADER_LEN) {
            return false;
        } else {
            // ethh ok.

            uint8_t* buf = (uint8_t*) malloc(length);
            memcpy(buf, frame, length);
            ethh_ = (pkt_eth_t*) buf;
        }
    }

    uint16_t proto = ethh_->h_proto;

    if (proto == network_order_ether_vlan) {
        pkt_veth_t *vethh = (pkt_veth_t*) ethh_;
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
    pkt_ip_t* iph = reinterpret_cast<pkt_ip_t*>(frame);

    if ((length < sizeof(pkt_ip_t)) ||
        (iph->ihl < 5) ||
        (iph->version != 4) ||
        (length < iph->ihl*4U)) {
        return false;
    }

    IPHeader* parsed = mutable_ipv4();

    parsed->mutable_saddr()->Add(ntohl(iph->saddr));
    parsed->mutable_daddr()->Add(ntohl(iph->daddr));
    parsed->set_is_ip_fragment(iph->frag_off & ~htons(1 << 14));
    length -= iph->ihl * 4;

    if ((iph->protocol != PKT_IP_PROTO_TCP)) {
        return true;
    }

    if ((ntohs(iph->frag_off) & 0x1fff) != 0 || length < 20) {
        return false;
    }

    return parse_tcp(reinterpret_cast<pkt_tcp_t*>(frame + iph->ihl*4),
                     length);
}

bool Packet::parse_ipv6(uint8_t* frame, size_t length) {
    pkt_ip6_t* ip6h = reinterpret_cast<pkt_ip6_t*>(frame);

    if ((length < sizeof(pkt_ip6_t)) ||
        ((ip6h->version_class_flow[0] >> 4) != 6) ||
        (length < ntohs(ip6h->payload_len) + sizeof(pkt_ip6_t))) {
        return false;
    }

    IPHeader* parsed = mutable_ipv6();

    for (int i = 0; i < 16; ++i) {
        parsed->mutable_saddr()->Add(ntohl(ip6h->saddr[i]));
        parsed->mutable_daddr()->Add(ntohl(ip6h->daddr[i]));
    }

    length -= sizeof(pkt_ip6_t);

    size_t options_length = 0;
    unsigned char *next_header = &ip6h->next_header;
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
            parsed->set_is_ip_fragment(true);
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

    length -= (40 + options_length);
    return parse_tcp(reinterpret_cast<pkt_tcp_t*>(frame + 40 + options_length),
                     length);
}

bool Packet::parse_tcp(pkt_tcp_t* tcph, size_t length) {
    mutable_tcp()->set_syn(tcph->syn);
    mutable_tcp()->set_ack(tcph->ack);
    mutable_tcp()->set_rst(tcph->rst);
    mutable_tcp()->set_fin(tcph->fin);

    mutable_tcp()->set_source_port(ntohs(tcph->source));
    mutable_tcp()->set_dest_port(ntohs(tcph->dest));

    mutable_tcp()->set_seq(ntohl(tcph->seq));
    mutable_tcp()->set_ack_seq(ntohl(tcph->ack_seq));

    uint32_t payload_len = length - tcph->doff * 4;
    mutable_tcp()->set_end_seq(tcp().seq() + payload_len);

    return true;
}
