// Regression test: the controller SWC must leave its receive loop on SHUTDOWN
// even when the sensor SWC is slower than the controller (empty queue).
// Before the fix, MessageQueue::receive() blocked forever and join() hung.
#include "../include/controller_swc.hpp"
#include "../include/lifecycle.hpp"
#include "../include/message_queue.hpp"
#include "../include/sensor_swc.hpp"
#include "../include/sensor_types.hpp"
#include <atomic>
#include <chrono>
#include <csignal>
#include <cstdio>
#include <future>
#include <thread>

using namespace std::chrono;

int main() {
    MessageQueue<SensorData> queue;
    // Sensor period (3000 ms) >> controller period (20 ms): controller starves.
    std::thread sensorThread(sensorApp, std::ref(queue), 20.0f, 1.0f, 3000);
    std::thread controllerThread(controllerApp, 25.0f, 20);

    std::this_thread::sleep_for(milliseconds(200));
    // Same path the SIGINT handler takes.
    handleSignal(SIGINT);

    auto joined = std::async(std::launch::async, [&] {
        sensorThread.join();
        queue.close();
        controllerThread.join();
    });
    bool finished = joined.wait_for(seconds(10)) == std::future_status::ready;
    std::printf("%s both SWC threads joined within 10 s after SHUTDOWN\n",
                finished ? "ok:  " : "FAIL:");
    if (!finished) {
        std::printf("FAILED (1 failures)\n");
        std::_Exit(1);   // threads are stuck; cannot join, exit hard
    }
    std::printf("PASSED (0 failures)\n");
    return 0;
}
