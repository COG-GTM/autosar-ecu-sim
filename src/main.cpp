//File: src/main.cpp
#include "../include/execution_manager.hpp"

int main(int argc, char* argv[]){
    return argc > 1 ? runExecutionManager(argv[1]) : runExecutionManager();
}
