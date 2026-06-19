#include "speculor/plugin/plugin_loader.hpp"
#include <iostream>
#include <mutex>

#ifdef _WIN32
#include <windows.h>
#else
#include <dlfcn.h>
#endif

namespace speculor {

PluginLoader::PluginLoader() {}
PluginLoader::~PluginLoader() { unload_all(); }

bool PluginLoader::load_plugin(const std::string& path) {
#ifdef _WIN32
    HMODULE handle = LoadLibraryA(path.c_str());
    if (!handle) {
        DWORD err = GetLastError();
        std::cerr << "Failed to load plugin '" << path << "': error code " << err << std::endl;
        return false;
    }
    using CreateFn = PluginBase* (*)();
    CreateFn create = (CreateFn)GetProcAddress(handle, "create_plugin");
    if (!create) {
        std::cerr << "Cannot load symbol 'create_plugin' from " << path << ": error code " << GetLastError() << std::endl;
        FreeLibrary(handle);
        return false;
    }
#else
    void* handle = dlopen(path.c_str(), RTLD_NOW);
    if (!handle) {
        std::cerr << "Failed to load plugin '" << path << "': " << dlerror() << std::endl;
        return false;
    }
    dlerror();
    using CreateFn = PluginBase* (*)();
    CreateFn create = (CreateFn)dlsym(handle, "create_plugin");
    const char* dlsym_error = dlerror();
    if (dlsym_error) {
        std::cerr << "Cannot load symbol 'create_plugin' from " << path << ": " << dlsym_error << std::endl;
        dlclose(handle);
        return false;
    }
#endif

    PluginBase* raw_ptr = create();
    if (!raw_ptr) {
        std::cerr << "Plugin factory returned null for " << path << std::endl;
#ifdef _WIN32
        FreeLibrary(handle);
#else
        dlclose(handle);
#endif
        return false;
    }

    std::shared_ptr<PluginBase> plugin_ptr(raw_ptr, [handle](PluginBase* p) {
        p->shutdown();
        delete p;
#ifdef _WIN32
        FreeLibrary(handle);
#else
        dlclose(handle);
#endif
    });

    std::string name = plugin_ptr->name();
    if (!validate_plugin(plugin_ptr)) {
        std::cerr << "Plugin validation failed for " << name << std::endl;
        return false;
    }

    std::lock_guard<std::mutex> lock(mutex_);
    plugins_.emplace(name, PluginHandle{reinterpret_cast<void*>(handle), plugin_ptr, path});
    return true;
}

bool PluginLoader::unload_plugin(const std::string& name) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = plugins_.find(name);
    if (it == plugins_.end()) return false;
    plugins_.erase(it);
    return true;
}

std::vector<std::shared_ptr<PluginBase>> PluginLoader::get_plugins() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<std::shared_ptr<PluginBase>> result;
    for (const auto& kv : plugins_) {
        result.push_back(kv.second.instance);
    }
    return result;
}

std::shared_ptr<PluginBase> PluginLoader::get_plugin(const std::string& name) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = plugins_.find(name);
    if (it != plugins_.end()) {
        return it->second.instance;
    }
    return nullptr;
}

bool PluginLoader::is_loaded(const std::string& name) const {
    std::lock_guard<std::mutex> lock(mutex_);
    return plugins_.count(name) > 0;
}

std::vector<std::string> PluginLoader::loaded_plugins() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<std::string> names;
    for (const auto& kv : plugins_) {
        names.push_back(kv.first);
    }
    return names;
}

void PluginLoader::unload_all() {
    std::lock_guard<std::mutex> lock(mutex_);
    plugins_.clear();
}

bool PluginLoader::validate_plugin(const std::shared_ptr<PluginBase>& plugin) {
    if (plugin->name().empty() || plugin->version().empty()) return false;
    return true;
}

} // namespace speculor