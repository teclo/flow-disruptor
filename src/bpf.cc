/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- */
/*
 * Copyright 2010 Teclo Networks AG
 */

#include "bpf.h"

#include <stdlib.h>

#include "log.h"
#include "packet.h"

PacketFilter::PacketFilter(const std::string& filter)
    : valid_(false) {
    memset(&program_, 0, sizeof(program_));

    pcap_t *p = pcap_open_dead(DLT_EN10MB, PKT_IP_MAX_SIZE);
    if (pcap_compile(p,
                     &program_,
                     filter.c_str(),
                     1,
                     PCAP_NETMASK_UNKNOWN) < 0) {
        warn("invalid filter '%s': %s",
             filter.c_str(),
             pcap_geterr(p));
    } else {
        valid_ = true;
    }

    pcap_close(p);
}

PacketFilter::~PacketFilter() {
    if (valid_) {
        pcap_freecode(&program_);
    }
}

bool PacketFilter::is_valid() const {
    return valid_;
}

bool PacketFilter::packet_matches_filter(const Packet* p) const {
    return bpf_filter(program_.bf_insns,
                      (const u_char *) p->ethh_, p->length_, p->length_);
}
