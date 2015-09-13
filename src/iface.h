/* -*- mode: c++; c-basic-offset: 4 indent-tabs-mode: nil -*- */
/*
 * Copyright 2011 Teclo Networks AG
 */

#ifndef _IFACE_H_
#define _IFACE_H_

#include <cstdbool>
#include <string>

class IoBackend;

// A named physical interface, paired with another interface and having
// a packet IO backend for reading / writing packets.
class IoInterface {
public:
    enum Direction {
        DOWNLINK,
        UPLINK,
    };

    IoInterface(const std::string& name, Direction direction)
        : name_(name),
          direction_(direction) {
    }

    virtual ~IoInterface() {
    }

    IoBackend* io() { return io_; }
    void set_io(IoBackend* io) { io_ = io; }

    // The opposite side of this interface pair.
    IoInterface* other() { return other_; }
    void set_other(IoInterface* other) { other_ = other; }

    const std::string& name() const { return name_; }
    Direction direction() const { return direction_; }

private:

    // Not owned
    IoBackend* io_;
    // Not owned.
    IoInterface* other_;
    const std::string name_;
    Direction direction_;
};

#endif	/* _IFACE_H_ */
