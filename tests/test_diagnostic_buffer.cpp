// Host unit tests for the DiagnosticEvent ring buffer and JSON dump (SWR-210/211/212).
// Build and run: make test
#include "../include/diagnostic_swc.hpp"
#include "../include/lifecycle.hpp"
#include "../include/service_registry.hpp"
#include <chrono>
#include <cstdio>
#include <string>
#include <fstream>
#include <regex>
#include <thread>

using json = nlohmann::json;
using namespace std::chrono;

static int failures = 0;
#define CHECK(cond, msg)                                                       \
    do {                                                                       \
        if (!(cond)) { std::printf("FAIL: %s\n", msg); failures++; }           \
        else { std::printf("ok:   %s\n", msg); }                               \
    } while (0)

static DiagnosticEvent makeEvent(float value, EventKind kind = EventKind::Raised) {
    return DiagnosticEvent{system_clock::time_point(milliseconds(1790000000123LL)), Channel::Pressure,
                           value, 3.0f, kind};
}

int main() {
    // Overflow drops the oldest event and counts it
    {
        DiagnosticEventBuffer buffer(3);
        for (int i = 0; i < 5; ++i) {
            buffer.push(makeEvent(static_cast<float>(i)));
        }
        auto events = buffer.events();
        CHECK(buffer.size() == 3 && buffer.capacity() == 3, "buffer bounded at capacity");
        CHECK(buffer.dropped() == 2, "overflow counter counts dropped events");
        CHECK(events.size() == 3 && events[0].value == 2.0f && events[1].value == 3.0f && events[2].value == 4.0f,
              "oldest events dropped, order preserved");
    }

    // ISO-8601 UTC timestamps with millisecond precision
    {
        CHECK(toIso8601(system_clock::time_point(milliseconds(1790000000123LL))) == "2026-09-21T14:13:20.123Z",
              "toIso8601 formats UTC with milliseconds");
        CHECK(toIso8601(system_clock::time_point(milliseconds(0))) == "1970-01-01T00:00:00.000Z",
              "toIso8601 formats epoch");
    }

    // JSON schema
    {
        DiagnosticEventBuffer buffer(4);
        buffer.push(makeEvent(3.1f));
        buffer.push(DiagnosticEvent{system_clock::now(), Channel::Temperature, 32.9f, 35.0f, EventKind::Cleared});
        json doc = diagnosticEventsToJson(buffer);
        const json& events = doc["events"];
        std::regex iso(R"(\d{4}-\d{2}-\d{2}T\d{2}:\d{2}:\d{2}\.\d{3}Z)");
        CHECK(doc["capacity"] == 4 && doc["dropped"] == 0 && events.size() == 2, "document has capacity, dropped, events");
        CHECK(events[0]["channel"] == "pressure" && events[0]["kind"] == "raise" && events[0]["state"] == "ALARM" &&
                  events[0]["value"] == 3.1 && events[0]["threshold"] == 3.0,
              "raise event fields serialised");
        CHECK(events[1]["channel"] == "temperature" && events[1]["kind"] == "clear" && events[1]["state"] == "NORMAL",
              "clear event fields serialised");
        CHECK(std::regex_match(events[1]["timestamp"].get<std::string>(), iso), "timestamp is ISO-8601");
    }

    // diagnosticApp registers the service, drains the queue and dumps on close
    {
        const std::string path = "build/diagnostics_test/events.json";
        std::remove(path.c_str());
        MessageQueue<DiagnosticEvent> queue;
        DiagnosticConfig config;
        config.capacity = 2;
        config.outputFile = path;
        std::thread app(diagnosticApp, std::ref(queue), config);
        auto* service = static_cast<MessageQueue<DiagnosticEvent>*>(
            ServiceRegistry::instance().discoverService("DiagnosticEventService"));
        CHECK(service == &queue, "diagnosticApp registers DiagnosticEventService");
        for (int i = 0; i < 3; ++i) {
            service->send(makeEvent(static_cast<float>(i)));
        }
        queue.close();
        app.join();
        std::ifstream in(path);
        json doc = json::parse(in, nullptr, false);
        CHECK(!doc.is_discarded() && doc["events"].size() == 2 && doc["dropped"] == 1 &&
                  doc["events"][0]["value"] == 1.0,
              "events.json written at shutdown with retained events and overflow count");
        CHECK(diagnosticState == AppState::SHUTDOWN, "diagnosticApp reports SHUTDOWN");
    }

    std::printf("%s (%d failures)\n", failures ? "FAILED" : "PASSED", failures);
    return failures ? 1 : 0;
}
