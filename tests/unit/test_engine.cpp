#include <gtest/gtest.h>
#include "speculor/core/engine.hpp"
#include "speculor/core/event.hpp"
#include "speculor/core/config.hpp"

TEST(SpeculorEngineTest, BasicInitialization) {
    speculor::SpeculorEngine engine;
    EXPECT_FALSE(engine.is_running());
    
    speculor::Config config;
    config.enable_persistence = false;
    config.thread_pool_size = 2;
    
    // Test process_async starts background threads if running
    // Since process() works synchronously, we can test it directly
    speculor::Event event("cpu_usage", 0.45);
    auto alert = engine.process(event);
    
    // First event should normally not trigger an anomaly unless threshold is very low
    EXPECT_FALSE(alert.has_value());
}

TEST(SpeculorEngineTest, ConfigureAndRun) {
    speculor::SpeculorEngine engine;
    speculor::Config config;
    config.enable_persistence = false;
    config.thread_pool_size = 1;
    
    engine.configure(config);
    
    speculor::Event normal_event("api_latency", 50.0);
    engine.process(normal_event);
    
    // Cleanup
    engine.stop();
}
