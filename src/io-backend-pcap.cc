/* -*- mode: c++; c-basic-offset: 4 indent-tabs-mode: nil -*- */
/*
 * Copyright 2012 Teclo Networks AG
 */

#include <pcap.h>

#include "log.h"
#include "io-backend.h"

#define PCAP(pcap_handle, form)                         \
    do {                                                \
        int ret = (form);                               \
        if (ret < 0) {                                  \
            warn("%s returned %d: %s\n", #form, ret,    \
                 pcap_geterr(pcap_handle));             \
            return false;                               \
        }                                               \
    } while (0)

class IoBackendPcap : public IoBackend {
public:
    IoBackendPcap(IoInterface* iface)
        : IoBackend(iface), pcap_(NULL) {
    };

    virtual ~IoBackendPcap() {
        if (pcap_) {
            close();
        }
    };

    virtual bool open() {
        char errbuf[PCAP_ERRBUF_SIZE] = "";

        iface()->set_io(this);

        pcap_ = pcap_create(iface()->name().c_str(), errbuf);
        if (pcap_ == NULL) {
            warn("pcap_create(): %s\n", errbuf);
            return false;
        }

        const int snaplen = 2048;

        PCAP(pcap_, pcap_set_snaplen(pcap_, snaplen));
        PCAP(pcap_, pcap_set_timeout(pcap_, 0));
        PCAP(pcap_, pcap_set_promisc(pcap_, 1));

        // 8MB buffer size. Not a totally arbitrary number. The assumptions:
        //
        // - A single process will not do more than 1Gbps on a 1G link.
        // - Average packet size is 750 bytes -> 16k packets/second.
        // - Each packet regardless of actual size requires buffer space for
        //   snaplen rounded up to the next power of 2.
        // - We'd like to buffer about 0.1s worth of data. No point in
        //   buffering any more than that. Ideally our normal latency
        //   is in the microsecond range, and with realtime priority
        //   no disaster should cause a latency spike of more than a
        //   few milliseconds.
        PCAP(pcap_, pcap_set_buffer_size(pcap_, 8*1024*1024));
        PCAP(pcap_, pcap_activate(pcap_));
        PCAP(pcap_, pcap_setnonblock(pcap_, 1, errbuf));
        PCAP(pcap_, pcap_setnonblock(pcap_, 1, errbuf));

        return true;
    }

    virtual void close() {
        pcap_close(pcap_);
        pcap_ = NULL;
    }

    virtual bool inject(Packet* p) {
        return pcap_inject(pcap_, (char*) p->ethh_, p->length_);
    }

    virtual bool receive(Packet* p) {
        struct pcap_pkthdr pkthdr;
        uint8_t *frame = (unsigned char*) pcap_next(pcap_, &pkthdr);
        uint32_t length = pkthdr.caplen;

        if (frame == NULL)
            return false;

        switch (pcap_datalink(pcap_)) {
        case DLT_EN10MB:
            break;
        default:
            fail("Error: Unsupported linktype: %s\n",
                 pcap_datalink_val_to_name(pcap_datalink(pcap_)));
        }

        p->init(frame, length, iface());
        p->from_iface_ = iface();

        return true;
    }

    virtual int select_fd() const {
        return pcap_fileno(pcap_);
    }

    virtual bool read_stats(uint64_t* userspace_dropped,
                            uint64_t* kernel_dropped) {
        struct pcap_stat stats;
        if (pcap_stats(pcap_, &stats) == 0) {
            *userspace_dropped = stats.ps_drop;
            *kernel_dropped = stats.ps_ifdrop;
        } else {
            return false;
        }

        return true;
    }

private:
    pcap_t* pcap_;
};

IoBackend* io_new_pcap(IoInterface* iface) {
    return new IoBackendPcap(iface);
}

