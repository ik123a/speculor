#pragma once
#include "speculor/plugin/plugin_base.hpp"
#include <string>
#include <vector>
#include <memory>
#include <unordered_map>

namespace speculor {

class PluginLoader {
public:
    PluginLoader();
    ~PluginLoader();

    // Load a plugin from a shared library path
    bool load_plugin(const std::string& path);
    bool unload_plugin(const std::string& name);

    // Get all loaded plugins
    std::vector<std::shared_ptr<PluginBase>> get_plugins() const;
    std::shared_ptr<PluginBase> get_plugin(const std::string& name) const;

    // Check if a plugin is loaded
    bool is_loaded(const std::string& name) const;

    // Get list of loaded plugin names
    std::vector<std::string> loaded_plugins() const;

    void unload_all();

private:
    struct PluginHandle {
        void* handle{nullptr};
        std::shared_ptr<PluginBase> instance;
        std::string path;
    };

    std::unordered_map<std::string, PluginHandle> plugins_;
    mutable std::mutex mutex_;

    bool validate_plugin(const std::shared_ptr<PluginBase>& plugin);
};

} // namespace speculor