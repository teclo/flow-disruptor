/* -*- mode: c++; c-basic-offset: 4 indent-tabs-mode: nil -*- */
/*
 * Copyright 2014 Teclo Networks AG
 */

#ifndef _CONFIG_H_
#define _CONFIG_H_

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "bpf.h"
#include "FlowDisruptorConfig.pb.h"
#include "packet.h"

class Profile {
public:
    Profile(const FlowDisruptorProfile& profile);

    void update(const FlowDisruptorProfile& profile);

    const PacketFilter* filter() { return filter_.get(); }
    const FlowDisruptorProfile& profile_config() const { return profile_; }

private:
    FlowDisruptorProfile profile_;
    std::unique_ptr<PacketFilter> filter_;
};

class Config {
public:
    Config() {
    }

    bool update(const std::string& filename);

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
