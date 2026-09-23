// Regression test: service discovery is typed and bounded, so consumers never
// perform an unchecked void* downcast nor dereference a missing service.
#include "../include/message_queue.hpp"
#include "../include/sensor_types.hpp"
#include "../include/service_registry.hpp"
#include <chrono>
#include <cstdio>
#include <string>

using namespace std::chrono;

static int failures = 0;

static void check(bool condition, const std::string& what) {
    std::printf("%s %s\n", condition ? "ok:  " : "FAIL:", what.c_str());
    if (!condition) {
        ++failures;
    }
}

int main() {
    ServiceRegistry& registry = ServiceRegistry::instance();

    auto missing = registry.discoverService<MessageQueue<SensorData>>("NoSuchService", milliseconds(50));
    check(missing.status == DiscoveryStatus::TimedOut && missing.service == nullptr,
          "discovery of an unregistered service times out and yields nullptr");

    MessageQueue<SensorData> queue;
    registry.registerService("SensorDataService", &queue);
    auto found = registry.discoverService<MessageQueue<SensorData>>("SensorDataService", milliseconds(50));
    check(found && found.service == &queue, "typed discovery returns the registered pointer");

    auto mismatched = registry.discoverService<MessageQueue<int>>("SensorDataService", milliseconds(50));
    check(mismatched.status == DiscoveryStatus::TypeMismatch && mismatched.service == nullptr,
          "discovery with the wrong type is rejected instead of downcast");

    MessageQueue<SensorData>* nullService = nullptr;
    registry.registerService("NullService", nullService);
    auto nullDiscovery = registry.discoverService<MessageQueue<SensorData>>("NullService", milliseconds(50));
    check(nullDiscovery.status == DiscoveryStatus::TimedOut && nullDiscovery.service == nullptr,
          "registering a null pointer is ignored");

    if (failures > 0) {
        std::printf("FAILED (%d failures)\n", failures);
        return 1;
    }
    std::printf("PASSED (0 failures)\n");
    return 0;
}
