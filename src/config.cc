/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- */
/*
 * Copyright 2014 Teclo Networks AG
 */

#include "config.h"

#include <fcntl.h>
#include <google/protobuf/text_format.h>
#include <google/protobuf/io/zero_copy_stream_impl.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "log.h"

Profile::Profile(const FlowDisruptorProfile& profile) {
    update(profile);
}

void Profile::update(const FlowDisruptorProfile& profile) {
    profile_.CopyFrom(profile);

    filter_.reset(new PacketFilter(profile_.filter()));
    if (!filter_->is_valid()) {
        warn("Invalid filter '%s' for profile '%s'",
             profile_.filter().c_str(),
             profile_.id().c_str());
    }
}

bool Config::update(const std::string& filename) {
    int fd = open(filename.c_str(), O_RDONLY);
    if (fd < 0) {
        warn_with_errno("couldn't open '%s'", filename.c_str());
        return false;
    }

    google::protobuf::io::FileInputStream stream(fd);

    FlowDisruptorConfig config;
    if (!google::protobuf::TextFormat::Parse(&stream, &config)) {
        warn("Error parsing configuration");
        return false;
    }

    config_.CopyFrom(config);
    update_profiles();

    return true;
}

void Config::update_profiles() {
    profiles_by_priority_.clear();

    for (auto profile_config : config_.profile()) {
        Profile* profile = profiles_by_id_[profile_config.id()];

        if (profile == NULL) {
            profile = profiles_by_id_[profile_config.id()] =
                new Profile(profile_config);
        } else {
            profile->update(profile_config);
        }

        profiles_by_priority_.push_back(profile);
    }
}
