// Dependency-free regression: public headers must tolerate repeated inclusion.
#include "speculor/core/event.hpp"
#include "speculor/core/event.hpp"

int main() {
    speculor::Event event("header_check", 1.0);
    return event.metric_name == "header_check" && event.value == 1.0 ? 0 : 1;
}
