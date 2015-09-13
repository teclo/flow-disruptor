/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * Copyright 2010 Teclo Networks AG
 */

#ifndef _TECLO_BPF_H_
#define _TECLO_BPF_H_

#include <pcap.h>
#include <pcap-bpf.h>
#include <string>

#include "base.h"
#include "packet.h"

// A wrapper class for pcap filters. Takes a filter expression and
// compiles it, and can then be used for matching packets against the
// compiled expression.
class PacketFilter {
public:
    explicit PacketFilter(const std::string& filter);
    ~PacketFilter();

    // Was the provided filter expression valid?
    bool is_valid() const;
    // Does this packet match the filter expression?
    bool packet_matches_filter(const Packet* p) const;

private:
    DISALLOW_COPY_AND_ASSIGN(PacketFilter);

    struct bpf_program program_;
    bool valid_;
};

#endif /* _TECLO_BPF_H_ */
