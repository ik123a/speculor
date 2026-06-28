cmake_minimum_required(VERSION 3.20)
project(speculor VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Options (Disabled by default as directories are currently empty)
option(SPECULOR_BUILD_TESTS "Build tests" ON)
option(SPECULOR_BUILD_BENCHMARKS "Build benchmarks" OFF)
option(SPECULOR_BUILD_CLI "Build CLI tool" ON)
option(SPECULOR_BUILD_REST "Build REST server" ON)
option(SPECULOR_BUILD_EXAMPLES "Build examples" OFF)
option(SPECULOR_BUILD_PLUGINS "Build plugins" OFF)
option(SPECULOR_ENABLE_COVERAGE "Enable code coverage" OFF)

# Find dependencies via vcpkg
find_package(fmt CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)
find_package(CLI11 CONFIG REQUIRED)
find_package(flatbuffers CONFIG REQUIRED)
find_package(zstd CONFIG REQUIRED)
find_package(lz4 CONFIG REQUIRED)
find_package(RocksDB CONFIG REQUIRED)
find_package(GTest CONFIG REQUIRED)
find_package(benchmark CONFIG REQUIRED)

if(SPECULOR_BUILD_REST)
  find_package(httplib CONFIG REQUIRED)
endif()

# Compiler warnings
if(MSVC)
  add_compile_options(/W4 /WX-)
else()
  add_compile_options(-Wall -Wextra -Wpedantic -Wno-unused-parameter)
endif()

# Include directories
include_directories(${CMAKE_SOURCE_DIR}/include)

# Core library sources
set(SPECULOR_CORE_SOURCES
    src/core/engine.cpp
    src/core/config.cpp
    src/detectors/bloom_detector.cpp
    src/detectors/cms_detector.cpp
    src/detectors/kdtree_detector.cpp
    src/detectors/composite_detector.cpp
    src/adaptive/threshold_engine.cpp
    src/concurrency/ring_buffer.cpp
    src/concurrency/thread_pool.cpp
    src/concurrency/scheduler.cpp
    src/storage/serializer.cpp
    src/storage/compressor.cpp
    src/storage/persistent_store.cpp
    src/plugin/plugin_loader.cpp
)

# Main library
add_library(speculor_core STATIC ${SPECULOR_CORE_SOURCES})
target_link_libraries(speculor_core PUBLIC
    fmt::fmt
    spdlog::spdlog
    flatbuffers::flatbuffers
    zstd::libzstd_static
    lz4::lz4
    RocksDB::rocksdb
)

set_target_properties(speculor_core PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
)

# CLI executable
if(SPECULOR_BUILD_CLI)
    add_executable(speculor_cli src/cli/main.cpp)
    target_link_libraries(speculor_cli PRIVATE speculor_core CLI11::CLI11)
    set_target_properties(speculor_cli PROPERTIES OUTPUT_NAME "speculor")
endif()

# REST server
if(SPECULOR_BUILD_REST)
    add_executable(speculor_rest src/rest/rest_server.cpp)
    target_link_libraries(speculor_rest PRIVATE speculor_core httplib::httplib)
endif()

# Tests
if(SPECULOR_BUILD_TESTS)
    enable_testing()
    add_executable(speculor_tests
        tests/unit/test_bloom.cpp
        tests/unit/test_cms.cpp
        tests/unit/test_kdtree.cpp
        tests/unit/test_engine.cpp
        tests/integration/test_pipeline.cpp
        tests/integration/test_persistence.cpp
    )
    target_link_libraries(speculor_tests PRIVATE speculor_core GTest::gtest_main)
    add_test(NAME speculor_unit_tests COMMAND speculor_tests)
endif()

# Benchmarks
if(SPECULOR_BUILD_BENCHMARKS)
    add_executable(speculor_benchmarks
        benchmarks/bench_ingestion.cpp
        benchmarks/bench_detection.cpp
        benchmarks/bench_adaptive.cpp
    )
    target_link_libraries(speculor_benchmarks PRIVATE speculor_core benchmark::benchmark)
endif()

# Examples
if(SPECULOR_BUILD_EXAMPLES)
    add_executable(simple_detection examples/simple_detection.cpp)
    target_link_libraries(simple_detection PRIVATE speculor_core)

    add_executable(async_processing examples/async_processing.cpp)
    target_link_libraries(async_processing PRIVATE speculor_core)

    add_executable(custom_detector_plugin examples/custom_detector_plugin.cpp)
    target_link_libraries(custom_detector_plugin PRIVATE speculor_core)
endif()

# Plugins
if(SPECULOR_BUILD_PLUGINS)
    # Commented out as directories are currently empty
    # add_subdirectory(plugins/prometheus)
    # add_subdirectory(plugins/datadog)
endif()

# Install targets
install(TARGETS speculor_core
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin
)
install(DIRECTORY include/speculor DESTINATION include)

# CPack packaging
set(CPACK_PACKAGE_NAME "speculor")
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
include(CPack)