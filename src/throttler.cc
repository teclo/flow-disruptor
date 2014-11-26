/* -*- mode: c++; c-basic-offset: 4 indent-tabs-mode: nil -*- */
/*
 * Copyright 2014 Teclo Networks AG
 */

#include "throttler.h"

#include "log.h"

Throttler::Throttler(State* state)
    : enabled_(false),
      total_bytes_(0),
      tick_timer_(state, [this] (Timer*) { tick(); }),
      queued_cost_(0),
      max_queue_(0),
      drop_bytes_(0) {
}

void Throttler::enable(const LinkProperties& properties) {
    if (properties.has_throughput_kbps()) {
        enabled_ = true;
        throttle_kbps_ = properties.throughput_kbps();
        if (properties.has_max_queue_bytes()) {
            max_queue_ = properties.max_queue_bytes();
        }
    }

    for (auto event : properties.volume_event()) {
        if (event.has_trigger_at_bytes()) {
            pending_events_.insert(std::make_pair(event.trigger_at_bytes(),
                                                 event));
        }
    }

    recompute();
    tick_timer_.reschedule(0.001);
}

void Throttler::insert(uint64_t cost, const Throttler::Callback& callback) {
    for (auto it = pending_events_.begin();
         it != pending_events_.end();
         it = pending_events_.begin()) {
        auto bytes = it->first;
        if (bytes > total_bytes_) {
            break;
        }

        auto event = it->second;
        pending_events_.erase(it);
        apply(event.effect());

        if (event.has_active_for_bytes()) {
            uint64_t stop_at_bytes = event.active_for_bytes() +
                bytes;
            active_events_.insert(std::make_pair(stop_at_bytes,
                                                 event));
        } else if (event.has_repeat_after_bytes()) {
            uint64_t retrigger_bytes = event.repeat_after_bytes() +
                bytes;
            pending_events_.insert(std::make_pair(retrigger_bytes,
                                                  event));
        }
    }

    for (auto it = active_events_.begin();
         it != active_events_.end();
         it = active_events_.begin()) {
        auto bytes = it->first;
        if (bytes > total_bytes_) {
            break;
        }

        auto event = it->second;
        active_events_.erase(it);
        revert(event.effect());
        if (event.has_repeat_after_bytes()) {
            uint64_t retrigger_bytes = event.repeat_after_bytes() +
                bytes;
            pending_events_.insert(std::make_pair(retrigger_bytes,
                                                  event));
        }
    }

    total_bytes_ += cost;

    if (drop_bytes_) {
        drop_bytes_ -= cost;
        if (drop_bytes_ < 0) {
            drop_bytes_ = 0;
        }
        return;
    }

    if (!enabled_) {
        callback();
    } else if (max_queue_ && queued_cost_ > max_queue_) {
        // Queue full, drop the packet.
    } else {
        queue_.push_back(std::make_pair(cost, callback));
        queued_cost_ += cost;
        transmit();
    }
}

void Throttler::transmit() {
    while (!queue_.empty()) {
        uint64_t cost = queue_.front().first;
        if (capacity_ < cost) {
            break;
        }
        capacity_ -= cost;
        queued_cost_ -= cost;
        queue_.front().second();
        queue_.pop_front();
    }
}

void Throttler::tick() {
    capacity_ = std::min(max_capacity_,
                         capacity_ + capacity_per_tick_);
    transmit();
    tick_timer_.reschedule(0.001);
}

void Throttler::recompute() {
    uint64_t actual_throttle = std::max(throttle_kbps_, INT64_C(0));

    // 0.1 seconds of capacity.
    max_capacity_ = actual_throttle * 1000 / 8.0 * 0.1;
    capacity_ = std::min(max_capacity_, capacity_);
    // Ticks every 0.001 seconds.
    capacity_per_tick_ = max_capacity_ * 0.01;
}

void Throttler::apply(const LinkPropertiesChange& properties) {
    int32_t delta = properties.throughput_kbps_change();

    if (delta && enabled_) {
        throttle_kbps_ += delta;
        info("Applying throughput change of %d kbps (now %ld kbps)",
             delta,
             throttle_kbps_);
        recompute();
    }

    if (properties.has_drop_bytes()) {
        drop_bytes_ += properties.drop_bytes();
        info("Dropping next %d bytes", drop_bytes_);
    }
}

void Throttler::revert(const LinkPropertiesChange& properties) {
    int32_t delta = properties.throughput_kbps_change();

    if (delta && enabled_) {
        throttle_kbps_ -= delta;
        info("Reverting throughput change of %d (now %ld)",
             delta,
             throttle_kbps_);
        recompute();
    }
}

