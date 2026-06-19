#include "speculor/core/engine.hpp"
#include "speculor/core/event.hpp"
#include "speculor/core/alert.hpp"
#include <httplib.h>
#include <nlohmann/json.hpp>
#include <thread>
#include <chrono>

int main(int argc, char** argv) {
    // Simple command line parsing (port optional)
    int port = 8080;
    if (argc > 1) {
        port = std::stoi(argv[1]);
    }

    speculor::SpeculorEngine engine;
    // Load default config (if any) – user can call configure later via API
    // For this demo, we keep defaults.

    httplib::Server svr;

    // POST /event – JSON body with metric_name, value, optional labels
    svr.Post("/event", [&](const httplib::Request& req, httplib::Response& res) {
        try {
            auto j = nlohmann::json::parse(req.body);
            speculor::Event ev;
            ev.metric_name = j.value("metric_name", std::string("unknown"));
            ev.value = j.value("value", 0.0);
            if (j.contains("labels")) {
                for (auto& el : j["labels"].items()) {
                    ev.labels[el.key()] = el.value().get<std::string>();
                }
            }
            auto alert_opt = engine.process(ev);
            if (alert_opt) {
                const auto& a = *alert_opt;
                nlohmann::json out = {
                    {"metric_name", a.metric_name},
                    {"value", a.metric_value},
                    {"score", a.score},
                    {"severity", a.severity},
                    {"description", a.description}
                };
                res.set_content(out.dump(), "application/json");
            } else {
                res.set_content("{}", "application/json");
            }
        } catch (const std::exception& e) {
            res.status = 400;
            res.set_content(std::string("Error: ") + e.what(), "text/plain");
        }
    });

    // GET /alerts – return recent alerts as JSON array
    svr.Get("/alerts", [&](const httplib::Request&, httplib::Response& res) {
        auto alerts = engine.get_alerts();
        nlohmann::json arr = nlohmann::json::array();
        for (const auto& a : alerts) {
            arr.push_back({
                {"metric_name", a.metric_name},
                {"value", a.metric_value},
                {"score", a.score},
                {"severity", a.severity},
                {"description", a.description}
            });
        }
        res.set_content(arr.dump(), "application/json");
    });

    std::cout << "Vigil REST server listening on port " << port << "..." << std::endl;
    svr.listen("0.0.0.0", port);
    return 0;
}
