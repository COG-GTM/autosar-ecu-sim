// File: src/diagnostic_swc.cpp
#include "../include/diagnostic_swc.hpp"
#include "../include/service_registry.hpp"
#include "../include/lifecycle.hpp"
#include <cmath>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>

using json = nlohmann::json;

extern std::atomic<AppState> diagnosticState;

DiagnosticEventBuffer::DiagnosticEventBuffer(std::size_t capacity) : slots_(capacity) {}

void DiagnosticEventBuffer::push(const DiagnosticEvent& event) {
    if (slots_.empty()) {
        ++dropped_;
        return;
    }
    std::size_t tail = (head_ + size_) % slots_.size();
    slots_[tail] = event;
    if (size_ < slots_.size()) {
        ++size_;
    } else {
        head_ = (head_ + 1) % slots_.size();
        ++dropped_;
    }
}

std::vector<DiagnosticEvent> DiagnosticEventBuffer::events() const {
    std::vector<DiagnosticEvent> ordered;
    ordered.reserve(size_);
    for (std::size_t i = 0; i < size_; ++i) {
        ordered.push_back(slots_[(head_ + i) % slots_.size()]);
    }
    return ordered;
}

std::size_t DiagnosticEventBuffer::size() const { return size_; }
std::size_t DiagnosticEventBuffer::capacity() const { return slots_.size(); }
std::size_t DiagnosticEventBuffer::dropped() const { return dropped_; }

std::string toIso8601(std::chrono::system_clock::time_point ts) {
    using namespace std::chrono;
    std::time_t seconds = system_clock::to_time_t(ts);
    long long millis = duration_cast<milliseconds>(ts.time_since_epoch()).count() % 1000;
    if (millis < 0) {
        millis += 1000;
        --seconds;
    }
    std::tm utc{};
    gmtime_r(&seconds, &utc);
    std::ostringstream out;
    out << std::put_time(&utc, "%Y-%m-%dT%H:%M:%S") << '.' << std::setw(3) << std::setfill('0')
        << millis << 'Z';
    return out.str();
}

static double roundMilli(float value) {
    return std::round(static_cast<double>(value) * 1000.0) / 1000.0;
}

json diagnosticEventsToJson(const DiagnosticEventBuffer& buffer) {
    json events = json::array();
    for (const DiagnosticEvent& event : buffer.events()) {
        bool raised = event.kind == EventKind::Raised;
        events.push_back({
            {"timestamp", toIso8601(event.ts)},
            {"channel", event.channel == Channel::Temperature ? "temperature" : "pressure"},
            {"kind", raised ? "raise" : "clear"},
            {"state", raised ? "ALARM" : "NORMAL"},
            {"value", roundMilli(event.value)},
            {"threshold", roundMilli(event.threshold)},
        });
    }
    return {
        {"capacity", buffer.capacity()},
        {"dropped", buffer.dropped()},
        {"events", events},
    };
}

bool writeDiagnosticEvents(const DiagnosticEventBuffer& buffer, const std::string& path) {
    std::filesystem::path outputPath(path);
    std::error_code ec;
    if (outputPath.has_parent_path()) {
        std::filesystem::create_directories(outputPath.parent_path(), ec);
    }
    std::ofstream out(outputPath);
    if (!out.is_open()) {
        return false;
    }
    out << diagnosticEventsToJson(buffer).dump(2) << std::endl;
    return static_cast<bool>(out);
}

void diagnosticApp(MessageQueue<DiagnosticEvent>& queue, DiagnosticConfig config) {
    diagnosticState = AppState::RUNNING;
    ServiceRegistry::instance().registerService("DiagnosticEventService", &queue);
    DiagnosticEventBuffer buffer(config.capacity);

    // Runs until the ExecutionManager closes the queue after the controller has stopped.
    while (std::optional<DiagnosticEvent> event = queue.receive()) {
        buffer.push(*event);
    }
    diagnosticState = AppState::SHUTDOWN;

    std::ostringstream line;
    if (writeDiagnosticEvents(buffer, config.outputFile)) {
        line << "[Diagnostics] Wrote " << buffer.size() << " events (" << buffer.dropped()
             << " dropped) to " << config.outputFile << "\n";
        std::cout << line.str() << std::flush;
    } else {
        line << "[Diagnostics] Failed to write " << config.outputFile << "\n";
        std::cerr << line.str() << std::flush;
    }
    std::cout << "[Diagnostics] Shutting down." << std::endl;
}
