/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- */
/*
 * Copyright 2014 Teclo Networks AG
 */

#include "pcap-dumper.h"

#include <cassert>

#include "log.h"
#include "packet.h"

PcapDumper::PcapDumper(const std::string& file)
    : filename_(file),
      open_(false) {
}

PcapDumper::~PcapDumper() {
    if (open_) {
        close();
    }
}

bool PcapDumper::open() {
    assert(!open_);

    pcap_ = pcap_open_dead(DLT_EN10MB, PKT_IP_MAX_SIZE);
    pcap_dumper_ = pcap_dump_open(pcap_, filename_.c_str());

    if (pcap_dumper_ == NULL) {
        warn("pcap_dump_open failed: %s", pcap_geterr(pcap_));
        return false;
    }

    open_ = true;
    return true;
}

void PcapDumper::close() {
    assert(open_);
    pcap_dump_flush(pcap_dumper_);
    pcap_dump_close(pcap_dumper_);
    pcap_close(pcap_);
}

void PcapDumper::dump_packet(Packet* packet) {
    if (!open_) {
        return;
    }

    struct pcap_pkthdr h;
    h.caplen = packet->length_;
    h.len = packet->length_;
    gettimeofday(&h.ts, NULL);
    pcap_dump(reinterpret_cast<uint8_t*>(pcap_dumper_),
              &h,
              reinterpret_cast<uint8_t*>(packet->ethh_));
}
