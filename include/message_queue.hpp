//File: include/message_queue.hpp
#pragma once
#include <chrono>
#include <condition_variable>
#include <cstddef>
#include <mutex>
#include <optional>
#include <queue>

template <typename T>
class MessageQueue {
public:
    static constexpr std::size_t kDefaultCapacity = 128;

    explicit MessageQueue(std::size_t capacity = kDefaultCapacity)
        : capacity_(capacity == 0 ? 1 : capacity) {}

    // Drops the oldest pending messages to stay within capacity; never blocks the producer.
    void send(const T& message) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (closed_) {
            return;
        }
        while (queue_.size() >= capacity_) {
            queue_.pop();
            ++dropped_;
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

    std::size_t droppedCount() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return dropped_;
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
    std::size_t dropped_ = 0;
    mutable std::mutex mutex_;
    std::condition_variable cond_;
    bool closed_ = false;
};
