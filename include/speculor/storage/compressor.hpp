#pragma once
#include <vector>
#include <string>
#include <memory>

namespace speculor {

class Compressor {
public:
    enum class Algorithm {
        None,
        Zstd,
        Lz4
    };

    explicit Compressor(Algorithm algo = Algorithm::Zstd);
    ~Compressor();

    std::vector<uint8_t> compress(const std::vector<uint8_t>& data);
    std::vector<uint8_t> decompress(const std::vector<uint8_t>& data);

    static std::string algorithm_name(Algorithm algo);

private:
    Algorithm algorithm_{Algorithm::Zstd};
    int compression_level_{3};
};

} // namespace speculor