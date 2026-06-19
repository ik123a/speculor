#include "speculor/core/engine.hpp"
#include "speculor/core/event.hpp"
#include "speculor/core/config.hpp"
#include <CLI11.hpp>
#include <iostream>
#include <fstream>
#include <nlohmann/json.hpp>

int main(int argc, char** argv) {
    CLI::App app{"Vigil CLI - stream events and get anomaly alerts"};
    std::string config_path;
    std::string input_file;
    bool async = false;
    app.add_option("-c,--config", config_path, "Path to YAML/JSON config file")->required();
    app.add_option("-i,--input", input_file, "Input file (JSON events), defaults to stdin");
    app.add_flag("-a,--async", async, "Process events asynchronously");
    CLI11_PARSE(app, argc, argv);

    speculor::SpeculorEngine engine;
    if (!config_path.empty()) {
        engine.configure(config_path);
    }

    std::istream* in = &std::cin;
    std::ifstream fin;
    if (!input_file.empty()) {
        fin.open(input_file);
        if (!fin.is_open()) {
            std::cerr << "Failed to open input file " << input_file << std::endl;
            return 1;
        }
        in = &fin;
    }

    std::string line;
    while (std::getline(*in, line)) {
        if (line.empty()) continue;
        nlohmann::json j;
        try {
            j = nlohmann::json::parse(line);
        } catch (const std::exception& e) {
            std::cerr << "JSON parse error: " << e.what() << std::endl;
            continue;
        }
        // Expect fields: metric_name, value, optional labels map
        speculor::Event ev;
        ev.metric_name = j.value("metric_name", std::string("unknown"));
        ev.value = j.value("value", 0.0);
        if (j.contains("labels")) {
            for (auto& el : j["labels"].items()) {
                ev.labels[el.key()] = el.value().get<std::string>();
            }
        }
        if (async) {
            engine.process_async(ev);
        } else {
            auto alert_opt = engine.process(ev);
            if (alert_opt) {
                const auto& alert = *alert_opt;
                std::cout << "ALERT: " << alert.metric_name << " value=" << alert.metric_value << " score=" << alert.score << " severity=" << alert.severity << std::endl;
            }
        }
    }
    // Flush any remaining async tasks
    if (async) {
        // Give the scheduler a moment to finish
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    return 0;
}
