/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- */
/*
 * Copyright 2014 Teclo Networks AG
 */

#include "connection.h"

#include <functional>
#include <google/protobuf/text_format.h>

#include "io-backend.h"
#include "log.h"
#include "sequtil.h"
#include "strutil.h"

TcpFlow::TcpFlow(State* state, Profile* profile,
                 IoInterface* iface, const std::string& id)
    : state_(state),
      iface_(iface),
      dumper_(stringprintf("%s-%s-%s.cap",
                           iface->name().c_str(),
                           profile->profile_config().id().c_str(),
                           id.c_str())),
      transmit_timer_(state, [this] (Timer* t) { transmit_timeout(); }),
      delay_s_(0),
      received_rst_(false),
      received_fin_(false),
      throttler_(state) {
    if (profile->profile_config().dump_pcap()) {
        if (!dumper_.open()) {
            fail("Failed to open trace file.");
        }
    }
}

TcpFlow::~TcpFlow() {
    for (auto packet : packets_) {
        delete packet.second;
    }
}

void TcpFlow::record_packet_rx(Packet* p) {
    if (p->tcp().fin()) {
        received_fin_ = true;
    }
    if (p->tcp().rst()) {
        received_rst_ = true;
    }

    if (p->tcp().syn()) {
        other_->snd_una_ = p->tcp().seq();
        snd_nxt_ = p->tcp().seq();
    }

    if (p->tcp().ack()) {
        snd_una_ = seq_max(snd_una_, p->tcp().ack_seq());
    }
    if (p->tcp().has_end_seq()) {
        snd_nxt_ = seq_max(snd_nxt_, p->tcp().end_seq());
    }

    dumper_.dump_packet(p);
}

void TcpFlow::queue_packet_tx(Packet* p) {
    auto callback = [&] (Packet copy) {
        packets_.push_back(std::make_pair(ev_now(state_->loop) + delay_s_,
                                          new Packet(copy)));
        transmit();
    };

    throttler_.insert(p->length_, std::bind(callback, *p));
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
        dumper_.dump_packet(p);

        packets_.pop_front();
        delete p;
    }

    reschedule_transmit_timer();
}

void TcpFlow::transmit_timeout() {
    transmit();
}

bool TcpFlow::is_valid_synack(Packet* p) {
    // FIXME: Stub
    if (p->tcp().syn() && p->tcp().ack()) {
        return true;
    }

    return false;
}

bool TcpFlow::is_valid_3whs_ack(Packet* p) {
    // FIXME: Stub
    if (!p->tcp().syn() && p->tcp().ack()) {
        return true;
    }

    return false;
}

bool TcpFlow::is_identical_syn(Packet* p) {
    // FIXME: Stub
    if (p->tcp().syn() && !p->tcp().ack()) {
        return true;
    }

    return false;
}

bool TcpFlow::is_identical_synack(Packet* p) {
    // FIXME: Stub
    if (p->tcp().syn() && p->tcp().ack()) {
        return true;
    }

    return false;
}

bool TcpFlow::should_close() {
    if (received_rst_ ||
        (received_fin_ && other_->received_fin_)) {
        return true;
    }

    return false;
}

bool TcpFlow::can_close() {
    if (received_rst_ ||
        (packets_.empty() && !throttler_.has_queued_data())) {
        return true;
    }

    return false;
}

Connection::Connection(Profile* profile, Packet* p, State* state)
    : state_(state),
      profile_(profile),
      first_syn_timestamp_(ev_now(state_->loop)),
      connection_state_(STATE_SYN),
      id_(stringprintf("%.9lf", ev_time())),
      client_(state, profile, p->from_iface_, id_),
      server_(state, profile, p->from_iface_->other(), id_),
      idle_timer_(state, ([this] (Timer* t) { close(); })) {
    info("New connection %p using profile %s\n", this,
         profile->profile_config().id().c_str());

    client_.set_other(&server_);
    server_.set_other(&client_);

    if (profile->profile_config().has_downlink()) {
        client_.throttler()->enable(profile->profile_config().downlink());
    }

    if (profile->profile_config().has_uplink()) {
        server_.throttler()->enable(profile->profile_config().uplink());
    }

    client_.record_packet_rx(p);
    server_.queue_packet_tx(p);

    idle_timer_.reschedule(120);

    for (auto event : profile->profile_config().timed_event()) {
        auto apply = [this, event] (Timer* t) {
            apply_timed_effect(t, event);
        };
        auto revert = [this, event] (Timer* t) {
            revert_timed_effect(t, event);
        };

        event_timers_.push_back(new Timer(state, apply));
        event_timers_.back()->reschedule(event.trigger_time());

        if (event.has_duration()) {
            event_timers_.push_back(new Timer(state, revert));
            event_timers_.back()->reschedule(event.trigger_time() +
                                             event.duration());
        }
    }
}

Connection::~Connection() {
    for (auto event_timer : event_timers_) {
        delete event_timer;
    }
}

void Connection::apply_timed_effect(Timer* timer, const TimedEvent& event) {
    if (event.has_extra_rtt()) {
        info("Applying extra delay of %lf", event.extra_rtt());
        client_.set_delay(client_.delay() + event.extra_rtt());
    }

    client_.throttler()->apply(event.downlink());
    server_.throttler()->apply(event.uplink());

    if (event.has_repeat_interval()) {
        timer->reschedule(event.repeat_interval());
    }
}

void Connection::revert_timed_effect(Timer* timer, const TimedEvent& event) {
    if (event.has_extra_rtt()) {
        info("Reverting extra delay of %lf", event.extra_rtt());
        client_.set_delay(client_.delay() - event.extra_rtt());
    }

    client_.throttler()->revert(event.downlink());
    server_.throttler()->revert(event.uplink());

    if (event.has_repeat_interval()) {
        timer->reschedule(event.repeat_interval());
    }
}

void Connection::receive(Packet* p) {
    TcpFlow* source_flow = packet_source_flow(p);
    TcpFlow* target_flow = source_flow->other();

    bool from_client = source_flow == &client_;

    // {
    //     std::string text;
    //     google::protobuf::TextFormat::PrintToString(*p, &text);
    //     info("rx: %s\n", text.c_str());
    // }

    source_flow->record_packet_rx(p);

    if (source_flow->should_close()) {
        connection_state_ = STATE_CLOSING;
    }

    switch (connection_state_) {
    case STATE_SYN:
        if (!from_client && client_.is_valid_synack(p)) {
            connection_state_ = STATE_SYN_ACK;
            double server_side_rtt = ev_now(state_->loop) - first_syn_timestamp_;
            double target_rtt = profile_->profile_config().target_rtt();
            if (target_rtt && target_rtt > server_side_rtt) {
                double delay_s = target_rtt - server_side_rtt;
                client_.set_delay(delay_s);
            }
        } else if (from_client && source_flow->is_identical_syn(p)) {
            // Retransmit. Do nothing.
        } else {
            // Getting packets that don't make sense for this handshake.
            // Give up on the connection.
            warn("Handshake confusion, expected SYN-ACK from server. "
                 "Bailing out.\n");
            std::string text;
            google::protobuf::TextFormat::PrintToString(*p, &text);
            warn("packet: %s\n", text.c_str());
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
        if (source_flow->can_close() &&
            target_flow->can_close()) {
            close();
            return;
        }
        break;
    }

    target_flow->queue_packet_tx(p);
    idle_timer_.reschedule(120);

    return;

fail:
    close();
    return;
}

void Connection::close() {
    info("Closing connection %p\n", this);
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
    if (p->has_ipv4()) {
        connection = new ConnectionIpv4(profile, p, state);
    } else if (p->has_ipv6()) {
        connection = new ConnectionIpv6(profile, p, state);
    } else {
        fail("TCP without IPv4 or IPv6?");
    }

    state->connections->add_connection_for_packet(connection, p);

    return connection;
}
