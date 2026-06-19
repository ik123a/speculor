#pragma once
#include "speculor/detectors/detector_base.hpp"
#include <string>
#include <memory>
#include <vector>

namespace speculor {

class Config;

class PluginBase {
public:
    virtual ~PluginBase() = default;
    virtual std::string name() const = 0;
    virtual std::string version() const = 0;
    virtual std::unique_ptr<Detector> create_detector(const Config& config) = 0;
    virtual void initialize(const Config& config) = 0;
    virtual void shutdown() = 0;
};

} // namespace speculor
