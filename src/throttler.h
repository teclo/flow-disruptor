/* -*- mode: c++; c-basic-offset: 4 indent-tabs-mode: nil -*- */
/*
 * Copyright 2014 Teclo Networks AG
 */

#ifndef _THROTTLER_H_
#define _THROTTLER_H_

#include <deque>
#include <functional>

#include "state.h"

class Throttler {
public:
    Throttler(State* state);

    typedef std::function<void()> Callback;

    void enable(const LinkProperties& properties);

    void apply(const LinkProperties& properties);
    void revert(const LinkProperties& properties);

    void insert(uint64_t cost, const Callback& callback);

private:
    void tick();
    void recompute();

    bool enabled_;

    int64_t throttle_kbps_;
    uint64_t capacity_;
    uint64_t max_capacity_;
    uint64_t capacity_per_tick_;

    Timer tick_timer_;

    std::deque<std::pair<uint32_t, Callback>> queue_;
    uint64_t queued_cost_;
    uint64_t max_queue_;

    int32_t drop_bytes_;
};

#endif // _THROTTLER_H_
