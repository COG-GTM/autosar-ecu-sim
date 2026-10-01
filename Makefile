CXX=g++
CXXFLAGS=-std=c++17 -Iinclude -pthread -Wall -Wextra
LDFLAGS=

SRC=src/sensor_swc.cpp src/controller_swc.cpp src/execution_manager.cpp src/service_registry.cpp src/lifecycle.cpp

all: ecu build/stub_interfaces

build:
	mkdir -p build

ecu: build src/main.cpp $(SRC)
	$(CXX) $(CXXFLAGS) -o build/ecu src/main.cpp $(SRC) $(LDFLAGS)

# AE-9 design gate: the B2.0 interface headers must compile as stubs.
build/stub_interfaces: build tests/stub_interfaces.cpp
	$(CXX) $(CXXFLAGS) -o $@ tests/stub_interfaces.cpp $(LDFLAGS)

build/test_message_queue: build tests/test_message_queue.cpp include/message_queue.hpp
	$(CXX) $(CXXFLAGS) -o $@ tests/test_message_queue.cpp $(LDFLAGS)

build/test_shutdown: build tests/test_shutdown.cpp $(SRC)
	$(CXX) $(CXXFLAGS) -o $@ tests/test_shutdown.cpp $(SRC) $(LDFLAGS)

.PHONY: test
test: build/test_message_queue build/test_shutdown
	./build/test_message_queue
	./build/test_shutdown

.PHONY: clean
clean:
	rm -f build/ecu build/test_message_queue build/test_shutdown sensor_log.txt controller_log.txt
