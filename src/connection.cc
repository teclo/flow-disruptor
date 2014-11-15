/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- */
/*
 * Copyright 2014 Teclo Networks AG
 */

#include "connection.h"

#include "io-backend.h"
#include "log.h"

TcpFlow::TcpFlow(State* state, IoInterface* iface)
    : state_(state),
      iface_(iface),
      transmit_timer_(state, this, std::mem_fn(&TcpFlow::transmit_timeout)),
      delay_s_(0) {
}

TcpFlow::~TcpFlow() {
    for (auto packet : packets_) {
        delete packet.second;
    }
}

void TcpFlow::queue_packet(Packet* p) {
    // FIXME: Update sequence number state.
    packets_.push_back(std::make_pair(ev_now(state_->loop) + delay_s_,
                                      p->from_iface_->io()->retain_packet(p)));
    transmit();
    reschedule_transmit_timer();
}

void TcpFlow::reschedule_transmit_timer() {
    transmit_timer_.stop();

    if (!packets_.empty()) {
        ev_tstamp delay = packets_.front().first - ev_now(state_->loop);
        transmit_timer_.reschedule(delay);
    }
}

void TcpFlow::transmit() {
    while (!packets_.empty()) {
        ev_tstamp target = packets_.front().first;
        if (target > ev_now(state_->loop)) {
            break;
        }

        Packet* p = packets_.front().second;

        iface_->io()->inject(p);

        packets_.pop_front();
        delete p;
    }
}

void TcpFlow::transmit_timeout() {
    transmit();
    reschedule_transmit_timer();
}

bool TcpFlow::is_valid_synack(Packet* p) {
    // FIXME: Stub
    if (p->tcph_->syn && p->tcph_->ack) {
        return true;
    }

    return false;
}

bool TcpFlow::is_valid_3whs_ack(Packet* p) {
    // FIXME: Stub
    if (!p->tcph_->syn && p->tcph_->ack) {
        return true;
    }

    return false;
}

bool TcpFlow::is_identical_syn(Packet* p) {
    // FIXME: Stub
    if (p->tcph_->syn && !p->tcph_->ack) {
        return true;
    }

    return false;
}

bool TcpFlow::is_identical_synack(Packet* p) {
    // FIXME: Stub
    if (p->tcph_->syn && p->tcph_->ack) {
        return true;
    }

    return false;
}

Connection::Connection(Profile* profile, Packet* p, State* state)
    : state_(state),
      profile_(profile),
      first_syn_timestamp_(ev_now(state_->loop)),
      connection_state_(STATE_SYN),
      client_(state, p->from_iface_),
      server_(state, p->from_iface_->other()),
      idle_timer_(state, this, std::mem_fn(&Connection::close)) {
    printf("New connection %p\n", this);

    client_.set_other(&server_);
    server_.set_other(&client_);

    server_.queue_packet(p);

    idle_timer_.reschedule(120);
}

void Connection::receive(Packet* p) {
    TcpFlow* source_flow = packet_source_flow(p);
    TcpFlow* target_flow = source_flow->other();

    bool from_client = source_flow == &client_;

    switch (connection_state_) {
    case STATE_SYN:
        if (!from_client && client_.is_valid_synack(p)) {
            connection_state_ = STATE_SYN_ACK;
            double server_side_rtt = ev_now(state_->loop) - first_syn_timestamp_;
            printf("server side rtt: %lf\n", server_side_rtt);
            double target_rtt = profile_->profile_config().target_rtt();
            if (target_rtt && target_rtt > server_side_rtt) {
                double delay_s = target_rtt - server_side_rtt;
                printf("setting delay: %lf\n", delay_s);
                client_.set_delay(delay_s);
            }
        } else if (from_client && source_flow->is_identical_syn(p)) {
            // Retransmit. Do nothing.
        } else {
            // Getting packets that don't make sense for this handshake.
            // Give up on the connection.
            printf("%d %d %d\n",
                   from_client,
                   p->tcph_->syn,
                   p->tcph_->ack);
            warn("Handshake confusion, expected SYN-ACK from server. "
                 "Bailing out.\n");
            goto fail;
        }
        // Note: we don't support SYN-SYN connection opening.

        break;

    case STATE_SYN_ACK:
        if ((!from_client && source_flow->is_identical_synack(p)) ||
            (from_client && source_flow->is_identical_syn(p))) {
            // Retransmit. Do nothing.
        } else if (from_client && source_flow->is_valid_3whs_ack(p)) {
            connection_state_ = STATE_ESTABLISHED;
        } else {
            printf("%d %d %d\n",
                   from_client,
                   p->tcph_->syn,
                   p->tcph_->ack);
            // Getting packets that don't make sense for this handshake.
            // Give up on the connection.
            warn("Handshake confusion, expected ACK from client. "
                 "Bailing out.\n");
            goto fail;
        }

        break;

    case STATE_ESTABLISHED:
        break;

    case STATE_CLOSING:
        break;
    }

    target_flow->queue_packet(p);
    idle_timer_.reschedule(120);
    return;

fail:
    close();
    return;
}

void Connection::close() {
    printf("close %p\n", this);
    state_->connections->remove(this);
    delete this;
}

TcpFlow* Connection::packet_source_flow(Packet* p) {
    if (p->from_iface_ == client_.iface()) {
        return &client_;
    } else {
        return &server_;
    }
}

class ConnectionIpv4 : public Connection {
public:
    ConnectionIpv4(Profile* profile, Packet* p, State* state)
        : Connection(profile, p, state) {
    }

    virtual int addr_bytes() const { return 4; }
};

class ConnectionIpv6 : public Connection {
public:
    ConnectionIpv6(Profile* profile, Packet* p, State* state)
        : Connection(profile, p, state) {
    }

    virtual int addr_bytes() const { return 16; }
};

Connection* Connection::make(Profile* profile, Packet* p, State* state) {
    Connection* connection;
    if (p->iph_) {
        connection = new ConnectionIpv4(profile, p, state);
    } else {
        connection = new ConnectionIpv6(profile, p, state);
    }

    state->connections->add_connection_for_packet(connection, p);

    return connection;
}
