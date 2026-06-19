#pragma once
#include <string>
#include <vector>
#include <memory>
#include <functional>

namespace speculor {

class PersistentStore {
public:
    virtual ~PersistentStore() = default;

    virtual bool open(const std::string& path) = 0;
    virtual void close() = 0;
    virtual bool is_open() const = 0;

    virtual bool put(const std::string& key, const std::vector<uint8_t>& value) = 0;
    virtual std::vector<uint8_t> get(const std::string& key) = 0;
    virtual bool remove(const std::string& key) = 0;
    virtual bool exists(const std::string& key) = 0;

    virtual std::vector<std::string> keys() = 0;
    virtual void flush() = 0;
};

// RocksDB implementation
class RocksDbStore : public PersistentStore {
public:
    RocksDbStore();
    ~RocksDbStore() override;

    bool open(const std::string& path) override;
    void close() override;
    bool is_open() const override;

    bool put(const std::string& key, const std::vector<uint8_t>& value) override;
    std::vector<uint8_t> get(const std::string& key) override;
    bool remove(const std::string& key) override;
    bool exists(const std::string& key) override;

    std::vector<std::string> keys() override;
    void flush() override;

private:
    class Impl;
    std::unique_ptr<Impl> pimpl;
};

} // namespace speculor