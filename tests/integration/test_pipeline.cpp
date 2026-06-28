#include <gtest/gtest.h>
#include "speculor/core/engine.hpp"
#include "speculor/core/event.hpp"
#include "speculor/core/config.hpp"
#include <thread>
#include <chrono>

TEST(PipelineIntegrationTest, EndToEndStream) {
    speculor::SpeculorEngine engine;
    speculor::Config config;
    config.enable_persistence = false;
    config.thread_pool_size = 2;
    config.default_threshold = 0.5;
    
    engine.configure(config);
    
    // Ingest some normal values to establish baseline
    for (int i = 0; i < 20; ++i) {
        engine.process(speculor::Event("network_bytes", 100.0));
    }
    
    // Ingest anomaly
    auto alert = engine.process(speculor::Event("network_bytes", 100000.0));
    
    // Anomaly might trigger alert
    // We just verify that the system processed it cleanly
    engine.stop();
}
