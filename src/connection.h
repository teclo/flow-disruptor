/* -*- mode: c++; c-basic-offset: 4 indent-tabs-mode: nil -*- */
/*
 * Copyright 2014 Teclo Networks AG
 */

#ifndef _CONNECTION_H_
#define _CONNECTION_H_

#include <cstdint>
#include <deque>
#include <string>

#include "connection-table.h"
#include "pcap-dumper.h"
#include "state.h"
#include "throttler.h"

class TcpFlow {
public:
    TcpFlow(State* state, Profile* profile,
            IoInterface* iface, const std::string& id);
    ~TcpFlow();

    TcpFlow* other() { return other_; }
    void set_other(TcpFlow* other) { other_ = other; }

    IoInterface* iface() { return iface_; }
    Throttler* throttler() { return &throttler_; }

    void transmit_timeout();

    void record_packet_rx(Packet* p);
    void queue_packet_tx(Packet* p);

    bool is_valid_synack(Packet* p);
    bool is_valid_3whs_ack(Packet* p);
    bool is_identical_syn(Packet* p);
    bool is_identical_synack(Packet* p);

    double delay() const { return delay_s_; }
    void set_delay(double delay_s) { delay_s_ = delay_s; }

    bool should_close();
    bool can_close();

private:
    void reschedule_transmit_timer();
    void transmit();

    State* state_;
    TcpFlow* other_;
    IoInterface* iface_;
    PcapDumper dumper_;

    // Packet transmit queue. The packet at the head of the queue
    // should be transmitted at the timestamp indicated in the first
    // element of the pair.
    std::deque<std::pair<ev_tstamp, Packet*> > packets_;
    Timer transmit_timer_;

    double delay_s_;

    bool received_rst_;
    bool received_fin_;

    // First unacknowledged sequence number.
    uint32_t snd_una_;
    // First sequence number not sent yet.
    uint32_t snd_nxt_;
    // seq of latest window update
    uint32_t snd_wl1_;
    // ack of last window update
    uint32_t snd_wl2_;
    // Advertised window space
    uint32_t snd_wnd_;

    Throttler throttler_;
};

// Collection of pointers that make up a TCP packet
class Connection {
public:
    virtual ~Connection();

    void receive(Packet* p);
    void close();

    TcpFlow* packet_source_flow(Packet* p);

    virtual int addr_bytes() const = 0;
    ConnectionKey* key() { return &key_; }

    static Connection* make(Profile* profile, Packet* p, State* state);

    enum ConnectionState {
        STATE_SYN,
        STATE_SYN_ACK,
        STATE_ESTABLISHED,
        STATE_CLOSING,
    };

protected:
    explicit Connection(Profile* profile, Packet* p, State* state);

    void apply_timed_effect(Timer* timer, const TimedEvent& event);
    void revert_timed_effect(Timer* timer, const TimedEvent& event);

private:
    State* state_;
    Profile* profile_;
    ev_tstamp first_syn_timestamp_;

    ConnectionState connection_state_;
    ConnectionKey key_;
    std::string id_;
    TcpFlow client_;
    TcpFlow server_;
    Timer idle_timer_;

    std::vector<Timer*> event_timers_;
};

#endif // CONNECTION_H
