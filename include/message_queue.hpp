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

    void send(const T& message) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (shutdown_) {
            return;
        }
        while (queue_.size() >= capacity_) {
            queue_.pop();
            ++dropped_;
        }
        queue_.push(message);
        cond_.notify_one();
    }

    // Returns no value once the queue is shut down and drained.
    std::optional<T> receive() {
        std::unique_lock<std::mutex> lock(mutex_);
        cond_.wait(lock, [this] { return !queue_.empty() || shutdown_; });
        return pop(lock);
    }

    // Returns no value on timeout, or once the queue is shut down and drained.
    template <typename Rep, typename Period>
    std::optional<T> receiveFor(const std::chrono::duration<Rep, Period>& timeout) {
        std::unique_lock<std::mutex> lock(mutex_);
        if (!cond_.wait_for(lock, timeout, [this] { return !queue_.empty() || shutdown_; })) {
            return std::nullopt;
        }
        return pop(lock);
    }

    void shutdown() {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            shutdown_ = true;
        }
        cond_.notify_all();
    }

    bool isShutdown() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return shutdown_;
    }

    std::size_t size() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return queue_.size();
    }

    std::size_t droppedCount() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return dropped_;
    }

private:
    std::optional<T> pop(std::unique_lock<std::mutex>&) {
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
    bool shutdown_ = false;
    mutable std::mutex mutex_;
    std::condition_variable cond_;
};
