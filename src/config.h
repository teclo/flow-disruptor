/* -*- mode: c++; c-basic-offset: 4 indent-tabs-mode: nil -*- */
/*
 * Copyright 2014 Teclo Networks AG
 */

// Parsed / compiled program configuration. See
// proto/FlowDisruptorConfig.proto for the actual configuration variables.

#ifndef _CONFIG_H_
#define _CONFIG_H_

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "bpf.h"
#include "FlowDisruptorConfig.pb.h"
#include "packet.h"

// A compiled traffic profile, matching some traffic as specified by a pcap
// filter and applying the settings as per the FlowDisruptorProfile.
class Profile {
public:
    Profile(const FlowDisruptorProfile& profile);

    // Update the configuration for this profile.
    void update(const FlowDisruptorProfile& profile);

    // The compiled filter for traffic this profile is supposed to match.
    const PacketFilter* filter() { return filter_.get(); }
    // The configuration for this profile.
    const FlowDisruptorProfile& profile_config() const { return profile_; }

private:
    FlowDisruptorProfile profile_;
    std::unique_ptr<PacketFilter> filter_;
};

class Config {
public:
    Config() {
    }

    // Update the configuration from this filename.
    bool update(const std::string& filename);

    // List of profiles, ordered by priority (highest priority first). If
    // traffic matches two profiles, use the first profile in this list.
    const std::vector<Profile*>& profiles_by_priority() {
        return profiles_by_priority_;
    }

private:
    void update_profiles();

    FlowDisruptorConfig config_;
    std::vector<Profile*> profiles_by_priority_;
    std::map<std::string, Profile*> profiles_by_id_;
};

#endif
