//File: /src/service_registry.cpp
#include "../include/service_registry.hpp"
#include <iostream>

constexpr std::chrono::milliseconds ServiceRegistry::kDefaultDiscoveryTimeout;

ServiceRegistry& ServiceRegistry::instance(){
    static ServiceRegistry registry;
    return registry;
}

void ServiceRegistry::registerServiceImpl(const std::string& name, void* servicePtr,
                                          std::type_index type){
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = services_.find(name);
    if(it == services_.end()){
        services_.emplace(name, ServiceEntry{servicePtr, type});
    } else {
        it->second = ServiceEntry{servicePtr, type};
    }
    cond_.notify_all();
}

void* ServiceRegistry::discoverServiceImpl(const std::string& name, std::type_index type,
                                           std::chrono::milliseconds timeout){
    std::unique_lock<std::mutex> lock(mutex_);
    const bool found = cond_.wait_for(lock, timeout, [&] {
        return services_.find(name) != services_.end();
    });
    if(!found){
        std::cerr << "[ServiceRegistry] Timed out waiting for service '" << name << "'"
                  << std::endl;
        return nullptr;
    }

    const ServiceEntry& entry = services_.at(name);
    if(entry.type != type){
        std::cerr << "[ServiceRegistry] Type mismatch for service '" << name
                  << "': registered as " << entry.type.name() << ", requested as "
                  << type.name() << std::endl;
        return nullptr;
    }
    return entry.ptr;
}
