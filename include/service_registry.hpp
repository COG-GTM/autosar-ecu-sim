//File: include/service_registry.hpp
#pragma once
#include <chrono>
#include <condition_variable>
#include <mutex>
#include <string>
#include <unordered_map>

class ServiceRegistry{
    public:
        static ServiceRegistry& instance();

        void registerService(const std::string& name, void* servicePtr);

        // Waits at most `timeout` for `name` to be registered; nullptr on timeout.
        void* discoverServiceFor(const std::string& name, std::chrono::milliseconds timeout);

        // Non-blocking lookup; nullptr when the service is not registered.
        void* findService(const std::string& name);

        template <typename T>
        T* discoverServiceAs(const std::string& name, std::chrono::milliseconds timeout){
            return static_cast<T*>(discoverServiceFor(name, timeout));
        }

    private:
        std::unordered_map<std::string, void*> services_;
        std::mutex mutex_;
        std::condition_variable cond_;

};
