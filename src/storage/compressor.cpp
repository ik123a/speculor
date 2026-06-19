#include "speculor/storage/compressor.hpp"
#include <zstd.h>
#include <lz4.h>

namespace speculor {

Compressor::Compressor(Algorithm algo) : algorithm_(algo), compression_level_(3) {}

Compressor::~Compressor() {}

std::vector<uint8_t> Compressor::compress(const std::vector<uint8_t>& data) {
    if (algorithm_ == Algorithm::None) {
        return data;
    }
    if (algorithm_ == Algorithm::Zstd) {
        size_t bound = ZSTD_compressBound(data.size());
        std::vector<uint8_t> out(bound);
        size_t compressedSize = ZSTD_compress(out.data(), bound, data.data(), data.size(), compression_level_);
        out.resize(compressedSize);
        return out;
    }
    if (algorithm_ == Algorithm::Lz4) {
        int maxDstSize = LZ4_compressBound(data.size());
        std::vector<uint8_t> out(maxDstSize);
        int compressedSize = LZ4_compress_default(reinterpret_cast<const char*>(data.data()), reinterpret_cast<char*>(out.data()), data.size(), maxDstSize);
        if (compressedSize <= 0) {
            return {};
        }
        out.resize(compressedSize);
        return out;
    }
    return data;
}

std::vector<uint8_t> Compressor::decompress(const std::vector<uint8_t>& data) {
    if (algorithm_ == Algorithm::None) {
        return data;
    }
    if (algorithm_ == Algorithm::Zstd) {
        unsigned long long const originalSize = ZSTD_getFrameContentSize(data.data(), data.size());
        if (originalSize == ZSTD_CONTENTSIZE_ERROR || originalSize == ZSTD_CONTENTSIZE_UNKNOWN) {
            return {};
        }
        std::vector<uint8_t> out(originalSize);
        size_t const dSize = ZSTD_decompress(out.data(), originalSize, data.data(), data.size());
        if (ZSTD_isError(dSize)) {
            return {};
        }
        return out;
    }
    if (algorithm_ == Algorithm::Lz4) {
        // For simplicity we cannot know original size; user must provide it via context
        return {};
    }
    return data;
}

std::string Compressor::algorithm_name(Algorithm algo) {
    switch (algo) {
        case Algorithm::None: return "none";
        case Algorithm::Zstd: return "zstd";
        case Algorithm::Lz4: return "lz4";
    }
    return "unknown";
}

} // namespace speculor