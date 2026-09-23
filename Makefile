CXX=g++
CXXFLAGS=-std=c++17 -Iinclude -pthread
LDFLAGS=

SRC=src/sensor_swc.cpp src/controller_swc.cpp src/execution_manager.cpp src/service_registry.cpp src/lifecycle.cpp

all: ecu

ecu: src/main.cpp $(SRC)
	mkdir -p build
	$(CXX) $(CXXFLAGS) -o build/ecu src/main.cpp $(SRC) $(LDFLAGS)

test: tests/test_service_registry.cpp src/service_registry.cpp
	mkdir -p build
	$(CXX) $(CXXFLAGS) -o build/test_service_registry tests/test_service_registry.cpp src/service_registry.cpp $(LDFLAGS)
	./build/test_service_registry

clean:
	rm -f build/ecu build/test_service_registry sensor_log.txt controller_log.txt

.PHONY: all test clean
