#include "speculor/storage/persistent_store.hpp"
#include <rocksdb/db.h>
#include <rocksdb/options.h>
#include <iostream>

namespace speculor {

class RocksDbStore::Impl {
public:
    rocksdb::DB* db = nullptr;
    rocksdb::Options options;
    std::string path;
};

RocksDbStore::RocksDbStore() : pimpl(std::make_unique<Impl>()) {
    pimpl->options.create_if_missing = true;
}

RocksDbStore::~RocksDbStore() {
    close();
}

bool RocksDbStore::open(const std::string& path) {
    pimpl->path = path;
    rocksdb::Status s = rocksdb::DB::Open(pimpl->options, path, &pimpl->db);
    if (!s.ok()) {
        std::cerr << "Failed to open RocksDB at " << path << ": " << s.ToString() << std::endl;
        return false;
    }
    return true;
}

void RocksDbStore::close() {
    if (pimpl->db) {
        delete pimpl->db;
        pimpl->db = nullptr;
    }
}

bool RocksDbStore::is_open() const {
    return pimpl->db != nullptr;
}

bool RocksDbStore::put(const std::string& key, const std::vector<uint8_t>& value) {
    if (!pimpl->db) return false;
    rocksdb::Status s = pimpl->db->Put(rocksdb::WriteOptions(), key, rocksdb::Slice(value.data(), value.size()));
    return s.ok();
}

std::vector<uint8_t> RocksDbStore::get(const std::string& key) {
    std::vector<uint8_t> result;
    if (!pimpl->db) return result;
    std::string value;
    rocksdb::Status s = pimpl->db->Get(rocksdb::ReadOptions(), key, &value);
    if (s.ok()) {
        result.assign(value.begin(), value.end());
    }
    return result;
}

bool RocksDbStore::remove(const std::string& key) {
    if (!pimpl->db) return false;
    rocksdb::Status s = pimpl->db->Delete(rocksdb::WriteOptions(), key);
    return s.ok();
}

bool RocksDbStore::exists(const std::string& key) {
    if (!pimpl->db) return false;
    std::string value;
    rocksdb::Status s = pimpl->db->Get(rocksdb::ReadOptions(), key, &value);
    return s.IsNotFound() ? false : true;
}

std::vector<std::string> RocksDbStore::keys() {
    std::vector<std::string> result;
    if (!pimpl->db) return result;
    rocksdb::Iterator* it = pimpl->db->NewIterator(rocksdb::ReadOptions());
    for (it->SeekToFirst(); it->Valid(); it->Next()) {
        result.push_back(it->key().ToString());
    }
    delete it;
    return result;
}

void RocksDbStore::flush() {
    if (pimpl->db) {
        pimpl->db->Flush(rocksdb::FlushOptions());
    }
}

} // namespace speculor