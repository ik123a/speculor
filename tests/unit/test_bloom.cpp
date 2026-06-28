#include <gtest/gtest.h>
#include "speculor/detectors/bloom_detector.hpp"
#include <vector>
#include <cstdint>

TEST(CountingBloomFilterTest, BasicOperations) {
    speculor::CountingBloomFilter filter(1000, 3);
    
    std::vector<uint8_t> item1 = {1, 2, 3};
    std::vector<uint8_t> item2 = {4, 5, 6};
    
    EXPECT_FALSE(filter.contains(item1));
    EXPECT_FALSE(filter.contains(item2));
    
    filter.add(item1);
    EXPECT_TRUE(filter.contains(item1));
    EXPECT_FALSE(filter.contains(item2));
    
    filter.add(item2);
    EXPECT_TRUE(filter.contains(item1));
    EXPECT_TRUE(filter.contains(item2));
    
    filter.remove(item1);
    EXPECT_FALSE(filter.contains(item1));
    EXPECT_TRUE(filter.contains(item2));
}

TEST(CountingBloomFilterTest, ClearAndDecay) {
    speculor::CountingBloomFilter filter(1000, 3);
    std::vector<uint8_t> item = {7, 8, 9};
    
    filter.add(item);
    EXPECT_TRUE(filter.contains(item));
    
    filter.decay(0.0);
    EXPECT_FALSE(filter.contains(item));
    
    filter.add(item);
    EXPECT_TRUE(filter.contains(item));
    filter.clear();
    EXPECT_FALSE(filter.contains(item));
}
