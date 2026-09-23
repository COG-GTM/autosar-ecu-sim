//File: include/service_registry.hpp
#pragma once
#include <string>
#include <unordered_map>
#include <mutex>
#include <memory>
#include <condition_variable>
#include <chrono>
#include <typeindex>
#include <typeinfo>

class ServiceRegistry{
    public:
        static constexpr std::chrono::milliseconds kDefaultDiscoveryTimeout{5000};

        static ServiceRegistry& instance();

        template <typename T>
        void registerService(const std::string& name, T* servicePtr){
            registerServiceImpl(name, servicePtr, std::type_index(typeid(T)));
        }

        // Returns nullptr if the service is not registered within the timeout or
        // if it was registered under a different type.
        template <typename T>
        T* discoverService(const std::string& name,
                           std::chrono::milliseconds timeout = kDefaultDiscoveryTimeout){
            return static_cast<T*>(
                discoverServiceImpl(name, std::type_index(typeid(T)), timeout));
        }

    private:
        struct ServiceEntry{
            void* ptr;
            std::type_index type;
        };

        void registerServiceImpl(const std::string& name, void* servicePtr, std::type_index type);
        void* discoverServiceImpl(const std::string& name, std::type_index type,
                                  std::chrono::milliseconds timeout);

        std::unordered_map<std::string, ServiceEntry> services_;
        std::mutex mutex_;
        std::condition_variable cond_;

};
