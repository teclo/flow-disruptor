/* -*- mode: c++; c-basic-offset: 4 indent-tabs-mode: nil -*- */
/*
 * Copyright 2014 Teclo Networks AG
 */

#ifndef PCAP_DUMPER_H_
#define PCAP_DUMPER_H_

#include <pcap.h>

#include "packet.h"

class PcapDumper {
public:
    PcapDumper(const std::string& file);
    ~PcapDumper();

    bool open();
    void close();

    void dump_packet(Packet* packet);

private:
    std::string filename_;

    pcap_t* pcap_;
    pcap_dumper_t* pcap_dumper_;
    bool open_;
};

#endif // PCAP_DUMPER_H
