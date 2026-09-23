//File: include/message_queue.hpp
#pragma once
#include <queue>
#include <mutex>
#include <condition_variable>

template <typename T>
class MessageQueue {
public:
    void send(const T& message) {
        std::lock_guard<std::mutex> lock(mutex_);
        queue_.push(message);
        cond_.notify_one();
    }

    T receive() {
        std::unique_lock<std::mutex> lock(mutex_);
        cond_.wait(lock, [this] { return !queue_.empty(); });
        T message = queue_.front();
        queue_.pop();
        return message;
    }

    private:
    std::queue<T> queue_;
    std::mutex mutex_;
    std::condition_variable cond_;
};