//File: /src/service_registry.cpp
#include "../include/service_registry.hpp"
ServiceRegistry& ServiceRegistry::instance(){
    static ServiceRegistry registry;
    return registry;
}

void ServiceRegistry::registerService(const std::string& name, void* servicePtr){
    std::lock_guard<std::mutex> lock(mutex_);
    services_[name] = servicePtr;
    cond_.notify_all();
}

void* ServiceRegistry::discoverServiceFor(const std::string& name, std::chrono::milliseconds timeout){
    std::unique_lock<std::mutex> lock(mutex_);
    cond_.wait_for(lock, timeout, [&] { return services_.find(name) != services_.end(); });
    auto it = services_.find(name);
    return it == services_.end() ? nullptr : it->second;
}

void* ServiceRegistry::findService(const std::string& name){
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = services_.find(name);
    return it == services_.end() ? nullptr : it->second;
}
