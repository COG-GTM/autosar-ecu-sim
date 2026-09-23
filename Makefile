CXX=g++
CXXFLAGS=-std=c++17 -Iinclude -pthread -Wall -Wextra
LDFLAGS=

SRC=src/sensor_swc.cpp src/controller_swc.cpp src/execution_manager.cpp src/service_registry.cpp src/lifecycle.cpp

all: ecu

build:
	mkdir -p build

ecu: build src/main.cpp $(SRC)
	$(CXX) $(CXXFLAGS) -o build/ecu src/main.cpp $(SRC) $(LDFLAGS)

build/test_message_queue: build tests/test_message_queue.cpp include/message_queue.hpp
	$(CXX) $(CXXFLAGS) -o $@ tests/test_message_queue.cpp $(LDFLAGS)

build/test_shutdown: build tests/test_shutdown.cpp $(SRC)
	$(CXX) $(CXXFLAGS) -o $@ tests/test_shutdown.cpp $(SRC) $(LDFLAGS)

build/test_service_registry: build tests/test_service_registry.cpp src/service_registry.cpp
	$(CXX) $(CXXFLAGS) -o $@ tests/test_service_registry.cpp src/service_registry.cpp $(LDFLAGS)

.PHONY: test
test: build/test_message_queue build/test_shutdown build/test_service_registry
	./build/test_message_queue
	./build/test_shutdown
	./build/test_service_registry

.PHONY: clean
clean:
	rm -f build/ecu build/test_message_queue build/test_shutdown build/test_service_registry sensor_log.txt controller_log.txt
