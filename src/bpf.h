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

class PacketFilter {
public:
    explicit PacketFilter(const std::string& filter_);
    ~PacketFilter();

    bool is_valid() const;
    bool packet_matches_filter(const Packet* p) const;

private:
    DISALLOW_COPY_AND_ASSIGN(PacketFilter);

    struct bpf_program program_;
    bool valid_;
};

#endif /* _TECLO_BPF_H_ */
