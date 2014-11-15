/* -*- mode: c++; c-basic-offset: 4 indent-tabs-mode: nil -*- */
/*
 * Copyright 2014 Teclo Networks AG
 */

#ifndef _STATE_H_
#define _STATE_H_

#include <ev.h>
#include <functional>

#include "config.h"
#include "connection-table.h"

struct State {
    State() :
        connections(ConnectionTable::make()),
        loop(EV_DEFAULT) {
    }

    Config config;
    ConnectionTable* connections;
    struct ev_loop *loop;
};

template<class WatcherType, class Payload>
struct libev_watcher {
    WatcherType watcher;
    Payload payload;
};

template<class T>
struct Timer {
    typedef std::function<void(T*)> Method;
    typedef std::function<void()> Callback;
    typedef libev_watcher<ev_timer, Callback> watcher;

    Timer(State* state, T* obj, const Method& method)
        : state_(state) {
        timer_.payload = std::bind(method, obj);
        ev_timer_init(&timer_.watcher, callback, 0, 0);
    }

    ~Timer() {
        stop();
    }

    void reschedule(ev_tstamp delay) {
        stop();
        ev_timer_set(&timer_.watcher, delay, 0);
        ev_timer_start(state_->loop, &timer_.watcher);
    }

    void stop() {
        ev_timer_stop(state_->loop, &timer_.watcher);
    }

    static void callback(struct ev_loop* loop, ev_timer* w, int revents) {
        auto timer = reinterpret_cast<watcher*>(w);
        timer->payload();
    }

    State* state_;
    watcher timer_;
};

#endif
