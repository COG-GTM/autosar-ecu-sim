// File: include/diagnostic_swc.hpp
#pragma once
#include "diagnostic_types.hpp"
#include "message_queue.hpp"
#include <nlohmann/json.hpp>
#include <cstddef>
#include <string>
#include <vector>

struct DiagnosticConfig {
    std::size_t capacity = 256;
    std::string outputFile = "diagnostics/events.json";
};

// Fixed-capacity ring buffer; on overflow the oldest event is overwritten
// and counted in dropped(). Owned by the diagnosticApp thread only.
class DiagnosticEventBuffer {
public:
    explicit DiagnosticEventBuffer(std::size_t capacity);

    void push(const DiagnosticEvent& event);
    // Retained events, oldest first.
    std::vector<DiagnosticEvent> events() const;
    std::size_t size() const;
    std::size_t capacity() const;
    std::size_t dropped() const;

private:
    std::vector<DiagnosticEvent> slots_;
    std::size_t head_ = 0;
    std::size_t size_ = 0;
    std::size_t dropped_ = 0;
};

std::string toIso8601(std::chrono::system_clock::time_point ts);
nlohmann::json diagnosticEventsToJson(const DiagnosticEventBuffer& buffer);
bool writeDiagnosticEvents(const DiagnosticEventBuffer& buffer, const std::string& path);

void diagnosticApp(MessageQueue<DiagnosticEvent>& queue, DiagnosticConfig config);
