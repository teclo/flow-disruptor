/* -*- mode: c++; c-basic-offset: 4 indent-tabs-mode: nil -*- */
/*
 * Copyright 2012 Teclo Networks AG
 */

#ifndef _IO_BACKEND_H_
#define _IO_BACKEND_H_

#include "iface.h"
#include "packet.h"

// An abstract class for doing IO on a network interface. Implement
// all methods in subclasses. Subclasses are generally not constructed
// directly, but via io_open().
class IoBackend {
public:
    explicit IoBackend(IoInterface* iface)
        : iface_(iface) {
    }

    virtual ~IoBackend() { }

    // ** Control

    // Open the interface for IO. Return false on error.
    virtual bool open() = 0;

    // Close the interface.
    virtual void close() = 0;

    // ** IO

    // Write the contents of this raw ethernet packet into the interface.
    // The backing memory of the packet should either be returned by
    // receive(), or allocated with allocate_buffer() /
    // allocate_inject_buffer().
    virtual bool inject(Packet* p) = 0;

    // Receive a raw ethernet packet from the interface. The caller should
    // call either retain_packet() or release_packet() before calling receive
    // again.
    virtual bool receive(Packet* p) = 0;

    // Flush outbound packets.
    virtual void flush() { }

    // ** Memory management

    // Request ownership of the backing storage of this packet from
    // the for an indefinite time. Does not modify the packet. Returns
    // a pointer to a newly allocated packet, with a memory buffer
    // that has the same contents as the original packet's buffer. The
    // pointer might be to the same or different memory.
    virtual Packet* retain_packet(Packet* p) = 0;

    // Release a packet's memory.
    virtual void release_packet(Packet* p) = 0;

    // ** Misc.

    // Return a file descriptor that can be select()ed on to wait for
    // packets to arrive on this interface. -1 if the backend does not
    // support select().
    virtual int select_fd() const = 0;

    // Fill in statistics about packet drops on this interface. Return
    // true if stats were available.
    virtual bool read_stats(uint64_t* userspace_dropped,
                            uint64_t* kernel_dropped) {
        return false;
    }

    // Return the interface object that this IO backend was created for
    // (there is always a 1:1 mapping).
    IoInterface* iface() const {
        return iface_;
    }

    virtual bool healthy() {
        return true;
    }

private:
    // Not owned
    IoInterface* iface_;
};

// Concrete IO backends:
enum IoBackendype {
    IO_CALLBACK,
    IO_INTEL,
    IO_PCAP,
    IO_RAW,
    IO_SNF,
    IO_TAP,
    IO_TUN,
    IO_TRACE,
};

// ... Constructors
IoBackend* io_new_pcap(IoInterface* iface);

#endif	/* _IO_BACKEND_H_ */
