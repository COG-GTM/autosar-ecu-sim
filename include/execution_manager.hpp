//File: include/execution_manager.hpp
#pragma once
#include <string>

// Returns the process exit code: 0 on clean shutdown, non-zero if the config is unusable.
int runExecutionManager(const std::string& configPath = "config.json");
