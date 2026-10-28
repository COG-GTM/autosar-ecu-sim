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
#include <fstream>
#include <string>
#include <thread>

using namespace std::chrono;

static int countLines(const char* path, const char* needle) {
    std::ifstream f(path);
    int n = 0;
    for (std::string line; std::getline(f, line);) {
        if (line.find(needle) != std::string::npos) ++n;
    }
    return n;
}

int main() {
    std::remove("sensor_log.txt");
    std::remove("controller_log.txt");
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
    int failures = 0;
    bool backlogOk = maxBacklog <= 30;
    failures += !backlogOk;
    std::printf("%s max queue backlog %zu samples (limit 30)\n", backlogOk ? "ok:  " : "FAIL:", maxBacklog);

    // Samples must be evaluated, not dropped: every published sample above 25 C (start 20, +1 per
    // sample) must raise a warning, except at most one period's worth still queued at shutdown.
    int published = countLines("sensor_log.txt", "Temp =");
    int warnings = countLines("controller_log.txt", "WARNING");
    int expected = published - 6;
    bool warnOk = warnings >= expected - 30;
    failures += !warnOk;
    std::printf("%s %d/%d over-threshold samples raised a warning\n", warnOk ? "ok:  " : "FAIL:", warnings, expected);

    std::printf("%s (%d failures)\n", failures ? "FAILED" : "PASSED", failures);
    return failures ? 1 : 0;
}
