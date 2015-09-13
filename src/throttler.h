/* -*- mode: c++; c-basic-offset: 4 indent-tabs-mode: nil -*- */
/*
 * Copyright 2014 Teclo Networks AG
 */

#ifndef _THROTTLER_H_
#define _THROTTLER_H_

#include <deque>
#include <map>
#include <functional>

#include "state.h"

// A token bucket based throttler.
class Throttler {
public:
    Throttler(State* state);

    typedef std::function<void()> Callback;

    // Activate the throttler, using these initial properties.
    void enable(const LinkProperties& properties);

    // Change the properties of the throttler / undo the changes.
    void apply(const LinkPropertiesChange& properties);
    void revert(const LinkPropertiesChange& properties);

    // Call this callback as soon as we have at least "cost" units of
    // capacity available. Note, callback might never get called.
    void insert(uint64_t cost, const Callback& callback);

    // True if this throttler has any callbacks we haven't yet called.
    bool has_queued_data() { return !queue_.empty(); }

private:
    // Add more tokens to the bucket.
    void tick();
    // Recompute the per-tick effects after a property change.
    void recompute();
    // Trigger callbacks until the queue is empty or we're out of tokens.
    void transmit();

    // True if the throttler is enabled (false if no bandwidth throttler
    // was specified in config -- in that case just call the callback
    // right away).
    bool enabled_;

    // Number of bytes of data ever put in the queue.
    uint64_t total_bytes_;
    // Current configured bandwidth limit.
    int64_t throttle_kbps_;

    // How many tokens are in the bucket.
    uint64_t capacity_;
    // Maximum tokens in the bucket (if reached, tick() does nothing).
    uint64_t max_capacity_;
    // Number of tokens to add per tick.
    uint64_t capacity_per_tick_;

    Timer tick_timer_;

    std::multimap<uint64_t, VolumeTriggeredEvent> pending_events_;
    std::multimap<uint64_t, VolumeTriggeredEvent> active_events_;

    std::deque<std::pair<uint32_t, Callback>> queue_;

    // Amoutn of data currently in queue.
    uint64_t queued_cost_;
    // Maximum amount of data in queue.
    uint64_t max_queue_;
    // If larger than zero, drop the next callback and decrement this
    // by the cost of the dropped callback.
    int32_t drop_bytes_;
};

#endif // _THROTTLER_H_
