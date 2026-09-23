// File: tests/test_service_registry.cpp
#include "../include/service_registry.hpp"
#include "../include/message_queue.hpp"
#include "../include/sensor_types.hpp"
#include <cassert>
#include <chrono>
#include <iostream>
#include <thread>

using namespace std::chrono_literals;

static void testDiscoverAfterLateRegistration(){
    MessageQueue<SensorData> queue;
    std::thread producer([&]{
        std::this_thread::sleep_for(50ms);
        ServiceRegistry::instance().registerService<MessageQueue<SensorData>>("LateService", &queue);
    });

    auto* found = ServiceRegistry::instance()
                      .discoverService<MessageQueue<SensorData>>("LateService", 2000ms);
    producer.join();
    assert(found == &queue);
}

static void testMissingServiceTimesOut(){
    const auto start = std::chrono::steady_clock::now();
    auto* found = ServiceRegistry::instance()
                      .discoverService<MessageQueue<SensorData>>("NeverRegistered", 100ms);
    const auto elapsed = std::chrono::steady_clock::now() - start;
    assert(found == nullptr);
    assert(elapsed >= 100ms);
}

static void testTypeMismatchIsRejected(){
    MessageQueue<SensorData> queue;
    ServiceRegistry::instance().registerService<MessageQueue<SensorData>>("TypedService", &queue);
    auto* wrong = ServiceRegistry::instance().discoverService<MessageQueue<int>>("TypedService", 100ms);
    assert(wrong == nullptr);
    auto* right = ServiceRegistry::instance()
                      .discoverService<MessageQueue<SensorData>>("TypedService", 100ms);
    assert(right == &queue);
}

int main(){
    testDiscoverAfterLateRegistration();
    testMissingServiceTimesOut();
    testTypeMismatchIsRejected();
    std::cout << "All service registry tests passed." << std::endl;
    return 0;
}
