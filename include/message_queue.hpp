//File: include/message_queue.hpp
#pragma once
#include <algorithm>
#include <chrono>
#include <condition_variable>
#include <cstddef>
#include <mutex>
#include <optional>
#include <queue>

// Bounded FIFO with evict-oldest overflow, matching the smart-ring sensor ring:
// a full queue drops its oldest element so the consumer always sees the freshest
// samples and memory stays bounded regardless of producer/consumer rate skew.
template <typename T>
class MessageQueue {
public:
    static constexpr std::size_t kDefaultCapacity = 64;

    explicit MessageQueue(std::size_t capacity = kDefaultCapacity)
        : capacity_(std::max<std::size_t>(capacity, 1)) {}

    // Never blocks: when the queue is full the oldest message is evicted and
    // counted in overflowCount().
    void send(const T& message) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (closed_) {
            return;
        }
        if (queue_.size() >= capacity_) {
            queue_.pop();
            ++overflowCount_;
        }
        queue_.push(message);
        cond_.notify_one();
    }

    // Blocks until a message is available or the queue is closed.
    std::optional<T> receive() {
        std::unique_lock<std::mutex> lock(mutex_);
        cond_.wait(lock, [this] { return !queue_.empty() || closed_; });
        return popLocked();
    }

    // Blocks for at most `timeout`; returns nullopt on timeout or when closed and drained.
    template <typename Rep, typename Period>
    std::optional<T> receiveFor(const std::chrono::duration<Rep, Period>& timeout) {
        std::unique_lock<std::mutex> lock(mutex_);
        cond_.wait_for(lock, timeout, [this] { return !queue_.empty() || closed_; });
        return popLocked();
    }

    // Wakes every blocked receiver; further sends are dropped, pending messages stay readable.
    void close() {
        std::lock_guard<std::mutex> lock(mutex_);
        closed_ = true;
        cond_.notify_all();
    }

    bool closed() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return closed_;
    }

    std::size_t size() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return queue_.size();
    }

    std::size_t capacity() const {
        return capacity_;
    }

    // Number of messages evicted because the queue was full.
    std::size_t overflowCount() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return overflowCount_;
    }

private:
    std::optional<T> popLocked() {
        if (queue_.empty()) {
            return std::nullopt;
        }
        T message = queue_.front();
        queue_.pop();
        return message;
    }

    std::queue<T> queue_;
    const std::size_t capacity_;
    std::size_t overflowCount_ = 0;
    mutable std::mutex mutex_;
    std::condition_variable cond_;
    bool closed_ = false;
};
