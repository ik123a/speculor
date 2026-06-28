#include <gtest/gtest.h>
#include "speculor/core/engine.hpp"
#include "speculor/core/event.hpp"
#include "speculor/core/config.hpp"
#include <string>
#include <fstream>

TEST(PersistenceIntegrationTest, SnapshotAndRestore) {
    speculor::SpeculorEngine engine;
    speculor::Config config;
    config.enable_persistence = false;
    config.thread_pool_size = 1;
    
    engine.configure(config);
    
    // Ingest events
    engine.process(speculor::Event("disk_io", 45.2));
    engine.process(speculor::Event("disk_io", 50.1));
    
    // Snapshot state
    std::string snapshot_path = "./test_speculor_snapshot.db";
    engine.snapshot(snapshot_path);
    
    // Create new engine and restore
    speculor::SpeculorEngine engine2;
    engine2.configure(config);
    engine2.restore(snapshot_path);
    
    // Process after restore
    auto alert = engine2.process(speculor::Event("disk_io", 48.0));
    EXPECT_FALSE(alert.has_value());
    
    // Clean up
    engine.stop();
    engine2.stop();
}
