// Regression test: a sensor faster than the controller period must not build an
// unbounded MessageQueue backlog. Before the fix the controller consumed one sample
// per period, so warnings lagged further behind the sensor every cycle.
#include "../include/controller_swc.hpp"
#include "../include/lifecycle.hpp"
#include "../include/message_queue.hpp"
#include "../include/sensor_swc.hpp"
#include "../include/sensor_types.hpp"
#include <chrono>
#include <csignal>
#include <cstdio>
#include <thread>

using namespace std::chrono;

int main() {
    MessageQueue<SensorData> queue;
    // Sensor every 10 ms, controller every 100 ms: ~10 samples arrive per controller cycle.
    std::thread sensorThread(sensorApp, std::ref(queue), 20.0f, 1.0f, 10);
    std::thread controllerThread(controllerApp, 25.0f, 100);

    std::size_t maxBacklog = 0;
    for (int i = 0; i < 150; ++i) {   // ~1.5 s, ~150 samples
        maxBacklog = std::max(maxBacklog, queue.size());
        std::this_thread::sleep_for(milliseconds(10));
    }
    handleSignal(SIGINT);
    sensorThread.join();
    queue.close();
    controllerThread.join();

    // One controller period's worth of samples (+ scheduling slack) is the most that may queue.
    bool ok = maxBacklog <= 30;
    std::printf("%s max queue backlog %zu samples (limit 30)\n", ok ? "ok:  " : "FAIL:", maxBacklog);
    std::printf("%s (%d failures)\n", ok ? "PASSED" : "FAILED", ok ? 0 : 1);
    return ok ? 0 : 1;
}
