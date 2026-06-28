#include <gtest/gtest.h>
#include "speculor/detectors/cms_detector.hpp"
#include <vector>
#include <cstdint>

TEST(CountMinSketchTest, BasicOperations) {
    speculor::CountMinSketch sketch(512, 4);
    
    std::vector<uint8_t> item1 = {1, 2, 3};
    std::vector<uint8_t> item2 = {4, 5, 6};
    
    EXPECT_EQ(sketch.estimate(item1), 0);
    EXPECT_EQ(sketch.estimate(item2), 0);
    
    sketch.increment(item1);
    EXPECT_GE(sketch.estimate(item1), 1);
    EXPECT_EQ(sketch.estimate(item2), 0);
    
    sketch.increment(item1);
    sketch.increment(item2);
    EXPECT_GE(sketch.estimate(item1), 2);
    EXPECT_GE(sketch.estimate(item2), 1);
}

TEST(CountMinSketchTest, ClearAndDecay) {
    speculor::CountMinSketch sketch(512, 4);
    std::vector<uint8_t> item = {7, 8, 9};
    
    sketch.increment(item);
    EXPECT_GE(sketch.estimate(item), 1);
    
    sketch.decay(0.5);
    EXPECT_LE(sketch.estimate(item), 1); // decayed value will be static_cast<uint32_t>(val * 0.5)
    
    sketch.clear();
    EXPECT_EQ(sketch.estimate(item), 0);
}
