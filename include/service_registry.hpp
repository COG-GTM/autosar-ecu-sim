//File: include/service_registry.hpp
#pragma once
#include <chrono>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <string>
#include <typeindex>
#include <typeinfo>
#include <unordered_map>

enum class DiscoveryStatus {
    Found,
    TimedOut,
    TypeMismatch
};

// Result of a typed discovery: `service` is only non-null when status == Found.
template <typename T>
struct DiscoveryResult {
    DiscoveryStatus status = DiscoveryStatus::TimedOut;
    T* service = nullptr;

    explicit operator bool() const { return status == DiscoveryStatus::Found && service != nullptr; }
};

class ServiceRegistry{
    public:
        static ServiceRegistry& instance();

        template <typename T>
        void registerService(const std::string& name, T* servicePtr){
            registerServiceImpl(name, static_cast<void*>(servicePtr), std::type_index(typeid(T)));
        }

        // Waits at most `timeout` for `name` to be registered as a T*.
        template <typename T>
        DiscoveryResult<T> discoverService(const std::string& name,
                                           std::chrono::milliseconds timeout){
            DiscoveryResult<T> result;
            void* raw = discoverServiceImpl(name, std::type_index(typeid(T)), timeout, result.status);
            result.service = static_cast<T*>(raw);
            return result;
        }

    private:
        struct Entry{
            void* ptr = nullptr;
            std::type_index type = std::type_index(typeid(void));
        };

        void registerServiceImpl(const std::string& name, void* servicePtr, std::type_index type);
        void* discoverServiceImpl(const std::string& name,
                                  std::type_index type,
                                  std::chrono::milliseconds timeout,
                                  DiscoveryStatus& status);

        std::unordered_map<std::string, Entry> services_;
        std::mutex mutex_;
        std::condition_variable cond_;

};
