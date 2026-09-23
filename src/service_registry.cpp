//File: /src/service_registry.cpp
#include "../include/service_registry.hpp"
ServiceRegistry& ServiceRegistry::instance(){
    static ServiceRegistry registry;
    return registry;
}

void ServiceRegistry::registerServiceImpl(const std::string& name, void* servicePtr, std::type_index type){
    if(servicePtr == nullptr){
        return;
    }
    std::lock_guard<std::mutex> lock(mutex_);
    services_[name] = Entry{servicePtr, type};
    cond_.notify_all();
}

void* ServiceRegistry::discoverServiceImpl(const std::string& name,
                                           std::type_index type,
                                           std::chrono::milliseconds timeout,
                                           DiscoveryStatus& status){
    std::unique_lock<std::mutex> lock(mutex_);
    const bool present = cond_.wait_for(lock, timeout, [&] { return services_.find(name) != services_.end(); });
    if(!present){
        status = DiscoveryStatus::TimedOut;
        return nullptr;
    }
    const Entry& entry = services_[name];
    if(entry.type != type){
        status = DiscoveryStatus::TypeMismatch;
        return nullptr;
    }
    status = DiscoveryStatus::Found;
    return entry.ptr;
}
