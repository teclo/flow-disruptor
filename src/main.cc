/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- */
/*
 * Copyright 2014 Teclo Networks AG
 */

#include <google/gflags.h>
#include <memory>
#include <vector>

#include "connection.h"
#include "connection-table.h"
#include "iface.h"
#include "io-backend.h"
#include "log.h"
#include "state.h"

DEFINE_string(config, "", "");
DEFINE_string(downlink_iface, "", "");
DEFINE_string(uplink_iface, "", "");

State state;

static void handle_tcp(Packet* p) {
    Connection* connection = state.connections->get_connection_for_packet(p);

    if (!connection) {
        if (p->tcph_->syn && !p->tcph_->ack) {
            for (auto profile : state.config.profiles_by_priority()) {
                if (profile->filter()->packet_matches_filter(p)) {
                    connection = Connection::make(profile, p, &state);
                    return;
                }
            }
        }

        p->from_iface_->other()->io()->inject(p);
        return;
    } else {
        connection->receive(p);
    }
}

static void handle_packet(struct ev_loop *loop, ev_io *w, int revents) {
    IoBackend* io = reinterpret_cast<struct libev_watcher<ev_io, IoBackend*>*>(w)->payload;

    Packet p;

    while (revents--) {
        if (!io->receive(&p)) {
            return;
        }

        if (p.tcph_) {
            handle_tcp(&p);
        } else {
            // Forward all other packets straight through
            p.from_iface_->other()->io()->inject(&p);
        }

        io->release_packet(&p);
    }
}

int main(int argc, char** argv) {
    google::SetUsageMessage("flow-disruptor [flags]");
    google::ParseCommandLineFlags(&argc, &argv, true);

    if (!FLAGS_config.empty()) {
        if (!state.config.update(FLAGS_config)) {
            fail("Failed to read config during initial startup, quitting\n");
        }
    }

    IoInterface downlink_iface(FLAGS_downlink_iface, IoInterface::DOWNLINK);
    IoInterface uplink_iface(FLAGS_uplink_iface, IoInterface::UPLINK);

    downlink_iface.set_other(&uplink_iface);
    uplink_iface.set_other(&downlink_iface);

    std::vector<IoBackend*> ios;

    ios.push_back(io_new_pcap(&downlink_iface));
    ios.push_back(io_new_pcap(&uplink_iface));

    std::vector<struct libev_watcher<ev_io, IoBackend*>*> io_watchers;

    for (auto io : ios) {
        if (!io->open()) {
            fail("Could not open interface: '%s'", io->iface()->name().c_str());
        }

        int fd = io->select_fd();
        if (fd >= 0) {
            auto watcher = new libev_watcher<ev_io, IoBackend*>();
            watcher->payload = io;
            io_watchers.push_back(watcher);
            ev_io_init(&watcher->watcher, handle_packet, fd, EV_READ);
            ev_io_start(state.loop, &watcher->watcher);
        }
    }

    ev_run(state.loop, 0);

    for (auto io : ios) {
        io->close();
        delete io;
    }

    return EXIT_SUCCESS;
}
