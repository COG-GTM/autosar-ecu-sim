// Regression test: service discovery must be bounded, so a never-registered
// (or misnamed) service can no longer park the calling thread forever.
#include "../include/controller_swc.hpp"
#include "../include/lifecycle.hpp"
#include "../include/message_queue.hpp"
#include "../include/sensor_swc.hpp"
#include "../include/sensor_types.hpp"
#include "../include/service_registry.hpp"
#include <chrono>
#include <csignal>
#include <cstdio>
#include <future>
#include <thread>

using namespace std::chrono;

static int failures = 0;

static void check(bool condition, const char* what) {
    std::printf("%s %s\n", condition ? "ok:  " : "FAIL:", what);
    if (!condition) {
        ++failures;
    }
}

int main() {
    auto& registry = ServiceRegistry::instance();

    check(registry.findService("MissingService") == nullptr,
          "findService returns nullptr for an unknown service");

    auto start = steady_clock::now();
    void* missing = registry.discoverServiceFor("MissingService", milliseconds(50));
    auto waited = duration_cast<milliseconds>(steady_clock::now() - start);
    check(missing == nullptr && waited >= milliseconds(50) && waited < milliseconds(2000),
          "discoverServiceFor times out instead of blocking forever");

    int service = 42;
    std::thread producer([&] {
        std::this_thread::sleep_for(milliseconds(50));
        registry.registerService("LateService", &service);
    });
    int* found = registry.discoverServiceAs<int>("LateService", milliseconds(5000));
    producer.join();
    check(found == &service, "discoverServiceAs wakes on a late registration");

    // Controller thread must not hang when the sensor never registers its queue
    // and SHUTDOWN arrives (same path the SIGINT handler takes).
    controllerState = AppState::INIT;
    std::thread controllerThread(controllerApp, 25.0f, 20);
    std::this_thread::sleep_for(milliseconds(100));
    handleSignal(SIGINT);
    auto joined = std::async(std::launch::async, [&] { controllerThread.join(); });
    bool finished = joined.wait_for(seconds(10)) == std::future_status::ready;
    check(finished, "controller leaves discovery on SHUTDOWN when no service is registered");
    if (!finished) {
        std::printf("FAILED (%d failures)\n", failures);
        std::_Exit(1);   // controller is stuck in discovery; cannot join
    }

    // SHUTDOWN requested before the SWC threads start must not be erased by startup.
    sensorState = AppState::INIT;
    controllerState = AppState::INIT;
    handleSignal(SIGINT);
    MessageQueue<SensorData> queue;
    std::thread earlySensor(sensorApp, std::ref(queue), 20.0f, 1.0f, 3000);
    std::thread earlyController(controllerApp, 25.0f, 3000);
    auto earlyJoined = std::async(std::launch::async, [&] {
        earlySensor.join();
        queue.close();
        earlyController.join();
    });
    bool earlyFinished = earlyJoined.wait_for(seconds(10)) == std::future_status::ready;
    check(earlyFinished, "SWCs started after SHUTDOWN exit immediately");
    if (!earlyFinished) {
        std::printf("FAILED (%d failures)\n", failures);
        std::_Exit(1);   // threads latched RUNNING over SHUTDOWN; cannot join
    }

    if (failures > 0) {
        std::printf("FAILED (%d failures)\n", failures);
        return 1;
    }
    std::printf("PASSED (0 failures)\n");
    return 0;
}
