// Host unit tests for include/message_queue.hpp. Build and run: make test
#include "../include/message_queue.hpp"
#include <chrono>
#include <cstdio>
#include <thread>

using namespace std::chrono;

static int failures = 0;
#define CHECK(cond, msg)                                                       \
    do {                                                                       \
        if (!(cond)) { std::printf("FAIL: %s\n", msg); failures++; }           \
        else { std::printf("ok:   %s\n", msg); }                               \
    } while (0)

int main() {
    // FIFO order and size accounting
    {
        MessageQueue<int> q;
        q.send(1); q.send(2); q.send(3);
        CHECK(q.size() == 3, "size counts pending messages");
        CHECK(q.receive().value() == 1 && q.receive().value() == 2 && q.receive().value() == 3,
              "receive returns messages in FIFO order");
        CHECK(q.size() == 0, "queue empty after draining");
    }

    // receiveFor returns nullopt on timeout instead of blocking forever
    {
        MessageQueue<int> q;
        auto t0 = steady_clock::now();
        auto r = q.receiveFor(milliseconds(50));
        auto elapsed = duration_cast<milliseconds>(steady_clock::now() - t0);
        CHECK(!r.has_value(), "receiveFor on empty queue times out with nullopt");
        CHECK(elapsed >= milliseconds(45) && elapsed < milliseconds(1000),
              "receiveFor honours the timeout (~50 ms)");
    }

    // close() wakes a receiver blocked in receive()
    {
        MessageQueue<int> q;
        std::optional<int> got = 7;
        std::thread receiver([&] { got = q.receive(); });
        std::this_thread::sleep_for(milliseconds(30));
        q.close();
        receiver.join();
        CHECK(!got.has_value(), "close() unblocks receive() with nullopt");
        CHECK(q.closed(), "closed() reports true after close()");
    }

    // pending messages remain readable after close, new sends are dropped
    {
        MessageQueue<int> q;
        q.send(42);
        q.close();
        q.send(99);
        CHECK(q.size() == 1, "send after close is dropped");
        CHECK(q.receive().value() == 42, "pending message readable after close");
        CHECK(!q.receive().has_value(), "drained closed queue returns nullopt immediately");
    }

    std::printf("%s (%d failures)\n", failures ? "FAILED" : "PASSED", failures);
    return failures ? 1 : 0;
}
