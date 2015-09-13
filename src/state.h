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

// All application state.
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

// A C++ wrapper around libev timers.
struct Timer {
    typedef std::function<void(Timer*)> Callback;
    typedef libev_watcher<ev_timer, Timer*> Watcher;

    // Make a timer bound to the event loop of the specified application
    // instance. Call the callback() every time the timer expires.
    Timer(State* state, const Callback& callback)
        : state_(state),
          callback_(callback) {
        watcher_.payload = this;
        ev_timer_init(&watcher_.watcher, callback_tramp, 0, 0);
    }

    ~Timer() {
        stop();
    }

    // Schedule the timer to be triggered at this many seconds from now
    // (whether it's currently scheduled or not).
    void reschedule(ev_tstamp delay) {
        stop();
        ev_timer_set(&watcher_.watcher, delay, 0);
        ev_timer_start(state_->loop, &watcher_.watcher);
    }

    // Cancel the timer.
    void stop() {
        ev_timer_stop(state_->loop, &watcher_.watcher);
    }

    static void callback_tramp(struct ev_loop* loop, ev_timer* w, int revents) {
        auto watcher = reinterpret_cast<Watcher*>(w);
        auto timer = watcher->payload;
        timer->callback_(timer);
    }

private:
    State* state_;
    Watcher watcher_;
    Callback callback_;
};

// Wrapper around libev signal handlers.
struct SignalHandler {
    typedef std::function<void()> Callback;
    typedef libev_watcher<ev_signal, SignalHandler*> Watcher;

    SignalHandler(State* state, const Callback& callback, int signum)
        : state_(state),
          callback_(callback) {
        watcher_.payload = this;
        ev_signal_init(&watcher_.watcher, callback_tramp, signum);
        ev_signal_start(state->loop, &watcher_.watcher);
    }

    ~SignalHandler() {
    }

    static void callback_tramp(struct ev_loop* loop,
                               ev_signal* w,
                               int revents) {
        auto watcher = reinterpret_cast<Watcher*>(w);
        auto handler = watcher->payload;
        handler->callback_();
    }

private:
    State* state_;
    Watcher watcher_;
    Callback callback_;
};

#endif
