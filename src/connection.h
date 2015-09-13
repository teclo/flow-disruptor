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

// One half of a TCP connection.
class TcpFlow {
public:
    TcpFlow(State* state, Profile* profile,
            IoInterface* iface, const std::string& id);
    ~TcpFlow();

    TcpFlow* other() { return other_; }
    void set_other(TcpFlow* other) { other_ = other; }

    IoInterface* iface() { return iface_; }
    Throttler* throttler() { return &throttler_; }

    // Check if the first packet in the transmit queue is ready to be
    // sent, and if it is, send it.
    void transmit_timeout();

    // Read the information from a received packet and update the flow status.
    void record_packet_rx(Packet* p);
    // Queue a packet for transmission in this direction.
    void queue_packet_tx(Packet* p);

    // Is this packet a valid SYNACK (compared to the SYN)?
    bool is_valid_synack(Packet* p);
    // Is this packet a valid ACK to finish the three way handshake by?
    bool is_valid_3whs_ack(Packet* p);
    // Is this a SYN identical to the original SYN of the connection?
    bool is_identical_syn(Packet* p);
    // Is this a SYNACK identical to the original SYNACK of the connection?
    bool is_identical_synack(Packet* p);

    double delay() const { return delay_s_; }
    void set_delay(double delay_s) { delay_s_ = delay_s; }

    // True if we've gotten an RST in one direction, or a FIN in both
    // directions.
    bool should_close();
    // True if we've gotten an RST in one direction, or if there's no
    // data queued for transmission on this queue.
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

    // Amount of time to delay each packet transmitted toward this direction.
    double delay_s_;

    // True if a RST / FIN has been received on this flow.
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

    // Bandwidth limit on data sent to this flow.
    Throttler throttler_;
};

// A TCP connection.
class Connection {
public:
    virtual ~Connection();

    // Record a new packet for this connection.
    void receive(Packet* p);
    // Remove this connection from the socket table, and delete it.
    void close();

    // The connection consists of two flows. Which of the two was this
    // packet received on?
    TcpFlow* packet_source_flow(Packet* p);

    // Number of bytes in the IP address (ipv4 or ipv6).
    virtual int addr_bytes() const = 0;
    // The socket table key for this connection.
    ConnectionKey* key() { return &key_; }

    // Make a new connection based on a SYN packet, using the specified
    // profile. Insert the connection in the socket table.
    static Connection* make(Profile* profile, Packet* p, State* state);

    // TCP state machine for the connection.
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

    void apply_effect(const Effect& effect);
    void revert_effect(const Effect& effect);

private:
    State* state_;
    Profile* profile_;
    ev_tstamp first_syn_timestamp_;

    ConnectionState connection_state_;
    ConnectionKey key_;
    // Unique id for this connection (based on timestamp).
    std::string id_;
    // The two component flows.
    TcpFlow client_;
    TcpFlow server_;
    // Close the connection if it is idle for too long.
    Timer idle_timer_;

    // Timed network events for this connection (as per configuration).
    std::vector<Timer*> event_timers_;
};

#endif // CONNECTION_H
